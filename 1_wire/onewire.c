/*
 * onewire.c - functions to work with 1-wire devices
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include "onewire.h"
#include "user_proto.h"

OW_ID id_array[OW_MAX_NUM]; // 1-wire devices ID buffer (not more than eight)
uint8_t dev_amount = 0;   // amount of 1-wire devices


// states of 1-wire processing queue
typedef enum{
	OW_OFF_STATE,    // not working
	OW_RESET_STATE,  // reset bus
	OW_SEND_STATE,   // send data
	OW_READ_STATE,   // wait for reading
} OW_States;
volatile OW_States OW_State = OW_OFF_STATE; // 1-wire state, 0-not runned

void (*ow_process_resdata)() = NULL;


uint16_t tim2_buff[TIM2_DMABUFF_SIZE];
uint16_t tim2_inbuff[TIM2_DMABUFF_SIZE];
int tum2buff_ctr = 0;
uint8_t ow_done = 1;

/**
 * this function sends bits of ow_byte (LSB first) to 1-wire line
 * @param ow_byte - byte to convert
 * @param Nbits   - number of bits to send
 * @param ini     - 1 to zero counter
 */
uint8_t OW_add_byte(uint8_t ow_byte){
	uint8_t i, byte;
	for(i = 0; i < 8; i++){
		if(ow_byte & 0x01){
			byte = BIT_ONE_P;
		}else{
			byte = BIT_ZERO_P;
		}
		if(tum2buff_ctr == TIM2_DMABUFF_SIZE){
			ERR("Tim2 buffer overflow");
			return 0; // avoid buffer overflow
		}
		tim2_buff[tum2buff_ctr++] = byte;
		ow_byte >>= 1;
	}
	INT(tum2buff_ctr);
	DBG(" bytes in send buffer\n");
	return 1;
}



/**
 * Adds Nbytes bytes 0xff  for reading sequence
 */
uint8_t OW_add_read_seq(uint8_t Nbytes){
	uint8_t i;
	if(Nbytes == 0) return 0;
	Nbytes *= 8; // 8 bits for each byte
	for(i = 0; i < Nbytes; i++){
		if(tum2buff_ctr == TIM2_DMABUFF_SIZE){
			ERR("Tim2 buffer overflow");
			return 0;
		}
		tim2_buff[tum2buff_ctr++] = BIT_READ_P;
	}
	INT(tum2buff_ctr);
	DBG(" bytes in send buffer\n");
	return 1;
}

/**
 * Fill output buffer with data from 1-wire
 * @param start_idx - index from which to start (byte number)
 * @param N         - data length (in **bytes**)
 * @outbuf          - where to place data
 */
void read_from_OWbuf(uint8_t start_idx, uint8_t N, uint8_t *outbuf){
	start_idx *= 8;
	uint8_t i, j, last = start_idx + N * 8, byte;
	if(last >= TIM2_DMABUFF_SIZE) last = TIM2_DMABUFF_SIZE;
	for(i = start_idx; i < last;){
		byte = 0;
		for(j = 0; j < 8; j++){
			byte >>= 1;
			INT(tim2_inbuff[i]);
			DBG(" ");
			if(tim2_inbuff[i++] < ONE_ZERO_BARRIER)
				byte |= 0x80;
		}
		*outbuf++ = byte;
		DBG("readed \n");
	}
}
// there's a mistake in opencm3, so redefine this if needed (TIM_CCMR2_CC3S_IN_TI1 -> TIM_CCMR2_CC3S_IN_TI4)
#ifndef TIM_CCMR2_CC3S_IN_TI4
#define TIM_CCMR2_CC3S_IN_TI4		(2)
#endif
void init_ow_dmatimer(){ // tim2_ch4 - PA3, no remap
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO3);
	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_clock_enable(RCC_DMA1);
	timer_reset(TIM2);
	// timers have frequency of 1MHz -- 1us for one step
	// 36MHz of APB1
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	// 72MHz div 72 = 1MHz
	// TODO: WHY 71 if freq = 36MHz?
	TIM2_PSC = 71;  // prescaler is (div - 1)
	TIM2_CR1 = TIM_CR1_ARPE; // bufferize ARR/CCR
	TIM2_ARR = RESET_LEN;
	// PWM_OUT: TIM2_CH4; capture: TIM2_CH3
	// PWM edge-aligned mode & enable preload for CCR4, CC3 takes input from TI4
	TIM2_CCMR2 = TIM_CCMR2_OC4M_PWM1 | TIM_CCMR2_OC4PE | TIM_CCMR2_CC3S_IN_TI4;
	TIM2_CCR4 = 0; // set output value to 1 by clearing CCR4
	TIM2_EGR = TIM_EGR_UG; // update values of ARR & CCR4
	// set low polarity for CC4, high for CC3 & enable CC4 out and CC3 in
	TIM2_CCER = TIM_CCER_CC4P | TIM_CCER_CC4E | TIM_CCER_CC3E;

	// TIM2_CH4 - DMA1, channel 7
	dma_channel_reset(DMA1, DMA_CHANNEL7);
	DMA1_CCR7 = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_PSIZE_16BIT | DMA_CCR_MSIZE_16BIT
			| DMA_CCR_TEIE | DMA_CCR_TCIE | DMA_CCR_PL_HIGH;
	nvic_enable_irq(NVIC_DMA1_CHANNEL7_IRQ); // enable dma1_channel7_isr
	tum2buff_ctr = 0;
	DBG("OW INITED\n");
