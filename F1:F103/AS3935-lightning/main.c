/*
 * This file is part of the as3935 project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "as3935.h"
#include "flash.h"
#include "hardware.h"
#include "commproto.h"
#include "flash.h"
#include "spi.h"
#include "strfunc.h"
#include "usb_dev.h"

uint8_t DISPLCO[SENSORS_AMOUNT] = {0};

volatile uint32_t Tms = 0;
static char inbuff[RBINSZ];

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(){
    uint8_t oldint[2] = {0}; // ==1 for interrupted pins
    uint32_t lastT = 0;
    StartHSE();
    //flashstorage_init();
    hw_setup();
    USBPU_OFF();
    SysTick_Config(72000);
    flashstorage_init();
    USB_setup();
#ifndef EBUG
    iwdg_setup();
#endif
    //usart_setup();
    USBPU_ON();
    // wake-up all sensors and run auto-calibration
    for(uint8_t ch = 0; ch < SENSORS_AMOUNT; ++ch){
        as3935_channel = ch;
        as3935_wakeup();
        as3935_calib_rco();
        if(the_conf.flags.restore){
            sens_pars_t *p = &the_conf.spars[ch];
            as3935_gain(p->AFE_GB);
            as3935_wdthres(p->WDTH);
            as3935_nflev(p->NF_LEV);
            as3935_srej(p->SREJ);
            as3935_minnumlig(p->MIN_NUM_LIG);
            as3935_maskdist(p->MASK_DIST);
            as3935_lco_fdiv(p->LCO_FDIV);
            as3935_tuncap(p->TUN_CAP);
        }
    }
    while(1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        int l = USB_receivestr(inbuff, RBINSZ);
        if(l < 0) USB_sendstr("ERROR: CMD USB buffer overflow or string was too long");
        else if(l){
            const char *ans = parse_cmd(USB_sendstr, inbuff);
            if(ans) USB_sendstr(ans);
        }
        for(uint8_t ch = 0; ch < SENSORS_AMOUNT; ++ch){
            if(DISPLCO[ch] != DISPLCO_NOTHING) continue; // don't check IRQ in if it used as clock output
            if(CHK_INT(ch)){
                if(oldint[ch] == 0){
                    oldint[ch] = 1;
                    uint8_t code;
                    as3935_channel = ch;
                    int result = as3935_intcode(&code);
                    if(!result) USB_sendstr("CANTGET\n");
                    else if(code){
                        uint8_t savedcode = code;
                        USB_sendstr("INTERRUPT"); USB_putbyte('0'+ch); USB_putbyte('=');
                        const char *delim = NULL, *comma = ",";
                        if(code & INT_NH){ USB_sendstr("NOICE"); delim = comma; code &= ~INT_NH; }
                        if(code & INT_D){ USB_sendstr(delim); USB_sendstr("DISTURBER"); delim = comma; code &= ~INT_D; }
                        if(code & INT_L){ USB_sendstr(delim); USB_sendstr("LIGHTNING"); code &= ~INT_L; }
                        if(code) USB_sendstr(u2str(code));
                        newline();
                        if(savedcode == INT_L){ // clear lightning - show distance and power
                            lightning_info(USB_sendstr, ch);
                        }
                    }
                }
            }else oldint[ch] = 0;
        }
    }
    return 0;
}
