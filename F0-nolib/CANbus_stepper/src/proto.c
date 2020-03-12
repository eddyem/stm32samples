/*
 *                                                                                                  geany_encoding=koi8-r
 * proto.c
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */
#include "adc.h"
#include "hardware.h"
#include "proto.h"
#include "usart.h"
#include "usb.h"
#include <string.h> // strlen, strcpy(

extern volatile uint8_t canerror;

#define BUFSZ UARTBUFSZ

static char buff[BUFSZ+1], *bptr = buff;
static uint8_t blen = 0, // length of data in `buff`
    USBcmd = 0; // ==1 if buffer prepared for USB

void buftgt(uint8_t isUSB){
    USBcmd = isUSB;
}

void sendbuf(){
    IWDG->KR = IWDG_REFRESH;
    if(blen == 0) return;
    if(USBcmd){
        *bptr = 0;
        USB_send(buff);
    }else while(LINE_BUSY == usart_send(buff, blen)){IWDG->KR = IWDG_REFRESH;}
    bptr = buff;
    blen = 0;
}

void addtobuf(const char *txt){
    IWDG->KR = IWDG_REFRESH;
    int l = strlen(txt);
    if(l > BUFSZ){
        sendbuf();
        if(USBcmd) USB_send(txt);
        else usart_send_blocking(txt, l);
    }else{
        if(blen+l > BUFSZ){
            sendbuf();
        }
        strcpy(bptr, txt);
        bptr += l;
    }
    blen += l;
}

void bufputchar(char ch){
    if(blen > BUFSZ-1){
        sendbuf();
    }
    *bptr++ = ch;
    ++blen;
}

// show all ADC values
static inline void showADCvals(){
    char msg[] = "ADCn=";
    for(int n = 0; n < NUMBER_OF_ADC_CHANNELS; ++n){
        msg[3] = n + '0';
        addtobuf(msg);
        printu(getADCval(n));
        newline();
    }
}

static inline void printmcut(){
    SEND("MCUT=");
    int32_t T = getMCUtemp();
    if(T < 0){
        bufputchar('-');
        T = -T;
    }
    printu(T);
    newline();
}

static inline void showUIvals(){
    uint16_t *vals = getUval();
    SEND("V12="); printu(vals[0]);
    SEND("\nV5="); printu(vals[1]);
    SEND("\nV33="); printu(getVdd());
    newline();
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
 */
void cmd_parser(const char *txt, uint8_t isUSB){
    sendbuf();
    USBcmd = isUSB;
    //int16_t L = strlen(txt);
    char _1st = txt[0];
    switch(_1st){
        case 'a':
            showADCvals();
        break;
        case 'D':
            SEND("Jump to bootloader.\n");
            sendbuf();
            Jump2Boot();
        break;
        case 'g':
            SEND("Board address: ");
            printuhex(refreshBRDaddr());
            newline();
        break;
        case 'j':
            printmcut();
        break;
        case 'k':
            showUIvals();
        break;
        case 't':
            if(ALL_OK != usart_send("TEST test\n", 10))
                addtobuf("Can't send data over RS485\n");
            else addtobuf("Sent\n");
        break;
        default: // help
            SEND(
            "a - get raw ADC values\n"
            "D - switch to bootloader\n"
            "g - get board address\n"
            "j - get MCU temperature\n"
            "k - get U values\n"
            "t - send test sequence over RS-485\n"
            );
        break;
    }
    sendbuf();
}

// print 32bit unsigned int
void printu(uint32_t val){
    char buf[11], *bufptr = &buf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    addtobuf(bufptr);
}

// print 32bit unsigned int as hex
void printuhex(uint32_t val){
    addtobuf("0x");
    uint8_t *ptr = (uint8_t*)&val + 3;
    int i, j;
    for(i = 0; i < 4; ++i, --ptr){
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) bufputchar(half + '0');
            else bufputchar(half - 10 + 'a');
        }
    }
}
