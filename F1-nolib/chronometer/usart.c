/*
 * This file is part of the chronometer project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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


#include "stm32f1.h"
#include "flash.h"
#include "usart.h"
#include "lidar.h"

extern volatile uint32_t Tms;
static volatile int idatalen[4][2] = {0}; // received data line length (including '\n')
static volatile int odatalen[4][2] = {0};

volatile int linerdy[4] = {0},  // received data ready
    dlen[4] = {0},              // length of data (including '\n') in current buffer
    bufovr[4] = {0},            // input buffer overfull
    txrdy[4] = {0,1,1,1}        // transmission done
;


int rbufno[4] = {0}, tbufno[4] = {0}; // current rbuf/tbuf numbers
static char rbuf[4][2][UARTBUFSZ], tbuf[4][2][UARTBUFSZ]; // receive & transmit buffers
static char *recvdata[4] = {0};

/**
 * return length of received data (without trailing zero)
 */
int usart_getline(int n, char **line){
    if(bufovr[n]){
        bufovr[n] = 0;
        linerdy[n] = 0;
        return 0;
    }
    *line = recvdata[n];
    linerdy[n] = 0;
    return dlen[n];
}

// transmit current tbuf and swap buffers
void transmit_tbuf(int n){
    if(n < 1 || n > 3) return;
    uint32_t tmout = 72000;
    while(!txrdy[n]){if(--tmout == 0) return;}; // wait for previos buffer transmission
    register int l = odatalen[n][tbufno[n]];
    if(!l) return;
    txrdy[n] = 0;
    odatalen[n][tbufno[n]] = 0;
    DMA_Channel_TypeDef *DMA;
    IWDG->KR = IWDG_REFRESH;
    switch(n){
        case 1:
            DMA = DMA1_Channel4;
        break;
        case 2:
            DMA = DMA1_Channel7;
        break;
        case 3:
            DMA = DMA1_Channel2;
        break;
    }
    DMA->CCR &= ~DMA_CCR_EN;
    DMA->CMAR = (uint32_t) tbuf[n][tbufno[n]]; // mem
    DMA->CNDTR = l;
    DMA->CCR |= DMA_CCR_EN;
    tbufno[n] = !tbufno[n];
}

void usart_putchar(int n, char ch){
    for(int i = 0; odatalen[n][tbufno[n]] == UARTBUFSZ && i < 1024; ++i) transmit_tbuf(n);
    tbuf[n][tbufno[n]][odatalen[n][tbufno[n]]++] = ch;
}

void usart_send(int n, const char *str){
    uint32_t x = 512;
    while(*str && --x){
        if(odatalen[n][tbufno[n]] == UARTBUFSZ){
            transmit_tbuf(n);
            continue;
        }
        tbuf[n][tbufno[n]][odatalen[n][tbufno[n]]++] = *str++;
    }
}
#if defined EBUG || defined USART1PROXY
// only for USART1
void newline(){
    usart_putchar(1, '\n');
    transmit_tbuf(1);
}
#endif

/*
 * USART speed: baudrate = Fck/(USARTDIV)
 * USARTDIV stored in USART->BRR
 *
 * for 72MHz USARTDIV=72000/f(kboud); so for 115200 USARTDIV=72000/115.2=625 -> BRR=0x271
 *         9600: BRR = 7500 (0x1D4C)
 */
