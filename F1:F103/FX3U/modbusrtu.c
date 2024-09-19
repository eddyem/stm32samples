/*
 * This file is part of the fx3u project.
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

#include "modbusrtu.h"
#include "flash.h"
#include "strfunc.h"

#ifdef EBUG
#include "usart.h"
#endif

#include <stm32f1.h>
#include <string.h> // memcpy

static volatile int modbus_txrdy = 1;
static volatile int idatalen[2] = {0,0}; // received data line length (including '\n')

static volatile int modbus_rdy = 0      // received data ready
    ,dlen = 0                           // length of data (including '\n') in current buffer
    ,bufovr = 0                         // input buffer overfull
;

static int rbufno = 0, tbufno = 0; // current rbuf/tbuf numbers
static uint8_t rbuf[2][MODBUSBUFSZI], tbuf[2][MODBUSBUFSZO]; // receive & transmit buffers
static uint8_t *recvdata = NULL;

#define packCRC(d, l)   *((uint16_t*) &d[l])

// calculate CRC for given data
static uint16_t getCRC(uint8_t *data, int l){
    uint16_t crc = 0xFFFF;
    for(int pos = 0; pos < l; ++pos){
        crc ^= (uint16_t)data[pos];
        for(int i = 8; i; --i){
            if((crc & 1)){
                crc >>= 1;
                crc ^= 0xA001;
            }else crc >>= 1;
        }
    }
    // CRC have swapped bytes, so we can just send it as *((uint16_t*)&data[x]) = CRC
    return crc;
}

/**
 * return length of received data without CRC or -1 if buffer overflow or bad CRC
 */
int modbus_receive(uint8_t **packet){
    if(!modbus_rdy) return 0;
    if(bufovr){
        DBG("Modbus buffer overflow\n");
        bufovr = 0;
        modbus_rdy = 0;
        return -1;
    }
    *packet = recvdata;
    modbus_rdy = 0;
    int x = dlen - 2;
    dlen = 0;
    uint16_t chk = getCRC(recvdata, x);
    if(packCRC(recvdata, x) != chk){
        DBG("Bad CRC\n");
        return -1;
    }
    return x;
}

// send current tbuf
static int senddata(int l){
    uint32_t tmout = 1600000;
    while(!modbus_txrdy){
        IWDG->KR = IWDG_REFRESH;
        if(--tmout == 0) return 0;
    }; // wait for previos buffer transmission
    modbus_txrdy = 0;
    DMA2_Channel5->CCR &= ~DMA_CCR_EN;
    DMA2_Channel5->CMAR = (uint32_t) tbuf[tbufno]; // mem
    DMA2_Channel5->CNDTR = l + 2; // + CRC
    DMA2_Channel5->CCR |= DMA_CCR_EN;
    tbufno = !tbufno;
    return l;
}

// transmit raw data with length l; amount of bytes (without CRC) sent
int modbus_send(uint8_t *data, int l){
    if(l < 1) return 0;
    if(l > MODBUSBUFSZO - 2) return -1;
    memcpy(tbuf[tbufno], data, l);
    packCRC(tbuf[tbufno], l) = getCRC(data, l);
    return senddata(l);
}

// send request: return the same as modbus_receive()
int modbus_send_request(modbus_request *r){
    uint8_t *curbuf = tbuf[tbufno];
    int n = 6;
    *curbuf++ = r->ID;
    *curbuf++ = r->Fcode;
    *curbuf++ = r->startreg >> 8; // H
    *curbuf++ = (uint8_t) r->startreg; // L
    *curbuf++ = r->regno >> 8; // H
    *curbuf   = (uint8_t) r->regno; // L
    // if r->datalen == 0 - this is responce for request with fcode > 4
    if((r->Fcode == MC_WRITE_MUL_COILS || r->Fcode == MC_WRITE_MUL_REGS) && r->datalen){ // request with data
        *(++curbuf) = r->datalen;
        memcpy(curbuf, r->data, r->datalen);
        n += r->datalen;
    }
    return senddata(n);
}