#ifdef EBUG
	gpio_set(GPIOC, GPIO10);
#endif
}

void run_dmatimer(){
	ow_done = 0;
	TIM2_CR1 = 0;
	adc_disable_dma(ADC1); // turn off DMA & ADC
	adc_off(ADC1);
	DMA1_IFCR = DMA_ISR_TEIF7|DMA_ISR_HTIF7|DMA_ISR_TCIF7|DMA_ISR_GIF7 |
		DMA_ISR_TEIF1|DMA_ISR_HTIF1|DMA_ISR_TCIF1|DMA_ISR_GIF1; // clear flags
	DMA1_CCR7 &= ~DMA_CCR_EN; // disable (what if it's enabled?) to set address
	DMA1_CPAR7 = (uint32_t) &(TIM_CCR4(TIM2)); // dma_set_peripheral_address(DMA1, DMA_CHANNEL7, (uint32_t) &(TIM_CCR4(TIM2)));
	DMA1_CMAR7 = (uint32_t) &tim2_buff[1]; // dma_set_memory_address(DMA1, DMA_CHANNEL7, (uint32_t)tim2_buff);
	DMA1_CNDTR7 = tum2buff_ctr-1;//dma_set_number_of_data(DMA1, DMA_CHANNEL7, tum2buff_ctr);
	// TIM2_CH4 - DMA1, channel 7
	dma_channel_reset(DMA1, DMA_CHANNEL1);
	DMA1_CCR1 = DMA_CCR_MINC | DMA_CCR_PSIZE_16BIT | DMA_CCR_MSIZE_16BIT
			| DMA_CCR_TEIE | DMA_CCR_TCIE | DMA_CCR_PL_HIGH;
	DMA1_CPAR1 = (uint32_t) &(TIM_CCR3(TIM2)); //dma_set_peripheral_address(DMA1, DMA_CHANNEL1, (uint32_t) &(TIM_CCR3(TIM2)));
	DMA1_CMAR1 = (uint32_t) tim2_inbuff; //dma_set_memory_address(DMA1, DMA_CHANNEL1, (uint32_t) tim2_inbuff);
	DMA1_CNDTR1 = tum2buff_ctr; //dma_set_number_of_data(DMA1, DMA_CHANNEL1, tum2buff_ctr);
	nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);

	DMA1_CCR7 |= DMA_CCR_EN; //dma_enable_channel(DMA1, DMA_CHANNEL7);
	DMA1_CCR1 |= DMA_CCR_EN; //dma_enable_channel(DMA1, DMA_CHANNEL1);

	TIM2_SR = 0; // clear all flags
	TIM2_ARR = BIT_LEN; // bit length
	TIM2_CCR4 = tim2_buff[0]; // we should manually set first bit to avoid zero in tim2_inbuff[0]
	TIM2_EGR = TIM_EGR_UG; // update value of ARR
	TIM2_CR1 = TIM_CR1_ARPE; // bufferize ARR/CCR

	TIM2_CR2 &= ~TIM_CR2_CCDS; // timer_set_dma_on_compare_event(TIM2);
	TIM2_DIER = TIM_DIER_CC4DE | TIM_DIER_CC3DE; // enable DMA events
	// set low polarity, enable cc out & enable input capture
	TIM2_CR1 |= TIM_CR1_CEN; // run timer
