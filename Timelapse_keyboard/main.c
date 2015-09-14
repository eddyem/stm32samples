/*
 * main.c
 *
 * Copyright 2015 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "main.h"
#include "usbkeybrd.h"
#include "hardware_ini.h"
#include "uart.h"
#include "GPS.h"
#ifdef ULTRASONIC
#include "ultrasonic.h"
#endif
#include "adc.h"

volatile uint32_t Timer = 0; // global timer (milliseconds)
volatile uint32_t msctr = 0; // global milliseconds for different purposes
volatile int32_t systick_val = 0;
volatile int32_t timer_val = 0;
volatile int need_sync = 1;
volatile uint32_t last_corr_time = 0; // time of last PPS correction (seconds from midnight)

// STK_CVR values for all milliseconds (RVR0) and last millisecond (RVR1)
volatile uint32_t RVR0 = STK_RVR_DEFAULT_VAL, RVR1 = STK_RVR_DEFAULT_VAL;

curtime current_time = {25,61,61};

curtime trigger_time = {25, 61, 61};
curtime adc_time[ADC_CHANNEL_NUMBER] = {{25, 61, 61}, {25, 61, 61}};
#ifdef ULTRASONIC
curtime ultrasonic_time = {25, 61, 61};
#endif
uint32_t trigger_ms = DIDNT_TRIGGERED, adc_ms[ADC_CHANNEL_NUMBER] = {DIDNT_TRIGGERED, DIDNT_TRIGGERED};
#ifdef ULTRASONIC
uint32_t ultrasonic_ms = DIDNT_TRIGGERED;
#endif
void time_increment(){
	Timer = 0;
	if(current_time.H == 25) return; // Time not initialized
	if(++current_time.S == 60){
		current_time.S = 0;
		if(++current_time.M == 60){
			current_time.M = 0;
			if(++current_time.H == 24)
				current_time.H = 0;
		}
	}
}

int main(void){
	uint8_t *string;
	int i;
	rcc_clock_setup_in_hse_8mhz_out_72mhz();
	// init systick (1ms)
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8); // Systyck: 72/8=9MHz
	systick_set_reload(STK_RVR_DEFAULT_VAL); // 9000 pulses: 1kHz
	systick_interrupt_enable();
	systick_counter_enable();

	GPIO_init();
/*
	// if PC11 connected to usb 1.5kOhm pull-up through transistor
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set(GPIOC, GPIO11);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO11);
*/
	usb_disconnect(); // turn off USB while initializing all
	usbkeybrd_setup();
	UART_init(USART2); // init GPS UART
	#ifdef ULTRASONIC
	tim2_init(); // ultrasonic timer
	//tim4_init(); // beeper timer
	#endif
