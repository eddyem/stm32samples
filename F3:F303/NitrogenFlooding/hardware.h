/*
 * This file is part of the nitrogen project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#pragma once

#include <stm32f3.h>

// "PCLK" frequency
// really APB1=36M -> TIM2/3/4/6/7 f=72M, APB2=72M -> TIM1/8/15/16/17/20 f=72M
#ifndef PCLK
#define PCLK    (72000000)
#endif

// Max PWM CCR1 value (->1)
#define PWM_CCR_MAX (100)

// USB pullup: PC9
#define USBPU_port  GPIOC
#define USBPU_pin   (1<<9)
#define USBPU_ON()  pin_clear(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_set(USBPU_port, USBPU_pin)

// LEDs PE8, PE9, PE10, PE11
#define CONC(a,b,c)  a ## b ## c
#define MKPIN(a,n)   CONC(a, n, _pin)
#define LEDs_port   (GPIOE)
#define LEDS_AMOUNT (4)
#define LED0_pin    (1<<8)
#define LED1_pin    (1<<9)
#define LED2_pin    (1<<10)
#define LED3_pin    (1<<11)
#define LED_blink(x)  do{if(LEDsON) pin_toggle(LEDs_port, 1<<(8+x));}while(0)
#define LED_off(x)  do{pin_set(LEDs_port, 1<<(8+x));}while(0)
#define LED_on(x)  do{if(LEDsON) pin_clear(LEDs_port, 1<<(8+x));}while(0)
#define LED_get(x)  (LEDs_port->IDR & 1<<(8+x) ? 0 : 1)
#define LEDS_OFF()  do{LEDsON = 0; for(int _ = 0; _ < 4; ++_) LED_off(_);}while(0)
#define LEDS_ON()   do{LEDsON = 1;}while(0)

// screen LEDon/off - PB12; reset - PB11; data/command - PB10
#define SCRN_LED_pin    (1<<12)
#define SCRN_LED_port   (GPIOB)
#define SCRN_LED_set(a) do{if(a) pin_set(SCRN_LED_port, SCRN_LED_pin); else pin_clear(SCRN_LED_port, SCRN_LED_pin);}while(0)
#define SCRN_LED_get()  (SCRN_LED_port->IDR & SCRN_LED_pin ? 1: 0)
#define SCRN_CS_pin     (1<<11)
#define SCRN_CS_port    (GPIOB)
#define SCRN_CS_set(a)  do{if(a) pin_set(SCRN_CS_port, SCRN_CS_pin); else pin_clear(SCRN_CS_port, SCRN_CS_pin);}while(0)
#define SCRN_CS_get()  (SCRN_CS_port->IDR & SCRN_CS_pin ? 1: 0)
#define SCRN_DCX_pin    (1<<10)
#define SCRN_DCX_port   (GPIOB)
#define SCRN_DCX_get()  (SCRN_DCX_port->IDR & SCRN_DCX_pin ? 1: 0)
#define SCRN_Data()     do{pin_set(SCRN_DCX_port, SCRN_DCX_pin);}while(0)
#define SCRN_Command()  do{pin_clear(SCRN_DCX_port, SCRN_DCX_pin);}while(0)

// Buttons amount
#define BTNSNO      (7)
#define BTNs_port   GPIOD
// Buttons: PD9, PD10, PD11, PD12, PD13, PD14, PD15: pullup (active - 0)
#define BTN0_pin    (1<<9)
#define BTN1_pin    (1<<10)
#define BTN2_pin    (1<<11)
#define BTN3_pin    (1<<12)
#define BTN4_pin    (1<<13)
#define BTN5_pin    (1<<14)
#define BTN6_pin    (1<<15)
// state 0 - pressed, 1 - released
#define BTN_state(x)    (BTNs_port->IDR & 1<<(9+x) ? 0 : 1)
// timeout to turn off screen and LEDs after no keys activity - 300 seconds
#define BTN_ACTIVITY_TIMEOUT    (300000)
// refresh interval for BME280 and other data  - 2.5s
#define SENSORS_DATA_TIMEOUT    (2500)
// refresh interval for screen windows
#define WINDOW_REFRESH_TIMEOUT  (1000)

typedef uint16_t btnevtmask; // event mask: less 8 - press, high 8 - hold
// buttons masks bit0 - button0 etc
#define BTN_ESC_MASK    (1<<0)
#define BTN_LEFT_MASK   (1<<1)
#define BTN_RIGHT_MASK  (1<<2)
#define BTN_SEL_MASK    (1<<3)
#define BTN_PRESS(e, mask)  ((e) & (mask))
#define BTN_HOLD(e, mask)   ((e) & ((mask) << 8))
#define BTN_PRESSHOLD(e, mask)  ((e) & ((mask) | ((mask)<<8)))

// global sensors' data
extern float Temperature, Pressure, Humidity, Dewpoint;
extern uint32_t Sens_measured_time;
extern uint16_t ADCvals[];

// buzzer, ADC voltage
#define BUZZER_port     GPIOB
#define BUZZER_pin      (1<<0)
#define BUZZER_ON()     do{pin_clear(BUZZER_port, BUZZER_pin);}while(0)
#define BUZZER_OFF()    do{pin_set(BUZZER_port, BUZZER_pin);}while(0)
#define BUZZER_STATE()  (BUZZER_port->IDR & BUZZER_pin ? 0 : 1)
#define ADCON_port      GPIOF
#define ADCON_pin       (1<<10)
#define ADCON(x)        do{if(x==0) pin_set(ADCON_port, ADCON_pin); else pin_clear(ADCON_port, ADCON_pin);}while(0)
#define ADCONSTATE()    (ADCON_port->IDR & ADCON_pin ? 0 : 1)

extern volatile uint32_t Tms;
extern int LEDsON;

uint8_t MSB(uint16_t val);
void hw_setup();

void setPWM(int nch, uint16_t val);
uint16_t getPWM(int nch);

#ifndef EBUG
void iwdg_setup();
#endif
