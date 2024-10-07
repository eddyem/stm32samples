/*
 * This file is part of the shutter project.
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

#include "adc.h"
#include "flash.h"
#include "hardware.h"

TRUE_INLINE void iwdg_setup(){
    uint32_t tmout = 16000000;
    /* Enable the peripheral clock RTC */
    /* (1) Enable the LSI (40kHz) */
    /* (2) Wait while it is not ready */
    RCC->CSR |= RCC_CSR_LSION; /* (1) */
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;} /* (2) */
    /* Configure IWDG */
    /* (1) Activate IWDG (not needed if done in option bytes) */
    /* (2) Enable write access to IWDG registers */
    /* (3) Set prescaler by 64 (1.6ms for each tick) */
    /* (4) Set reload value to have a rollover each 2s */
    /* (5) Check if flags are reset */
    /* (6) Refresh counter */
    IWDG->KR = IWDG_START; /* (1) */
    IWDG->KR = IWDG_WRITE_ACCESS; /* (2) */
    IWDG->PR = IWDG_PR_PR_1; /* (3) */
    IWDG->RLR = 1250; /* (4) */
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;} /* (5) */
    IWDG->KR = IWDG_REFRESH; /* (6) */
}

/*
Pinout:
PB10 (pullup in) - CCD or button input (depends on settings)
PB11 (alternate: TIM2_CH4) - PWM output for shutter feeding
PB12 (pullup in) - hall (or reed switch) sensor input (opened/closed shutter detector)
PB13 (PP out) - USB pullup (active low)

PA3 (ADC in) - shutter voltage (approx 1/12 U)
PA9 (reserved) - USART1 Tx
PA10 (reserved) - USART1 Rx
PA11,12 - USB
PA13,14 - SWD
*/

TRUE_INLINE void gpio_setup(){
    // Enable clocks to the GPIO subsystems, turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    __DSB();
    // turn off SWJ/JTAG
//    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_DISABLE;
    // AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE; // for PA15
    GPIOA->CRL = CRL(3, CNF_ANALOG | MODE_INPUT);
    GPIOB->CRH = CRH(10, CNF_PUDINPUT | MODE_INPUT) | CRH(12, CNF_PUDINPUT | MODE_INPUT) |
                 CRH(13, CNF_PPOUTPUT | MODE_NORMAL);
    // if ccdactive==0 - shutter is closed when no output signals
    //GPIOB->ODR = ((the_conf.hallactive) ? 0: 1) | ((the_conf.ccdactive) ? 0 : 1<<11);
    GPIOB->ODR = (1<<10) | (1<<11); // PB10 & PB12 pullup
}

// TIM2_CH4 (36MHz) as PWM @ 65kHz
TRUE_INLINE void pwm_setup(){
    // remap TIM2_CH4 to PB11
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_TIM2_REMAP) |
                 AFIO_MAPR_TIM2_REMAP_PARTIALREMAP2;
    GPIOB->CRH |= CRH(11, CNF_AFPP | MODE_NORMAL);
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    // 36 MHz / 100 / 65kHz = 5 ==> 7.2MHz with PWMfreq = 72kHz
    TIM2->PSC = 4;
    TIM2->ARR = 99;
    TIM2->CCR4 = 0; // turned off at start
    // mode1: active->inactive; CCR4 preload enable
    TIM2->CCMR2 = TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4PE;
    // enable PWM output
    TIM2->CCER = TIM_CCER_CC4E;
    // turn on and generate update event to refresh preloaded buffers
    TIM2->CR1 = TIM_CR1_CEN;
    TIM2->EGR = TIM_EGR_UG;
}

// set PWM to 0..100%, return 0 if `percent`>100
int set_pwm(uint32_t percent){
    if(percent > 100) return 0;
    TIM2->CCR4 = percent;
    return 1;
}

void hw_setup(){
    iwdg_setup();
    gpio_setup();
    adc_setup();
    pwm_setup();
}

uint32_t getShutterVoltage(){
    uint32_t val = getADCvoltage(CHSHTR);
    val *= the_conf.shtrVmul;
    return val / the_conf.shtrVdiv;
}