#ifdef EBUG
	gpio_clear(GPIOC, GPIO10);
#endif
	DBG("RUN transfer of ");
	INT(tum2buff_ctr);
	DBG(" bits\n");
}

uint16_t rstat = 0, lastcc3 = 3;
void ow_reset(){
	//init_ow_dmatimer();
	ow_done = 0;
	rstat = 0;
	TIM2_SR = 0; // clear all flags
	TIM2_CR1 = 0;
	TIM2_DIER = 0; // disable timer interrupts
	TIM2_ARR = RESET_LEN; // set period to 1ms
	TIM2_CCR4 = RESET_P; // zero pulse length
	TIM2_EGR = TIM_EGR_UG; // update values of ARR & CCR4
	//TIM2_CCMR2 = TIM_CCMR2_OC4M_PWM1 | TIM_CCMR2_OC4PE | TIM_CCMR2_CC3S_IN_TI4;
	//TIM2_CCER = TIM_CCER_CC4P | TIM_CCER_CC4E | TIM_CCER_CC3E;
	DBG("OW RESET in process");
	TIM2_DIER = TIM_DIER_CC3IE;
#ifdef EBUG
	gpio_clear(GPIOC, GPIO10);
#endif
	TIM2_CR1 = TIM_CR1_OPM | TIM_CR1_CEN | TIM_CR1_UDIS; // we need only single pulse & run timer; disable UEV
	TIM2_SR = 0; // clear update flag generated after timer's running
	nvic_enable_irq(NVIC_TIM2_IRQ);
}

void tim2_isr(){
	if(TIM2_SR & TIM_SR_UIF){ // update interrupt
		TIM2_DIER = 0; // disable all timer interrupts
#ifdef EBUG
		gpio_set(GPIOC, GPIO10);
#endif
		TIM2_CCR4 = 0; // set output value to 1
		TIM2_SR = 0; // clear flag
		nvic_disable_irq(NVIC_TIM2_IRQ);
		ow_done = 1;
		rstat = lastcc3;
		DBG(" ... done!\n");
	}
	if(TIM2_SR & TIM_SR_CC3IF){ // we need this interrupt to store CCR3 value
		lastcc3 = TIM2_CCR3;
		TIM2_CR1 &= ~TIM_CR1_UDIS; // enable UEV
		TIM2_SR = 0; // clear flag (we've manage TIM_SR_UIF before, so can simply do =0)
		TIM2_DIER |= TIM_DIER_UIE; // Now allow also Update interrupts to turn off everything
	}
}

/**
 * DMA interrupt in 1-wire mode
 */
void dma1_channel1_isr(){
			int i;
	if(DMA1_ISR & DMA_ISR_TCIF1){
#ifdef EBUG
		gpio_set(GPIOC, GPIO10);
#endif
		DMA1_IFCR = DMA_IFCR_CTCIF1;
		TIM2_CR1 &= ~TIM_CR1_CEN;    // timer_disable_counter(TIM2);
		DMA1_CCR1 &= ~DMA_CCR_EN; // disable DMA1 channel 1
		nvic_disable_irq(NVIC_DMA1_CHANNEL1_IRQ);
		ow_done = 1;
			for(i = 0; i < tum2buff_ctr; i++){
				print_int(tim2_inbuff[i]);
				P(" ");
			}
			P("\n");
	}else if(DMA1_ISR & DMA_ISR_TEIF1){
		DMA1_IFCR = DMA_IFCR_CTEIF1;
		DBG("DMA in transfer error\n");
	}
}

void dma1_channel7_isr(){
	if(DMA1_ISR & DMA_ISR_TCIF7){
		DMA1_IFCR = DMA_IFCR_CTCIF7; // clear flag
		DMA1_CCR7 &= ~DMA_CCR_EN; // disable DMA1 channel 7
	}else if(DMA1_ISR & DMA_ISR_TEIF7){
		DMA1_IFCR = DMA_IFCR_CTEIF7;
		DBG("DMA out transfer error\n");
	}
}


uint8_t OW_get_reset_status(){
	if(rstat < RESET_BARRIER) return 0; // no devices
	return 1;
}


