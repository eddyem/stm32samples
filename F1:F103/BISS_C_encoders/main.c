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
//#include "usart.h"
#include "usb_dev.h"

volatile uint32_t Tms = 0;
static char inbuff[RBINSZ];

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
    uint8_t encbuf[ENCODER_BUFSZ];
    static uint32_t lastMSG[2], gotgood[2], gotwrong[2];
    int iface = idx ? I_Y : I_X;
    char ifacechr = idx ? 'Y' : 'X';
    if(CDCready[iface]){
        int l = USB_receivestr(iface, inbuff, RBINSZ);
        if(CDCready[I_CMD]){
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
    }
    if(!spi_read_enc(idx, encbuf)) return;
    BiSS_Frame result = parse_biss_frame(encbuf, ENCODER_BUFSZ);
    char *str = result.crc_valid ? u2str(result.data) : NULL;
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
        }else{
            printResult(&result);
            CMDWR("ENC"); USB_putbyte(I_CMD, ifacechr);
            USB_putbyte(I_CMD, '=');
            hexdump(I_CMD, encbuf, ENCODER_BUFSZ);
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
    if(testflag) spi_start_enc(idx);
}

int main(){
    uint32_t lastT = 0;//, lastS = 0;
    StartHSE();
    flashstorage_init();
    hw_setup();
    USBPU_OFF();
    SysTick_Config(72000);
/*
#ifdef EBUG
    usart_setup();
    uint32_t tt = 0;
#endif
*/
    USB_setup();
#ifndef EBUG
    iwdg_setup();
#endif
    USBPU_ON();
    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
/*
#ifdef EBUG
        if(Tms != tt){
            __disable_irq();
            usart_transmit();
            tt = Tms;
            __enable_irq();
        }
#endif
*/
        if(CDCready[I_CMD]){
            /*if(Tms - lastS > 9999){
                USB_sendstr(I_CMD, "Tms=");
                USB_sendstr(I_CMD, u2str(Tms));
                CMDn();
                lastS = Tms;
            }*/
            int l = USB_receivestr(I_CMD, inbuff, RBINSZ);
            if(l < 0) CMDWRn("ERROR: CMD USB buffer overflow or string was too long");
            else if(l) parse_cmd(inbuff);
        }
        proc_enc(0);
        proc_enc(1);
    }
    return 0;
}
