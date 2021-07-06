/*
 * This file is part of the F0testbrd project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "i2c.h"
#include "proto.h"
#include "spi.h"
#include "usart.h"
#include "usb.h"
#include "usb_lib.h"

#define LOCBUFFSZ       (32)
// local buffer for I2C and SPI data to send
static uint8_t locBuffer[LOCBUFFSZ];

void USB_sendstr(const char *str){
    uint16_t l = 0;
    const char *b = str;
    while(*b++) ++l;
    USB_send((const uint8_t*)str, l);
}

static inline char *chPWM(volatile uint32_t *reg, char *buf){
    char *lbuf = buf;
    lbuf = omit_spaces(lbuf);
    char cmd = *lbuf;
    lbuf = omit_spaces(lbuf + 1);
    uint32_t N;
    if(getnum(lbuf, &N) == lbuf) N = 1;
    uint32_t oldval = *reg;
    if(cmd == '-'){ // decrement
        if(oldval < N) return "Already at minimum";
        else *reg -= N;
    }else if(cmd == '+'){ // increment
        if(oldval + N > 255) return "Already at maximum";
        else *reg += N;
    }else{
        USND("Wrong command: ");
        return buf;
    }
    return "OK";
}

static inline char *TIM3pwm(char *buf){
    uint8_t channel = *buf - '1';
    if(channel > 3) return "Wrong channel number";
    volatile uint32_t *reg = &TIM3->CCR1;
    return chPWM(&reg[channel], buf+1);
}

static inline char *getPWMvals(){
    USND("TIM1CH1: "); USB_sendstr(u2str(TIM1->CCR1));
    USND("\nTIM3CH1: "); USB_sendstr(u2str(TIM3->CCR1));
    USND("\nTIM3CH2: "); USB_sendstr(u2str(TIM3->CCR2));
    USND("\nTIM3CH3: "); USB_sendstr(u2str(TIM3->CCR3));
    USND("\nTIM3CH4: "); USB_sendstr(u2str(TIM3->CCR4));
    USND("\n");
    return NULL;
}

static inline char *USARTsend(char *buf){
    uint32_t N;
    if(buf == getnum(buf, &N)) return "Point number of USART";
    if(N < 1 || N > USARTNUM) return "Wrong USART number";
    buf = omit_spaces(buf + 1);
    usart_send(N, buf);
    transmit_tbuf();
    return "OK";
}

// read N numbers from buf, @return 0 if wrong or none
static uint16_t readNnumbers(char *buf){
    uint32_t D;
    char *nxt;
    uint16_t N = 0;
    while((nxt = getnum(buf, &D)) && nxt != buf && N < LOCBUFFSZ){
        buf = nxt;
        locBuffer[N++] = (uint8_t) D&0xff;
        USND("add byte: "); USB_sendstr(uhex2str(D&0xff)); USND("\n");
    }
    USND("Send "); USB_sendstr(u2str(N)); USND(" bytes\n");
    return N;
}

// dump memory buffer
static void hexdump(uint8_t *arr, uint16_t len){
    char buf[52], *bptr = buf;
    for(uint16_t l = 0; l < len; ++l, ++arr){
        for(int16_t j = 1; j > -1; --j){
            register uint8_t half = (*arr >> (4*j)) & 0x0f;
            if(half < 10) *bptr++ = half + '0';
            else *bptr++ = half - 10 + 'a';
        }
        if(l % 16 == 15){
            *bptr++ = '\n';
            *bptr = 0;
            USB_sendstr(buf);
            bptr = buf;
        }else *bptr++ = ' ';
    }
    if(bptr != buf){
        *bptr++ = '\n';
        *bptr = 0;
        USB_sendstr(buf);
    }
}

static uint8_t i2cinited = 0;
static inline char *setupI2C(char *buf){
    buf = omit_spaces(buf);
    if(*buf < '0' || *buf > '2') return "Wrong speed";
    i2c_setup(*buf - '0');
    i2cinited = 1;
    return "OK";
}

static uint8_t I2Caddress = 0;
static inline char *saI2C(char *buf){
    uint32_t addr;
    if(!getnum(buf, &addr) || addr > 0x7f) return "Wrong address";
    I2Caddress = (uint8_t) addr << 1;
    USND("I2Caddr="); USB_sendstr(uhex2str(addr)); USND("\n");
    return "OK";
}
static inline void rdI2C(char *buf){
    uint32_t N;
    char *nxt = getnum(buf, &N);
    if(!nxt || buf == nxt || N > 0xff){
        USND("Bad register number\n");
        return;
    }
    buf = nxt;
    uint8_t reg = N;
    nxt = getnum(buf, &N);
    if(!nxt || buf == nxt || N > LOCBUFFSZ){
        USND("Bad length\n");
        return;
    }
    if(!read_i2c_reg(I2Caddress, reg, locBuffer, N)){
        USND("Error reading I2C\n");
        return;
    }
    if(N == 0){ USND("OK"); return; }
    USND("Register "); USB_sendstr(uhex2str(reg)); USND(":\n");
    hexdump(locBuffer, N);
    /*for(uint32_t i = 0; i < N; ++i){
        if(i < 10) USND(" ");
        USB_sendstr(u2str(i)); USND(": "); USB_sendstr(uhex2str(locBuffer[i]));
        USND("\n");
    }*/
}
static inline char *wrI2C(char *buf){
    uint16_t N = readNnumbers(buf);
    if(!write_i2c(I2Caddress, locBuffer, N)) return "Error writing I2C";
    return "OK";
}

