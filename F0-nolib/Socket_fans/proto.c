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
#include "monitor.h"
#include "proto.h"
#include "usb.h"

#include <string.h> // strlen


uint8_t showMon = 0; // start monitoring @ each 5s

static char buff[BUFSZ+1], *bptr = buff;
static uint8_t blen = 0;

void sendbuf(){
    IWDG->KR = IWDG_REFRESH;
    if(blen == 0) return;
    *bptr = 0;
    USB_sendstr(buff);
    bptr = buff;
    blen = 0;
}

void bufputchar(char ch){
    if(blen > BUFSZ-1){
        sendbuf();
    }
    *bptr++ = ch;
    ++blen;
}

void addtobuf(const char *txt){
    IWDG->KR = IWDG_REFRESH;
    while(*txt) bufputchar(*txt++);
}

char *omit_spaces(char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return buf;
}

// THERE'S NO OVERFLOW PROTECTION IN NUMBER READ PROCEDURES!
// read decimal number
static char *getdec(char *buf, uint32_t *N){
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '9'){
            break;
        }
        num *= 10;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return buf;
}
// read hexadecimal number (without 0x prefix!)
static char *gethex(char *buf, uint32_t *N){
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        uint8_t M = 0;
        if(c >= '0' && c <= '9'){
            M = '0';
        }else if(c >= 'A' && c <= 'F'){
            M = 'A' - 10;
        }else if(c >= 'a' && c <= 'f'){
            M = 'a' - 10;
        }
        if(M){
            num <<= 4;
            num += c - M;
        }else{
            break;
        }
        ++buf;
    }
    *N = num;
    return buf;
}
// read binary number (without 0b prefix!)
static char *getbin(char *buf, uint32_t *N){
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '1'){
            break;
        }
        num <<= 1;
        if(c == '1') num |= 1;
        ++buf;
    }
    *N = num;
    return buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf (if it is == buf, there's no number)
 */
char *getnum(char *txt, uint32_t *N){
    if(*txt == '0'){
        if(txt[1] == 'x' || txt[1] == 'X') return gethex(txt+2, N);
        if(txt[1] == 'b' || txt[1] == 'B') return getbin(txt+2, N);
    }
    return getdec(txt, N);
}

// change pin state
static void onoff(char state, GPIO_TypeDef *port, uint32_t pin, char *text){
    switch(state){
        case '0': // off
            pin_clear(port, pin);
        break;
        case '1': // on
            pin_set(port, pin);
        break;
        default:
        break;
    }
    SEND(text);
    bufputchar('=');
    bufputchar('0' + pin_read(port, pin));
}

// get raw ADC value of channel chno + '0'
static inline void ADCget(char chno){
    if(chno < '0' || chno > '7'){
        SEND("Channel number shoul be from 0 to 7");
        return;
    }
    int nch = chno - '0';
    SEND("ADCVAL");
    bufputchar(chno);
    bufputchar('=');
    printu(getADCval(nch));
}

// get coolerx RPS
static inline void coolerRPS(char n){
    if(n < '0' || n > '1'){
        SEND("Cooler number should be 0 or 1");
        return;
    }
    SEND("RPS");
    bufputchar(n);
    bufputchar('=');
    printu(Coolerspeed[n - '0']);
}

static inline void getPWM(char n){
    if(n < '0' || n > '2'){
        SEND("Cooler number should be from 0 to 2");
        return;
    }
    SEND("PWM");
    bufputchar(n);
    bufputchar('=');
    uint32_t pwm;
    switch(n){
        case '0':
            pwm = TIM1->CCR1;
        break;
        case '1':
            pwm = TIM1->CCR2;
        break;
        default:
            pwm = TIM1->CCR3;
    }
    printu(pwm);
}

// change Coolerx PWM
static inline void changePWM(char *str){
    char channel = *str++;
    str = omit_spaces(str);
    uint32_t rpm;
    getnum(str, &rpm);
    if(rpm > 100){
        SEND("PWM should be from 0 to 100%");
        return;
    }
    switch(channel){
        case '0':
            TIM1->CCR1 = rpm;
        break;
        case '1':
            TIM1->CCR2 = rpm;
        break;
        case '2':
            TIM1->CCR3 = rpm;
        break;
        default:
            SEND("Cooler number should be from 0 to 2");
    }
}

static inline void buzzercmd(char cmd){
    SEND("BUZZER=");
    switch(cmd){
        case '0':
            buzzer = BUZZER_OFF;
            SEND("OFF");
        break;
        case '1':
            buzzer = BUZZER_ON;
            SEND("ON");
        break;
        case 'l':
            buzzer = BUZZER_LONG;
            SEND("LONG");
        break;
        case 's':
            buzzer = BUZZER_SHORT;
            SEND("SHORT");
        break;
        default:
            SEND("BADSTATE");
    }
}

void gett(char chno){
    if(chno < '0' || chno > '3'){
        SEND("Temperature channel should be 0..3");
        return;
    }
    bufputchar('T'); bufputchar(chno); bufputchar('=');
    printi(getNTC(chno - '0'));
}

static inline void chkButtons(){
    SEND("BUTTON0=");
    bufputchar('1' - CHK(BUTTON0)); // buttons are inverted
    SEND("\nBUTTON1=");
    bufputchar('1' - CHK(BUTTON1));
}

// show current device state: T, RPM, PWM, relay
void showState(){
    SEND("TIME=");
    printu(Tms);
    newline();
    for(char x = '0'; x < '4'; ++x){
        gett(x);
        newline();
    }
    SEND("BUZZER=");
    switch(buzzer){
        case BUZZER_ON:
            SEND("ON");
        break;
        case BUZZER_OFF:
            SEND("OFF");
        break;
        case BUZZER_SHORT:
            SEND("SHORT");
        break;
        case BUZZER_LONG:
            SEND("LONG");
        break;
    }
    newline();
    chkButtons();
    newline();
    coolerRPS('0');
    newline();
    coolerRPS('1');
    newline();
    for(char x = '0'; x < '3'; ++x){
        getPWM(x);
        newline();
    }
    onoff('c', COOLER0_port, COOLER0_pin, "COOLER0");
    newline();
    onoff('c', COOLER1_port, COOLER1_pin, "COOLER1");
    newline();
    onoff('c', RELAY_port, RELAY_pin, "RELAY");
    NL();
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
 */
void cmd_parser(char *txt){
    char _1st = txt[0];
    /*
     * parse long commands here
     */
    switch(_1st){
        case '0': // cooler0: set/clear/check
            onoff(txt[1], COOLER0_port, COOLER0_pin, "COOLER0");
            goto eof;
        break;
        case '1': // cooler1: set/clear/check
            onoff(txt[1], COOLER1_port, COOLER1_pin, "COOLER1");
            goto eof;
        break;
        case 'A': // ADC raw values
            ADCget(txt[1]);
            goto eof;
        break;
        case 'b': // buzzer
            buzzercmd(txt[1]);
            goto eof;
        break;
        case 'C': // Get real Cooler RPM
            coolerRPS(txt[1]);
            goto eof;
        break;
        case 'G': // get cooler PWM from settings
            getPWM(txt[1]);
            goto eof;
        break;
        case 'r': // relay: set/clear/check
            onoff(txt[1], RELAY_port, RELAY_pin, "RELAY");
            goto eof;
        break;
        case 'S': // set PWM
            changePWM(txt+1);
            goto eof;
        break;
        case 't':
            gett(txt[1]);
            goto eof;
        break;
    }
    if(txt[1] != '\n') *txt = '?'; // help for wrong message length
    switch(_1st){
        case 'B':
            chkButtons();
        break;
        case 'D':
            SEND("Go into DFU mode\n");
            sendbuf();
            Jump2Boot();
        break;
        case 'E':
            showState();
        break;
        case 'M': // MCU temperature
            SEND("MCUTEMP=");
            printu(getMCUtemp());
        break;
        case 'm': // toggle monitoring
            showMon = !showMon;
            SEND("MONITORING=");
            if(showMon) SEND("on");
            else SEND("off");
        break;
        case 'R':
            SEND("Soft reset\n");
            sendbuf();
            pause_ms(5); // a little pause to transmit data
            NVIC_SystemReset();
        break;
        case 's':
            showSettings();
        break;
        case 'T':
            SEND("TIME=");
            printu(Tms);
            newline();
        break;
        case 'V':
            SEND("V3_3=");
            printu(getVdd());
            SEND("\nV5=");
            printu(getU5());
            SEND("\nV12=");
            printu(getU12());
        break;
        case 'Z':
            adc_setup();
            SEND("ADC restarted\n");
        break;
        case '!':
            SetDontProcess(!GetDontProcess());
            if(GetDontProcess()) SEND("\n\tManual");
            else SEND("\n\tAutomatic");
            SEND(" mode\n");
        break;
        default: // help
            SEND(
            "'0x' - turn cooler0 on/off (x=1/0) or get status (x - any other)\n"
            "'1x' - turn cooler1 on/off (x=1/0) or get status (x - any other)\n"
            "'Ax' - get ADC raw value (x=0..7)\n"
            "'B' - buttons' state\n"
            "'bx' - buzzer: x==0 - off, x==1 - on, x==l - long beeps, x==s - short beeps\n"
            "'Cx' - get cooler x (0/1) RPS\n"
            "'D' - activate DFU mode\n"
            "'E' - show current state\n"
            "'Gx' - get cooler x (0..3) PWM settings\n"
            "'M' - get MCU temperature\n"
            "'m' - toggle monitoring\n"
            "'R' - software reset\n"
            "'rx' - relay on/off (x=1/0) or get status\n"
            "'Sx y' - set coolerx PWM to y\n"
            "'s' - show settings\n"
            "'T' - get time from start (ms)\n"
            "'tx' - get temperature x (0..3)\n"
            "'V' - get voltage\n"
            "'Z' - reinit ADC\n"
            "'!' - switch between manual and automatic modes\n"
            );
        break;
    }
eof:
    newline();
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

// print 32bit signed int
void printi(int32_t val){
    if(val < 0){
        val = -val;
        bufputchar('-');
    }
    printu(val);
}

// print 32bit unsigned int as hex
void printuhex(uint32_t val){
    addtobuf("0x");
    uint8_t *ptr = (uint8_t*)&val + 3;
    int8_t i, j, z=1;
    for(i = 0; i < 4; ++i, --ptr){
        if(*ptr == 0){ // omit leading zeros
            if(i == 3) z = 0;
            if(z) continue;
        }
        else z = 0;
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) bufputchar(half + '0');
            else bufputchar(half - 10 + 'a');
        }
    }
}

