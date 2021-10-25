/*
 * This file is part of the 3steppers project.
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

#pragma once
#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include <stm32f0.h>

// default CAN bus speed in kbaud
#define DEFAULT_CAN_SPEED       (250)

#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

/** Pinout:
---- Motors' encoders ---------------
PA0  Enc2a (motor2 encoder) - TIM2CH1/2 [AF2]
PA1  Enc2b [AF2]
PA8  Enc1a (motor1 encoder) - TIM1CH1/2 [AF2]
PA9  Enc1b [AF2]
PB4  Enc3a (motor3 encoder) - TIM3CH1/2 [AF1]
PB5  Enc3b [AF1]
---- Motors' clocks + PWM -----------
PA2  CLK1 (motor1 clock) - TIM15CH1 [AF0]
PA4  CLK2 (motor2 clock) - TIM14CH1 [AF4]
PA6  CLK3 (motor3 clock) - TIM16CH1 [AF5]
PA7  PWM  (opendrain PWM, up to 12V) - TIM17CH1 [AF5]
---- GPIO out (push-pull) -----------
PB0  ~EN1 (motor1 not enable)
PB1  DIR1 (motor1 direction)
PB2  ~EN2 (motor2 not enable)
PB3  Buzzer (external buzzer or other non-inductive opendrain load up to 12V)
PB10 DIR2 (motor2 direction)
PB11 ~EN3 (motor3 not enable)
PB12 DIR3 (motor3 direction)
PB13 Ext0 (3 external outputs: 5V, up to 20mA)
PB14 Ext1
PB15 Ext2
PF0  Relay (10A 250VAC, 10A 30VDC)
---- GPIO in -----------------------
PA10 BTN1 (user button 1) - pullup
PA13 BTN2 (user button 2) - pullup
PA14 BTN3 (user button 3) - pullup
PA15 BTN4 (user button 4) - pullup
PC13 ESW1 (motor1 zero limit switch) - pulldown
PC14 ESW2 (motor2 zero limit switch) - pulldown
PC15 ESW3 (motor3 zero limit switch) - pulldown
---- ADC ----------------------------
PA3  ADC1 (ADC1 in, 0-3.3V) [ADC3]
PA5  ADC2 (ADC2 in, 0-3.3V) [ADC5]
---- USB ----------------------------
PA11 USBDM
PA12 USBDP
---- I2C ----------------------------
PB6  I2C SCL (external I2C bus, have internal pullups of 4.7kOhm to +3.3V) - I2C1 [AF1]
PB7  I2C SDA [AF1]
---- CAN ----------------------------
PB8  CAN Rx (external CAN bus, with local galvanic isolation) [AF4]
PB9  CAN Tx [AF4]

COMMON setup:
PORT  FN   AFR[1]idx
PA0   AF2
PA1   AF2
PA2   AF0
PA3   AI
PA4   AF4
PA5   AI
PA6   AF5
PA7   AF5
PA8   AF2   0
PA9   AF2   1
PA10  PU
PA11  USB
PA12  USB
PA13  PU
PA14  PU
PA15  PU
PB0   PP
PB1   PP
PB2   PP
PB3   PP
PB4   AF1
PB5   AF1
PB6   AF1
PB7   AF1
PB8   CAN
PB9   CAN
PB10  PP
PB11  PP
PB12  PP
PB13  PP
PB14  PP
PB15  PP
PC13  PD
PC14  PD
PC15  PD
PF0   PP
**/

// buzzer
#define BUZZERport  (GPIOB)
#define BUZZERpin   (1<<3)
// relay
#define RELAYport   (GPIOF)
#define RELAYpin    (1<<0)

// ON(RELAY), OFF(BUZZER) etc
#define ON(x)       do{pin_set(CONCAT(x, port), CONCAT(x, pin));}while(0)
#define OFF(x)      do{pin_clear(CONCAT(x, port), CONCAT(x, pin));}while(0)
#define TGL(x)      do{pin_toggle(CONCAT(x, port), CONCAT(x, pin));}while(0)
#define CHK(x)      (pin_read(CONCAT(x, port), CONCAT(x, pin)))

// max value of PWM
#define PWMMAX          (255)
// max index of PWM channels
#define PWMCHMAX        (0)
#define PWMset(val)     do{TIM17->CCR1 = val;}while(0)
#define PWMget()        (TIM17->CCR1)

// extpins amount
#define EXTNO       (3)
extern volatile GPIO_TypeDef *EXTports[EXTNO];
extern const uint32_t   EXTpins[EXTNO];
#define EXT_SET(x)      do{ pin_set(EXTports[x], EXTpins[x]); }while(0)
#define EXT_CLEAR(x)    do{ pin_clear(EXTports[x], EXTpins[x]); }while(0)
#define EXT_TOGGLE(x)   do{ pin_toggle(EXTports[x], EXTpins[x]); }while(0)
#define EXT_CHK(x)      (pin_read(EXTports[x], EXTpins[x]))

// Buttons amount
#define BTNSNO      (4)
// Buttons ports & pins
extern volatile GPIO_TypeDef *BTNports[BTNSNO];
extern const uint32_t BTNpins[BTNSNO];
// state 1 - pressed, 0 - released (pin active is zero)
#define BTN_state(x)    ((BTNports[x]->IDR & BTNpins[x]) ? 0 : 1)

// Limit switches
#define ESWNO       (3)
// ESW ports & pins
extern volatile GPIO_TypeDef *ESWports[ESWNO];
extern const uint32_t ESWpins[ESWNO];
// state 1 - pressed, 0 - released (pin active is zero)
#define ESW_state(x)    ((ESWports[x]->IDR & ESWpins[x]) ? 0 : 1)

// motors
#define MOTORSNO    (3)
extern volatile GPIO_TypeDef *ENports[MOTORSNO];
extern const uint32_t ENpins[MOTORSNO];
#define MOTOR_EN(x)  do{ pin_clear(ENports[x], ENpins[x]); }while(0)
#define MOTOR_DIS(x) do{ pin_set(ENports[x], ENpins[x]); }while(0)
extern volatile GPIO_TypeDef *DIRports[MOTORSNO];
extern const uint32_t DIRpins[MOTORSNO];
#define MOTOR_CW(x)  do{ pin_set(DIRports[x], DIRpins[x]); }while(0)
#define MOTOR_CCW(x) do{ pin_clear(DIRports[x], DIRpins[x]); }while(0)

extern volatile uint32_t Tms;

// timers of motors
extern TIM_TypeDef *mottimers[];
// timers for encoders
extern TIM_TypeDef *enctimers[];

void gpio_setup();
void iwdg_setup();
void timers_setup();
void pause_ms(uint32_t pause);
void Jump2Boot();

#endif // __HARDWARE_H__