// return -1 in case of error, 0 if no data received, 1 if got good packet
int modbus_get_request(modbus_request* r){
    if(!r) return -1;
    uint8_t *pack;
    int l = modbus_receive(&pack);
    if(l < 1) return l;
    if(l < 6) return -1; // not a request
    // check "broadcasting" and common requests
    if(*pack && *pack != the_conf.modbusID) return 0; // alien request
    r->ID = *pack++;
    r->Fcode = *pack++;
    r->startreg = pack[0] << 8 | pack[1];
    r->regno = pack[2] << 8 | pack[3];
    if(l > 6){ // request with data
        if(r->Fcode != MC_WRITE_MUL_COILS && r->Fcode != MC_WRITE_MUL_REGS) return -1; // bad request
        r->datalen = pack[4];
        if(r->datalen > l-6) r->datalen = l-6; // fix if data bytes less than field
        r->data = pack + 5;
    }else{
        r->datalen = 0;
        r->data = NULL;
    }
    return 1;
}

// send responce: return the same as modbus_receive()
int modbus_send_response(modbus_response *r){
    uint8_t *curbuf = tbuf[tbufno];
    int len = 3; // packet data length without CRC
    *curbuf++ = r->ID;
    *curbuf++ = r->Fcode;
    *curbuf++ = r->datalen;
    if(0 == (r->Fcode & MODBUS_RESPONSE_ERRMARK)){ // data
        len += r->datalen;
        if(len > MODBUSBUFSZO - 2) return -1; // too much data
        memcpy(curbuf, r->data, r->datalen);
    }
    return senddata(len);
}

// get recponce; warning: all data is a pointer to last rbuf, it could be corrupted after next reading
int modbus_get_response(modbus_response* r){
    if(!r) return -1;
    uint8_t *pack;
    int l = modbus_receive(&pack);
    if(l < 1) return l;
    if(l < 3) return -1; // not a responce
    r->ID = *pack++;
    r->Fcode = *pack++;
    r->datalen = *pack++;
    // error
    if(r->Fcode & MODBUS_RESPONSE_ERRMARK) r->data = NULL;
    else{
        // wrong datalen - fix
        if(r->datalen != l-3){ r->datalen = l-3; }
        r->data = pack;
    }
    return 1;
}

// USART4: PC10 - Tx, PC11 - Rx
void modbus_setup(uint32_t speed){
    uint32_t tmout = 16000000;
    // PA9 - Tx, PA10 - Rx
    RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
    GPIOA->CRH = (GPIOA->CRH & ~(CRH(9,0xf)|CRH(10,0xf))) |
        CRH(9, CNF_AFPP|MODE_NORMAL) | CRH(10, CNF_FLINPUT|MODE_INPUT);
    // UART4 Tx DMA - Channel5 (Rx - channel 3)
    DMA2_Channel5->CPAR = (uint32_t) &UART4->DR; // periph
    DMA2_Channel5->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    // Tx CNDTR set @ each transmission due to data size
    NVIC_SetPriority(DMA2_Channel4_5_IRQn, 2);
    NVIC_EnableIRQ(DMA2_Channel4_5_IRQn);
    NVIC_SetPriority(UART4_IRQn, 2);
    // setup uart4
    UART4->BRR = 36000000 / speed; // APB1 is 36MHz
    UART4->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // 1start,8data,nstop; enable Rx,Tx,USART
    while(!(UART4->SR & USART_SR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    UART4->SR = 0; // clear flags
    UART4->CR1 |= USART_CR1_RXNEIE | USART_CR1_IDLEIE; // allow Rx and IDLE IRQ
    UART4->CR3 = USART_CR3_DMAT; // enable DMA Tx
    NVIC_EnableIRQ(UART4_IRQn);
}

void uart4_isr(){
    if(UART4->SR & USART_SR_IDLE){ // idle - end of frame
        modbus_rdy = 1;
        dlen = idatalen[rbufno];
        recvdata = rbuf[rbufno];
        // prepare other buffer
        rbufno = !rbufno;
        idatalen[rbufno] = 0;
        (void) UART4->DR; // clear IDLE flag by reading DR
    }
    if(UART4->SR & USART_SR_RXNE){ // RX not emty - receive next char
        uint8_t rb = UART4->DR; // clear RXNE flag
        if(idatalen[rbufno] < MODBUSBUFSZI){ // put next char into buf
            rbuf[rbufno][idatalen[rbufno]++] = rb;
        }else{ // buffer overrun
            bufovr = 1;
            idatalen[rbufno] = 0;
        }
    }
}

void dma2_channel4_5_isr(){
    if(DMA2->ISR & DMA_ISR_TCIF5){ // Tx
        DMA2->IFCR = DMA_IFCR_CTCIF5; // clear TC flag
        modbus_txrdy = 1;
    }
}
