/*
 * This file is part of the TM1637 project.
 * Copyright 2019 Edward Emelianov <eddy@sao.ru>.
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
#include "hardware.h"
#include "i2c.h"
#include "protocol.h"
#include "usart.h"

#define DOT   1
#define MIN   0b00000010
#define DIG0  0b11111100
#define DIG1  0b01100000
#define DIG2  0b11011010
#define DIG3  0b11110010
#define DIG4  0b01100110
#define DIG5  0b10110110
#define DIG6  0b10111110
#define DIG7  0b11100000
#define DIG8  0b11111110
#define DIG9  0b11110110
#define DIGA  0b11101110
#define DIGB  0b00111110
#define DIGC  0b10011100
#define DIGD  0b01111010
#define DIGE  0b10011110
#define DIGF  0b10001110
#define DIGN  0b00000000

static const uint8_t digits[] = {DIG0, DIG1, DIG2, DIG3, DIG4, DIG5, DIG6, DIG7, DIG8, DIG9,
                                DIGA, DIGB, DIGC, DIGD, DIGE, DIGF};

static void putdata(uint8_t var){
    put_string("Got: ");
    put_char('0'+var);
    put_char('\n');
    // 0x40, 0xC0, ff .. 8f ; (0x44, 0xc0, 0x55, 0x89)
    const uint8_t commands[] = {2, 3, DIG0, DIG1, DIG2, DIG3, 0xf1, 0x22, 3, 0xaa, 0x91};
    //const uint8_t commands[] = {2, 3, 2, 4, 8, 0x10, 0xf1, 0x22, 3, 0xaa, 0x91};
    const uint8_t commands1[] = {2, 3, DIG4, DIG5|DOT, DIG6, DIG7, 0xf1, 0x22, 3, 0xee, 0x91};
    //const uint8_t commands1[] = {2, 3, 0x20, 0x40|DOT, 0x80, DIG5, 0xf1, 0x22, 3, 0xee, 0x91};
    switch(var){
        case 0:
            write_i2c(commands, 1);
            write_i2c(&commands[1], 5);
            write_i2c(&commands[6], 1);
        break;
        case 1:
            write_i2c(&commands[7], 1);
            write_i2c(&commands[8], 2);
            write_i2c(&commands[10], 1);
        break;
        case 2:
            write_i2c(commands1, 1);
            write_i2c(&commands1[1], 5);
            write_i2c(&commands1[6], 1);
        break;
        case 3:
            write_i2c(&commands1[7], 1);
            write_i2c(&commands1[8], 2);
            write_i2c(&commands1[10], 1);
        break;
    }
}

static void getk(){
    //uint8_t k[] = {0x42, 0xff};
    uint8_t k;
    if(read_i2c(0x42, &k)){ // 0x42
    //if(write_i2c(k,2)){
        put_string("Keys: ");
        put_uint(k);
        put_char('\n');
    }else SEND("ERR\n");
}

static uint8_t display_number(int32_t N){
    if(N < -999 || N > 9999) return 1;
    uint8_t buf[] = {2, 3, DIGN, DIGN, DIGN, DIGN, 0xf1};
    if(N < 0){
        buf[2] = MIN;
        N = -N;
    }
    if(N == 0){
        buf[5] = DIG0;
    }else{
        for(uint8_t idx = 5; idx > 1 && N; --idx){
            buf[idx] = digits[N%10];
            N /= 10;
        }
    }
    write_i2c(buf, 1);
    write_i2c(&buf[1], 5);
    write_i2c(&buf[6], 1);
    return 0;
}

static uint8_t display_hex(int32_t N){
    if(N < -0xfff || N > 0xffff) return 1;
    uint8_t buf[] = {2, 3, DIGN, DIGN, DIGN, DIGN, 0xf1};
    if(N < 0){
        buf[2] = MIN;
        N = -N;
    }
    if(N == 0){
        buf[5] = DIG0;
    }else{
        for(uint8_t idx = 5; idx > 1 && N; --idx){
            buf[idx] = digits[N&0xf];
            N >>= 4;
        }
    }
    write_i2c(buf, 1);
    write_i2c(&buf[1], 5);
    write_i2c(&buf[6], 1);
    return 0;
}

static void dispnum(const char *n, uint8_t hex){
    int32_t N = -1;
    uint8_t er = 0;
    if(!getnum(n, &N)) er = 1;
    else{
        if(hex) er = display_hex(N);
        else er = display_number(N);
    }
    if(er) put_string("Wrong number!\n");
}

static void dispabcd(){
    uint8_t buf[] = {2, 3, DIGA, DIGB, DIGC, DIGD, 0xf1};
    write_i2c(buf, 1);
    write_i2c(&buf[1], 5);
    write_i2c(&buf[6], 1);
}

/**
 * @brief process_command - command parser
 * @param command - command text (all inside [] without spaces)
 * @return text to send over terminal or NULL
 */
char *process_command(const char *command){
    char *ret = NULL;
    usart1_sendbuf(); // send buffer (if it is already filled)
    if(*command < '0' || *command > '9'){
        switch(*command){
            case '?': // help
                SEND_BLK(
                    "0..9 - send data\n"
                    "A - display 'ABCD'\n"
                    "G - get keqboard status\n"
                    "Hhex - display 'hex'\n"
                    "Nnum - display 'num'\n"
                    "R - reset\n"
                    );
            break;
            case 'A':
                dispabcd();
            break;
            case 'G':
                getk();
            break;
            case 'H':
                dispnum(++command, 1);
            break;
            case 'N':
                dispnum(++command, 0);
            break;
            case 'R': // reset MCU
                NVIC_SystemReset();
            break;
        }
    }else putdata(*command - '0');
    usart1_sendbuf();
    return ret;
}
