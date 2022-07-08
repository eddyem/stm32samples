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
#include "adc.h"
#include "can.h"
#include "can_process.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "spi.h"
#include "usart.h"
#include "usb.h"
#include "usb_lib.h"

#include <string.h>

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

static void uhex(uint8_t *arr, uint8_t l){
    if(l > 4 || l == 0) return;
    char buf[13] = "0x";
    int8_t i, j, bidx = 2;
    for(i = 0; i < l; ++i, ++arr){
        for(j = 1; j > -1; --j){
            uint8_t half = (*arr >> (4*j)) & 0x0f;
            if(half < 10) buf[bidx++] = half + '0';
            else buf[bidx++] = half - 10 + 'a';
        }
    }
    buf[bidx++] = '\n';
    buf[bidx] = 0;
    SEND(buf);
    sendbuf();
}

/*
static void print32bits(uint8_t *arr){
    uint8_t _16[2];
    _16[0] = (arr[0]>>4)&3; _16[1] = (arr[0]<<4) | (arr[1] >> 4);
    uhex(_16, 2);
}*/

#define USBBUF 63
// usb getline
static char *get_USB(){
    static char tmpbuf[USBBUF+1], *curptr = tmpbuf;
    static int rest = USBBUF;
    uint8_t x = USB_receive((uint8_t*)curptr);
    if(!x) return NULL;
    curptr[x] = 0;
    if(x == 1 && *curptr == 0x7f){ // backspace
        if(curptr > tmpbuf){
            --curptr;
            USND("\b \b");
        }
        return NULL;
    }
    USB_sendstr(curptr); // echo
    if(curptr[x-1] == '\n'){
        curptr = tmpbuf;
        rest = USBBUF;
        // omit empty lines
        if(tmpbuf[0] == '\n') return NULL;
        // and wrong empty lines
        if(tmpbuf[0] == '\r' && tmpbuf[1] == '\n') return NULL;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = USBBUF;
    }
    return NULL;
}

// send encoder data
static void CANsendEnc(uint8_t *buf){
    uint32_t ctr = TIM2->CNT;
    uint8_t msg[8];
    memcpy(msg, buf, 4);
    msg[4] = 0;
    msg[5] = (ctr >> 16) & 0xff;
    msg[6] = (ctr >> 8 ) & 0xff;
    msg[7] = (ctr >> 0 ) & 0xff;
    can_send(msg, 8, the_conf.encoderID);
}
// send limit-switches data
static void CANsendLim(){
    uint8_t msg[8] = {0};
    msg[2] = ESW_STATE();
    can_send(msg, 8, the_conf.limitsID);
}

int main(void){
    uint32_t    lastT = 0,  // send buffer time
                encT = 0    // send encoder & limit-switches data time
    ;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup(); // + read board address
    usart_setup();
    adc_setup();
    flashstorage_init();
    CAN_setup(the_conf.CANspeed);
    USB_setup();
    spi_setup();
    tim2_Setup();
    iwdg_setup();

     while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            sendbuf();
            lastT = Tms;
        }
        if((the_conf.sendenc || the_conf.sendsw) && Tms - encT > ENCODER_PERIOD){
            encT = Tms;
            if(the_conf.sendenc) SPI_transmit(NULL, 4);
            if(the_conf.sendsw) CANsendLim();
        }
        can_proc();
        usb_proc();
        usart_proc(); // switch RS-485 to Rx after last byte sent
        char *txt;
        if((txt = get_USB())){
            cmd_parser(txt, TARGET_USB);
        }
        IWDG->KR = IWDG_REFRESH;
        if(usartrx()){ // usart1 received data, store in in buffer
            usart_getline(&txt);
            cmd_parser(txt, TARGET_USART);
        }
        IWDG->KR = IWDG_REFRESH;
        can_messages_proc();
        uint8_t buf[4];
        uint8_t a = SPI_getdata(buf, 4);
        if(a){
            if(the_conf.sendenc) CANsendEnc(buf);
            SEND("Inverse code: "); uhex(buf, 4);
            uint32_t *_u = (uint32_t*) buf; *_u = ~*_u;
            SEND("Direct code: "); uhex(buf, 4);
        }
/*
        if(ostctr != Tms){ // check steppers not more than once in 1ms
            ostctr = Tms;
        }
*/
    }
    return 0;
}

