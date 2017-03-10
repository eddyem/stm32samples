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

#include "stm32f0.h"
#include "usart.h"
#include "i2c.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

static void gpio_setup(void){
    // Set green led (PB3) as output
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER = GPIO_MODER_MODER3_O;
}

// print 32bit unsigned int
void printu(uint32_t val){
    char buf[11], rbuf[10];
    int l = 0, bpos = 0;
    if(!val){
        buf[0] = '0';
        l = 1;
    }else{
        while(val){
            rbuf[l++] = val % 10 + '0';
            val /= 10;
        }
        int i;
        bpos += l;
        for(i = 0; i < l; ++i){
            buf[--bpos] = rbuf[i];
        }
    }
    while(ALL_OK != usart2_send_blocking(buf, l+bpos));
}

void showcoeffs(uint8_t addr){ // show norm coefficiens
    int i;
    const uint8_t regs[5] = {0xAA, 0xA8, 0xA6, 0xA4, 0xA2}; // commands for coefficients
    uint32_t K;
    for(i = 0; i < 5; ++i){
        if(write_i2c(addr, regs[i])){
            if(read_i2c(addr, &K, 2)){
                char b[3] = {'K', i+'0', '='};
                while(ALL_OK != usart2_send_blocking(b, 3));
                printu(K);
                while(ALL_OK != usart2_send_blocking("\n", 1));
            }
        }
    }
}

int main(void){
    uint32_t lastT = 0;
    int16_t L = 0;
    int _5sec = 0; // five second counter
    uint32_t started0=0, started1=0; // time of measurements for given sensor started
    uint32_t msctr = 0;
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    usart2_setup();
    i2c_setup();
    // reset on start
    write_i2c(TSYS01_ADDR0, TSYS01_RESET);
    write_i2c(TSYS01_ADDR1, TSYS01_RESET);

    while (1){
        if(lastT > Tms || Tms - lastT > 499){
            pin_toggle(GPIOB, 1<<3); // blink by onboard LED once per second
            lastT = Tms;
        }
        if(started0 && Tms - started0 > CONV_TIME){ // poll sensor0
            if(write_i2c(TSYS01_ADDR0, TSYS01_ADC_READ)){
                uint32_t t;
                if(read_i2c(TSYS01_ADDR0, &t, 3)){
                    while(ALL_OK != usart2_send_blocking("T0=", 3));
                    printu(t);
                    while(ALL_OK != usart2_send_blocking("\n", 1));
                    started0 = 0;
                }
            }
        }
        if(started1 && Tms - started1 > CONV_TIME){ // poll sensor1
            if(write_i2c(TSYS01_ADDR1, TSYS01_ADC_READ)){
                uint32_t t;
                if(read_i2c(TSYS01_ADDR1, &t, 3)){
                    while(ALL_OK != usart2_send_blocking("T1=", 3));
                    printu(t);
                    while(ALL_OK != usart2_send_blocking("\n", 1));
                    started1 = 0;
                }
            }
        }
        if(usart2rx()){ // usart1 received data, store in in buffer
            L = usart2_getline(&txt);
            if(L == 2){
                if(txt[0] == 'C'){ // 'C' - show coefficients
                    showcoeffs(TSYS01_ADDR0);
                    showcoeffs(TSYS01_ADDR1);
                }else if(txt[0] == 'R'){ // 'R' - reset both
                    write_i2c(TSYS01_ADDR0, TSYS01_RESET);
                    write_i2c(TSYS01_ADDR1, TSYS01_RESET);
                }else if(txt[0] == 'I'){ // 'I' - reinit I2C
                    i2c_setup();
                }
            }
        }
        if(L){ // text waits for sending
            if(ALL_OK == usart2_send(txt, L)){
                L = 0;
            }
        }
        if(msctr != Tms){
            msctr = Tms;
            if(++_5sec >= 5000){ // once per 5 second
                _5sec = 0;
                if(write_i2c(TSYS01_ADDR0, TSYS01_START_CONV)) started0 = Tms;
                else started0 = 0;
                if(write_i2c(TSYS01_ADDR1, TSYS01_START_CONV)) started1 = Tms;
                else started1 = 0;
            }
        }
    }
    return 0;
}

