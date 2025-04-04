/*
 * This file is part of the canrelay project.
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

#include "can.h"
#include "canproto.h"
#include "hardware.h"
#include "proto.h"
#include "usb.h"

#include <string.h> // memcpy

// circular buffer for  received messages
static CAN_message messages[CAN_INMESSAGE_SIZE];
static uint8_t first_free_idx = 0;    // index of first empty cell
static int8_t first_nonfree_idx = -1; // index of first data cell
static uint16_t oldspeed = DEFAULT_CAN_SPEED; // speed of last init

#ifdef EBUG
static uint32_t last_err_code = 0;
#endif
static CAN_status can_status = CAN_STOP;

static void can_process_fifo(uint8_t fifo_num);

static CAN_message loc_flood_msg;
static CAN_message *flood_msg = NULL; // == loc_flood_msg - to flood


CAN_status CAN_get_status(){
    CAN_status st = can_status;
    if(st == CAN_FIFO_OVERRUN) can_status = CAN_READY;
    return st;
}

// push next message into buffer; return 1 if buffer overfull
static int CAN_messagebuf_push(CAN_message *msg){
    //MSG("Try to push\n");
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
    #ifdef EBUG
    //MSG("read from idx "); printu(first_nonfree_idx); NL();
    #endif
    CAN_message *msg = &messages[first_nonfree_idx++];
    if(first_nonfree_idx == CAN_INMESSAGE_SIZE) first_nonfree_idx = 0;
    if(first_nonfree_idx == first_free_idx){ // buffer is empty - refresh it
        first_nonfree_idx = -1;
        first_free_idx = 0;
    }
    return msg;
}

void CAN_reinit(uint16_t speed){
    CAN->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
    RCC->APB1RSTR |= RCC_APB1RSTR_CANRST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_CANRST;
    CAN_setup(speed);
}

/*
Can filtering: FSCx=0 (CAN->FS1R) -> 16-bit identifiers
MASK: FBMx=0 (CAN->FM1R), two filters (n in FR1 and n+1 in FR2)
    ID:   CAN->sFilterRegister[x].FRn[0..15]
    MASK: CAN->sFilterRegister[x].FRn[16..31]
    FR bits:  STID[10:0] RTR IDE EXID[17:15]
LIST: FBMx=1, four filters (n&n+1 in FR1, n+2&n+3 in FR2)
    IDn:   CAN->sFilterRegister[x].FRn[0..15]
    IDn+1: CAN->sFilterRegister[x].FRn[16..31]
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
    DBG("Start CAN ini");
    // Configure GPIO: PB8 - CAN_Rx, PB9 - CAN_Tx
    /* (1) Select AF mode (10) on PB8 and PB9 */
    /* (2) AF4 for CAN signals */
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9))
                 | (GPIO_MODER_MODER8_AF | GPIO_MODER_MODER9_AF); /* (1) */
    GPIOB->AFR[1] = (GPIOB->AFR[1] &~ (GPIO_AFRH_AFRH0 | GPIO_AFRH_AFRH1))\
                  | (4 << (0 * 4)) | (4 << (1 * 4)); /* (2) */
    /* Enable the peripheral clock CAN */
    RCC->APB1ENR |= RCC_APB1ENR_CANEN;
    // Configure CAN
    CAN->MCR |= CAN_MCR_INRQ; // Enter CAN init mode to write the configuration
    while((CAN->MSR & CAN_MSR_INAK) != CAN_MSR_INAK){
        IWDG->KR = IWDG_REFRESH;
        if(--tmout == 0) break;
    }
    CAN->MCR &=~ CAN_MCR_SLEEP;
    CAN->MCR |= CAN_MCR_ABOM; // allow automatically bus-off
    CAN->BTR =  2 << 20 | 3 << 16 | (6000/speed - 1); // speed
    CAN->MCR &=~ CAN_MCR_INRQ;
    tmout = 16000000;
    while((CAN->MSR & CAN_MSR_INAK) == CAN_MSR_INAK){ // Wait the init mode leaving
        IWDG->KR = IWDG_REFRESH;
        if(--tmout == 0){
            DBG("Can't enter CAN INIT mode");
            break;
        }
    }
    DBG("CAN initing");
    // accept self ID at filter 0, ALL other at filters 1 and 2
    CAN->FMR = CAN_FMR_FINIT;
    CAN->FA1R = CAN_FA1R_FACT0 | CAN_FA1R_FACT1 | CAN_FA1R_FACT2;
    CAN->FM1R = CAN_FM1R_FBM0; // identifier mode for bank#0, mask mode for #1 and #2
    // set to 1 all needed bits of CAN->FFA1R to switch given filters to FIFO1
    CAN->sFilterRegister[0].FR1 = CANID << 5; // self ID
    CAN->sFilterRegister[1].FR1 = (1<<21)|(1<<5); // all odd IDs
    CAN->sFilterRegister[2].FR1 = (1<<21); // all even IDs
    CAN->FFA1R = 2; // filter 1 for FIFO1, filters 0&2 - for FIFO0
    CAN->FMR &=~ CAN_FMR_FINIT; // end of filters init
    CAN->IER |= CAN_IER_ERRIE | CAN_IER_FOVIE0 | CAN_IER_FOVIE1;

    /* Configure IT */
    NVIC_SetPriority(CEC_CAN_IRQn, 0);
    NVIC_EnableIRQ(CEC_CAN_IRQn);
    can_status = CAN_READY;
    DBG("CAN ready");
}

