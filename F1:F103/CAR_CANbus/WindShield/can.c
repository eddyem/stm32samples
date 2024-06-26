/*
 * This file is part of the windshield project.
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

#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "usart.h"

// REMAPPED to PB8/PB9!!!

#include <string.h> // memcpy

// circular buffer for  received messages
static CAN_message messages[CAN_INMESSAGE_SIZE];
static uint8_t first_free_idx = 0;    // index of first empty cell
static int8_t first_nonfree_idx = -1; // index of first data cell
static uint16_t oldspeed = 100; // speed of last init

static CAN_status can_status = CAN_STOP;

static void can_process_fifo(uint8_t fifo_num);

CAN_status CAN_get_status(){
    int st = can_status;
    can_status = CAN_OK;
    return st;
}

// push next message into buffer; return 1 if buffer overfull
static int CAN_messagebuf_push(CAN_message *msg){
    //MSG("Try to push\n");
#ifdef EBUG
        usart_send("push\n");
#endif
    if(first_free_idx == first_nonfree_idx){
#ifdef EBUG
        usart_send("INBUF OVERFULL\n");
#endif
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

void CAN_reinit(uint16_t speed){
    CAN1->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
    RCC->APB1RSTR |= RCC_APB1RSTR_CAN1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_CAN1RST;
    CAN_setup(speed);
}

/*
Can filtering: FSCx=0 (CAN1->FS1R) -> 16-bit identifiers
MASK: FBMx=0 (CAN1->FM1R), two filters (n in FR1 and n+1 in FR2)
    ID:   CAN1->sFilterRegister[x].FRn[0..15]
    MASK: CAN1->sFilterRegister[x].FRn[16..31]
    FR bits:  STID[10:0] RTR IDE EXID[17:15]
LIST: FBMx=1, four filters (n&n+1 in FR1, n+2&n+3 in FR2)
    IDn:   CAN1->sFilterRegister[x].FRn[0..15]
    IDn+1: CAN1->sFilterRegister[x].FRn[16..31]
*/

/*
Can timing: main freq - APB (PLL=48MHz)
segment = 1sync + TBS1 + TBS2, sample point is between TBS1 and TBS2,
so if TBS1=4 and TBS2=3, sum=8, bit sampling freq is 48/8 = 6MHz
-> to get   100kbps we need prescaler=60
            250kbps - 24
            500kbps - 12
            1MBps   - 6
*/

// speed - in kbps
void CAN_setup(uint16_t speed){
    if(speed == 0) speed = oldspeed;
    else if(speed < 50) speed = 50;
    else if(speed > 3000) speed = 3000;
    oldspeed = speed;
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
    /* Configure CAN */
    /* (1) Enter CAN init mode to write the configuration */
    /* (2) Wait the init mode entering */
    /* (3) Exit sleep mode */
    /* (4) Normal mode, set timing to 100kb/s: TBS1 = 4, TBS2 = 3, prescaler = 60 */
    /* (5) Leave init mode */
    /* (6) Wait the init mode leaving */
    /* (7) Enter filter init mode, (16-bit + mask, bank 0 for FIFO 0) */
    /* (8) Acivate filter 0 for two IDs */
    /* (9) Identifier list mode */
    /* (10) Set the Id list */
    /* (12) Leave filter init */
    /* (13) Set error interrupts enable (& bus off) */
    CAN1->MCR |= CAN_MCR_INRQ; /* (1) */
    while((CAN1->MSR & CAN_MSR_INAK) != CAN_MSR_INAK) /* (2) */
        if(--tmout == 0) break;
    CAN1->MCR &=~ CAN_MCR_SLEEP; /* (3) */
    CAN1->MCR |= CAN_MCR_ABOM; /* allow automatically bus-off */

    CAN1->BTR =  2 << 20 | 3 << 16 | (4500/speed - 1); //| CAN_BTR_SILM | CAN_BTR_LBKM; /* (4) */
    CAN1->MCR &= ~CAN_MCR_INRQ; /* (5) */
    tmout = 16000000;
    while((CAN1->MSR & CAN_MSR_INAK) == CAN_MSR_INAK) /* (6) */
        if(--tmout == 0) break;
    // accept ALL
    CAN1->FMR = CAN_FMR_FINIT; /* (7) */
    CAN1->FA1R = CAN_FA1R_FACT0 | CAN_FA1R_FACT1; /* (8) */
    // set to 1 all needed bits of CAN1->FFA1R to switch given filters to FIFO1
    CAN1->sFilterRegister[0].FR1 = (1<<21)|(1<<5); // all odd IDs
    CAN1->FFA1R = 2; // filter 1 for FIFO1, filter 0 - for FIFO0
    CAN1->sFilterRegister[1].FR1 = (1<<21); // all even IDs
    CAN1->FMR &= ~CAN_FMR_FINIT; /* (12) */
    CAN1->IER |= CAN_IER_ERRIE | CAN_IER_FOVIE0 | CAN_IER_FOVIE1 | CAN_IER_BOFIE; /* (13) */

    /* Configure IT */
    NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 0); // RX FIFO0 IRQ
    NVIC_SetPriority(CAN1_RX1_IRQn, 0); // RX FIFO1 IRQ
    NVIC_SetPriority(CAN1_SCE_IRQn, 0); // RX status changed IRQ
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    NVIC_EnableIRQ(CAN1_RX1_IRQn);
    NVIC_EnableIRQ(CAN1_SCE_IRQn);
    CAN1->MSR = 0; // clear SLAKI, WKUI, ERRI
    can_status = CAN_READY;
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
        usart_send("\nToo much errors, restarting CAN!\n");
        // request abort for all mailboxes
        CAN1->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
        // reset CAN bus
        RCC->APB1RSTR |= RCC_APB1RSTR_CAN1RST;
        RCC->APB1RSTR &= ~RCC_APB1RSTR_CAN1RST;
        CAN_setup(0);
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
        //msg.filterNo = (box->RDTR >> 8) & 0xff;
        //msg.fifoNum = fifo_num;
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
    //if(*RFxR & CAN_RF0R_FULL0) *RFxR &= ~CAN_RF0R_FULL0;
    *RFxR = 0; // clear FOVR & FULL
}

void usb_lp_can_rx0_isr(){ // Rx FIFO0 (overrun)
    if(CAN1->RF0R & CAN_RF0R_FOVR0){ // FIFO overrun
        CAN1->RF0R &= ~CAN_RF0R_FOVR0;
        can_status = CAN_FIFO_OVERRUN;
    }
}

void can_rx1_isr(){ // Rx FIFO1 (overrun)
    if(CAN1->RF1R & CAN_RF1R_FOVR1){
        CAN1->RF1R &= ~CAN_RF1R_FOVR1;
        can_status = CAN_FIFO_OVERRUN;
    }
}

void can_sce_isr(){ // status changed
    if(CAN1->MSR & CAN_MSR_ERRI){ // Error
        CAN1->MSR &= ~CAN_MSR_ERRI;
        // request abort for problem mailbox
        if(CAN1->TSR & CAN_TSR_TERR0) CAN1->TSR |= CAN_TSR_ABRQ0;
        if(CAN1->TSR & CAN_TSR_TERR1) CAN1->TSR |= CAN_TSR_ABRQ1;
        if(CAN1->TSR & CAN_TSR_TERR2) CAN1->TSR |= CAN_TSR_ABRQ2;
        can_status = CAN_ERR;
    }
}
