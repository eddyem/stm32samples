/*
 * This file is part of the F1_testbrd project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "hardware.h"
#include "proto.h"
//#include "spi.h"
#include "usart.h"
#include "usb.h"
#include "usb_lib.h"

#define LOCBUFFSZ       (32)
// local buffer for I2C and SPI data to send
static uint8_t locBuffer[LOCBUFFSZ];

static inline const char *chPWM(volatile uint32_t *reg, const char *buf){
    const char *lbuf = buf;
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
        return "Wrong command\n";
    }
    return "OK";
}

static inline const char *TIM3pwm(const char *buf){
    uint8_t channel = *buf - '1';
    if(channel > 3) return "Wrong channel number";
    volatile uint32_t *reg = (uint32_t*)&(TIM3->CCR1);
    return chPWM(&reg[channel], buf+1);
}

static inline char *getPWMvals(int (*sendfun)(const char *s)){
    sendfun("TIM1CH1: "); sendfun(u2str(TIM1->CCR1));
    sendfun("\nTIM3CH1: "); sendfun(u2str(TIM3->CCR1));
    sendfun("\nTIM3CH2: "); sendfun(u2str(TIM3->CCR2));
    sendfun("\nTIM3CH3: "); sendfun(u2str(TIM3->CCR3));
    sendfun("\nTIM3CH4: "); sendfun(u2str(TIM3->CCR4));
    sendfun("\n");
    return NULL;
}

static inline const char *USARTsend(const char *buf){
    usart_send(buf);
    transmit_tbuf();
    return "OK";
}

// read N numbers from buf, @return 0 if wrong or none
static uint16_t readNnumbers(int (*sendfun)(const char *s), const char *buf){
    uint32_t D;
    const char *nxt;
    uint16_t N = 0;
    while((nxt = getnum(buf, &D)) && nxt != buf && N < LOCBUFFSZ){
        buf = nxt;
        locBuffer[N++] = (uint8_t) D&0xff;
        sendfun("add byte: "); sendfun(uhex2str(D&0xff)); sendfun("\n");
    }
    sendfun("Send "); sendfun(u2str(N)); sendfun(" bytes\n");
    return N;
}

static uint8_t i2cinited = 1;
static inline const char *setupI2C(const char *buf){
    buf = omit_spaces(buf);
    if(*buf < '0' || *buf > '2') return "Wrong speed";
    i2c_setup(*buf - '0');
    i2cinited = 1;
    return "OK";
}

static inline const char *saI2C(int (*sendfun)(const char *s), const char *buf){
    uint32_t addr;
    if(!getnum(buf, &addr) || addr > 0x7f) return "Wrong address";
    sendfun("I2Caddr="); sendfun(uhex2str(addr)); sendfun("\n");
    i2c_set_addr7(addr);
    return "OK";
}
static inline void rdI2C(int (*sendfun)(const char *s), const char *buf){
    uint32_t N;
    const char *nxt = getnum(buf, &N);
    if(!nxt || buf == nxt || N > 0xff){
        sendfun("Bad register number\n");
        return;
    }
    buf = nxt;
    uint8_t reg = N;
    nxt = getnum(buf, &N);
    if(!nxt || buf == nxt || N > LOCBUFFSZ){
        sendfun("Bad length\n");
        return;
    }
    if(!read_i2c_reg(reg, locBuffer, N)){
        sendfun("Error reading I2C\n");
        return;
    }
    if(N == 0){ sendfun("OK\n"); return; }
    sendfun("Register "); sendfun(uhex2str(reg)); sendfun(":\n");
    hexdump(sendfun, locBuffer, N);
}
static inline const char *wrI2C(int (*sendfun)(const char *s), const char *buf){
    uint16_t N = readNnumbers(sendfun, buf);
    if(!write_i2c(locBuffer, N)) return "Error writing I2C";
    if(N < 1) return "bad";
    return "OK";
}

// write locBuffer to SPI
static inline void wrSPI(int (*sendfun)(const char *s), int SPIidx, const char *buf){
    if(SPIidx < 0 || SPIidx > 2) return;
    uint16_t N = readNnumbers(sendfun, buf);
    if(N < 1){
        *(uint8_t *)&(SPI1->DR) = 0xea;
        sendfun("Enter at least 1 number (max: ");
        sendfun(u2str(LOCBUFFSZ)); sendfun(")\n");
        return;
    }
    //if(SPI_transmit(SPIidx, locBuffer, N)) USND("Error: busy?\n");
    else sendfun("done");
}
static inline void rdSPI(int (*sendfun)(const char *s), int SPIidx){
    if(SPIidx < 0 || SPIidx > 2) return;
    //if(SPI_isoverflow(SPIidx)) sendfun("SPI buffer overflow\n");
    uint8_t len = LOCBUFFSZ;
    /*
    if(SPI_getdata(SPIidx, locBuffer, &len)){
        sendfun("Error getting data: busy?\n");
        return;
    }*/
    if(len == 0){
        sendfun("Nothing to read\n");
        return;
    }
    if(len > LOCBUFFSZ) sendfun("Can't get full message: buffer too small\n");
    sendfun("SPI data:\n");
    hexdump(sendfun, locBuffer, len);
}

