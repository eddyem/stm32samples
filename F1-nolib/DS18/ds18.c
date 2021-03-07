/*
 * This file is part of the DS18 project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ds18.h"
#include "proto.h"
#include "usb.h"

//#define EBUG
// ID1: 0x28 0xba 0xc7 0xea 0x05 0x00 0x00 0xc2
// ID2: 0x28 0x68 0xc9 0xe9 0x05 0x00 0x00 0x42
// ID3: 0x28 0x23 0xef 0xe9 0x05 0x00 0x00 0xab


#ifdef EBUG
#define DBG(x)  do{USB_send(x);}while(0)
#else
#define DBG(x)
#endif

/* TIMING */
// freq = 1MHz
// ARR values: 1000 for reset, 100 for data in/out
// CCR1 values: 500 for reset, 60 for sending 0 or reading, <15 for sending 1
// CCR2 values: >550 if there's devices on line (on reset), >12 (typ.15) - read 0, < 12 (typ.1) - read 1
#define RESET_LEN         ((uint16_t)1000)
#define BIT_LEN           ((uint16_t)100)
#define RESET_P           ((uint16_t)500)
#define BIT_ONE_P         ((uint16_t)10)
#define BIT_ZERO_P        ((uint16_t)80)
#define BIT_READ_P        ((uint16_t)5)
#define RESET_BARRIER     ((uint16_t)550)
#define ONE_ZERO_BARRIER  ((uint16_t)10)

// answer for polling: conversion done
#define CONVERSION_DONE     (0xff)

/*
 * thermometer commands
 * send them with bus reset!
 */
// find devices
#define OW_SEARCH_ROM       (0xf0)
// read device (when it is alone on the bus)
#define OW_READ_ROM         (0x33)
// send device ID (after this command - 8 bytes of ID)
#define OW_MATCH_ROM        (0x55)
// broadcast command
#define OW_SKIP_ROM         (0xcc)
// find devices with critical conditions
#define OW_ALARM_SEARCH     (0xec)
/*
 * thermometer functions
 * send them without bus reset!
 */
// start themperature reading
#define OW_CONVERT_T         (0x44)
// write critical temperature to device's RAM
#define OW_SCRATCHPAD        (0x4e)
// read whole device flash
#define OW_READ_SCRATCHPAD   (0xbe)
// copy critical themperature from device's RAM to its EEPROM
#define OW_COPY_SCRATCHPAD   (0x48)
// copy critical themperature from EEPROM to RAM (when power on this operation runs automatically)
#define OW_RECALL_E2         (0xb8)
// check whether there is devices wich power up from bus
#define OW_READ_POWER_SUPPLY (0xb4)

/*
 * thermometer identificator is: 8bits CRC, 48bits serial, 8bits device code (10h)
 * Critical temperatures is T_H and T_L
 * T_L is lowest allowed temperature
 * T_H is highest -//-
 * format T_H and T_L: 1bit sigh + 7bits of data
 */


/*
 * RAM register:
 * 0 - themperature: 1 ADU == 0.5 degrC
 * 1 - sign (0 - T>0 degrC ==> T=byte0; 1 - T<0 degrC ==> T=byte0-0xff+1)
 * 2 - T_H
 * 3 - T_L
 * 4 - 0xff (reserved)
 * 5 - 0xff (reserved)
 * 6 - COUNT_REMAIN (0x0c)
 * 7 - COUNT PER DEGR (0x10)
 * 8 - CRC
 */

// max amount of bits (20bytes x 8bits)
#define NmeasurementMax     (160)
// reset pulse length (ms+1..2)
#define DS18_RESETPULSE_LEN (3)
// max length of measurement (ms+1..2)
#define DS18_MEASUR_LEN     (800)
// maximal received data bytes
#define IMAXCTR             (10)

static uint8_t DS18ID[8] = {0}, matchID = 0;

static void (*ow_process_resdata)() = NULL; // what to do with received data

static DS18_state dsstate = DS18_SLEEP;
static uint32_t Tstart = 0; // time of start pulse
// sent data,  + 1 zero for last
static uint8_t CC1array[NmeasurementMax];
static uint8_t totbytesctr = 0; // total (i/o) data bytes counter
static int cc1buff_ctr = 0;
// received data
static uint8_t CC2array[NmeasurementMax];
static uint8_t receivectr = 0; // data bytes amount to receive
// prepare buffers to sending
#define OW_reset_buffer()   do{cc1buff_ctr = 0; receivectr = 0; totbytesctr = 0;}while(0)