/*
	for (i = 0; i < 0x80000; i++)
		__asm__("nop");
*/
	usb_connect(); // turn on USB
	GPS_send_start_seq();
	init_adc_sensor();
	// time (in milliseconds from MCU start) for trigger, adc & power LED status; power LED blink interval
	// blink time: (1000ms - powerLEDblink) - LED ON
	// GPSstatus_tm - timer for blinking by GPS LED if there's no GPS after timer is good
	// powerLEDblink - LED blinking time (depends on power level)
	uint32_t usbkbrdtm = 0, trigrtm = 0, powerLEDtm = 0, GPSstatus_tm = 0, powerLEDblink = 1;
	// istriggered == 1 after ANY trigger's event (set it to 1 at start to prevent false events)
	// GPSLEDblink - GPS LED blinking
	uint8_t  istriggered = 1, GPSLEDblink = 0;
	while(1){
		poll_usbkeybrd();
		if(usbkbrdtm != msctr){ // process USB not frequently than once per 1ms
			process_usbkbrd();
			usbkbrdtm = msctr;
		}
		#ifdef ULTRASONIC
		poll_ultrasonic();
		#endif
		poll_ADC();
		if((string = check_UART2())){
			GPS_parse_answer(string);
		}
		if(istriggered){ // there was any trigger event
			if(msctr - trigrtm > TRIGGER_DELAY || trigrtm > msctr){ // turn off LED & beeper
				istriggered = 0;
				gpio_set(LEDS_Y_PORT, LEDS_Y1_PIN);
				gpio_set(BEEPER_PORT, BEEPER_PIN);
				trigger_ms = DIDNT_TRIGGERED;
				adc_ms[0] = DIDNT_TRIGGERED;
				adc_ms[1] = DIDNT_TRIGGERED;
				#ifdef ULTRASONIC
				ultrasonic_ms = DIDNT_TRIGGERED;
				#endif
			}
		}else{
			if(trigger_ms != DIDNT_TRIGGERED){
				trigrtm = msctr;
				istriggered = 1;
				P("Button time: ");
				print_time(&trigger_time, trigger_ms);
			}
			//#if 0
			for(i = 0; i < 2; ++i){
				if(adc_ms[i] != DIDNT_TRIGGERED && !istriggered){
					trigrtm = msctr;
					istriggered = 1;
					if(i == 0) P("Infrared");
					else P("Laser");
					P(" time: ");
					print_time(&adc_time[i], adc_ms[i]);
				}
			}
			//#endif
			#ifdef ULTRASONIC
			if(ultrasonic_ms != DIDNT_TRIGGERED){
				trigrtm = msctr;
				istriggered = 1;
				P("Ultrasonic time: ");
				print_time(&ultrasonic_time, ultrasonic_ms);
			}
			#endif
			if(istriggered){ // turn on Y1 LED
				gpio_clear(LEDS_Y_PORT, LEDS_Y1_PIN);
				//beep(); // turn on beeper
				gpio_clear(BEEPER_PORT, BEEPER_PIN);
			}
		}
		// check 12V power level (once per 1ms)
		if(powerLEDtm != msctr){
			uint16_t _12V = ADC_value[2];
			if(_12V < GOOD_POWER_LEVEL){ // insufficient power? - blink LED R2
				// calculate blink time only if there's [was] too low level
				if(_12V < POWER_ALRM_LEVEL || powerLEDblink){
					powerLEDblink = GOOD_POWER_LEVEL - _12V;
					if(powerLEDblink > 900) powerLEDblink = 900; // shadow LED not more than 0.9s
				}
			}else{ // power restored - LED R2 shines
				if(powerLEDblink){
					gpio_clear(LEDS_R_PORT, LEDS_R2_PIN);
					powerLEDblink = 0;
				}
				powerLEDtm = msctr;
			}
			if(powerLEDblink){
				if(GPIO_ODR(LEDS_R_PORT) & LEDS_R2_PIN){ // LED is OFF
					if(msctr - powerLEDtm > powerLEDblink || msctr < powerLEDtm){ // turn LED ON
						powerLEDtm = msctr;
						gpio_clear(LEDS_R_PORT, LEDS_R2_PIN);
					}
				}else{
					if(msctr - powerLEDtm > (1000 - powerLEDblink) || msctr < powerLEDtm){ // turn LED OFF
						powerLEDtm = msctr;
						gpio_set(LEDS_R_PORT, LEDS_R2_PIN);
					}
				}
			}
		}
		// check GPS status to turn on/off GPS LED
		if(current_time.H < 24){ // timer OK
			if(GPS_status != GPS_VALID || need_sync)
				GPSLEDblink = 1;
			else if(GPSLEDblink){
				GPSLEDblink = 0;
				gpio_clear(LEDS_G_PORT, LEDS_G1_PIN); // turn ON G1 LED
			}
			if(GPSLEDblink){
				if(msctr - GPSstatus_tm > 500 || msctr < GPSstatus_tm){
					GPSstatus_tm = msctr;
					if(GPIO_ODR(LEDS_G_PORT) & LEDS_G1_PIN){ // LED is OFF
						gpio_clear(LEDS_G_PORT, LEDS_G1_PIN);
					}else{
						gpio_set(LEDS_G_PORT, LEDS_G1_PIN);
					}
				}
			}
		}else{ // something bad with timer - turn OFF G1 LED
			if(!(GPIO_ODR(LEDS_G_PORT) & LEDS_G1_PIN)){
				gpio_set(LEDS_G_PORT, LEDS_G1_PIN);
			}
		}
	}
}


