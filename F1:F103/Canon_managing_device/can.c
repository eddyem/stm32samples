/*
 * This file is part of the canonmanage project.
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

#include "can.h"
#include <stm32f1.h>

// REMAPPED to PB8/PB9!!!

#include <string.h> // memcpy

// circular buffer for  received messages
static CAN_message messages[CAN_INMESSAGE_SIZE];
static uint8_t first_free_idx = 0;    // index of first empty cell
static int8_t first_nonfree_idx = -1; // index of first data cell
static uint16_t oldspeed = 0, oldID = 0;

static void can_process_fifo(uint8_t fifo_num);

// push next message into buffer; return 1 if buffer overfull
static int CAN_messagebuf_push(CAN_message *msg){
    if(first_free_idx == first_nonfree_idx){
        return 1; // no free space
    }
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

// speed - in kbps, ID - identificator
void CAN_setup(uint16_t speed, uint16_t ID){
    if(speed == 0) return; // didn't initialized yet
    if(speed < 25) speed = 25;
    else if(speed > 3000) speed = 3000;
    oldspeed = speed;
    oldID = ID;
    uint32_t tmout = 16000000;
    // Configure GPIO: PB8 - CAN_Rx, PB9 - CAN_Tx
    /* (1) Select AF mode (10) on PB8 and PB9 */
    /* (2) AF4 for CAN signals */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN;
    AFIO->MAPR |= AFIO_MAPR_CAN_REMAP_REMAP2;
    GPIOB->CRH = 0;
    //pin_set(GPIOB, 1<<8);
    GPIOB->CRH = (GPIOB->CRH & ~(CRH(8,0xf)|CRH(9,0xf))) |
                 CRH(8, CNF_FLINPUT | MODE_INPUT) | CRH(9, CNF_AFPP | MODE_NORMAL);
    /* Enable the peripheral clock CAN */
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    CAN1->MCR |= CAN_MCR_INRQ;
    while((CAN1->MSR & CAN_MSR_INAK) != CAN_MSR_INAK)
        if(--tmout == 0) break;
    CAN1->MCR &=~ CAN_MCR_SLEEP;
    CAN1->MCR |= CAN_MCR_ABOM; /* allow automatically bus-off */

    CAN1->BTR =  2 << 20 | 3 << 16 | (4500/speed - 1);
    CAN1->MCR &= ~CAN_MCR_INRQ;
    tmout = 16000000;
    while((CAN1->MSR & CAN_MSR_INAK) == CAN_MSR_INAK)
        if(--tmout == 0) break;
    // accept ALL
    CAN1->FMR = CAN_FMR_FINIT;
    CAN1->FM1R = CAN_FM1R_FBM0; // filter in list mode
    CAN1->FA1R = CAN_FA1R_FACT0; // activate filter0
    CAN1->sFilterRegister[0].FR1 = ID;
    CAN1->FFA1R = 1; // filter 0 for FIFO1
    CAN1->FMR &= ~CAN_FMR_FINIT; /* (12) */
    CAN1->IER |= CAN_IER_ERRIE | CAN_IER_FOVIE0 | CAN_IER_FOVIE1 | CAN_IER_BOFIE;

    /* Configure IT */
    NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 0); // RX FIFO0 IRQ
    NVIC_SetPriority(CAN1_RX1_IRQn, 0); // RX FIFO1 IRQ
    NVIC_SetPriority(CAN1_SCE_IRQn, 0); // RX status changed IRQ
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    NVIC_EnableIRQ(CAN1_RX1_IRQn);
    NVIC_EnableIRQ(CAN1_SCE_IRQn);
    CAN1->MSR = 0; // clear SLAKI, WKUI, ERRI
}

void can_proc(){
    // check for messages in FIFO0 & FIFO1
    if(CAN1->RF0R & CAN_RF0R_FMP0){
        can_process_fifo(0);
    }
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
        CAN_setup(oldspeed, oldID);
    }
}

CAN_status can_send(uint8_t *msg, uint8_t len, uint16_t target_id){
    uint8_t mailbox = 0;
    // check first free mailbox
    if(CAN1->TSR & (CAN_TSR_TME)){
        mailbox = (CAN1->TSR & CAN_TSR_CODE) >> 24;
    }else{ // no free mailboxes
        return CAN_BUSY;
    }
    CAN_TxMailBox_TypeDef *box = &CAN1->sTxMailBox[mailbox];
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
        CAN1->RF1R &= ~CAN_RF1R_FOVR1;
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
