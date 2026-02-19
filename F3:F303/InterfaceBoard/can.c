/*
 * This file is part of the multiiface project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <string.h> // memcpy

// read private defines from "canproto.h"
#define CANPRIVATE__

#include "can.h"
#include "canproto.h"
#include "Debug.h"
#include "hardware.h"
#include "strfunc.h"

// PB9 - Tx, PB8 - Rx !!!

// CAN settings
// CAN bus oscillator frequency: 36MHz (APB1)
#define CAN_F_OSC       (36000000UL)
// timing values TBS1 and TBS2 (in BTR [TBS1-1] and [TBS2-1])
// use 3 and 2 to get 6MHz
#define CAN_TBS1    (3)
#define CAN_TBS2    (2)
// bitrate oscillator frequency
#define CAN_BIT_OSC (CAN_F_OSC / (1+CAN_TBS1+CAN_TBS2))



// circular buffer for  received messages
static CAN_message messages[CAN_INMESSAGE_SIZE];
static uint8_t first_free_idx = 0;    // index of first empty cell
static int8_t first_nonfree_idx = -1; // index of first data cell
static uint32_t oldspeed = 100000; // speed of last init
uint32_t floodT = FLOOD_PERIOD_MS; // flood period in ms
static uint8_t incrflood = 0; // ==1 for incremental flooding
static uint32_t incrmessagectr = 0; // counter for incremental flooding

static uint32_t last_err_code = 0;
static CAN_status can_status = CAN_STOP;

static void can_process_fifo(uint8_t fifo_num);

static CAN_message loc_flood_msg;
static CAN_message *flood_msg = NULL; // == loc_flood_msg - to flood

CAN_status CAN_get_status(){
    int st = can_status;
    can_status = CAN_OK;
    return st;
}

// push next message into buffer; return 1 if buffer overfull
static int CAN_messagebuf_push(CAN_message *msg){
    //DBG("PUSH");
    if(first_free_idx == first_nonfree_idx){
        DBG("INBUF OVERFULL");
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
    //DBG("POP");
    return msg;
}

void CAN_reinit(uint32_t speed){
    DBG("CAN_reinit");
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
// GPIO configured in hw_setup
void CAN_setup(uint32_t speed){
    DBG("CAN_setup");
    if(speed == 0) speed = oldspeed;
    else if(speed < CAN_MIN_SPEED) speed = CAN_MIN_SPEED;
    else if(speed > CAN_MAX_SPEED) speed = CAN_MAX_SPEED;
    uint32_t tmout = 10000;
    // CAN clocking and pins enabled in hardware.c
    /* Configure CAN */
    /* (1) Enter CAN init mode to write the configuration */
    /* (2) Wait the init mode entering */
    /* (3) Exit sleep mode */
    /* (4) Normal mode, set timing: TBS1 = 4, TBS2 = 3, prescaler = 60 */
    /* (5) Leave init mode */
    /* (6) Wait the init mode leaving */
    /* (7) Enter filter init mode, (16-bit + mask, bank 0 for FIFO 0) */
    /* (8) Acivate filter 0 for two IDs */
    /* (9) Identifier list mode */
    /* (10) Set the Id list */
    /* (12) Leave filter init */
    /* (13) Set error interrupts enable (& bus off) */
    CAN->MCR |= CAN_MCR_INRQ; /* (1) */
    while((CAN->MSR & CAN_MSR_INAK) != CAN_MSR_INAK) /* (2) */
        if(--tmout == 0) break;
    if(tmout==0){ DBG("timeout!\n");}
    CAN->MCR &=~ CAN_MCR_SLEEP; /* (3) */
    CAN->MCR |= CAN_MCR_ABOM; /* allow automatically bus-off */
    DBG("new speed:"); DBGs(u2str(speed)); DBGn();
    CAN->BTR =  (CAN_TBS2-1) << 20 | (CAN_TBS1-1) << 16 | (CAN_BIT_OSC/speed - 1);  /* (4) */
    oldspeed = CAN_BIT_OSC/(uint32_t)((CAN->BTR & CAN_BTR_BRP) + 1);
    DBG("saved sped:"); DBGs(u2str(oldspeed)); DBGn();
    CAN->MCR &= ~CAN_MCR_INRQ; /* (5) */
    tmout = 10000;
    while(CAN->MSR & CAN_MSR_INAK) /* (6) */
        if(--tmout == 0) break;
    if(tmout==0){ DBG("timeout!\n");}
    // accept ALL
    CAN->FMR = CAN_FMR_FINIT; /* (7) */
    CAN->FA1R = CAN_FA1R_FACT0 | CAN_FA1R_FACT1; /* (8) */
    // set to 1 all needed bits of CAN->FFA1R to switch given filters to FIFO1
    CAN->sFilterRegister[0].FR1 = (1<<21)|(1<<5); // all odd IDs
    CAN->FFA1R = 2; // filter 1 for FIFO1, filter 0 - for FIFO0
    CAN->sFilterRegister[1].FR1 = (1<<21); // all even IDs
    CAN->FMR &= ~CAN_FMR_FINIT; /* (12) */
    CAN->IER |= CAN_IER_ERRIE | CAN_IER_FOVIE0 | CAN_IER_FOVIE1 | CAN_IER_BOFIE; /* (13) */

    /* Configure IT */
    NVIC_SetPriority(USB_LP_CAN_RX0_IRQn, 0); // RX FIFO0 IRQ
    NVIC_SetPriority(CAN_RX1_IRQn, 0); // RX FIFO1 IRQ
    NVIC_SetPriority(CAN_SCE_IRQn, 0); // RX status changed IRQ
    NVIC_EnableIRQ(USB_LP_CAN_RX0_IRQn);
    NVIC_EnableIRQ(CAN_RX1_IRQn);
    NVIC_EnableIRQ(CAN_SCE_IRQn);
    CAN->MSR = 0; // clear SLAKI, WKUI, ERRI
    can_status = CAN_READY;
    DBGs("CAN configured");
}