/**
 * SysTick interrupt: increment global time & send data buffer through USB
 */
void sys_tick_handler(){
	++Timer;
	++msctr;
	if(Timer == 999){
		STK_RVR = RVR1;
	}else if(Timer == 1000){
		STK_RVR = RVR0;
		time_increment();
	}
}
// STK_CVR - current systick val
// STK_RVR - ticks till interrupt - 1

// PA4 interrupt - PPS signal
void exti4_isr(){
	uint32_t t = 0, ticks;
	static uint32_t ticksavr = 0, N = 0;
	if(EXTI_PR & EXTI4){
		// correct
		systick_val = STK_CVR;
		STK_CVR = RVR0;
		timer_val = Timer;
		Timer = 0;
		systick_val = STK_RVR + 1 - systick_val; // Systick counts down!
		if(timer_val < 10) timer_val += 1000; // our closks go faster than real
		else if(timer_val < 990){ // something wrong
			RVR0 = RVR1 = STK_RVR_DEFAULT_VAL;
			STK_RVR = RVR0;
			need_sync = 1;
			goto theend;
		}else
			time_increment(); // ms counter less than 1000 - we need to increment time
		t = current_time.H * 3600 + current_time.M * 60 + current_time.S;
		if(t - last_corr_time == 1){ // PPS interval == 1s
			ticks =  systick_val + (timer_val-1)*(RVR0 + 1) + RVR1 + 1;
			++N;
			ticksavr += ticks;
			if(N > 20){
				ticks = ticksavr / N;
				RVR0 = ticks / 1000 - 1; // main RVR value
				STK_RVR = RVR0;
				RVR1 = RVR0 + ticks % 1000; // last millisecond RVR value (with fine correction)
				N = 0;
				ticksavr = 0;
				need_sync = 0;
			}
		}else{
			N = 0;
			ticksavr = 0;
		}
	theend:
		last_corr_time = t;
		EXTI_PR = EXTI4;
	}
}

/**
 * set current time by buffer hhmmss
 */
void set_time(uint8_t *buf){
	inline uint8_t atou(uint8_t *b){
		return (b[0]-'0')*10 + b[1]-'0';
	}
	uint8_t H = atou(buf) + TIMEZONE_GMT_PLUS;
	if(H > 23) H -= 24;
	current_time.H = H;
	current_time.M = atou(&buf[2]);
	current_time.S = atou(&buf[4]);
}

/**
 * print time: Tm - time structure, T - milliseconds
 */
void print_time(curtime *Tm, uint32_t T){
	int S = Tm->S, M = Tm->M, H = Tm->H;
	if(H < 10) put_char_to_buf('0');
	print_int(H); put_char_to_buf(':');
	if(M < 10) put_char_to_buf('0');
	print_int(M); put_char_to_buf(':');
	if(S < 10) put_char_to_buf('0');
	print_int(S); put_char_to_buf('.');
	if(T < 100) put_char_to_buf('0');
	if(T < 10) put_char_to_buf('0');
	print_int(T);
	P(", ");
	S += H*3600 + M*60;
	print_int(S);
	put_char_to_buf('.');
	if(T < 100) put_char_to_buf('0');
	if(T < 10) put_char_to_buf('0');
	print_int(T);
	if(GPS_status == GPS_NOT_VALID) P(" (not valid)");
	if(need_sync) P(" need synchronisation");
	newline();
}
/*
void print_curtime(){
	uint32_t T = Timer;
	if(current_time.H < 24 && GPS_status != GPS_WAIT){
		P("Current time: ");
		print_time(&current_time, T);
	}else
		P("Waiting for satellites\n");
}
*/