// several devices on line
void DS18_setID(const uint8_t ID[8]){
    uint32_t *O = (uint32_t*)DS18ID, *I = (uint32_t*)ID;
    *O++ = *I++; *O = *I;
    matchID = 1;
#ifdef EBUG
    USB_send("DS18_setID: ");
    for(int i = 0; i < 8; ++i){
        USB_send(" 0x");
        printhex(DS18ID[i]);
    }
    USB_send("\n");
#endif
}
// the only device on line
void DS18_clearID(){
    matchID = 0;
}

/**
 * add next data byte
 * @param ow_byte - byte to convert
 */
static uint8_t OW_add_byte(uint8_t ow_byte){
    uint8_t i, byte;
	for(i = 0; i < 8; i++){
		if(ow_byte & 0x01){
			byte = BIT_ONE_P;
		}else{
			byte = BIT_ZERO_P;
		}
		if(cc1buff_ctr == NmeasurementMax){
			DBG("Tim2 buffer overflow\n");
			return 0; // avoid buffer overflow
		}
		CC1array[cc1buff_ctr++] = byte;
		ow_byte >>= 1;
	}
    ++totbytesctr;
	return 1;
}

/**
 * Adds Nbytes bytes 0xff  for reading sequence
 */
static uint8_t OW_add_read_seq(uint8_t Nbytes){
	uint8_t i;
	if(Nbytes == 0) return 0;
    totbytesctr += Nbytes;
    receivectr += Nbytes;
	Nbytes *= 8; // 8 bits for each byte
	for(i = 0; i < Nbytes; i++){
		if(cc1buff_ctr == NmeasurementMax){
			DBG("Tim2 buffer overflow\n");
			return 0;
		}
		CC1array[cc1buff_ctr++] = BIT_READ_P;
	}
    DBG("add rseq, rctr="); DBG(i2str(receivectr)); DBG("\n");
	return 1;
}

// read received data (not more than IMAXCTR bytes!)
static uint8_t *OW_readbuf(){
#ifdef EBUG
    DBG("ctr1="); DBG(u2str(DMA1_Channel2->CNDTR)); DBG("\n");
    DBG("ctr2="); DBG(u2str(DMA1_Channel3->CNDTR)); DBG("\n");
    DBG("\nn\tCC1\tCC2\n");
    for(int i = 0; i < cc1buff_ctr; ++i){
        DBG(u2str(i)); DBG("\t"); DBG(u2str(CC1array[i]));
        DBG("\t"); DBG(u2str(CC2array[i])); DBG("\n");
    }
#endif
    static uint8_t got[IMAXCTR];
    uint8_t *gptr = got;
    DBG("rctr="); DBG(i2str(receivectr)); DBG("\n");
    if(receivectr > IMAXCTR || receivectr == 0) return NULL;
    uint8_t startidx = (totbytesctr - receivectr) * 8;
    for(int i = startidx; i < cc1buff_ctr;){
        uint8_t byte = 0;
        for(int j = 0; j < 8; ++j){
            byte >>= 1;
            if(CC2array[i++] < ONE_ZERO_BARRIER) byte |= 0x80;
        }
        *gptr++ = byte;
#ifdef EBUG
        USB_send("byte: "); printhex(byte); USB_send("\n");
#endif
    }
    return got;
}

DS18_state DS18_getstate(){return dsstate;}

static void DS18_detect(){
    DBG("DS18_detect()\n");
    TIM1->ARR = RESET_LEN;
    TIM1->CCR1 = RESET_P;
    TIM1->CCR2 = 0; // after interrupt it should be RESET_P+Tpdhigh+Tpdlow (575..800ms)
    TIM1->EGR = TIM_EGR_UG; // generate update to put CCR1 from buffer into register
    TIM1->SR = 0;
    // enable update IRQ
    TIM1->DIER = TIM_DIER_UIE;
    TIM1->CCR1 = 0; // next time level should be max
    // PA8 as AF opendrain output
    GPIOA->CRH = (GPIOA->CRH & ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8)) |
            CRH(8, CNF_AFOD | MODE_FAST);
    // turn TIM1 on in OPM mode
    TIM1->CR1 = TIM_CR1_CEN | TIM_CR1_OPM | TIM_CR1_URS;
    dsstate = DS18_DETECTING;
}

static void DS18_getReg(int n){
    uint8_t *r = OW_readbuf();
    if(!r || receivectr != n){
        DBG("Bad read value!\n");
        return;
    }
    printsp(r, n);
}

static int chkCRC(const uint8_t data[9]){
    uint8_t crc = 0;
    for(int n = 0; n < 8; ++n){
        crc = crc ^ data[n];
           for(int i = 0; i < 8; i++){
               if(crc & 1)
                   crc = (crc >> 1) ^ 0x8C;
               else
                   crc >>= 1;
           }
    }
    //USB_send("got crc: "); printhex(crc); USB_send("\n");
    if(data[8] == crc) return 0;
    return 1;
}

