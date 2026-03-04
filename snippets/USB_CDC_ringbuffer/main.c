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
#include "hardware.h"
#include "usart.h"
#include "dbg.h"
#include "usb_dev.h"

#define INBUFSZ     (256)

volatile uint32_t Tms = 0; // milliseconds

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

/*
 * Set up timer to fire every x milliseconds
 */
static void systick_setup(uint32_t xms){ // xms < 2098!!!
    Tms = 0;
    static uint32_t curms = 0;
    if(curms == xms) return;
    // this function also clears counter so it starts right away
    SysTick_Config(SysFreq / 8000 * xms); // arg should be < 0xffffff, so ms should be < 2098
    curms = xms;
}
#ifdef EBUG
volatile uint32_t uictr = 0, CTRctr = 0;
static uint32_t olduictr = 0, oldctrctr = 0;
#endif
int main(void){
    char inbuff[INBUFSZ];
    StartHSE();
    gpio_setup();
    systick_setup(1); // run each 1ms
    usart_setup(3000000);
    USB_setup();
    uint32_t M = 0;
    uint8_t ispressed = 0;
    const char *pressed = "Key pressed\n", *released = "Key released\n";
    while(1){
        if(Tms - M > 499){
            LED_TOGGLE();
            M = Tms;
#ifdef EBUG
            if(olduictr != uictr){
                usart_sendstr("USBirqctr="); usart_sendstr(u2str(uictr));
                usart_send("\n", 1);
                olduictr = uictr;
            }
            if(CTRctr != oldctrctr){
                usart_sendstr("CTRctr="); usart_sendstr(u2str(CTRctr));
                usart_send("\n", 1);
                oldctrctr = CTRctr;
            }
#endif
        }
        if(KEY_PRESSED()){ // key pressed - send to all "Pressed"
            if(!ispressed){
                ispressed = 1;
                USB_sendstr(pressed);
                usart_sendstr(pressed);
            }
        }else{ // released -> send
            if(ispressed){
                ispressed = 0;
                USB_sendstr(released);
                usart_sendstr(released);
            }
        }
        USART_flags_t f = usart_process();
        if(f.rxovrfl) usart_sendstr("Rx overflow!\n");
        if(f.txerr) usart_sendstr("Tx error!\n");
        char *got = usart_getline();
        if(got){
            usart_sendstr("You sent:\n");
            usart_sendstr(got);
            usart_send("\n", 1);
        }
        int l = USB_receivestr(inbuff, INBUFSZ);
        if(l < 0) usart_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l){
            usart_sendstr("USB:\n");
            usart_sendstr(inbuff);
            usart_send("\n", 1);
            USB_sendstr(inbuff);
            newline();
            //const char *ans = parse_cmd(inbuff);
            //if(ans) USB_sendstr(ans);
        }

    }
}