uint8_t ow_data_ready = 0;   // flag of reading OK

/**
 * Process 1-wire commands depending on its state
 */
void OW_process(){
	static uint8_t ow_was_reseting = 0;
	switch(OW_State){
		case OW_OFF_STATE:
			return;
		break;
		case OW_RESET_STATE:
			DBG("OW reset\n");
			OW_State = OW_SEND_STATE;
			ow_was_reseting = 1;
			ow_reset();
		break;
		case OW_SEND_STATE:
			if(!ow_done) return; // reset in work
			if(ow_was_reseting){
				if(rstat < RESET_BARRIER){
					ERR("Error: no 1-wire devices found");
					INT(rstat);
					DBG("\n");
					ow_was_reseting = 0;
					OW_State = OW_OFF_STATE;
					return;
				}
			}
			ow_was_reseting = 0;
			OW_State = OW_READ_STATE;
			run_dmatimer(); // turn on data transfer
			DBG("OW send\n");
		break;
		case OW_READ_STATE:
			if(!ow_done) return; // data isn't ready
			OW_State = OW_OFF_STATE;
		//	adc_dma_on(); // return DMA1_1 to ADC at end of data transmitting
			if(ow_process_resdata){
				ow_process_resdata();
				ow_process_resdata = NULL;
			}
			ow_data_ready = 1;
			DBG("OW read\n");
		break;
	}
}


uint8_t *read_buf = NULL;    // buffer for storing readed data
/**
 * fill ID buffer with readed data
 */
void fill_buff_with_data(){
	if(!read_buf) return;
	read_from_OWbuf(1, 8, read_buf);
	int i, j;
	// now check stored ROMs
	for(i = 0; i < dev_amount; ++i){
		uint8_t *ROM = id_array[i].bytes;
		for(j = 0; j < 8; j++)
			if(ROM[j] != read_buf[j]) break;
		if(j == 8){ // we found this cell
			DBG("Such ID exists\n");
			goto ret;
		}
	}
	++dev_amount;
ret:
	read_buf = NULL;
}

/**
 * fill Nth array with identificators
 */
//uint8_t comtosend = 0;
void OW_fill_next_ID(){
	if(dev_amount >= OW_MAX_NUM){
		ERR("No memory left for new device\n");
		return;
	}
	ow_data_ready = 0;
	OW_State = OW_RESET_STATE;
	OW_reset_buffer();
	OW_add_byte(OW_READ_ROM);
	OW_add_read_seq(8); // wait for 8 bytes
	read_buf = id_array[dev_amount].bytes;
	ow_process_resdata = fill_buff_with_data;
	DBG("wait for ID\n");
}

/**
 * Procedure of 1-wire communications
 * variables:
 * @param sendReset - send RESET before transmission
 * @param command - bytes sent to the bus (if we want to read, send OW_READ_SLOT)
 * @param cLen - command buffer length (how many bytes to send)
 * @return 1 if succeed, 0 if failure
 */
uint8_t OW_Send(uint8_t sendReset, uint8_t *command, uint8_t cLen){
	ow_data_ready = 0;
	// if reset needed - send RESET and check bus
	if(sendReset)
		OW_State = OW_RESET_STATE;
	else
		OW_State = OW_SEND_STATE;
	OW_reset_buffer();
	while(cLen-- > 0){
		if(!OW_add_byte(*command++)) return 0;
	}
	return 1;
}

/**
 * convert temperature from scratchpad
 * in case of error return 200000 (ERR_TEMP_VAL)
 * return value in 10th degrees centigrade
 *
 * 0 - themperature LSB
 * 1 - themperature MSB (all higher bits are sign)
 * 2 - T_H
 * 3 - T_L
 * 4 - B20: Configuration register (only bits 6/5 valid: 9..12 bits resolution); 0xff for S20
 * 5 - 0xff (reserved)
 * 6 - (reserved for B20); S20: COUNT_REMAIN (0x0c)
 * 7 - COUNT PER DEGR (0x10)
 * 8 - CRC
 */
