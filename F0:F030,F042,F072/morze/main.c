/*
 * main.c
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#include <string.h>
#include "stm32f0.h"
#include "usart.h"
#include "morse.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

static void gpio_setup(void){
    /* Enable clocks to the GPIO subsystems (A&B) */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    /* Set green leds (PA4 & PA6) as output; AF1 to enable timer on PA6 */
    GPIOA->MODER = GPIO_MODER_MODER4_O | GPIO_MODER_MODER6_AF;
    GPIOA->AFR[0] |= 1<<24; // AF1
    GPIOA->OTYPER = 1 << 6; // PA6 - opendrain, PA4 - pushpull
}

// Tim3_ch1 - PA6, 48MHz -> 1kHz
static void tim3_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; // enable clocking
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    TIM3->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1; // PWM mode 1: acive->inactive, preload enable
    TIM3->PSC = 47999; // 1kHz
    TIM3->ARR = WRD_GAP;
    TIM3->CCR1 = 0;
    TIM3->CCER = TIM_CCER_CC1P | TIM_CCER_CC1E; // turn it on, active low
    TIM3->EGR |= TIM_EGR_UG;
    // DMA ch3 - TIM3_UP
    DMA1_Channel3->CPAR = (uint32_t)(&(TIM3->DMAR)); // TIM3_ch1
    DMA1_Channel3->CMAR = (uint32_t)(mbuff);
    DMA1_Channel3->CNDTR = 0;
    // memsiz 16bit, psiz 32bit, memincrement, from memory
    DMA1_Channel3->CCR |=  DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_1
                       | DMA_CCR_MINC | DMA_CCR_DIR;
    TIM3->DCR = (2 << 8) // three bytes
                + (((uint32_t)&TIM3->ARR - (uint32_t)&TIM3->CR1) >> 2);
    TIM3->DIER = TIM_DIER_UDE; // enable DMA requests
    DMA1_Channel3->CCR |= DMA_CCR_TCIE; // enable TC interrupt
}

char buftoping[256] = {0}, *nxtltr = buftoping;

int main(void){
    uint32_t lastT = 0; // last time counter value
    int L = 0; // length
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    tim3_setup();
    USART1_config();
    int first = 1; // first letter in phrase
    while (1){
        if(dmaflag){
            dmaflag = 0;
            if(*nxtltr){
                DMA1_Channel3->CCR &= ~DMA_CCR_EN;
                while(ALL_OK != usart1_send(nxtltr, 1));
                uint8_t x;
                nxtltr = fillbuffer(nxtltr, &x);
                if(x){
                    DMA1_Channel3->CNDTR = 3 * x;
                    DMA1_Channel3->CCR |= DMA_CCR_EN;
                    //x += '0';
                    //while(ALL_OK != usart1_send((char*)&x, 1));
                    if(first){ // first letter in message
                        first = 0;
                        TIM3->CR1 |= TIM_CR1_CEN;
                        TIM3->EGR |= TIM_EGR_UG; // force update generation
                    }
                }
            }else{
                TIM3->CR1 &= ~TIM_CR1_CEN; // turn timer off
                first = 1;
            }
        }
        if(lastT > Tms || Tms - lastT > 499){
            pin_toggle(GPIOA, 1<<4); // blink by onboard LED once per second
            lastT = Tms;
        }
        if(usart1rx()){ // usart1 received data, store in in buffer
            L = usart1_getline(&txt);
            if(L < 256 && !*nxtltr){
                memcpy(buftoping, txt, L);
                buftoping[L] = 0;
                nxtltr = buftoping;
                dmaflag = 1; //
                usart1_send("Send ", 5);
                while(ALL_OK != usart1_send(txt, L));
            }else usart1_send("Please, wait\n", 13);
        }
       /* if(L){ // text waits for sending
            if(ALL_OK == usart1_send(txt, L)){
                L = 0;
            }
        }*/
    }
}