static inline char *DAC_chval(char *buf){
    uint32_t D;
    char *nxt = getnum(buf, &D);
    if(!nxt || nxt == buf || D > 4095) return "Wrong DAC amplitude\n";
    DAC->DHR12R1 = D;
    return "OK";
}

// write locBuffer to SPI
static inline void wrSPI(int SPIidx, char *buf){
    uint16_t N = readNnumbers(buf);
    if(N < 1){
        *(uint8_t *)&(SPI1->DR) = 0xea;
        USND("Enter at least 1 number (max: ");
        USB_sendstr(u2str(LOCBUFFSZ)); USND(")\n");
        return;
    }
    if(SPI_transmit(SPIidx, locBuffer, N)) USND("Error: busy?\n");
    else USND("done");
}
static inline void rdSPI(int SPIidx){
    if(SPI_isoverflow(SPIidx)) USND("SPI buffer overflow\n");
    uint8_t len = LOCBUFFSZ;
    if(SPI_getdata(SPIidx, locBuffer, &len)){
        USND("Error getting data: busy?\n");
        return;
    }
    if(len == 0){
        USND("Nothing to read\n");
        return;
    }
    if(len > LOCBUFFSZ) USND("Can't get full message: buffer too small\n");
    USND("SPI data:\n");
    hexdump(locBuffer, len);
}

static inline char *procSPI(char *buf){
    int idx = 0;
    if(*buf == 'p') idx = 1;
    buf = omit_spaces(buf + 1);
    if(*buf == 'w')  wrSPI(idx, buf + 1);
    else if(*buf == 'r') rdSPI(idx);
    else return "Enter `w` and data to write, `r` - to read";
    return NULL;
}

const char *helpstring =
        "+/-[num] - increase/decrease TIM1ch1 PWM by 1 or `num`\n"
        "1..4'+'/'-'[num] - increase/decrease TIM3chN PWM by 1 or `num`\n"
        "A - get ADC values\n"
        "dx - change DAC lowcal to x\n"
        "g - get PWM values\n"
        "i0..3 - setup I2C with lowest..highest speed (5.8, 10 and 100kHz)\n"
        "Ia addr - set I2C address\n"
        "Iw bytes - send bytes (hex/dec/oct/bin) to I2C\n"
        "Ir reg n - read n bytes from I2C reg\n"
        "Is - scan I2C bus\n"
        "L - send long string over USB\n"
        "m - monitor ADC on/off\n"
        "Pw bytes - send bytes over SPI1\n"
        "pw bytes - send bytes over SPI2\n"
        "Pr - get data from SPI1\n"
        "pr - get data from SPI2\n"
        "R - software reset\n"
        "S - send short string over USB\n"
        "s - setup SPI (and turn off USARTs)\n"
        "Ux str - send string to USARTx (1..3)\n"
        "u - setup USARTs (and turn off SPI)\n"
        "T - MCU temperature\n"
        "V - Vdd\n"
        "W - test watchdog\n"
;

void printADCvals(){
    USND("AIN0: "); USB_sendstr(u2str(getADCval(0)));
    USND(" ("); USB_sendstr(u2str(getADCvoltage(0)));
    USND("/100 V)\nAIN1: "); USB_sendstr(u2str(getADCval(1)));
    USND(" ("); USB_sendstr(u2str(getADCvoltage(1)));
    USND("/100 V)\nAIN5: "); USB_sendstr(u2str(getADCval(2)));
    USND(" ("); USB_sendstr(u2str(getADCvoltage(2)));
    USND("/100 V)\n");
}

