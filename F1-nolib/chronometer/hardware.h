/*
 *                                                                                                  geany_encoding=koi8-r
 * hardware.h
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */
#pragma once
#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "stm32f1.h"
#include "time.h"

// onboard LEDs - PB8/PB9
#define LED0_port   GPIOB
#define LED0_pin    (1<<8)
#define LED1_port   GPIOB
#define LED1_pin    (1<<9)

// buzzer (1 - active) - PC13
extern uint8_t buzzer_on;
#define BUZZER_port GPIOC
#define BUZZER_pin  (1<<13)
#define BUZZER_ON() do{if(buzzer_on)pin_set(BUZZER_port, BUZZER_pin);}while(0)
#define BUZZER_OFF() pin_clear(BUZZER_port, BUZZER_pin)
#define BUZZER_GET() (pin_read(BUZZER_port, BUZZER_pin))
// minimal time to buzzer to cheep (ms)
#define BUZZER_CHEEP_TIME   (500)

// PPS pin - PA1
#define PPS_port    GPIOA
#define PPS_pin     (1<<1)

// PPS and triggers state
// amount of triggers, should be less than 9; 5 - 0..2 - switches, 3 - LIDAR, 4 - ADC
#define TRIGGERS_AMOUNT     (5)
// number of LIDAR trigger
#define LIDAR_TRIGGER       (3)
// number of ADC trigger
#define ADC_TRIGGER         (4)
// amount of digital triggers (on interrupts)
#define DIGTRIG_AMOUNT      (3)
// max length of trigger event (ms)
#define MAX_TRIG_LEN        (1000)

#ifdef EBUG
uint8_t gettrig(uint8_t N);
#endif
void fillshotms(int i);
void fillunshotms();
void savetrigtime();
#define GET_PPS()       ((GPIOA->IDR & (1<<1)) ? 1 : 0)

// USB pullup - PA15
#define USBPU_port  GPIOA
#define USBPU_pin   (1<<15)
#define USBPU_ON()  pin_clear(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_set(USBPU_port, USBPU_pin)

#define LED_blink()    do{if(LEDSon)pin_toggle(LED0_port, LED0_pin);}while(0)
#define LED_on()       do{if(LEDSon)pin_clear(LED0_port, LED0_pin);}while(0)
#define LED_off()      do{pin_set(LED0_port, LED0_pin);}while(0)
#define LED1_blink()   do{if(LEDSon)pin_toggle(LED1_port, LED1_pin);}while(0)
#define LED1_on()      do{if(LEDSon)pin_clear(LED1_port, LED1_pin);}while(0)
#define LED1_off()     do{pin_set(LED1_port, LED1_pin);}while(0)

// GPS USART == USART2, LIDAR USART == USART3
#define GPS_USART   (2)
#define LIDAR_USART (3)

typedef struct{
    uint32_t millis;
    curtime Time;
} trigtime;

// turn on/off LEDs:
extern uint8_t LEDSon;
// time of triggers shot
extern trigtime shottime[TRIGGERS_AMOUNT];
// length (in ms) of trigger event (-1 if > MAX_TRIG_LEN
extern int16_t triglen[TRIGGERS_AMOUNT];
// if trigger[N] shots, the bit N will be 1
extern uint8_t trigger_shot;

void chk_buzzer();
void hw_setup();

#endif // __HARDWARE_H__