static inline const char *procSPI(int (*sendfun)(const char *s), const char *buf){
    int idx = 0;
    if(*buf == 'p') idx = 1;
    buf = omit_spaces(buf + 1);
    if(*buf == 'w')  wrSPI(sendfun, idx, buf + 1);
    else if(*buf == 'r') rdSPI(sendfun, idx);
    else return "Enter `w` and data to write, `r` - to read";
    return NULL;
}

const char *helpstring =
        "+/-[num] - increase/decrease TIM1ch1 PWM by 1 or `num`\n"
        "0 - USB Pullup change\n"
        "1..4'+'/'-'[num] - increase/decrease TIM3chN PWM by 1 or `num`\n"
        "A - get ADC values\n"
        "dx - change DAC lowcal to x\n"
        "g - get PWM values\n"
        "i0..2 - setup I2C with lowest..highest speed (5.8, 10 and 100kHz)\n"
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
        "U str - send string to USART1\n"
        "u - setup USARTs (and turn off SPI)\n"
        "T - MCU temperature\n"
        "V - Vdd\n"
        "W - test watchdog\n"
;

void printADCvals(int (*sendfun)(const char *s)){
    sendfun("AIN0: "); sendfun(u2str(getADCval(0)));
    sendfun(" ("); sendfun(u2str(getADCvoltage(0)));
    sendfun("/100 V)\nAIN1: "); sendfun(u2str(getADCval(1)));
    sendfun(" ("); sendfun(u2str(getADCvoltage(1)));
    sendfun("/100 V)\nAIN5: "); sendfun(u2str(getADCval(2)));
    sendfun(" ("); sendfun(u2str(getADCvoltage(2)));
    sendfun("/100 V)\n");
}

const char *parse_cmd(int (*sendfun)(const char *s), const char *buf){
    // "long" commands
    switch(*buf){
        case '+':
        case '-':
            return chPWM((uint32_t*)&TIM1->CCR1, buf);
        break;
        case '1':
        case '2':
        case '3':
        case '4':
            return TIM3pwm(buf);
        break;
        case 'd':
            return "DAC not supported @ F103";
        case 'i':
            return setupI2C(buf + 1);
        break;
        case 'I':
            if(!i2cinited) return "Init I2C first";
            buf = omit_spaces(buf + 1);
            if(*buf == 'a') return saI2C(sendfun, buf + 1);
            else if(*buf == 'r'){ rdI2C(sendfun, buf + 1); return NULL; }
            else if(*buf == 'w') return wrI2C(sendfun, buf + 1);
            else if(*buf == 's'){ i2c_init_scan_mode(); return "Start scan\n";}
            else return "Command should be 'Ia', 'Iw', 'Ir' or 'Is'\n";
        break;
        case 'p':
        case 'P':
            //return procSPI(sendfun, buf);
            return "TODO";
        break;
        case 'U':
            return USARTsend(buf + 1);
        break;
    }
    // "short" commands
    if(buf[1] && buf[1] != '\n') return buf;
    switch(*buf){
        case '0':
            pin_toggle(USBPU_port, USBPU_pin);
            if(USBPU_port->ODR & USBPU_pin) return "Pullup clear";
            else return "Pullup set";
        break;
        case 'g':
            return getPWMvals(sendfun);
        break;
        case 'A':
            printADCvals(sendfun);
        break;
        case 'L':
            USND("Very long test string for USB (it's length is more than 64 bytes).\n"
                     "This is another part of the string! Can you see all of this?\n");
            return "Long test sent";
        break;
        case 'm':
            ADCmon = !ADCmon;
            sendfun("Monitoring is ");
            if(ADCmon) sendfun("on\n");
            else sendfun("off\n");
        break;
        case 'R':
            USND("Soft reset");
            SEND("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'S':
            USND("Test string for USB");
            return "Short test sent";
        break;
        case 's':
            sendfun("SPI are ON, USART are OFF\n");
            //usart_stop();
            //spi_setup();
        break;
        case 'T':
            return u2str(getMCUtemp());
        break;
        case 'u':
            USND("USART are ON, SPI are OFF\n");
            //spi_stop();
            //usart_setup();
        break;
        case 'V':
            return u2str(getVdd());
        break;
        case 'W':
            USND("Wait for reboot\n");
            SEND("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return helpstring;
        break;
    }
    return NULL;
}
