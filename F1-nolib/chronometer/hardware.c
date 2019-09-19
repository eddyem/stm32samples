/*
 *                                                                                                  geany_encoding=koi8-r
 * hardware.c - hardware-dependent macros & functions
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

#include "adc.h"
#include "hardware.h"
#include "flash.h"
#include "lidar.h"
#include "str.h"
#include "time.h"
#include "usart.h"

#include <string.h> // memcpy

uint8_t buzzer_on = 1; // buzzer ON by default
uint8_t LEDSon = 1; // LEDS are working
// ports of triggers
static GPIO_TypeDef *trigport[DIGTRIG_AMOUNT] = {GPIOA, GPIOA, GPIOA};
// pins of triggers: PA13, PA14, PA4
static uint16_t trigpin[DIGTRIG_AMOUNT] = {1<<13, 1<<14, 1<<4};
// value of pin in `triggered` state
static uint8_t trigstate[DIGTRIG_AMOUNT];
// time of triggers shot
trigtime shottime[TRIGGERS_AMOUNT];
// Tms value when they shot
static uint32_t shotms[TRIGGERS_AMOUNT];
// trigger length (-1 if > MAX_TRIG_LEN)
int16_t triglen[TRIGGERS_AMOUNT];
// if trigger[N] shots, the bit N will be 1
uint8_t trigger_shot = 0;

static inline void gpio_setup(){
    BUZZER_OFF(); // turn off buzzer @start
    LED_on(); // turn ON LED0 @start
    LED1_off(); // turn off LED1 @start
    USBPU_OFF(); // turn off USB pullup @start
    // Enable clocks to the GPIO subsystems (PB for ADC), turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    // turn off SWJ/JTAG
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_DISABLE;
    // pullups: PA1 - PPS, PA15 - USB pullup
    GPIOA->ODR = (1<<12)|(1<<15);
    // buzzer (PC13): pushpull output
    GPIOC->CRH = CRH(13, CNF_PPOUTPUT|MODE_SLOW);
    // Set leds (PB8) as opendrain output
    GPIOB->CRH = CRH(8, CNF_ODOUTPUT|MODE_SLOW) | CRH(9, CNF_ODOUTPUT|MODE_SLOW);
    // PPS pin (PA1) - input with weak pullup
    GPIOA->CRL = CRL(1, CNF_PUDINPUT|MODE_INPUT);
    // Set USB pullup (PA15) - opendrain output
    GPIOA->CRH = CRH(15, CNF_ODOUTPUT|MODE_SLOW);
    // ---------------------> config-depengent block, interrupts & pullup inputs:
    GPIOA->CRH |= CRH(13, CNF_PUDINPUT|MODE_INPUT) | CRH(14, CNF_PUDINPUT|MODE_INPUT);
    GPIOA->CRL |= CRL(4, CNF_PUDINPUT|MODE_INPUT);
    // <---------------------
    // EXTI: all three EXTI are on PA -> AFIO_EXTICRx = 0
    // interrupt on pulse front: buttons - 1->0, PPS - 0->1
    EXTI->IMR = EXTI_IMR_MR1;
    EXTI->RTSR = EXTI_RTSR_TR1; // rising trigger
    // PA4/PA13/PA14 - buttons
    for(int i = 0; i < DIGTRIG_AMOUNT; ++i){
        uint16_t pin = trigpin[i];
        // fill trigstate array
        uint8_t trgs = (the_conf.trigstate & (1<<i)) ? 1 : 0;
        trigstate[i] = trgs;
        trigport[i]->ODR |= pin; // turn on pullups
        EXTI->IMR |= pin;
        if(trgs){ // triggered @1 -> rising interrupt
            EXTI->RTSR |= pin;
        }else{ // falling interrupt
            EXTI->FTSR |= pin;
        }
    }
    // ---------------------> config-depengent block, interrupts & pullup inputs:
    // !!! change AFIO_EXTICRx if some triggers not @GPIOA
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    // <---------------------
    NVIC_EnableIRQ(EXTI1_IRQn);
}

static inline void adc_setup(){
    GPIOB->CRL |= CRL(0, CNF_ANALOG|MODE_INPUT);
    uint32_t ctr = 0;
    // Enable clocking
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CFGR &= ~(RCC_CFGR_ADCPRE);
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV8; // ADC clock = RCC / 8
    // sampling time - 239.5 cycles for channels 8, 16 and 17
    ADC1->SMPR2 = ADC_SMPR2_SMP8;
    ADC1->SMPR1 = ADC_SMPR1_SMP16 | ADC_SMPR1_SMP17;
    // we have three conversions in group -> ADC1->SQR1[L] = 2, order: 8->16->17
    ADC1->SQR3 = 8 | (16<<5) | (17<<10);
    ADC1->SQR1 = ADC_SQR1_L_1;
    ADC1->CR1 |= ADC_CR1_SCAN; // scan mode
    // DMA configuration
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR));
    DMA1_Channel1->CMAR = (uint32_t)(ADC_array);
    DMA1_Channel1->CNDTR = NUMBER_OF_ADC_CHANNELS * 9;
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0
                          | DMA_CCR_CIRC | DMA_CCR_PL | DMA_CCR_EN;
    // continuous mode & DMA; enable vref & Tsens; wake up ADC
    ADC1->CR2 |= ADC_CR2_DMA | ADC_CR2_TSVREFE | ADC_CR2_CONT | ADC_CR2_ADON;
    // wait for Tstab - at least 1us
    while(++ctr < 0xff) nop();
    // calibration
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    ctr = 0; while((ADC1->CR2 & ADC_CR2_RSTCAL) && ++ctr < 0xfffff);
    ADC1->CR2 |= ADC_CR2_CAL;
    ctr = 0; while((ADC1->CR2 & ADC_CR2_CAL) && ++ctr < 0xfffff);
    // turn ON ADC
    ADC1->CR2 |= ADC_CR2_ADON;
}

void hw_setup(){
    gpio_setup();
    adc_setup();
}

void exti1_isr(){ // PPS - PA1
    systick_correction();
    LED_off(); // turn off LED0 @ each PPS
    EXTI->PR = EXTI_PR_PR1;
}

static trigtime trgtm;
void savetrigtime(){
    trgtm.millis = Timer;
    memcpy(&trgtm.Time, &current_time, sizeof(curtime));
}

/**
 * @brief fillshotms - save trigger shot time
 * @param i - trigger number
 */
