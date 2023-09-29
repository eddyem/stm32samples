/*
 *                                                                                                  geany_encoding=koi8-r
 * can.c
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

#include <string.h> // memcpy

#include "can.h"
#include "hardware.h"
#include "proto.h"


// incoming message buffer size
#define CAN_INMESSAGE_SIZE  (32)

extern volatile uint32_t Tms;

// circular buffer for  received messages
static CAN_message messages[CAN_INMESSAGE_SIZE];
static uint8_t first_free_idx = 0;    // index of first empty cell
static int8_t first_nonfree_idx = -1; // index of first data cell
int8_t cansniffer = 0; // ==1 to listen all CAN ID's

uint16_t curcanspeed = CAN_SPEED_DEFAULT; // speed of last init

uint16_t CANID = 0xFFFF;
uint8_t Controller_address = 0;
static CAN_status can_status = CAN_STOP;

static void can_process_fifo(uint8_t fifo_num);

CAN_status CAN_get_status(){
    CAN_status st = can_status;
    // give overrun message only once
    if(st == CAN_FIFO_OVERRUN) can_status = CAN_READY;
    return st;
}

// push next message into buffer; return 1 if buffer overfull
static int CAN_messagebuf_push(CAN_message *msg){
    MSG("Try to push\n");
    if(first_free_idx == first_nonfree_idx) return 1; // no free space
    if(first_nonfree_idx < 0) first_nonfree_idx = 0;  // first message in empty buffer
    memcpy(&messages[first_free_idx++], msg, sizeof(CAN_message));
    // need to roll?
    if(first_free_idx == CAN_INMESSAGE_SIZE) first_free_idx = 0;
    return 0;
}

// pop message from buffer
CAN_message *CAN_messagebuf_pop(){
    if(first_nonfree_idx < 0) return NULL;
    CAN_message *msg = &messages[first_nonfree_idx++];
    if(first_nonfree_idx == CAN_INMESSAGE_SIZE) first_nonfree_idx = 0;
    if(first_nonfree_idx == first_free_idx){ // buffer is empty - refresh it
        first_nonfree_idx = -1;
        first_free_idx = 0;
    }
    return msg;
}

// get CAN address data from GPIO pins
void readCANID(){
    uint8_t CAN_addr = READ_CAN_INV_ADDR();
    Controller_address = ~CAN_addr & 0x0f;
    CANID = (CAN_ID_PREFIX & CAN_ID_MASK) | Controller_address;
}

void CAN_setup(uint16_t speed){
    if(speed == 0) speed = curcanspeed;
    else if(speed < CAN_SPEED_MIN) speed = CAN_SPEED_MIN;
    else if(speed > CAN_SPEED_MAX) speed = CAN_SPEED_MAX;
    curcanspeed = speed;
    readCANID();
    CAN->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
    RCC->APB1RSTR |= RCC_APB1RSTR_CANRST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_CANRST;
    // Configure GPIO: PB8 - CAN_Rx, PB9 - CAN_Tx
    /* (1) Select AF mode (10) on PB8 and PB9 */
    /* (2) AF4 for CAN signals */
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9))
                 | (GPIO_MODER_MODER8_AF | GPIO_MODER_MODER9_AF); /* (1) */
    GPIOB->AFR[1] = (GPIOB->AFR[1] &~ (GPIO_AFRH_AFRH0 | GPIO_AFRH_AFRH1))\
                  | (4 << (0 * 4)) | (4 << (1 * 4)); /* (2) */
    /* Enable the peripheral clock CAN */
    RCC->APB1ENR |= RCC_APB1ENR_CANEN;
      /* Configure CAN */
    /* (1) Enter CAN init mode to write the configuration */
    /* (2) Wait the init mode entering */
    /* (3) Exit sleep mode */
    /* (4) Normal mode, set timing to 100kb/s: BS1 = 4, BS2 = 3, prescaler = 6000/speed */
    /* (5) Leave init mode */
    /* (6) Wait the init mode leaving */
    /* (7) Enter filter init mode, (16-bit + mask, filter 0 for FIFO 0) */
    /* (8) Acivate filter 0 (1,2) */
    /* (9) Identifier mode for bank#0, mask mode for #1 and #2 */
    /* (10) Set the Id list */
    /* (11) Set the mask list */
    /* (12) Leave filter init */
    /* (13) Set error interrupts enable */
    CAN->MCR |= CAN_MCR_INRQ; /* (1) */
    uint32_t tmout = 16000000;
    while((CAN->MSR & CAN_MSR_INAK)!=CAN_MSR_INAK){ /* (2) */
        if(--tmout == 0) break;
    }
    CAN->MCR &=~ CAN_MCR_SLEEP; /* (3) */
    CAN->MCR |= CAN_MCR_ABOM;

    CAN->BTR |=  2 << 20 | 3 << 16 | (6000/speed - 1); /* (4) */
    CAN->MCR &=~ CAN_MCR_INRQ; /* (5) */
    tmout = 16000000;
    while((CAN->MSR & CAN_MSR_INAK)==CAN_MSR_INAK){ /* (6) */
        if(--tmout == 0) break;
    }
    CAN->FMR = CAN_FMR_FINIT; /* (7) */
    CAN->FA1R = CAN_FA1R_FACT0; /* (8) */
    CAN->FM1R = CAN_FM1R_FBM0; /* (9) */
    CAN->sFilterRegister[0].FR1 = CANID << 5 | ((BCAST_ID << 5) << 16); /* (10) */
    if(cansniffer){ /* (11) */
        CAN->FA1R |= CAN_FA1R_FACT1 | CAN_FA1R_FACT2; // activate 1 & 2
        CAN->sFilterRegister[1].FR1 = (1<<21)|(1<<5); // all odd IDs
        CAN->sFilterRegister[2].FR1 = (1<<21); // all even IDs
        CAN->FFA1R = 2; // filter 1 for FIFO1, filters 0&2 - for FIFO0
    }
    CAN->FMR &=~ CAN_FMR_FINIT; /* (12) */
    CAN->IER |= CAN_IER_ERRIE | CAN_IER_FOVIE0 | CAN_IER_FOVIE1; /* (13) */

    /* Configure IT */
    /* (14) Set priority for CAN_IRQn */
    /* (15) Enable CAN_IRQn */
    NVIC_SetPriority(CEC_CAN_IRQn, 10); /* (14) */
    NVIC_EnableIRQ(CEC_CAN_IRQn); /* (15) */
    can_status = CAN_READY;
}