void can_proc(){
#ifdef EBUG
    if(last_err_code){
        USB_sendstr("Error, ESR=");
        printu(last_err_code);
        newline();
        last_err_code = 0;
    }
#endif
    // check for messages in FIFO0 & FIFO1
    if(CAN->RF0R & CAN_RF0R_FMP0){
        can_process_fifo(0);
    }
    if(CAN->RF1R & CAN_RF1R_FMP1){
        can_process_fifo(1);
    }
    IWDG->KR = IWDG_REFRESH;
    if(CAN->ESR & (CAN_ESR_BOFF | CAN_ESR_EPVF | CAN_ESR_EWGF)){ // much errors - restart CAN BUS
        USND("\nToo much errors, restarting CAN!");
        USB_sendstr("Receive error counter: ");
        printu((CAN->ESR & CAN_ESR_REC)>>24);
        USB_sendstr("\nTransmit error counter: ");
        printu((CAN->ESR & CAN_ESR_TEC)>>16);
        USB_sendstr("\nLast error code: ");
        int lec = (CAN->ESR & CAN_ESR_LEC) >> 4;
        const char *errmsg = "No";
        switch(lec){
            case 1: errmsg = "Stuff"; break;
            case 2: errmsg = "Form"; break;
            case 3: errmsg = "Ack"; break;
            case 4: errmsg = "Bit recessive"; break;
            case 5: errmsg = "Bit dominant"; break;
            case 6: errmsg = "CRC"; break;
            case 7: errmsg = "(set by software)"; break;
        }
        USB_sendstr(errmsg); USND(" error\n");
        if(CAN->ESR & CAN_ESR_BOFF) USND("Bus off");
        if(CAN->ESR & CAN_ESR_EPVF) USND("Passive error limit");
        if(CAN->ESR & CAN_ESR_EWGF) USND("Error counter limit");
        IWDG->KR = IWDG_REFRESH;
        // request abort for all mailboxes
        CAN->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
        // reset CAN bus
        RCC->APB1RSTR |= RCC_APB1RSTR_CANRST;
        RCC->APB1RSTR &= ~RCC_APB1RSTR_CANRST;
        CAN_setup(0);
    }
    static uint32_t lastFloodTime = 0;
    if(flood_msg && (Tms - lastFloodTime) > (FLOOD_PERIOD_MS-1)){ // flood every ~5ms
        lastFloodTime = Tms;
        can_send(flood_msg->data, flood_msg->length, flood_msg->ID);
    }
}

CAN_status can_send(uint8_t *msg, uint8_t len, uint16_t target_id){
    uint8_t mailbox = 0;
    // check first free mailbox
    if(CAN->TSR & (CAN_TSR_TME)){
        mailbox = (CAN->TSR & CAN_TSR_CODE) >> 24;
    }else{ // no free mailboxes
        //SEND("No free mailboxes"); NL();
        return CAN_BUSY;
    }
#ifdef EBUG
    USB_sendstr("Send data. Len="); printu(len);
    USB_sendstr(", tagid="); printuhex(target_id);
    USB_sendstr(", data=");
    for(int i = 0; i < len; ++i){
        USB_sendstr(" "); printuhex(msg[i]);
    }
    newline();
#endif
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

void set_flood(CAN_message *msg){
    if(!msg) flood_msg = NULL;
    else{
        memcpy(&loc_flood_msg, msg, sizeof(CAN_message));
        flood_msg = &loc_flood_msg;
    }
}

static void can_process_fifo(uint8_t fifo_num){
    if(fifo_num > 1) return;
    CAN_FIFOMailBox_TypeDef *box = &CAN->sFIFOMailBox[fifo_num];
    volatile uint32_t *RFxR = (fifo_num) ? &CAN->RF1R : &CAN->RF0R;
    // read all
    while(*RFxR & CAN_RF0R_FMP0){ // amount of messages pending
        // CAN_RDTxR: (16-31) - timestamp, (8-15) - filter match index, (0-3) - data length
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
        if(msg.ID == CANID) parseCANcommand(&msg);
        if(CAN_messagebuf_push(&msg)) return; // error: buffer is full, try later
        *RFxR |= CAN_RF0R_RFOM0; // release fifo for access to next message
    }
    //if(*RFxR & CAN_RF0R_FULL0) *RFxR &= ~CAN_RF0R_FULL0;
    *RFxR = 0; // clear FOVR & FULL
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
#ifdef EBUG
        last_err_code = CAN->ESR;
#endif
    }
}
