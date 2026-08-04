/*
 * This file is part of the cordic project.
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

#include <stm32g4.h>
#include <string.h>

#include "commproto.h"
#include "hardware.h"
#include "strfunc.h"
#include "usart.h"

// 40ms for debouncing
#define TDEBOUNCE   (40)

int main(void){
    StartHSE();
    // init storage first to take base values
    gpio_setup();
    usart_setup(115200);
    bool pressed = false;
    uint32_t Tblink = 0, Tkey = 0; // blink and key pressed time
    uint32_t Tpressed = 0;
    while(1){
        USART_flags_t f = usart_process();
        if(f.rxovrfl) usart_sendstr("Rx buffer overflow!\n");
        if(f.txerr) usart_sendstr("Tx error!\n");
        char *str = usart_getline();
        if(str){
            const char *ans = parse_cmd(usart_sendstr, str);
            if(ans) usart_sendstr(ans);
        }
        if(Tms - Tblink > 499){
            LED_TOGG();
            Tblink = Tms;
        }
        if(KEY_PRESSED()){
            if(!pressed){ // new event
                pressed = true;
                usart_sendstr("Key pressed\n");
                timer_start();
                Tpressed = Tms;
            }
            Tkey = Tms;
        }else{ // key released
            if(pressed && (Tms - Tkey) > TDEBOUNCE){ // wait for debounce
                timer_stop();
                uint32_t micros = timer_read();
                uint32_t millis = Tms - Tpressed;
                pressed = false;
                usart_sendstr("Key released\n");
                usart_sendstr("Tms="); usart_sendstr(u2str(millis));
                usart_sendstr("\nTus="); usart_sendstr(u2str(micros)); usart_sendstr("\n");
            }
        }
    }
}
