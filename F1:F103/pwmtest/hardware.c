/*
 * This file is part of the pwmtest project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "hardware.h"
#include "proto.h"
#include "usb.h"

static uint32_t PWM_freq = 80000; // 80kHz
static uint32_t PWM_duty = 50;    // PWM duty cycle in promille

static inline void gpio_setup(){
    // Enable clocks to the GPIO subsystems (PB for ADC), turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    // turn off SWJ/JTAG
//    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_DISABLE;
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE; // for PA15
    // Set led as opendrain output
    GPIOC->CRH |= CRH(13, CNF_ODOUTPUT|MODE_SLOW);
    // USB pullup (PA15) - pushpull output, PA8 - alternate
    GPIOA->CRH = CRH(15, CNF_PPOUTPUT|MODE_SLOW) | CRH(8, CNF_AFPP | MODE_FAST);
}

static inline void tim1_setup(){
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; // enable TIM1 clocking
    TIM1->PSC = 8;  // 8MHz
    TIM1->ARR = 99; // 100 ticks for 80kHz
    TIM1->CCR1 = 49; // 50%
    // PWM mode 1 (active->inactive)
    TIM1->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
    // main output
    TIM1->BDTR = TIM_BDTR_MOE;
    // main PWM output
    TIM1->CCER = TIM_CCER_CC1E;
    // turn it on
    TIM1->CR1 = TIM_CR1_CEN;// | TIM_CR1_ARPE;
    TIM1->EGR |= TIM_EGR_UG; // generate update event to refresh all
}

static void refresh_ccr1(){
    if(PWM_duty == 0){
        TIM1->CCR1 = 0;
        return;
    }
    uint32_t ccr1 = (PWM_duty*(TIM1->ARR+1.))/1000;
    USB_send("New CCR1: "); USB_send(u2str(ccr1)); USB_send("\n");
    TIM1->CCR1 = ccr1;
}

// return 0 if can't change or return 1
int changePWMfreq(uint32_t freq){
    if(!freq) return 0; // zero frequency
    uint32_t ARRPSC = (uint32_t)TIM1FREQ / freq;
    // both ARR and PSC have 16 bits
    if(ARRPSC < 100) return 0; // frequency too big
    // 1hz: ARRPSC=72000000, ARR=1098, PSC=65513; Freq=72000000/1099/65514=1.0000016Hz
    // 98kHz: ARRPSC=734, ARR=0, PSC=733; Freq=72000000/1/734=98092Hz
    uint32_t PSC = ARRPSC >> 16;
    uint32_t ARR = ARRPSC / (PSC+1) - 1;
    PWM_freq = TIM1FREQ/(ARR+1.)/(PSC+1.);
    TIM1->ARR = ARR;
    TIM1->PSC = PSC;
    USB_send("New PSC: "); USB_send(u2str(PSC));
    USB_send("\nNew ARR: "); USB_send(u2str(ARR)); USB_send("\n");
    refresh_ccr1();
    return 1;
}

// change duty; return 0 if wrong, 1 if OK (duty in promille)
int changePWMduty(uint32_t duty){
    if(duty > 1000) return 0; // wrong duty
    PWM_duty = duty;
    refresh_ccr1();
    return 1;
}

void hw_setup(){
    gpio_setup();
    tim1_setup();
}

void iwdg_setup(){
    uint32_t tmout = 16000000;
    RCC->CSR |= RCC_CSR_LSION;
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;} /* (2) */
    IWDG->KR = IWDG_START;
    IWDG->KR = IWDG_WRITE_ACCESS;
    IWDG->PR = IWDG_PR_PR_1;
    IWDG->RLR = 1250;
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;}
    IWDG->KR = IWDG_REFRESH;
}
