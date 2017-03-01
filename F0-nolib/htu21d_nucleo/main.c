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

void printi(int16_t val){
    char buf[6], rbuf[5];
    int l = 0, bpos = 0;
    if(!val){
        buf[0] = '0';
        l = 1;
    }else{
        if(val < 0){
            buf[0] = '-';
            bpos = 1;
            val = -val;
        }
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

int main(void){
    uint32_t lastT = 0;
    int16_t L = 0;
    int _1sec = 0, T_H = 0; // T_H - read t(1) or h(0)
    uint32_t msctr = 0;
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    usart2_setup();
    i2c_setup();

    while (1){
        if(lastT > Tms || Tms - lastT > 499){
            pin_toggle(GPIOB, 1<<3); // blink by onboard LED once per second
            lastT = Tms;
        }
        if(usart2rx()){ // usart1 received data, store in in buffer
            L = usart2_getline(&txt);
            printi(L);
            // do something with received data
        }
        if(L){ // text waits for sending
            if(ALL_OK == usart2_send(txt, L)){
                L = 0;
            }
        }
        if(msctr != Tms){
            msctr = Tms;
            if(++_1sec >= 1000){ // once per 1 second
                _1sec = 0;
                // send H/T
                uint8_t command = HTU21_READ_HUMID;
                if(T_H) command = HTU21_READ_TEMP;
                if(!htu_write_i2c(command))
                    while(ALL_OK != usart2_send_blocking("Error!\n", 7));
            }
        }
        // poll I2C
        uint16_t val = 0;
        if(htu_read_i2c(&val)){ // got data
            if(T_H){
                while(ALL_OK != usart2_send_blocking("Temperature: ", 13));
                int16_t t = convert_temperature(val);
                printi(t);
                while(ALL_OK != usart2_send_blocking("/10 degrC\n", 10));
            }else{
                while(ALL_OK != usart2_send_blocking("Humidity: ", 10));
                int16_t h = convert_humidity(val);
                printi(h);
                while(ALL_OK != usart2_send_blocking("/10 %\n", 6));
            }
            T_H = !T_H;
        }else{
            if(val) // CRC error
                while(ALL_OK != usart2_send_blocking("CRC error!\n", 11));
        }
    }
    return 0;
}