int32_t gettemp(uint8_t *scratchpad){
	// detect DS18S20
	int32_t t = 0;
	uint8_t l,m;
	int8_t v;
	if(scratchpad[7] == 0xff) // 0xff can be only if there's no such device or some other error
		return ERR_TEMP_VAL;
	m = scratchpad[1];
	l = scratchpad[0];
	if(scratchpad[4] == 0xff){ // DS18S20
		v = l >> 1 | (m & 0x80); // take signum from MSB
		t = ((int32_t)v) * 10L;
		if(l&1) t += 5L; // decimal 0.5
	}else{
		v = l>>4 | ((m & 7)<<4) | (m & 0x80);
		t = ((int32_t)v) * 10L;
		m = l & 0x0f; // add decimal
		t += (int32_t)m; // t = v*10 + l*1.25 -> convert
		if(m > 1) ++t; // 1->1, 2->3, 3->4, 4->5, 5->6
		else if(m > 5) t += 2L; // 6->8, 7->9
	}
	return t;
}

int32_t temperature = ERR_TEMP_VAL;
int8_t Ncur = 0;
/**
 * get temperature from buffer
 */
void convert_next_temp(){
	uint8_t scratchpad[9];
	if(dev_amount < 2){
		read_from_OWbuf(2, 9, scratchpad);
	}else{
		read_from_OWbuf(10, 9, scratchpad);
	}
	temperature = gettemp(scratchpad);
	DBG("Readed temperature: ");
	INT(temperature);
	DBG("/10 degrC\n");
}

/**
 * read next stored thermometer
 */
void OW_read_next_temp(){
	ow_data_ready = 0;
	OW_State = OW_RESET_STATE;
	OW_reset_buffer();
	int i;
	if(dev_amount < 2){
		Ncur = -1;
		OW_add_byte(OW_SKIP_ROM);
	}else{
		if(++Ncur >= dev_amount) Ncur = 0;
		OW_add_byte(OW_MATCH_ROM);
		uint8_t *ROM = id_array[Ncur].bytes;
		for(i = 0; i < 8; ++i)
			OW_add_byte(ROM[i]);
	}
	OW_add_byte(OW_READ_SCRATCHPAD);
	OW_add_read_seq(9); // wait for 9 bytes - ROM
	ow_process_resdata = convert_next_temp;
}

void OW_send_read_seq(){
	ow_data_ready = 0;
	OW_State = OW_RESET_STATE;
	OW_reset_buffer();
	OW_add_byte(OW_SKIP_ROM);
	OW_add_byte(OW_CONVERT_T);
	ow_process_resdata = NULL;
}
/*
 * scan 1-wire bus
 * WARNING! The procedure works in real-time, so it is VERY LONG
 * 		num - max number of devices
 * 		buf - array for devices' ID's (8*num bytes)
 * return amount of founded devices
 *
uint8_t OW_Scan(uint8_t *buf, uint8_t num){
	unsigned long path,next,pos;
	uint8_t bit,chk;
	uint8_t cnt_bit, cnt_byte, cnt_num;
	path=0;
	cnt_num=0;
	do{
		//(issue the 'ROM search' command)
		if( 0 == OW_WriteCmd(OW_SEARCH_ROM) ) return 0;
		OW_Wait_TX();
		OW_ClearBuff(); // clear RX buffer
		next = 0; // next path to follow
		pos = 1;  // path bit pointer
		for(cnt_byte = 0; cnt_byte != 8; cnt_byte++){
			buf[cnt_num*8 + cnt_byte] = 0;
			for(cnt_bit = 0; cnt_bit != 8; cnt_bit++){
				//(read two bits, 'bit' and 'chk', from the 1-wire bus)
				OW_SendBits(OW_R, 2);
				OW_Wait_TX();
				bit = -----OW_ReadByte();
				chk = bit & 0x02; // bit 1
				bit = bit & 0x01; // bit 0
				if(bit && chk) return 0; // error
				if(!bit && !chk){ // collision, both are zero
					if (pos & path) bit = 1;     // if we've been here before
					else next = (path&(pos-1)) | pos; // else, new branch for next
					pos <<= 1;
				}
				//(save this bit as part of the current ROM value)
				if (bit) buf[cnt_num*8 + cnt_byte]|=(1<<cnt_bit);
				//(write 'bit' to the 1-wire bus)
				OW_SendBits(bit, 1);
				OW_Wait_TX();
			}
		}
		path=next;
		cnt_num++;
	}while(path && cnt_num < num);
	return cnt_num;
}*/