const char *parse_cmd(char *buf){
    // "long" commands
    switch(*buf){
        case '+':
        case '-':
            return chPWM(&TIM1->CCR1, buf);
        break;
        case '1':
        case '2':
        case '3':
        case '4':
            return TIM3pwm(buf);
        break;
        case 'd':
            return DAC_chval(buf + 1);
        case 'i':
            return setupI2C(buf + 1);
        break;
        case 'I':
            if(!i2cinited) return "Init I2C first";
            buf = omit_spaces(buf + 1);
            if(*buf == 'a') return saI2C(buf + 1);
            else if(*buf == 'r'){ rdI2C(buf + 1); return NULL; }
            else if(*buf == 'w') return wrI2C(buf + 1);
            else if(*buf == 's') i2c_init_scan_mode();
            else return "Command should be 'Ia', 'Iw', 'Ir' or 'Is'\n";
        break;
        case 'p':
        case 'P':
            return procSPI(buf);
        break;
        case 'U':
            return USARTsend(buf + 1);
        break;
    }
    // "short" commands
    if(buf[1] != '\n') return buf;
    switch(*buf){
        case 'g':
            return getPWMvals();
        break;
        case 'A':
            printADCvals();
        break;
        case 'L':
            USND("Very long test string for USB (it's length is more than 64 bytes).\n"
                     "This is another part of the string! Can you see all of this?\n");
            return "Long test sent";
        break;
        case 'm':
            ADCmon = !ADCmon;
            USND("Monitoring is ");
            if(ADCmon) USND("on\n");
            else USND("off\n");
        break;
        case 'R':
            USND("Soft reset\n");
            //SEND("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'S':
            USND("Test string for USB\n");
            return "Short test sent";
        break;
        case 's':
            USND("SPI are ON, USART are OFF\n");
            usart_stop();
            spi_setup();
        break;
        case 'T':
            return u2str(getMCUtemp());
        break;
        case 'u':
            USND("USART are ON, SPI are OFF\n");
            spi_stop();
            usart_setup();
        break;
        case 'V':
            return u2str(getVdd());
        break;
        case 'W':
            USND("Wait for reboot\n");
            //SEND("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return helpstring;
        break;
    }
    return NULL;
}

// usb getline
char *get_USB(){
    static char tmpbuf[129], *curptr = tmpbuf;
    static int rest = 128;
    int x = USB_receive((uint8_t*)curptr);
    curptr[x] = 0;
    if(!x) return NULL;
    if(curptr[x-1] == '\n'){
        curptr = tmpbuf;
        rest = 128;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = 128;
    }
    return NULL;
}


static char *_2str(uint32_t  val, uint8_t minus){
    static char strbuf[12];
    char *bufptr = &strbuf[11];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    if(minus) *(--bufptr) = '-';
    return bufptr;
}

// return string with number `val`
char *u2str(uint32_t val){
    return _2str(val, 0);
}
char *i2str(int32_t i){
    uint8_t minus = 0;
    uint32_t val;
    if(i < 0){
        minus = 1;
        val = -i;
    }else val = i;
    return _2str(val, minus);
}
// print 32bit unsigned int as hex
char *uhex2str(uint32_t val){
    static char buf[12] = "0x";
    int npos = 2;
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
            if(half < 10) buf[npos++] = half + '0';
            else buf[npos++] = half - 10 + 'a';
        }
    }
    buf[npos] = 0;
    return buf;
}

char *omit_spaces(const char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return (char*)buf;
}

// In case of overflow return `buf` and N==0xffffffff
// read decimal number & return pointer to next non-number symbol
static char *getdec(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '9'){
            break;
        }
        if(num > 429496729 || (num == 429496729 && c > '5')){ // overflow
            *N = 0xffffff;
            return start;
        }
        num *= 10;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read hexadecimal number (without 0x prefix!)
static char *gethex(const char *buf, uint32_t *N){
    char *start = (char*)buf;
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
            if(num & 0xf0000000){ // overflow
                *N = 0xffffff;
                return start;
            }
            num <<= 4;
            num += c - M;
        }else{
            break;
        }
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read octal number (without 0 prefix!)
static char *getoct(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '7'){
            break;
        }
        if(num & 0xe0000000){ // overflow
            *N = 0xffffff;
            return start;
        }
        num <<= 3;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read binary number (without b prefix!)
static char *getbin(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '1'){
            break;
        }
        if(num & 0x80000000){ // overflow
            *N = 0xffffff;
            return start;
        }
        num <<= 1;
        if(c == '1') num |= 1;
        ++buf;
    }
    *N = num;
    return (char*)buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf
 *      (if it is == buf, there's no number or if *N==0xffffffff there was overflow)
 */
char *getnum(const char *txt, uint32_t *N){
    char *nxt = NULL;
    char *s = omit_spaces(txt);
    if(*s == '0'){ // hex, oct or 0
        if(s[1] == 'x' || s[1] == 'X'){ // hex
            nxt = gethex(s+2, N);
            if(nxt == s+2) nxt = (char*)txt;
        }else if(s[1] > '0'-1 && s[1] < '8'){ // oct
            nxt = getoct(s+1, N);
            if(nxt == s+1) nxt = (char*)txt;
        }else{ // 0
            nxt = s+1;
            *N = 0;
        }
    }else if(*s == 'b' || *s == 'B'){
        nxt = getbin(s+1, N);
        if(nxt == s+1) nxt = (char*)txt;
    }else{
        nxt = getdec(s, N);
        if(nxt == s) nxt = (char*)txt;
    }
    return nxt;
}
