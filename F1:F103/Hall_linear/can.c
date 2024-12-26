/*
 * This file is part of the hallinear project.
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

#include <stm32f1.h>
#include <string.h> // memcpy

#include "can.h"
#include "flash.h" // can ID
#include "usb.h"

// REMAPPED to PB8/PB9!!!

// circular buffer for  received messages
#define CANMESG_SZ      ((int)sizeof(CAN_message))
#define INMESG_MAX      ((int)((RBINSZ-1) / CANMESG_SZ))
#define OUTMESG_MAX     ((int)((RBOUTSZ-1) / CANMESG_SZ))

static void can_process_fifo(uint8_t fifo_num);

// push next message into receive buffer; return 1 if buffer overfull
static int CAN_messagebuf_push(CAN_message *msg){
    int l = RB_datalen((ringbuffer*)&rbin) / CANMESG_SZ;
    if(l < 1){ // error or  no free space
        return 1;
    }
    if(RB_write((ringbuffer*)&rbin, (uint8_t*)msg, CANMESG_SZ) != CANMESG_SZ) return 1;
    return 0;
}

// pop message from buffer
CAN_message *CAN_receive(){
    static CAN_message msg;
    if(CANMESG_SZ != RB_read((ringbuffer*)&rbin, (uint8_t*)&msg, CANMESG_SZ)) return NULL;
    return &msg;
}

CAN_status CAN_send(CAN_message *msg){
    if(!msg) return CAN_ERR;
    int l = RB_datalen((ringbuffer*)&rbout) / CANMESG_SZ;
    if(l < 1){ // error or  no free space
        return CAN_BUSY;
    }
    if(RB_write((ringbuffer*)&rbout, (uint8_t*)msg, CANMESG_SZ) != CANMESG_SZ) return 1;
    return 0;
}

// speed - in kbps, ID - identificator (my ID or 0 as broadcast)
void CAN_setup(){
    uint32_t speed = the_conf.canspeed;
    if(speed < CAN_MIN_SPEED) speed = CAN_MIN_SPEED;
    else if(speed > CAN_MAX_SPEED) speed = CAN_MAX_SPEED;
    uint32_t tmout = 16000000;
    // Configure GPIO: PB8 - CAN_Rx, PB9 - CAN_Tx
    /* Select AF mode on PB8 and PB9 */
    /* AF4 for CAN signals */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN;
    AFIO->MAPR |= AFIO_MAPR_CAN_REMAP_REMAP2;
    GPIOB->CRH = (GPIOB->CRH & ~(CRH(8,0xf)|CRH(9,0xf))) |
                 CRH(8, CNF_FLINPUT | MODE_INPUT) | CRH(9, CNF_AFPP | MODE_NORMAL);
    /* Enable the peripheral clock CAN */
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    CAN1->MCR |= CAN_MCR_INRQ;
    while((CAN1->MSR & CAN_MSR_INAK) != CAN_MSR_INAK){
        IWDG->KR = IWDG_REFRESH;
        if(--tmout == 0) break;
    }
    CAN1->MCR &=~ CAN_MCR_SLEEP;
    CAN1->MCR |= CAN_MCR_ABOM; /* allow automatically bus-off */

    CAN1->BTR = (CAN_TBS2-1) << 20 | (CAN_TBS1-1) << 16 | (CAN_BIT_OSC/speed - 1); //| CAN_BTR_SILM | CAN_BTR_LBKM; /* (4) */
    the_conf.canspeed = CAN_BIT_OSC/(uint32_t)((CAN1->BTR & CAN_BTR_BRP) + 1);

    CAN1->MCR &= ~CAN_MCR_INRQ;
    tmout = 16000000;
    while((CAN1->MSR & CAN_MSR_INAK) == CAN_MSR_INAK){
        IWDG->KR = IWDG_REFRESH;
        if(--tmout == 0) break;
    }
    // accept only my ID
    CAN1->FMR = CAN_FMR_FINIT;
    CAN1->FM1R = CAN_FM1R_FBM0; // filter in list mode
    CAN1->FA1R = CAN_FA1R_FACT0; // activate filter0:
    CAN1->sFilterRegister[0].FR1 = the_conf.canID << 5;// My ID and 0
    CAN1->FFA1R = 1; // filter 0 for FIFO1
    CAN1->FMR &= ~CAN_FMR_FINIT;
    CAN1->IER |= CAN_IER_ERRIE | CAN_IER_FOVIE1 | CAN_IER_BOFIE;

    /* Configure IT */
    NVIC_SetPriority(CAN1_RX1_IRQn, 2); // RX FIFO1 IRQ
    NVIC_SetPriority(CAN1_SCE_IRQn, 2); // RX status changed IRQ
    NVIC_EnableIRQ(CAN1_RX1_IRQn);
    NVIC_EnableIRQ(CAN1_SCE_IRQn);
    CAN1->MSR = 0; // clear SLAKI, WKUI, ERRI
}