// data processing functions
static void DS18_gettemp(){ // calculate T
    dsstate = DS18_SLEEP;
#ifdef EBUG
    for(int i = 0; i < NmeasurementMax; ++i){
        DBG(u2str(i)); DBG("\t"); DBG(u2str(CC2array[i])); DBG("\n");
    }
#endif
    uint8_t *r = OW_readbuf();
    if(!r || receivectr != 9){
        DBG("Bad count");
        dsstate = DS18_ERROR;
        return;
    }
    int ctr;
    for(ctr = 0; ctr < 9; ++ctr){
        if(r[ctr] != 0xff) break;
    }
    if(ctr == 9){
        USB_send("Target ID not found");
        return;
    }
    if(chkCRC(r)){
        USB_send("CRC is wrong\n");
        return;
    }
    uint16_t l = r[0], m = r[1], v;
    int32_t t;
    if(r[4] == 0xff){ // DS18S20
        t = ((uint32_t)l) * 10;
        t >>= 1;
	}else{ // DS18B20
        v = ((m & 7) << 8)| l;
        t = ((uint32_t)v)*10;
        t >>= 4;
	}
    if(m & 0x80) t = -t;
    printT(t);
}
static void DS18_pollt(){ // poll T
    uint8_t *r;
    if((r = OW_readbuf()) && receivectr == 1){
        if(*r == CONVERSION_DONE){
            OW_reset_buffer();
            if(matchID){ // add MATCH command & target ID
                OW_add_byte(OW_MATCH_ROM);
                for(int i = 0; i < 8; ++i)
                    OW_add_byte(DS18ID[i]);
            }else{
                OW_add_byte(OW_SKIP_ROM);
            }
            OW_add_byte(OW_READ_SCRATCHPAD);
            OW_add_read_seq(9);
            ow_process_resdata = DS18_gettemp;
            DS18_detect(); // reset
            return;
        }
    }
    OW_reset_buffer();
    OW_add_read_seq(1); // send read seq waiting for end of conversion
    ow_process_resdata = DS18_pollt;
    dsstate = DS18_RDYTOSEND;
}
static void DS18_getID(){
    DS18_getReg(8);
}
static void DS18_getSP(){
    DS18_getReg(9);
}

// TIM1_CH1 & TIM1_CH2 are used for data sending & pulse length measurement
// T1ch1 - DMA1Ch2, T1ch2 - DMA1Ch3
void DS18_pinsetup(){
    TIM1->CR1 = 0;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_TIM1EN | RCC_APB2ENR_AFIOEN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    pin_set(GPIOA, 1<<8); // 1 @ line
    // PA8 as opendrain output
    GPIOA->CRH = (GPIOA->CRH & ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8)) |
            CRH(8, CNF_ODOUTPUT | MODE_FAST);
    TIM1->PSC = 71; // 1MHz
    // CC1 is output (PWM mode 1, active->inactive, enable preload), CC2 is input @TI1
    TIM1->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE |
            TIM_CCMR1_CC2S_1;
    // enable CC1 & CC2, CC1 active low
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC1P;
    TIM1->DIER = 0; // disable IRQ & DMA events
    // main output enable
    TIM1->BDTR = TIM_BDTR_MOE;
    // T1Ch2: per->mem, T1Ch1: mem->per; 16bit->8bit,
    DMA1_Channel2->CCR = DMA_CCR_MINC | DMA_CCR_PSIZE_0 | DMA_CCR_TCIE | DMA_CCR_DIR;
    DMA1_Channel3->CCR = DMA_CCR_MINC | DMA_CCR_PSIZE_0 | DMA_CCR_TCIE;
    DMA1_Channel2->CPAR = (uint32_t)&TIM1->CCR1;
    DMA1_Channel3->CPAR = (uint32_t)&TIM1->CCR2;
    TIM1->CR1 = TIM_CR1_URS; // only ARR overflow generates interrupt

    NVIC_EnableIRQ(TIM1_UP_IRQn); // last interrupt to turn timer off
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    dsstate = DS18_SLEEP;
}

// start measurement
static void DS18_startmeas(uint32_t Tms){
    DBG("startmeas()\n");
    dsstate = DS18_READING;
    Tstart = Tms;
    DMA1->IFCR = DMA_IFCR_CGIF1 | DMA_IFCR_CGIF2; // clear all flags
    DMA1_Channel2->CMAR = (uint32_t)CC1array;
    DMA1_Channel3->CMAR = (uint32_t)CC2array;
    DMA1_Channel2->CNDTR = cc1buff_ctr+1; // +1 for last works
    DMA1_Channel3->CNDTR = cc1buff_ctr;
    // enable both DMA channels
    DMA1_Channel2->CCR |= DMA_CCR_EN;
    DMA1_Channel3->CCR |= DMA_CCR_EN;
    // enable  DMA events generation
    TIM1->DIER = TIM_DIER_CC1DE | TIM_DIER_CC2DE;
    TIM1->ARR = BIT_LEN;
    TIM1->SR = 0;
    GPIOA->CRH = (GPIOA->CRH & ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8)) |
            CRH(8, CNF_AFOD | MODE_FAST);
    // turn TIM1 on
    TIM1->CR1 = TIM_CR1_CEN | TIM_CR1_URS;
}


