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

#include "stm32g0.h"

#define KEY_PORT        GPIOC
#define KEY_PIN         (1<<13)
#define LED_PORT        GPIOC
#define LED_PIN         (1<<6)
#define KEY_PRESSED()   (pin_read(KEY_PORT, KEY_PIN) == 1)
#define LED_ON()        do{pin_set(LED_PORT, LED_PIN);}while(0)
#define LED_OFF()       do{pin_clear(LED_PORT, LED_PIN);}while(0)

// KEY (intpullup->0) - PC13
// LED - PC6

static volatile uint32_t blink_ctr = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++blink_ctr;
}

/*
 * Set up timer to fire every x milliseconds
 */
static void systick_setup(uint32_t xms){ // xms < 2098!!!
    blink_ctr = 0;
    static uint32_t curms = 0;
    if(curms == xms) return;
    // 8MHz - HCLK/8
    // this function also clears counter so it starts right away
    SysTick_Config(8000 * xms); // arg should be < 0xffffff, so ms should be < 2098
    curms = xms;
}

static void gpio_setup(void){
    RCC->IOPENR = RCC_IOPENR_GPIOCEN; // enable PC
    // set PC8 as opendrain output, PC0 is pullup input, other as default (AIN)
    GPIOC->MODER = (0xffffffff & ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE13)) | GPIO_MODER_MODER6_O; // GPIO_MODER_MODER13_I == 0
    GPIOC->PUPDR = GPIO_PUPDR13_PD; // pull down
}

static const uint32_t L[] = {125,100,125,100,125,200, 350,100,350,100,350,200, 125,100,125,100,125, 1000};

int main(void){
    StartHSE();
    gpio_setup();
    systick_setup(500);
    uint32_t M = 0;
    int pressed = 0;
    /* Do nothing in main loop */
    while (1){
        if(KEY_PRESSED()){ // key pressed - 'sos'
            pressed = 1;
            systick_setup(L[M]);
            if(M & 1) LED_OFF();
            else LED_ON();
            if(++M == 18) M = 0;
            while(blink_ctr == 0);
        }else{ // key not pressed - blink with period of 1s
            if(pressed){
                M = 0;
                pressed = 0;
                systick_setup(500);
            }
            if(blink_ctr & 1) LED_ON();
            else LED_OFF();
        }
    }
}