static void usart_setup(int n, uint32_t BRR){
    DMA_Channel_TypeDef *DMA;
    IRQn_Type DMAirqN, USARTirqN;
    USART_TypeDef *USART;
    switch(n){
        case 1:
            // USART1 Tx DMA - Channel4 (Rx - channel 5)
            DMA = DMA1_Channel4;
            DMAirqN = DMA1_Channel4_IRQn;
            USARTirqN = USART1_IRQn;
            // PA9 - Tx, PA10 - Rx
            RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
            GPIOA->CRH |= CRH(9, CNF_AFPP|MODE_NORMAL) | CRH(10, CNF_FLINPUT|MODE_INPUT);
            USART = USART1;
        break;
        case 2:
            // USART2 Tx DMA - Channel7
            DMA = DMA1_Channel7;
            DMAirqN = DMA1_Channel7_IRQn;
            USARTirqN = USART2_IRQn;
            // PA2 - Tx, PA3 - Rx
            RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
            RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
            GPIOA->CRL |= CRL(2, CNF_AFPP|MODE_NORMAL) | CRL(3, CNF_FLINPUT|MODE_INPUT);
            USART = USART2;
        break;
        case 3:
            // USART3 Tx DMA - Channel2
            DMA = DMA1_Channel2;
            DMAirqN = DMA1_Channel2_IRQn;
            USARTirqN = USART3_IRQn;
            // PB10 - Tx, PB11 - Rx
            RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
            RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
            GPIOB->CRH |= CRH(10, CNF_AFPP|MODE_NORMAL) | CRH(11, CNF_FLINPUT|MODE_INPUT);
            USART = USART3;
        break;
        default:
            return;
    }
    DMA->CPAR = (uint32_t) &USART->DR; // periph
    DMA->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    // setup usart(n)
    USART->BRR = BRR;
    USART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // 1start,8data,nstop; enable Rx,Tx,USART
    uint32_t tmout = 16000000;
    while(!(USART->SR & USART_SR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    USART->SR = 0; // clear flags
    USART->CR1 |= USART_CR1_RXNEIE; // allow Rx IRQ
    USART->CR3 = USART_CR3_DMAT; // enable DMA Tx
    // Tx CNDTR set @ each transmission due to data size
    NVIC_SetPriority(DMAirqN, n);
    NVIC_EnableIRQ(DMAirqN);
    NVIC_SetPriority(USARTirqN, n);
    NVIC_EnableIRQ(USARTirqN);
}

void usarts_setup(){
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
#if defined EBUG || defined USART1PROXY
    usart_setup(1, 72000000 / the_conf.USART_speed); // debug console or GPS proxy
#endif
    usart_setup(2, 36000000 / 9600); // GPS
    usart_setup(3, 36000000 / 115200); // LIDAR
}


void usart_isr(int n, USART_TypeDef *USART){
    #ifdef CHECK_TMOUT
    static uint32_t tmout[n] = 0;
    #endif
    IWDG->KR = IWDG_REFRESH;
    if(USART->SR & USART_SR_RXNE){ // RX not emty - receive next char
        #ifdef CHECK_TMOUT
        if(tmout[n] && Tms >= tmout[n]){ // set overflow flag
            bufovr[n] = 1;
            idatalen[n][rbufno[n]] = 0;
        }
        tmout[n] = Tms + TIMEOUT_MS;
        if(!tmout[n]) tmout[n] = 1; // prevent 0
        #endif
        uint8_t rb = USART->DR;
        if(idatalen[n][rbufno[n]] < UARTBUFSZ){ // put next char into buf
            rbuf[n][rbufno[n]][idatalen[n][rbufno[n]]++] = rb;
            if(rb == '\n'){ // got newline - line ready
                linerdy[n] = 1;
                dlen[n] = idatalen[n][rbufno[n]];
                recvdata[n] = rbuf[n][rbufno[n]];
                // prepare other buffer
                rbufno[n] = !rbufno[n];
                idatalen[n][rbufno[n]] = 0;
                #ifdef CHECK_TMOUT
                // clear timeout at line end
                tmout[n] = 0;
                #endif
            }
        }else{ // buffer overrun
            bufovr[n] = 1;
            idatalen[n][rbufno[n]] = 0;
            #ifdef CHECK_TMOUT
            tmout[n] = 0;
            #endif
        }
    }
}

#if defined EBUG || defined USART1PROXY
void usart1_isr(){
    usart_isr(1, USART1);
}
#endif

// GPS_USART
void usart2_isr(){
    usart_isr(2, USART2);
}

// LIDAR_USART
void usart3_isr(){
    IWDG->KR = IWDG_REFRESH;
    if(USART3->SR & USART_SR_RXNE){ // RX not emty - receive next char
        uint8_t rb = USART3->DR, L = idatalen[3][rbufno[3]];
        if(rb != LIDAR_FRAME_HEADER && (L == 0 || L == 1)){ // bad starting sequence
            idatalen[3][rbufno[3]] = 0;
            return;
        }
        if(L < LIDAR_FRAME_LEN){ // put next char into buf
            rbuf[3][rbufno[3]][idatalen[3][rbufno[3]]++] = rb;
            if(L == LIDAR_FRAME_LEN-1){ // got LIDAR_FRAME_LEN bytes - line ready
                linerdy[3] = 1;
                dlen[3] = idatalen[3][rbufno[3]];
                recvdata[3] = rbuf[3][rbufno[3]];
                // prepare other buffer
                rbufno[3] = !rbufno[3];
                idatalen[3][rbufno[3]] = 0;
            }
        }else{ // buffer overrun
            idatalen[3][rbufno[3]] = 0;
        }
    }
}

// return string buffer with val
char *u2str(uint32_t val){
    static char buf[11];
    char *bufptr = &buf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    return bufptr;
}

// print 32bit unsigned int
void printu(int n, uint32_t val){
    usart_send(n, u2str(val));
}

// print 32bit unsigned int as hex
void printuhex(int n, uint32_t val){
    usart_send(n, "0x");
    uint8_t *ptr = (uint8_t*)&val + 3;
    int i, j;
    IWDG->KR = IWDG_REFRESH;
    for(i = 0; i < 4; ++i, --ptr){
        for(j = 1; j > -1; --j){
            register uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) usart_putchar(n, half + '0');
            else usart_putchar(n, half - 10 + 'a');
        }
    }
}

#ifdef EBUG
// dump memory buffer
void hexdump(uint8_t *arr, uint16_t len){
    for(uint16_t l = 0; l < len; ++l, ++arr){
        IWDG->KR = IWDG_REFRESH;
        for(int16_t j = 1; j > -1; --j){
            register uint8_t half = (*arr >> (4*j)) & 0x0f;
            if(half < 10) usart_putchar(1, half + '0');
            else usart_putchar(1, half - 10 + 'a');
        }
        if(l % 16 == 15) usart_putchar(1, '\n');
        else if((l & 3) == 3) usart_putchar(1, ' ');
    }
}
#endif

#if defined EBUG || defined USART1PROXY
void dma1_channel4_isr(){ // USART1
    if(DMA1->ISR & DMA_ISR_TCIF4){ // Tx
        DMA1->IFCR = DMA_IFCR_CTCIF4; // clear TC flag
        txrdy[1] = 1;
    }
}
#endif

void dma1_channel7_isr(){ // USART2
    if(DMA1->ISR & DMA_ISR_TCIF7){ // Tx
        DMA1->IFCR = DMA_IFCR_CTCIF7; // clear TC flag
        txrdy[2] = 1;
    }
}

void dma1_channel2_isr(){ // USART3
    if(DMA1->ISR & DMA_ISR_TCIF2){ // Tx
        DMA1->IFCR = DMA_IFCR_CTCIF2; // clear TC flag
        txrdy[3] = 1;
    }
}

// read `buf` and get first integer `N` in it
// @return 0 if all OK or 1 if there's not a number; omit spaces and '='
int getnum(const char *buf, int32_t *N){
    char c;
    int positive = -1;
    int32_t val = 0;
    while((c = *buf++)){
        if(c == '\t' || c == ' ' || c == '='){
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
            if(val == 0) return 1; // single '-'
            val = -val;
        }
        *N = val;
    }else return 1;
    return 0;
}