void fillshotms(int i){
    if(i < 0 || i >= TRIGGERS_AMOUNT) return;
    if(Tms - shotms[i] > (uint32_t)the_conf.trigpause[i]){
        memcpy(&shottime[i], &trgtm, sizeof(trigtime));
        shotms[i] = Tms;
        trigger_shot |= 1<<i;
        BUZZER_ON();
    }
}

/**
 * @brief fillunshotms - calculate trigger length time
 */
void fillunshotms(){
    if(!trigger_shot) return;
    uint8_t X = 1;
    for(int i = 0; i < TRIGGERS_AMOUNT; ++i, X<<=1){
        IWDG->KR = IWDG_REFRESH;
        // check whether trigger is OFF but shot recently
        if(trigger_shot & X){
            uint32_t len = Tms - shotms[i];
            uint8_t rdy = 0;
            if(len > MAX_TRIG_LEN){
                triglen[i] = -1;
                rdy = 1;
            }else triglen[i] = (uint16_t) len;
            if(i == LIDAR_TRIGGER){
                if(!parse_lidar_data(NULL)) rdy = 1;
            }else if(i == ADC_TRIGGER){
                if(!chkADCtrigger()) rdy = 1;
            }else{
                uint8_t pinval = (trigport[i]->IDR & trigpin[i]) ? 1 : 0;
                if(pinval != trigstate[i]) rdy = 1; // trigger is OFF
            }
            if(rdy){
                shotms[i] = Tms;
                show_trigger_shot(X);
                trigger_shot &= ~X;
            }
        }
    }
}

void exti4_isr(){ // PA4 - trigger[2]
    savetrigtime();
    fillshotms(2);
    EXTI->PR = EXTI_PR_PR4;
}

void exti15_10_isr(){ // PA13 - trigger[0], PA14 - trigger[1]
    savetrigtime();
    if(EXTI->PR & EXTI_PR_PR13){
        fillshotms(0);
        EXTI->PR = EXTI_PR_PR13;
    }
    if(EXTI->PR & EXTI_PR_PR14){
        fillshotms(1);
        EXTI->PR = EXTI_PR_PR14;
    }
}

#ifdef EBUG
/**
 * @brief gettrig - get trigger state
 * @return 1 if trigger active or 0
 */
uint8_t gettrig(uint8_t N){
    if(N >= TRIGGERS_AMOUNT) return 0;
    uint8_t curval = (trigport[N]->IDR & trigpin[N]) ? 1 : 0;
    if(curval == trigstate[N]) return 1;
    else return 0;
}
#endif

void chk_buzzer(){
    static uint32_t Ton = 0; // Time of first buzzer check
    if(!BUZZER_GET()) return; // buzzer if OFF
    if(!trigger_shot){ // should we turn off buzzer?
        uint8_t notrg = 1;
        for(int i = 0; i < DIGTRIG_AMOUNT; ++i){
            uint8_t curval = (trigport[i]->IDR & trigpin[i]) ? 1 : 0;
            if(curval == trigstate[i]){
                notrg = 0; // cheep while digital trigger is ON
                break;
            }
        }
        if(notrg){ // turn off buzzer when there's no trigger events & timeout came
            if(Tms - Ton < BUZZER_CHEEP_TIME) return;
            Ton = 0;
            BUZZER_OFF();
        }
    }else{ // buzzer is ON - check timer
        if(Ton == 0){
            Ton = Tms;
            if(!Ton) Ton = 1;
        }
    }
}