// add filters for ALL ID's
void CAN_listenall(){
    cansniffer = 1;
    CAN_setup(0);
}
// listen only packets to self & broadcast - delete filters 1&2
void CAN_listenone(){
    cansniffer = 0;
    CAN_setup(0);
}

void can_proc(){
    // check for messages in FIFO0 & FIFO1
    if(CAN->RF0R & CAN_RF0R_FMP0){
        can_process_fifo(0);
    }
    if(CAN->RF1R & CAN_RF1R_FMP1){
        can_process_fifo(1);
    }
    if(CAN->ESR & (CAN_ESR_BOFF | CAN_ESR_EPVF | CAN_ESR_EWGF)){ // much errors - restart CAN BUS
        mesg("too much CAN errors, restart CAN");
        MSG("bus-off, restarting\n");
        // request abort for all mailboxes
        CAN->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
        // reset CAN bus
        RCC->APB1RSTR |= RCC_APB1RSTR_CANRST;
        RCC->APB1RSTR &= ~RCC_APB1RSTR_CANRST;
        can_status = CAN_ERROR;
    }
}

CAN_status can_send(uint8_t *msg, uint8_t len, uint16_t target_id){
    if(!noLED) LED_on(LED1); // turn ON LED1 at first data sent/receive
    uint8_t mailbox = 0;
    // check first free mailbox
    if(CAN->TSR & (CAN_TSR_TME)){
        mailbox = (CAN->TSR & CAN_TSR_CODE) >> 24;
    }else{ // no free mailboxes
        return CAN_BUSY;
    }
    CAN_TxMailBox_TypeDef *box = &CAN->sTxMailBox[mailbox];
    uint32_t lb = 0, hb = 0;
    switch(len){
        case 8:
            hb |= (uint32_t)msg[7] << 24;
            __attribute__((fallthrough));
        case 7:
            hb |= (uint32_t)msg[6] << 16;
            __attribute__((fallthrough));
        case 6:
            hb |= (uint32_t)msg[5] << 8;
            __attribute__((fallthrough));
        case 5:
            hb |= (uint32_t)msg[4];
            __attribute__((fallthrough));
        case 4:
            lb |= (uint32_t)msg[3] << 24;
            __attribute__((fallthrough));
        case 3:
            lb |= (uint32_t)msg[2] << 16;
            __attribute__((fallthrough));
        case 2:
            lb |= (uint32_t)msg[1] << 8;
            __attribute__((fallthrough));
        default:
            lb |= (uint32_t)msg[0];
    }
    box->TDLR = lb;
    box->TDHR = hb;
    box->TDTR = len;
    box->TIR  = (target_id & 0x7FF) << 21 | CAN_TI0R_TXRQ;
    return CAN_OK;
}

