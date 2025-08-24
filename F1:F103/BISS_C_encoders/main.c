/*
 * This file is part of the I2Cscan project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "bissC.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "spi.h"
#include "strfunc.h"
#include "usart.h"
#include "usb_dev.h"

volatile uint32_t Tms = 0;
static char inbuff[RBINSZ];
static uint32_t monitT[2] = {0};

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

// usefull data bytes: start/0+26bit+8bit=36
#define USEFULL_DATA_BYTES (5)

static void printResult(BiSS_Frame *result){
    if(result->frame_valid){
        CMDWR("Data: "); CMDWRn(uhex2str(result->data));
        if(result->error) CMDWRn("ERROR flag set");
        if(result->warning) CMDWRn("WARNING flag set");
        CMDWR("CRC is "); if(!result->crc_valid) CMDWR("NOT ");
        CMDWRn("valid");
    }else CMDWRn("Invalid frame");
}

static void proc_enc(uint8_t idx){
    static uint32_t lastMSG[2], gotgood[2], gotwrong[2];
    static uint32_t lastXval = 0, usartT = 0;
    int iface = idx ? I_Y : I_X;
    char ifacechr = idx ? 'Y' : 'X';
    if(CDCready[iface]){
        int l = USB_receivestr(iface, inbuff, RBINSZ);
        if(CDCready[I_CMD] && the_conf.flags.debug){
            if(l){
                CMDWR("Enc"); USB_putbyte(I_CMD, ifacechr);
                CMDWR(": ");
            }
            if(l < 0) CMDWRn("ERROR! USB buffer overflow or string was too long");
            else if(l){
                CMDWR("got string: '");
                CMDWR(inbuff);
                CMDWR("'\n");
            }
        }
        if(l > 0) spi_start_enc(idx); // start encoder reading on each request from given interface
    }
    uint8_t *encbuf = spi_read_enc(idx);
    if(!encbuf) return;
    BiSS_Frame result = parse_biss_frame(encbuf, the_conf.encbufsz);
    char *str = result.crc_valid ? u2str(result.data) : NULL;
    if(result.crc_valid){
        if(idx == 0) lastXval = result.data;
        else if(the_conf.send232_interval && Tms - usartT >= the_conf.send232_interval){
            usart_send_enc(lastXval, result.data);
            usartT = Tms;
        }
    }
    uint8_t testflag = (idx) ? user_pars.testy : user_pars.testx;
    if(CDCready[I_CMD]){
        if(testflag){
            if(Tms - lastMSG[idx] > 9999){
                USB_putbyte(I_CMD, ifacechr);
                CMDWR(" 10sec stat: good=");
                CMDWR(u2str(gotgood[idx]));
                CMDWR(", bad=");
                CMDWR(u2str(gotwrong[idx]));
                CMDn();
                gotgood[idx] = 0;
                gotwrong[idx] = 0;
                lastMSG[idx] = Tms;
            }else{
                if(str) ++gotgood[idx];
                else ++gotwrong[idx];
            }
        }else if(!the_conf.flags.monit && the_conf.flags.debug){
            printResult(&result);
            CMDWR("ENC"); USB_putbyte(I_CMD, ifacechr);
            USB_putbyte(I_CMD, '=');
            hexdump(I_CMD, encbuf, the_conf.encbufsz);
            CMDWR(" (");
            if(str) CMDWR(str);
            else CMDWR("wrong");
            CMDWR(")\n");
        }
    }
    if(CDCready[iface] && str && !result.error){
        USB_sendstr(iface, str);
        USB_putbyte(iface, '\n');
    }
    if(result.error) spi_setup(1+idx); // reinit SPI in case of error
    if(the_conf.flags.monit) monitT[idx] = Tms;
    else if(testflag) spi_start_enc(idx);
}

int main(){
    uint32_t lastT = 0, usartT = 0;
    uint8_t oldCDCready[bTotNumEndpoints] = {0};
    StartHSE();
    flashstorage_init();
    hw_setup();
    USBPU_OFF();
    SysTick_Config(72000);
    USB_setup();
#ifndef EBUG
    iwdg_setup();
#endif
    usart_setup();
    USBPU_ON();
    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            //LED_blink(LED0);
            lastT = Tms;
        }
        if(CDCready[I_CMD]){
            int l = USB_receivestr(I_CMD, inbuff, RBINSZ);
            if(l < 0) CMDWRn("ERROR: CMD USB buffer overflow or string was too long");
            else if(l) parse_cmd(inbuff);
            // check if interface connected/disconnected
            // (we CAN'T do much debug output in interrupt functions like linecoding_handler etc, so do it here)
            for(int i = 0; i < bTotNumEndpoints; ++i){
                if(oldCDCready[i] != CDCready[i]){
                    CMDWR("Interface ");
                    CMDWR(u2str(i));
                    USB_putbyte(I_CMD, ' ');
                    if(CDCready[i] == 0) CMDWR("dis");
                    CMDWRn("connected");
                    oldCDCready[i] = CDCready[i];
                }
            }
        }
        proc_enc(0);
        proc_enc(1);
        int started[2] = {0, 0};
        if(the_conf.flags.monit){
            for(int i = 0; i < 2; ++i){
                if(Tms - monitT[i] >= the_conf.monittime){
                    spi_start_enc(i);
                    started[i] = 1;
                }
            }
        }
        if(the_conf.send232_interval && Tms - usartT >= the_conf.send232_interval){
            for(int i = 0; i < 2; ++i) if(!started[i]) spi_start_enc(i);
            usartT = Tms;
        }
    }
    return 0;
}
