/*
 * This file is part of the multistepper project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f3.h>

#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "tmc2209.h"

extern volatile uint32_t Tms;
static uint8_t motorno = 0;

#define MAXBUFLEN           (8)
// timeout, milliseconds
#define PDNU_TMOUT          (5)

// buffers format: 0 - sync+reserved, 1 - address (0..3 - slave, 0xff - master)
//      2 - register<<1 | RW, 3 - CRC (r) or [ 3..6 - MSB data, 7 - CRC ]
// buf[0] - USART2, buf[1] - USART3
//static uint8_t notfound = 0; // not found mask (LSB - 0, MSB - 7)

// datalen == 3 for read request or 7 for writing
static void calcCRC(uint8_t *outbuf, int datalen){
    uint8_t crc = 0;
    for(int i = 0; i < datalen; ++i){
        uint8_t currentByte = outbuf[i];
        for(int j = 0; j < 8; ++j){
            if((crc >> 7) ^ (currentByte & 0x01)) crc = (crc << 1) ^ 0x07;
            else crc <<= 1;
            currentByte = currentByte >> 1;
        }
    }
    outbuf[datalen] = crc;
}

static volatile USART_TypeDef *USART[2] = {USART2, USART3};

static void setup_usart(int no){
    USART[no]->ICR = 0xffffffff; // clear all flags
    USART[no]->BRR = 72000000 / 256000; // 256 kbaud
    USART[no]->CR3 = USART_CR3_HDSEL; // enable DMA Tx/Rx, single wire
    USART[no]->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // 1start,8data,nstop; enable Rx,Tx,USART
    uint32_t tmout = 16000000;
    while(!(USART[no]->ISR & USART_ISR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    USART[no]->ICR = 0xffffffff; // clear all flags again
}

// USART2 (ch0..3), USART3 (ch4..7)
// pins are setting up in `hardware.c`
void pdnuart_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN;
    setup_usart(0);
    setup_usart(1);
}

static int rwreg(uint8_t reg, uint32_t data, int w){
    if(motorno >= MOTORSNO || reg & 0x80){
        DBG("Wrong motno or reg");
        return FALSE;
    }
    uint32_t x = Tms;
    while(Tms - x < 1);
    int no = motorno >> 2;
    uint8_t outbuf[MAXBUFLEN];
    outbuf[0] = 0x05;
    outbuf[1] = motorno - (no << 2);
    outbuf[2] = reg;
    int nbytes = 3;
    if(w){
        outbuf[2] |= 0x80;
        for(int i = 6; i > 2; --i){
            outbuf[i] = data & 0xff;
            data >>= 8;
        }
        nbytes = 7;
    }
    calcCRC(outbuf, nbytes);
    ++nbytes;
    for(int i = 0; i < nbytes; ++i){
        IWDG->KR = IWDG_REFRESH;
        /*
#ifdef EBUG
        USB_sendstr("Send byte "); USB_putbyte('0'+i); USB_sendstr(": "); printuhex(outbuf[i]); newline();
#endif
        */
        USART[no]->TDR = outbuf[i]; // transmit
        while(!(USART[no]->ISR & USART_ISR_TXE));
        int l = 0;
        for(; l < 10000; ++l) if(USART[no]->ISR & USART_ISR_RXNE) break;
        // clear Rx
        (void) USART[no]->RDR;
        /*
#ifdef EBUG
        if(l == 10000) USND("Nothing received");
        else {USB_sendstr("Rcv: "); printuhex(USART[no]->RDR); newline();}
#endif
        */
    }
    return TRUE;
}

// return FALSE if failed
int pdnuart_writereg(uint8_t reg, uint32_t data){
    return rwreg(reg, data, 1);
}

// return FALSE if failed
int pdnuart_readreg(uint8_t reg, uint32_t *data){
    if(!rwreg(reg, 0, 0)) return FALSE;
    uint32_t Tstart = Tms;
    uint8_t buf[8];
    int no = motorno >> 2;
    for(int i = 0; i < 8; ++i){
        IWDG->KR = IWDG_REFRESH;
        while(!(USART[no]->ISR & USART_ISR_RXNE))
            if(Tms - Tstart > PDNU_TMOUT){
                DBG("Read timeout");
                return FALSE;
            }
        buf[i] = USART[no]->RDR;
/*
#ifdef EBUG
        USB_sendstr("Read byte: "); printuhex(buf[i]); newline();
#endif
*/
    }
    uint32_t o = 0;
    for(int i = 3; i < 7; ++i){
        o <<= 8;
        o |= buf[i];
    }
    *data = o;
    return TRUE;
}

static int readregister(uint8_t no, uint8_t reg, uint32_t *data){
    int n = motorno; motorno = no;
    int r = pdnuart_readreg(reg, data);
    motorno = n;
    return r;
}

static int writeregister(uint8_t no, uint8_t reg, uint32_t data){
    int n = motorno; motorno = no;
    int r = pdnuart_writereg(reg, data);
    motorno = n;
    return r;
}

uint8_t pdnuart_getmotno(){
    return motorno;
}

int pdnuart_setmotno(uint8_t no){
    if(no >= MOTORSNO) return FALSE;
    motorno = no;
    return TRUE;
}

// write val into IHOLD_IRUN over UART to n'th motor
int pdnuart_setcurrent(uint8_t no, uint8_t val){
    TMC2209_ihold_irun_reg_t regval = {0};
    // IHOLD_IRUN is write-only register, so we should write all values!!!
    //if(!readregister(no, TMC2209Reg_IHOLD_IRUN, &regval.value)) return FALSE;
    if(the_conf.motflags[no].donthold == 0) regval.ihold = (1+val)/2; // hold current
    regval.irun = val;
    regval.iholddelay = 1; // 2^18 clock delay before power down motor
    return writeregister(no, TMC2209Reg_IHOLD_IRUN, regval.value);
}

// set microsteps over UART
int pdnuart_microsteps(uint8_t no, uint32_t val){
    if(val > 256) return FALSE;
    TMC2209_chopconf_reg_t regval;
    if(!readregister(no, TMC2209Reg_CHOPCONF, &regval.value)) return FALSE;
    if(val == 256) regval.mres = 0;
    else regval.mres = 8 - MSB(val);
    return writeregister(no, TMC2209Reg_CHOPCONF, regval.value);
}

// init driver number `no`, return FALSE if failed
int pdnuart_init(uint8_t no){
    TMC2209_gconf_reg_t gconf;
    if(!pdnuart_microsteps(no, the_conf.microsteps[no])) return FALSE;
    if(!pdnuart_setcurrent(no, the_conf.motcurrent[no])) return FALSE;
    if(!readregister(no, TMC2209Reg_GCONF, &gconf.value)) return FALSE;
    gconf.pdn_disable = 1; // PDN now is UART
    gconf.mstep_reg_select = 1; // microsteps are by MSTEP
    if(!writeregister(no, TMC2209Reg_GCONF, gconf.value)) return FALSE;
    return TRUE;
}

/*
static void parseRx(int no){
    USB_sendstr("Got from ");
    USB_putbyte('#'); printu(curslaveaddr[no] + no*4); USB_sendstr(": ");
    for(int i = 0; i < 8; ++i){
        printuhex(inbuf[no][i]); USB_putbyte(' ');
    }
    newline();
}
*/