static void DS18_stopmeas(){
    DBG("stopmeas()\n");
    // stop timer
    TIM1->CR1 = 0;
    // disable DMA channel
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
#ifdef EBUG
    DBG("ctr="); DBG(u2str(DMA1_Channel2->CNDTR)); DBG("\nn\tCC1\n");
    int nmax = NmeasurementMax - DMA1_Channel2->CNDTR;
    for(int i = 0; i < nmax; ++i){
        DBG(u2str(i)); DBG("\t"); DBG(u2str(CC1array[i])); DBG("\n");
    }
#endif
    dsstate = DS18_ERROR;
}

// processing, Tms - current time in milliseconds
void DS18_process(uint32_t Tms){
    switch(dsstate){
        case DS18_DETDONE:
            DBG("TIM1->CCR2="); DBG(u2str(TIM1->CCR2)); DBG("\n");
           /* DBG("SR="); DBG(u2str(TIM1->SR)); DBG("\n");
            if(TIM1->SR & TIM_SR_CC2OF){
                TIM1->SR = 0; DBG("Overcapture!\n");
            }*/
            if(TIM1->CCR2 > RESET_BARRIER) DS18_startmeas(Tms);
            else dsstate = DS18_ERROR;
        break;
        case DS18_RDYTOSEND: // send data without reset pulse
            DS18_startmeas(Tms);
        break;
        case DS18_READING:
            if(Tms - Tstart > DS18_MEASUR_LEN)
                DS18_stopmeas();
        break;
        case DS18_GOTANSWER:
            // disable DMA channels
            DMA1_Channel2->CCR &= ~DMA_CCR_EN;
            DMA1_Channel3->CCR &= ~DMA_CCR_EN;
            dsstate = DS18_SLEEP;
            if(ow_process_resdata) ow_process_resdata();
        break;
        default:
            return;
    }
}

int DS18_start(){
    if(dsstate != DS18_SLEEP && dsstate != DS18_ERROR) return 0;
    OW_reset_buffer();
    OW_add_byte(OW_SKIP_ROM);
	OW_add_byte(OW_CONVERT_T);
	OW_add_read_seq(1); // send read seq waiting for end of conversion
    DBG("start()\n");
    ow_process_resdata = DS18_pollt; // after data will be done calculate T and show it
    DS18_detect();
    return 1;
}

int DS18_readID(){
    if(dsstate != DS18_SLEEP && dsstate != DS18_ERROR) return 0;
    OW_reset_buffer();
	OW_add_byte(OW_READ_ROM);
	OW_add_read_seq(8); // wait for 8 bytes
    ow_process_resdata = DS18_getID;
    DBG("readID()\n");
    DS18_detect();
    return 1;
}

int DS18_readScratchpad(){
    if(dsstate != DS18_SLEEP && dsstate != DS18_ERROR) return 0;
    OW_reset_buffer();
    OW_add_byte(OW_SKIP_ROM);
	OW_add_byte(OW_READ_SCRATCHPAD);
	OW_add_read_seq(9);
    ow_process_resdata = DS18_getSP;
    DBG("readSP()\n");
    DS18_detect();
    return 1;
}

void dma1_channel2_isr(){ // wait for last data bit
    DMA1->IFCR = DMA_IFCR_CGIF2;
    TIM1->CR1 |= TIM_CR1_OPM;
    TIM1->CCR1 = 0; // don't go to zero
    TIM1->SR &= ~TIM_SR_UIF; // clear update flag (or interrupt will run just now)
    TIM1->DIER |= TIM_DIER_UIE;
}

// update IRQ
void tim1_up_isr(){
    TIM1->SR = 0; // clear all flags (and overcapture too!) &= ~TIM_SR_UIF;
    TIM1->CR1 = 0;
    if(dsstate == DS18_DETECTING) dsstate = DS18_DETDONE;
    else if(dsstate == DS18_READING) dsstate = DS18_GOTANSWER;
}

void DS18_poll(){
    OW_reset_buffer();
    OW_add_read_seq(1); // send read seq waiting for end of conversion
    ow_process_resdata = DS18_pollt;
    dsstate = DS18_RDYTOSEND;
}
