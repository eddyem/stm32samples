/*
 * user_proto.c
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "cdcacm.h"
#include "main.h"
#include "hardware_ini.h"
#include "timer.h"


void help(){
    P("g\tShow this help\n");
    P("t\tShow current approx. time\n");
    P("+\tIncrease speed by 100\n");
    P("-\tDecrease speed by 100\n");
    P("g\tGet current speed\n");
}

/**
 * parce command buffer buf with length len
 * return 0 if buffer processed or len if there's not enough data in buffer
 */
int parce_incoming_buf(char *buf, int len){
    uint8_t command;
    int i = 0;
    for(; i < len; i++){
        command = buf[i];
        if(!command) continue; // omit zero
        switch (command){
            case 'h': // show help
                help();
            break;
            case 't':
                newline();
                print_int(Timer); // be careful for Time >= 2^{31}!!!
                newline();
            break;
            case 'g':
                P("Current speed: ");
                print_int(current_RPM);
                P("rpm\n");
            break;
            case '+':
                increase_speed();
            break;
            case '-':
                decrease_speed();
            break;
            break;
            case '\n': // show newline, space and tab as is
            case '\r':
            case ' ':
            case '\t':
            break;
            default:
                command = '?'; // echo '?' on unknown command in byte mode
        }
        usb_send(command); // echo readed byte
    }
    return 0; // all data processed - 0 bytes leave in buffer
}

/**
 * Send char array wrd thru USB or UART
 */
void prnt(uint8_t *wrd){
    if(!wrd) return;
    while(*wrd) usb_send(*wrd++);
}

/**
 * Print decimal integer value
 * @param N - value to print
 * @param s - function to send a byte
 */
void print_int(int32_t N){
    uint8_t buf[10], L = 0;
    if(N < 0){
        usb_send('-');
        N = -N;
    }
    if(N){
        while(N){
            buf[L++] = N % 10 + '0';
            N /= 10;
        }
        while(L--) usb_send(buf[L]);
    }else usb_send('0');
}

