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

#include <stm32f0.h>
#include "spi.h"
#include "usart.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

static void gpio_setup(void){
    // Set green led (PB3) as output - no, PB3 is SCK!
    //RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    //GPIOB->MODER = GPIO_MODER_MODER3_O;
}

static void printuhex(uint8_t *arr, uint8_t l){
    if(l > 32 || l == 0) return;
    char buf[70] = "0x";
    int8_t i, j, bidx = 2;
    for(i = 0; i < l; ++i, ++arr){
        for(j = 1; j > -1; --j){
            uint8_t half = (*arr >> (4*j)) & 0x0f;
            if(half < 10) buf[bidx++] = half + '0';
            else buf[bidx++] = half - 10 + 'a';
        }
    }
    buf[bidx++] = '\n';
    SEND(buf, bidx);
}

static void print14bit(uint8_t *arr){
    uint8_t _16[2];
    _16[0] = (arr[0]>>4)&3; _16[1] = (arr[0]<<4) | (arr[1] >> 4);
    printuhex(_16, 2);
    //SEND("\n\n", 2);
}

int main(void){
    uint32_t /*lastT = 0,*/ dctr = 0;
    int16_t L = 0;
    uint8_t buf[SPIBUFSZ], len;
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    spi_setup();
    usart2_setup();

    while (1){
       /* if(Tms - lastT > 499){
            pin_toggle(GPIOB, 1<<3); // blink by onboard LED once per second
            lastT = Tms;
        }*/

        if(usart2rx()){ // usart1 received data, store in in buffer
            L = usart2_getline(&txt);
            // do something with received data
        }
        if(L){ // text waits for sending
            if(ALL_OK == usart2_send(txt, L)){
                L = 0;
            }
        }
        len = SPIBUFSZ;
        uint8_t a = SPI_getdata(buf, &len);
        if(a){
            //printuhex(buf, len);
            print14bit(buf);
        }
        if(Tms - dctr > 999){// once per 1 second
            dctr = Tms;
            //SEND("pre\n", 4);
            SPI_prep_receive();
        }
    }
    return 0;
}

