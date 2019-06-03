/*
 * usart.c
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
 */

#include "usart.h"
#include <string.h> // memcpy

extern volatile uint32_t Tms;

static int datalen[2] = {0,0}; // received data line length (including '\n')

uint8_t bufovr = 0;         // input buffer overfull

static uint8_t  linerdy = 0         // received data ready
                ,dlen = 0           // length of data in current buffer
                ,txrdy = 1          // transmission done
;

static int  rbufno = 0; // current rbuf number
static char rbuf[2][UARTBUFSZ+1], tbuf[UARTBUFSZ]; // receive & transmit buffers
static char *recvdata = NULL;

static char trbuf[UARTBUFSZ+1]; // auxiliary buffer for data transmission
static int trbufidx = 0;

int put_char(char c){
    if(trbufidx >= UARTBUFSZ - 1){
        for(int i = 0; i < 72000000 && ALL_OK != usart1_sendbuf(); ++i)
        if(i == 72000000) return 1;
    }
    trbuf[trbufidx++] = c;
    return 0;
}
// write zero-terminated string
int put_string(const char *str){
    while(*str){
        if(put_char(*str++)) return 1; //error! shouldn't be!!!
    }
    return 0; // all OK
}
/**
 * Fill trbuf with integer value
 * @param N - integer value
 * @return 1 if buffer overflow; oterwise return 0
 */
int put_int(int32_t N){
    if(N < 0){
        if(put_char('-')) return 1;
        N = -N;
    }
    return put_uint((uint32_t) N);
}
int put_uint(uint32_t N){
    char buf[10];
    int L = 0;
    if(N){
        while(N){
            buf[L++] = N % 10 + '0';
            N /= 10;
        }
        while(L--) if(put_char(buf[L])) return 1;
    }else if(put_char('0')) return 1;
    return 0;
}
/**
 * @brief usart1_sendbuf - send temporary transmission buffer (trbuf) over USART
 * @return Tx status
 */
TXstatus usart1_sendbuf(){
    int len = trbufidx;
    if(len == 0) return ALL_OK;
    else if(len > UARTBUFSZ) len = UARTBUFSZ;
    TXstatus s = usart1_send(trbuf, len);
    if(s == ALL_OK) trbufidx = 0;
    return s;
}

void USART1_config(){
    /* Enable the peripheral clock of GPIOA */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    /* GPIO configuration for USART1 signals */
    /* (1) Select AF mode (10) on PA9 and PA10 */
    /* (2) AF1 for USART1 signals */
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER9|GPIO_MODER_MODER10))\
                | (GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1); /* (1) */
    GPIOA->AFR[1] = (GPIOA->AFR[1] &~ (GPIO_AFRH_AFRH1 | GPIO_AFRH_AFRH2))\
                | (1 << (1 * 4)) | (1 << (2 * 4)); /* (2) */
    /* Enable the peripheral clock USART1 */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    /* Configure USART1 */
    /* (1) oversampling by 16, 115200 baud */
    /* (2) 8 data bit, 1 start bit, 1 stop bit, no parity */
    USART1->BRR = 480000 / 1152; /* (1) */
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; /* (2) */
    /* polling idle frame Transmission */
    while(!(USART1->ISR & USART_ISR_TC)){}
    USART1->ICR |= USART_ICR_TCCF; /* clear TC flag */
    USART1->CR1 |= USART_CR1_RXNEIE; /* enable TC, TXE & RXNE interrupt */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel2->CPAR = (uint32_t) &(USART1->TDR); // periph
    DMA1_Channel2->CMAR = (uint32_t) tbuf; // mem
    DMA1_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    USART1->CR3 = USART_CR3_DMAT;
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    /* Configure IT */
    /* (3) Set priority for USART1_IRQn */
    /* (4) Enable USART1_IRQn */
    NVIC_SetPriority(USART1_IRQn, 0); /* (3) */
    NVIC_EnableIRQ(USART1_IRQn); /* (4) */
}

#ifdef EBUG
#define TMO 0
#else
#define TMO 1
#endif