void CAN_reinit(){
    CAN1->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
    RCC->APB1RSTR |= RCC_APB1RSTR_CAN1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_CAN1RST;
    CAN_setup(0);
}

static void try2send(){
    CAN_message msg;
    if(CANMESG_SZ != RB_read((ringbuffer*)&rbout, (uint8_t*)&msg, CANMESG_SZ)) return;
    uint8_t mailbox = 0;
    // check first free mailbox
    if(CAN1->TSR & (CAN_TSR_TME)){
        mailbox = (CAN1->TSR & CAN_TSR_CODE) >> 24;
    }else{ // no free mailboxes
        return;
    }
    CAN_TxMailBox_TypeDef *box = &CAN1->sTxMailBox[mailbox];
    uint32_t lb = 0, hb = 0;
    uint8_t *d = msg.data;
    switch(msg.length){
        case 8:
            hb |= (uint32_t)d[7] << 24;
            __attribute__((fallthrough));
        case 7:
            hb |= (uint32_t)d[6] << 16;
            __attribute__((fallthrough));
        case 6:
            hb |= (uint32_t)d[5] << 8;
            __attribute__((fallthrough));
        case 5:
            hb |= (uint32_t)d[4];
            __attribute__((fallthrough));
        case 4:
            lb |= (uint32_t)d[3] << 24;
            __attribute__((fallthrough));
        case 3:
            lb |= (uint32_t)d[2] << 16;
            __attribute__((fallthrough));
        case 2:
            lb |= (uint32_t)d[1] << 8;
            __attribute__((fallthrough));
        default:
            lb |= (uint32_t)d[0];
    }
    box->TDLR = lb;
    box->TDHR = hb;
    box->TDTR = msg.length;
    box->TIR  = (msg.ID & 0x7FF) << 21 | CAN_TI0R_TXRQ;
}

void CAN_proc(){
    // check for messages in FIFO1
    if(CAN1->RF1R & CAN_RF1R_FMP1){
        can_process_fifo(1);
    }
    IWDG->KR = IWDG_REFRESH;
    if(CAN1->ESR & (CAN_ESR_BOFF | CAN_ESR_EPVF | CAN_ESR_EWGF)){ // much errors - restart CAN BUS
        // request abort for all mailboxes
        CAN1->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
        // reset CAN bus
        RCC->APB1RSTR |= RCC_APB1RSTR_CAN1RST;
        RCC->APB1RSTR &= ~RCC_APB1RSTR_CAN1RST;
        CAN_reinit();
    }
    try2send(); // and try to send user data
}

static void can_process_fifo(uint8_t fifo_num){
    if(fifo_num > 1) return;
    CAN_FIFOMailBox_TypeDef *box = &CAN1->sFIFOMailBox[fifo_num];
    volatile uint32_t *RFxR = (fifo_num) ? &CAN1->RF1R : &CAN1->RF0R;
    // read all
    while(*RFxR & CAN_RF0R_FMP0){ // amount of messages pending
        // CAN_RDTxR: (16-31) - timestamp, (8-15) - filter match index, (0-3) - data length
        /* TODO: check filter match index if more than one ID can receive */
        CAN_message msg;
        uint8_t *dat = msg.data;
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
    *RFxR = 0; // clear FOVR & FULL
}

void can_rx1_isr(){ // Rx FIFO1 (overrun)
    if(CAN1->RF1R & CAN_RF1R_FOVR1){
        CAN1->RF1R = CAN_RF1R_FOVR1;
    }
}

void can_sce_isr(){ // status changed
    if(CAN1->MSR & CAN_MSR_ERRI){ // Error
        CAN1->MSR &= ~CAN_MSR_ERRI;
        // request abort for problem mailbox
        if(CAN1->TSR & CAN_TSR_TERR0) CAN1->TSR |= CAN_TSR_ABRQ0;
        if(CAN1->TSR & CAN_TSR_TERR1) CAN1->TSR |= CAN_TSR_ABRQ1;
        if(CAN1->TSR & CAN_TSR_TERR2) CAN1->TSR |= CAN_TSR_ABRQ2;
    }
}