static void can_process_fifo(uint8_t fifo_num){
    if(fifo_num > 1) return;
    CAN_FIFOMailBox_TypeDef *box = &CAN->sFIFOMailBox[fifo_num];
    volatile uint32_t *RFxR = (fifo_num) ? &CAN->RF1R : &CAN->RF0R;
    if(!noLED) LED_on(LED1); // turn ON LED1 at first data sent/receive
    // read all
    while(*RFxR & CAN_RF0R_FMP0){ // amount of messages pending
        // CAN_RDTxR: (16-31) - timestamp, (8-15) - filter match index, (0-3) - data length
        /* TODO: check filter match index if more than one ID can receive */
        CAN_message msg;
        uint8_t *dat = msg.data;
        { // set all data to 0
            uint32_t *dptr = (uint32_t*)msg.data;
            dptr[0] = dptr[1] = 0;
        }
        uint8_t len = box->RDTR & 0x0f;
        msg.length = len;
        msg.ID = box->RIR >> 21;
        if(len){ // message can be without data
            uint32_t hb = box->RDHR, lb = box->RDLR;
            switch(len){
                case 8:
                    dat[7] = hb>>24;
                    __attribute__((fallthrough));
                case 7:
                    dat[6] = (hb>>16) & 0xff;
                    __attribute__((fallthrough));
                case 6:
                    dat[5] = (hb>>8) & 0xff;
                    __attribute__((fallthrough));
                case 5:
                    dat[4] = hb & 0xff;
                    __attribute__((fallthrough));
                case 4:
                    dat[3] = lb>>24;
                    __attribute__((fallthrough));
                case 3:
                    dat[2] = (lb>>16) & 0xff;
                    __attribute__((fallthrough));
                case 2:
                    dat[1] = (lb>>8) & 0xff;
                    __attribute__((fallthrough));
                case 1:
                    dat[0] = lb & 0xff;
            }
        }
        if(CAN_messagebuf_push(&msg)) return; // error: buffer is full, try later
        *RFxR |= CAN_RF0R_RFOM0; // release fifo for access to next message
    }
    if(*RFxR & CAN_RF0R_FULL0) *RFxR &= ~CAN_RF0R_FULL0;
}

void cec_can_isr(){
    if(CAN->RF0R & CAN_RF0R_FOVR0){ // FIFO overrun
        CAN->RF0R &= ~CAN_RF0R_FOVR0;
        can_status = CAN_FIFO_OVERRUN;
    }
    if(CAN->RF1R & CAN_RF1R_FOVR1){
        CAN->RF1R &= ~CAN_RF1R_FOVR1;
        can_status = CAN_FIFO_OVERRUN;
    }
    if(CAN->MSR & CAN_MSR_ERRI){ // Error
        CAN->MSR &= ~CAN_MSR_ERRI;
        // request abort for problem mailbox
        if(CAN->TSR & CAN_TSR_TERR0) CAN->TSR |= CAN_TSR_ABRQ0;
        if(CAN->TSR & CAN_TSR_TERR1) CAN->TSR |= CAN_TSR_ABRQ1;
        if(CAN->TSR & CAN_TSR_TERR2) CAN->TSR |= CAN_TSR_ABRQ2;
    }
}
