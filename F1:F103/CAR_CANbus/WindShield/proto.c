/*
 * This file is part of the windshield project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "adc.h"
#include "buttons.h"
#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "strfunc.h"
#include "usart.h"
#include "version.inc"

flags_t flags = {
    .can_monitor = 0
};

const char *helpmsg =
    "https://github.com/eddyem/stm32samples/tree/master/F1:F103/CAR_CANbus/WindShield build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
    "'0' - stop PWM\n"
    "'1' - start PWM\n"
    "'a' - RAW ADC values\n"
    "'b' - break motor (extremal stop)\n"
    "'f' - set PWM freq (Hz)\n"
    "'d xx' - set dT to xx ms\n"
    "'m xx' - move motor up (1), down (2), stop (-2) or emerg stop (-1)\n"
    "'p y xx' - set PWM y (l/r) to xx (0..100)\n"
    "'s' - get raw VSEN value\n"
    "'t' - get MCU temperature (*10)\n"
    "'u y xx' - turn on (xx==1) or off (xx==0) y (l/r) MOSFET\n"
    "'v' - get Vdd (*100)\n"
    "'B' - current buttons state\n"
    "'M' - CAN bus monitoring on/off\n"
    "'R' - software reset\n"
    "'T' - get time from start (ms)\n"
;

static void printans(int res){
    if(res) usart_send("OK");
    else usart_send("FAIL");
}

static void usetter(int(*fn)(uint32_t), char* str){
    uint32_t d = 0;
    if(str == getnum(str, &d)) printans(FALSE);
    else printans(fn(d));
}
static void isetter(int(*fn)(int32_t), char* str){
    int32_t d = 0;
    if(str == getint(str, &d)) printans(FALSE);
    else printans(fn(d));
}
static char pwmch = 0;
static int setdir(char d){
    pwmch = 0;
    switch(d){
        case 'r':
        case 'l':
            pwmch = d;
            break;
        default:
            usart_send("Direction: l/r\n");
            return FALSE;
    }
    return TRUE;
}
static int pwmvalsetter(uint32_t pwm){
    if(pwm > PWMMAX) return FALSE;
    if(pwmch == 'l') set_pwm(PWM_LEFT, pwm);
    else if(pwmch == 'r') set_pwm(PWM_RIGHT, pwm);
    else return FALSE;
    return TRUE;
}
static int upsetter(uint32_t set){
    int L = get_pwm(PWM_LEFT), R = get_pwm(PWM_RIGHT);
    switch(pwmch){
        case 'l':
            if(L && set) return FALSE; // shortened
            if(set) set_up(UP_LEFT);
            else up_off();
        break;
        case 'r':
            if(R && set) return FALSE;
            if(set) set_up(UP_RIGHT);
            else up_off();
        break;
        default:
            return FALSE;
    }
    return TRUE;
}
static int startPWM(){
    int l = read_upL(), r = read_upR();
    if(l && r) return FALSE; // both upper active (impossible case, but what if?)
    int L = get_pwm(PWM_LEFT), R = get_pwm(PWM_RIGHT);
    if(L && R) return FALSE; // both PWM active
    if((l && L) || (r && R)) return FALSE; // shortened
    start_pwm();
    return TRUE;
}

static const char* keyz[KEY_AMOUNT] = {
    [HALL_D] = "Hall down",
    [HALL_U] = "Hall up",
    [KEY_D] = "Key down",
    [KEY_U] = "Key up",
    [DIR_D] = "Ext down",
    [DIR_U] = "Ext up"
};


extern keybase allkeys[];
TRUE_INLINE void showbtnstate(){
    for(int i = 0; i < KEY_AMOUNT; ++i){
        usart_send(keyz[i]); usart_send(": ");
        switch(keyevt(i)){
        case EVT_PRESS:
            usart_send("pressed");
        break;
        case EVT_HOLD:
            usart_send("holded");
        break;
        case EVT_RELEASE:
            usart_send("released after hold");
        break;
        default:
            usart_send("none");
        }
        newline();
    }
    for(int i = 0; i < KEY_AMOUNT; ++i){
        usart_send(keyz[i]); usart_send(": ");
        keybase *k = &allkeys[i];
        if(!PRESSED(k->port, k->pin)) usart_send("not ");
        usart_send("pressed, ");
        if(!k->inverted) usart_send("not ");
        usart_send("inverted");
        newline();
    }
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 */
void cmd_parser(char *txt){
    char _1st = txt[0];
 /* usart_send("Got: '");
  usart_send(txt);
  usart_send("'\n");*/
    char *nxtsym = (char*)omit_spaces(txt + 1);
    uint32_t N = 0;
    int proc = 1;
    switch(_1st){ // parse long commands here
        case 'd':
            usetter(set_dT, nxtsym);
        break;
        case 'f':
            if(nxtsym != getnum(nxtsym, &N)){
                if(N < 100 || N > 100000) usart_send("100 <= F <= 100000");
                else TIM3->PSC = (TIM3FREQ/(N*PWMMAX)) - 1;
            }
            usart_send("TIM3->PSC="); usart_send(u2str(TIM3->PSC));
        break;
        case 'm':
            isetter(motor_ctl, nxtsym);
        break;
        case 'p':
            if(setdir(*nxtsym)){
                nxtsym = (char*)omit_spaces(nxtsym + 1);
                usetter(pwmvalsetter, nxtsym); newline();
            }
            usart_send("PWM1="); usart_send(u2str(get_pwm(PWM_RIGHT)));
            usart_send("\nPWM2="); usart_send(u2str(get_pwm(PWM_LEFT)));
        break;
        case 'u':
            if(setdir(*nxtsym)){
                nxtsym = (char*)omit_spaces(nxtsym + 1);
                usetter(upsetter, nxtsym); newline();
            }
            usart_send("UPL="); usart_putchar('0' + read_upL());
            usart_send("\nUPR="); usart_putchar('0' + read_upR());
        break;
        default:
            proc = 0;
        break;
    }
    if(proc) goto eof;
    if(txt[1] != 0) _1st = '?'; // help for wrong message length
    switch(_1st){
        case '0':
            stop_pwm(); printans(TRUE);
        break;
        case '1':
            printans(startPWM());
        break;
        case 'a':
            for(int i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i){
                usart_send("ADC"); usart_putchar('0' + i);
                usart_putchar('='); usart_send(u2str(getADCval(i)));
                newline();
            }
        break;
        case 'b':
            motor_break(); printans(TRUE);
        break;
        case 's':
            usart_send("VSEN="); usart_send(u2str(getADCval(CHVSEN)));
        break;
        case 't':
            usart_send("TMCU="); usart_send(u2str(getMCUtemp()));
        break;
        case 'v':
            usart_send("VDD="); usart_send(u2str(getVdd()));
        break;
        case 'B':
            showbtnstate();
        break;
        case 'M':
            flags.can_monitor = !flags.can_monitor;
            usart_send("CAN monitoring ");
            if(flags.can_monitor) usart_send("ON");
            else usart_send("OFF");
        break;
        case 'R':
            usart_send("Soft reset\n\n");
            usart_transmit();
            // wait until DMA & USART done
            while(!usart_txrdy);
            while(!(USART1->SR & USART_SR_TC));
            USART1->CR1 = 0; // stop USART
            NVIC_SystemReset();
        break;
        case 'T':
            usart_send("Time=");
            printu(Tms);
        break;
        default: // help
            usart_send(helpmsg);
        break;
    }
eof:
    newline();
}