void CAN_printerr(){
    if(!last_err_code) last_err_code = CAN->ESR;
    if(!last_err_code){
        PRIstr("No errors\n");
        return;
    }
    PRIstr("Receive error counter: ");
    PRIstr(u2str((last_err_code & CAN_ESR_REC)>>24));
    PRIstr("\nTransmit error counter: ");
    PRIstr(u2str((last_err_code & CAN_ESR_TEC)>>16));
    PRIstr("\nLast error code: ");
    int lec = (last_err_code & CAN_ESR_LEC) >> 4;
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
    PRIstr(errmsg); PRIstr(" error\n");
    if(last_err_code & CAN_ESR_BOFF) PRIstr("Bus off ");
    if(last_err_code & CAN_ESR_EPVF) PRIstr("Passive error limit ");
    if(last_err_code & CAN_ESR_EWGF) PRIstr("Error counter limit");
    last_err_code = 0;
    PRIn();
}

void CAN_proc(){
    // check for messages in FIFO0 & FIFO1
    if(CAN->RF0R & CAN_RF0R_FMP0){
        can_process_fifo(0);
    }
    if(CAN->RF1R & CAN_RF1R_FMP1){
        can_process_fifo(1);
    }
    IWDG->KR = IWDG_REFRESH;
    if(CAN->ESR & (CAN_ESR_BOFF | CAN_ESR_EPVF | CAN_ESR_EWGF)){ // much errors - restart CAN BUS
        PRIstr("\nToo much errors, restarting CAN!\n");
        CAN_printerr();
        // request abort for all mailboxes
        CAN->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
        // reset CAN bus
        RCC->APB1RSTR |= RCC_APB1RSTR_CANRST;
        RCC->APB1RSTR &= ~RCC_APB1RSTR_CANRST;
        CAN_setup(0);
    }
    static uint32_t lastFloodTime = 0;
    if(flood_msg && (Tms - lastFloodTime) >= floodT){ // flood every ~5ms
        lastFloodTime = Tms;
        CAN_send(flood_msg->data, flood_msg->length, flood_msg->ID);
    }else if(incrflood && (Tms - lastFloodTime) >= floodT){
        lastFloodTime = Tms;
        if(CAN_OK == CAN_send((uint8_t*)&incrmessagectr, 4, loc_flood_msg.ID)) ++incrmessagectr;
    }
}