void usart1_isr(){
    static uint8_t   timeout = TMO  // == 0 for human interface without timeout
                    ,nctr = 0       // counter of '#' received
                    ,incmd = 0      // ==1 - inside command
                    ;
    static uint32_t tmout = 0;
    if(USART1->ISR & USART_ISR_RXNE){ // RX not emty - receive next char
        // read RDR clears flag
        uint8_t rb = USART1->RDR;
        if(timeout && rb == '#'){ // chek '#' without timeout
            if(++nctr == 4){ // "####" received - turn off timeout
                timeout = 0;
                nctr = 0;
                datalen[rbufno] = 0; // reset all incoming data
                incmd = 0;
                return;
            }
        }else nctr = 0;
        if(!incmd){
            if(rb == '['){ // open command or reset previoud input
                datalen[rbufno] = 0;
                incmd = 1;
            }
            return;
        }
        if(timeout){ // check timeout only inside commands
            if(tmout && Tms >= tmout){ // set overflow flag
                bufovr = 1;
                datalen[rbufno] = 0;
            }else{
                tmout = Tms + TIMEOUT_MS;
                if(!tmout) tmout = 1; // prevent 0
            }
        }
        switch(rb){
            case '[':
                datalen[rbufno] = 0;
                tmout = 0;
            break;
            case ']': // close command - line ready!
                dlen = datalen[rbufno];
                if(dlen){
                    linerdy = 1;
                    incmd = 0;
                    recvdata = rbuf[rbufno];
                    rbuf[rbufno][dlen] = 0;
                    rbufno = !rbufno;
                    datalen[rbufno] = 0;
                }
                tmout = 0;
            break;
            case '\r':
            case '\n':
            case ' ':
            case '\t':
                return;
            break;
            default:
            break;
        }

        if(datalen[rbufno] < UARTBUFSZ){ // put next char into buf
            rbuf[rbufno][datalen[rbufno]++] = rb;
        }else{ // buffer overrun
            bufovr = 1;
            datalen[rbufno] = 0;
            incmd = 0;
            tmout = 0;
        }
    }
}

void dma1_channel2_3_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF2){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF2; // clear TC flag
        txrdy = 1;
    }
}

/**
 * return length of received data (without trailing zero)
 */
int usart1_getline(char **line){
    if(!linerdy) return 0;
    linerdy = 0;
    if(bufovr){
        bufovr = 0;
        return 0;
    }
    *line = recvdata;
    return dlen;
}

/**
 * @brief usart1_send send buffer `str` adding trailing '\n'
 * @param str - string to send (without '\n' at end)
 * @param len - its length or 0 to auto count
 * @return
 */
TXstatus usart1_send(const char *str, int len){
    if(!txrdy) return LINE_BUSY;
    if(len > UARTBUFSZ - 1) return STR_TOO_LONG;
    txrdy = 0;
    if(len == 0){
        const char *ptr = str;
        while(*ptr++) ++len;
    }
    if(len == 0) return ALL_OK;
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    memcpy(tbuf, str, len);
//    tbuf[len++] = '\n';
    DMA1_Channel2->CNDTR = len;
    DMA1_Channel2->CCR |= DMA_CCR_EN; // start transmission
    return ALL_OK;
}

TXstatus usart1_send_blocking(const char *str, int len){
    if(!txrdy) return LINE_BUSY;
    if(len == 0){
        const char *ptr = str;
        while(*ptr++) ++len;
    }
    if(len == 0) return ALL_OK;
    for(int i = 0; i < len; ++i){
        USART1->TDR = *str++;
        while(!(USART1->ISR & USART_ISR_TXE));
    }
//    USART1->TDR = '\n';
    while(!(USART1->ISR & USART_ISR_TC));
    txrdy = 1;
    return ALL_OK;
}

// read `buf` and get first integer `N` in it
// @return pointer to first non-number if all OK or NULL if first symbol isn't a space or number
char *getnum(const char *buf, int32_t *N){
    char c;
    int positive = -1;
    int32_t val = 0;
    //usart1_send_blocking(buf, 0);
    while((c = *buf++)){
        if(c == '\t' || c == ' '){
            if(positive < 0) continue; // beginning spaces
            else break; // spaces after number
        }
        if(c == '-'){
            if(positive < 0){
                positive = 0;
                continue;
            }else break; // there already was `-` or number
        }
        if(c < '0' || c > '9') break;
        if(positive < 0) positive = 1;
        val = val * 10 + (int32_t)(c - '0');
    }
    if(positive != -1){
        if(positive == 0){
            if(val == 0) return NULL; // single '-'
            val = -val;
        }
        *N = val;
    }else return NULL;
/*    usart1_sendbuf();
    put_uint(val);
    put_char('\n');*/
    return (char*)buf-1;
}
