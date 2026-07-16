/*
 * This file is part of the blink project.
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

#include "stm32g4.h"

// KEY (intpullup->0) - PC13
// LED - PC6
#define KEY_PORT    GPIOC
#define KEY_PIN     (1<<13)
#define LED_PORT    GPIOC
#define LED_PIN     (1<<6)
#define KEY_PRESSED() (pin_read(KEY_PORT, KEY_PIN) == 1)
#define LED_ON()    do{pin_set(LED_PORT, LED_PIN);}while(0)
#define LED_OFF()   do{pin_clear(LED_PORT, LED_PIN);}while(0)
#define LED_TOGG()  do{pin_toggle(LED_PORT, LED_PIN);}while(0)

static volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

static void gpio_setup(void){
    RCC->AHB2ENR = RCC_AHB2ENR_GPIOCEN; // enable PC
    __DSB();
    // set PC6 as push-pull output, PC13 is pulldown input, other as default (AIN)
    GPIOC->MODER = (0xffffffff & ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE13)) | MODER_O(6) | MODER_I(13);
    GPIOC->PUPDR = PUPD_PD(13); // pulldown
    GPIOC->OTYPER = OTYPER_PP(6); // push-pull (default)
    // count milliseconds
    SysTick_Config(SysFreq / 1000); // arg should be < 0xffffff
}

static const uint32_t L[] = {300, 125,100,125,100,125,200, 350,100,350,100,350,200, 125,100,125,100,125, 1000};

int main(void){
    StartHSE();
    gpio_setup();
    uint32_t idx = 0, Told = 0;
    int pressed = 0;
    /* Do nothing in main loop */
    while (1){
        if(KEY_PRESSED()){ // key not pressed - 'sos'
            if(idx & 1) LED_ON();
            else LED_OFF();
            while(Tms - Told < L[idx]);
            if(++idx == 19) idx = 0;
        }else{ // key pressed - blink with period of 1s
            if(pressed){
                idx = 0;
                pressed = 0;
            }
            LED_TOGG();
            while(Tms - Told < 500);
        }
        Told = Tms;
    }
}