CAN_status CAN_send(uint8_t *msg, uint8_t len, uint16_t target_id){
    uint8_t mailbox = 0;
    // check first free mailbox
    if(CAN->TSR & (CAN_TSR_TME)){
        mailbox = (CAN->TSR & CAN_TSR_CODE) >> 24;
    }else{ // no free mailboxes
        DBG("No free mailboxes");
        return CAN_BUSY;
    }
#if 0
    DBGs("Send data. Len="); DBGs(u2str(len));
    DBGs(", tagid="); DBGs(u2str(target_id));
    DBGs(", data=");
    for(int i = 0; i < len; ++i){
        DBGs(" "); DBGs(uhex2str(msg[i]));
    }
    DBGn();
#endif
    CAN_TxMailBox_TypeDef *box = &CAN->sTxMailBox[mailbox];
    uint32_t lb = 0, hb = 0;
    switch(len){
        case 8:
            hb |= (uint32_t)msg[7] << 24;
            // fallthrough
        case 7:
            hb |= (uint32_t)msg[6] << 16;
            // fallthrough
        case 6:
            hb |= (uint32_t)msg[5] << 8;
            // fallthrough
        case 5:
            hb |= (uint32_t)msg[4];
            // fallthrough
        case 4:
            lb |= (uint32_t)msg[3] << 24;
            // fallthrough
        case 3:
            lb |= (uint32_t)msg[2] << 16;
            // fallthrough
        case 2:
            lb |= (uint32_t)msg[1] << 8;
            // fallthrough
        default:
            lb |= (uint32_t)msg[0];
    }
    box->TDLR = lb;
    box->TDHR = hb;
    box->TDTR = len;
    box->TIR  = (target_id & 0x7FF) << 21 | CAN_TI0R_TXRQ;
    return CAN_OK;
}

// return: 1 - flood is ON, 0 - flood is OFF
int CAN_flood(CAN_message *msg, int incr){
    if(incr){
        incrmessagectr = 0;
        incrflood = 1;
        return 1;
    }else incrflood = 0;
    if(!msg){
        flood_msg = NULL;
        return 0;
    }else{
        memcpy(&loc_flood_msg, msg, sizeof(CAN_message));
        flood_msg = &loc_flood_msg;
    }
    return 1;
}

uint32_t CAN_speed(){
    return oldspeed;
}

static void can_process_fifo(uint8_t fifo_num){
    if(fifo_num > 1) return;
    CAN_FIFOMailBox_TypeDef *box = &CAN->sFIFOMailBox[fifo_num];
    volatile uint32_t *RFxR = (fifo_num) ? &CAN->RF1R : &CAN->RF0R;
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
                    // fallthrough
                case 7:
                    dat[6] = (hb>>16) & 0xff;
                    // fallthrough
                case 6:
                    dat[5] = (hb>>8) & 0xff;
                    // fallthrough
                case 5:
                    dat[4] = hb & 0xff;
                    // fallthrough
                case 4:
                    dat[3] = lb>>24;
                    // fallthrough
                case 3:
                    dat[2] = (lb>>16) & 0xff;
                    // fallthrough
                case 2:
                    dat[1] = (lb>>8) & 0xff;
                    // fallthrough
                case 1:
                    dat[0] = lb & 0xff;
            }
        }
        //if(msg.ID == the_conf.CANID) parseCANcommand(&msg);
        if(CAN_messagebuf_push(&msg)) return; // error: buffer is full, try later
        *RFxR |= CAN_RF0R_RFOM0; // release fifo for access to next message
    }
    //if(*RFxR & CAN_RF0R_FULL0) *RFxR &= ~CAN_RF0R_FULL0;
    *RFxR = 0; // clear FOVR & FULL
}

void usb_lp_can1_rx0_isr(){ // Rx FIFO0 (overrun)
    if(CAN->RF0R & CAN_RF0R_FOVR0){ // FIFO overrun
        CAN->RF0R = CAN_RF0R_FOVR0;
        can_status = CAN_FIFO_OVERRUN;
    }
}

void can1_rx1_isr(){ // Rx FIFO1 (overrun)
    if(CAN->RF1R & CAN_RF1R_FOVR1){
        CAN->RF1R = CAN_RF1R_FOVR1;
        can_status = CAN_FIFO_OVERRUN;
    }
}

void can1_sce_isr(){ // status changed
    if(CAN->MSR & CAN_MSR_ERRI){ // Error
        last_err_code = CAN->ESR;
        CAN->MSR = CAN_MSR_ERRI; // clear flag
        // request abort for problem mailbox
        if(CAN->TSR & CAN_TSR_TERR0) CAN->TSR |= CAN_TSR_ABRQ0;
        if(CAN->TSR & CAN_TSR_TERR1) CAN->TSR |= CAN_TSR_ABRQ1;
        if(CAN->TSR & CAN_TSR_TERR2) CAN->TSR |= CAN_TSR_ABRQ2;
        can_status = CAN_ERR;
    }
}
