/*
 * This file is part of the canbus4bta project.
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
#include "commonfunctions.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "strfunc.h"
#include "usb.h"

#ifdef EBUG
#undef EBUG
#endif

// PD1 - Tx, PD0 - Rx !!!

#include <string.h> // memcpy

// CAN bus oscillator frequency: 36MHz
#define CAN_F_OSC       (36000000UL)
// timing values TBS1 and TBS2 (in BTR [TBS1-1] and [TBS2-1])
// use 3 and 2 to get 6MHz
#define CAN_TBS1    (3)
#define CAN_TBS2    (2)
// bitrate oscillator frequency
#define CAN_BIT_OSC (CAN_F_OSC / (1+CAN_TBS1+CAN_TBS2))

uint8_t cansniffer = 0; // 0 - receive only 0 and myID, 1 - receive all

// circular buffer for  received messages
static CAN_message messages[CAN_INMESSAGE_SIZE];
static uint8_t first_free_idx = 0;    // index of first empty cell
static int8_t first_nonfree_idx = -1; // index of first data cell
static uint32_t oldspeed = 100000; // speed of last init
uint32_t floodT = FLOOD_PERIOD_MS; // flood period in ms
static uint8_t incrflood = 0; // ==1 for incremental flooding

static uint32_t last_err_code = 0;
static CAN_status can_status = CAN_STOP;

static void can_process_fifo(uint8_t fifo_num);

static CAN_message loc_flood_msg;
static CAN_message *flood_msg = NULL; // == loc_flood_msg - to flood

CAN_status CAN_get_status(){
    CAN_status st = can_status;
    //can_status = CAN_OK;
    if(st == CAN_FIFO_OVERRUN) can_status = CAN_READY;
    return st;
}

// push next message into buffer; return 1 if buffer overfull
static int CAN_messagebuf_push(CAN_message *msg){
    //MSG("Try to push\n");
#ifdef EBUG
        USB_sendstr("push: ");
        for(int i = 0; i < msg->length; ++i){
            printuhex(msg->data[i]); USB_putbyte(' ');
        }
        newline();
#endif
    if(first_free_idx == first_nonfree_idx){
#ifdef EBUG
        USB_sendstr("INBUF OVERFULL\n");
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
    #ifdef EBUG
    //MSG("read from idx "); printu(first_nonfree_idx); NL();
    #endif
    CAN_message *msg = &messages[first_nonfree_idx++];
    if(first_nonfree_idx == CAN_INMESSAGE_SIZE) first_nonfree_idx = 0;
    if(first_nonfree_idx == first_free_idx){ // buffer is empty - refresh it
        first_nonfree_idx = -1;
        first_free_idx = 0;
    }
#ifdef EBUG
        USB_sendstr("pop: ");
        for(int i = 0; i < msg->length; ++i){
            printuhex(msg->data[i]); USB_putbyte(' ');
        }
        newline();
#endif
    return msg;
}

void CAN_reinit(uint32_t speed){
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
so if TBS1=4 and TBS2=3, sum=8, bit sampling freq is 36/8 = 4.5MHz
-> to get   100kbps we need prescaler=60
            250kbps - 18
            500kbps - 9
            1MBps   - [4.5]
*/

// speed - in kbps
// GPIO configured in hw_setup
void CAN_setup(uint32_t speed){
    if(speed == 0) speed = oldspeed;
    else if(speed < CAN_MIN_SPEED) speed = CAN_MIN_SPEED;
    else if(speed > CAN_MAX_SPEED) speed = CAN_MAX_SPEED;
    uint32_t tmout = 10000;
    /* Enable the peripheral clock CAN */
    RCC->APB1ENR |= RCC_APB1ENR_CANEN;
    /* Configure CAN */
    /* (1) Enter CAN init mode to write the configuration */
    /* (2) Wait the init mode entering */
    /* (3) Exit sleep mode */
    /* (4) Normal mode, set timing : TBS1 = 4, TBS2 = 3 */
    /* (5) Leave init mode */
    /* (6) Wait the init mode leaving */
    /* (7) Enter filter init mode, (16-bit + mask, bank 0 for FIFO 0) */
    /* (8) Acivate filter 0 for two IDs */
    /* (9)  Identifier mode for bank#0, mask mode for #1  */
    /* (10) Set the Id list */
    /* (12) Leave filter init */
    /* (13) Set error interrupts enable (& bus off) */
    CAN->MCR |= CAN_MCR_INRQ; /* (1) */
    while((CAN->MSR & CAN_MSR_INAK) != CAN_MSR_INAK) /* (2) */
        if(--tmout == 0) break;
    if(tmout==0){ DBG("timeout!\n");}
    CAN->MCR &=~ CAN_MCR_SLEEP; /* (3) */
    CAN->MCR |= CAN_MCR_ABOM; /* allow automatically bus-off */
    CAN->BTR =  (CAN_TBS2-1) << 20 | (CAN_TBS1-1) << 16 | (CAN_BIT_OSC/speed - 1); //| CAN_BTR_SILM | CAN_BTR_LBKM; /* (4) */
    oldspeed = CAN_BIT_OSC/(uint32_t)((CAN->BTR & CAN_BTR_BRP) + 1);
    CAN->MCR &= ~CAN_MCR_INRQ; /* (5) */
    tmout = 10000;
    while(CAN->MSR & CAN_MSR_INAK) /* (6) */
        if(--tmout == 0) break;
    if(tmout==0){ DBG("timeout!\n");}
    // accept only 0 & ID or ALL depending on `cansniffer` value
    CAN->FMR = CAN_FMR_FINIT; /* (7) */
    CAN->FA1R = CAN_FA1R_FACT0; /* (8) */
    CAN->FM1R = CAN_FM1R_FBM0; /* (9) */
    // CAN->FSCx = 0 -> dual 16-bit scale configuration
    // filter 0 for FIFO0
    CAN->sFilterRegister[0].FR1 = the_conf.CANID << 5; // (10) CANID and 0
    if(cansniffer){ /* (11) */
        CAN->FA1R |= CAN_FA1R_FACT1; // activate filter1
        CAN->sFilterRegister[1].FR1 = 0; // all packets
        // mask mode, all odd and even IDs
        //CAN->sFilterRegister[1].FR1 = (1<<21)|(1<<5); // all odd IDs, mask mode
        //CAN->sFilterRegister[1].FR2 = (1<<21)|(0<<5); // all even IDs, mask mode
        CAN->FFA1R = 2; // filter 1 for FIFO1
    }
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
}

/**
 * @brief CAN_sniffer - reconfigure CAN in sniffer or normal mode
 * @param issniffer - ==0 for normal mode
 */
void CAN_sniffer(uint8_t issniffer){
    cansniffer = issniffer;
    CAN_reinit(0);
}

void CAN_printerr(){
    if(!last_err_code) last_err_code = CAN->ESR;
    if(!last_err_code){
        USB_sendstr("No errors\n");
        return;
    }
    USB_sendstr("Receive error counter: ");
    USB_sendstr(u2str((last_err_code & CAN_ESR_REC)>>24));
    USB_sendstr("\nTransmit error counter: ");
    USB_sendstr(u2str((last_err_code & CAN_ESR_TEC)>>16));
    USB_sendstr("\nLast error code: ");
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
    USB_sendstr(errmsg); USB_sendstr(" error\n");
    if(last_err_code & CAN_ESR_BOFF) USB_sendstr("Bus off ");
    if(last_err_code & CAN_ESR_EPVF) USB_sendstr("Passive error limit ");
    if(last_err_code & CAN_ESR_EWGF) USB_sendstr("Error counter limit");
    last_err_code = 0;
    USB_putbyte('\n');
}

void CAN_proc(){
#ifdef EBUG
    if(last_err_code){
        USB_sendstr("Error, ESR=");
        USB_sendstr(u2str(last_err_code));
        USB_putbyte('\n');
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
        USB_sendstr("error=canbuserr\n");
        CAN_printerr();
        CAN_reinit(0);
    }
    static uint32_t lastFloodTime = 0;
    static uint32_t incrmessagectr = 0;
    if(flood_msg && (Tms - lastFloodTime) >= floodT){ // flood every ~5ms
        lastFloodTime = Tms;
        CAN_send(flood_msg);
    }else if(incrflood && (Tms - lastFloodTime) >= floodT){
        lastFloodTime = Tms;
        *(uint32_t*)flood_msg->data = incrmessagectr;
        flood_msg->length = 4;
        if(CAN_OK == CAN_send(flood_msg)) ++incrmessagectr;
    }
}

CAN_status CAN_send(CAN_message *message){
    if(!message) return CAN_ERR;
    uint8_t *msg = message->data;
    uint8_t len = message->length;
    uint16_t target_id = message->ID;
    uint8_t mailbox = 0xff;
    // check first free mailbox
    uint32_t Tstart = Tms;
    while(Tms - Tstart < SEND_TIMEOUT_MS/10){
        IWDG->KR = IWDG_REFRESH;
        if(CAN->TSR & (CAN_TSR_TME)){
            mailbox = (CAN->TSR & CAN_TSR_CODE) >> 24;
            break;
        }
    }
    if(mailbox == 0xff){// no free mailboxes
#ifdef EBUG
        USB_sendstr("No free mailboxes\n");
#endif
        return CAN_BUSY;
    }
#ifdef EBUG
    USB_sendstr("Send data. Len="); USB_sendstr(u2str(len));
    USB_sendstr(", tagid="); USB_sendstr(u2str(target_id));
    USB_sendstr(", data=");
    for(int i = 0; i < len; ++i){
        USB_sendstr(" "); USB_sendstr(uhex2str(msg[i]));
    }
    USB_putbyte('\n');
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

void CAN_flood(CAN_message *msg, int incr){
    if(incr){ incrflood = 1; return; }
    incrflood = 0;
    if(!msg) flood_msg = NULL;
    else{
        memcpy(&loc_flood_msg, msg, sizeof(CAN_message));
        flood_msg = &loc_flood_msg;
    }
}

uint32_t CAN_speed(){
    return oldspeed;
}

/**
 * @brief parseCANcommand - parser
 * @param msg - incoming message @ my CANID
 * FORMAT:
 *  0 1   2      3    4 5 6 7
 * [CMD][PAR][errcode][VALUE]
 * CMD - uint16_t, PAR - uint8_t, errcode - one of CAN_errcodes, VALUE - int32_t
 * `errcode` of  incoming message doesn't matter
 * incoming data may have variable length
 */
TRUE_INLINE void parseCANcommand(CAN_message *msg){
    msg->ID = the_conf.CANID; // set own ID for broadcast messages
    // check PING
    if(msg->length != 0) run_can_cmd(msg);
    uint32_t Tstart = Tms;
    while(Tms - Tstart < SEND_TIMEOUT_MS){
        if(CAN_OK == CAN_send(msg)) return;
        IWDG->KR = IWDG_REFRESH;
    }
    // TODO: buzzer error can't send
    USB_sendstr("error=canbusy\n");
}

static void can_process_fifo(uint8_t fifo_num){
    if(fifo_num > 1) return;
    CAN_FIFOMailBox_TypeDef *box = &CAN->sFIFOMailBox[fifo_num];
    volatile uint32_t *RFxR = (fifo_num) ? &CAN->RF1R : &CAN->RF0R;
#ifdef EBUG
        USB_sendstr(u2str(*RFxR & CAN_RF0R_FMP0)); USB_sendstr(" messages in FIFO #");
        USB_sendstr(u2str(fifo_num)); newline();
#endif
    // read all
    while(*RFxR & CAN_RF0R_FMP0){ // amount of messages pending
        // CAN_RDTxR: (16-31) - timestamp, (8-15) - filter match index, (0-3) - data length
        /* TODO: check filter match index if more than one ID can receive */
        CAN_message msg;
        bzero(&msg, sizeof(msg));
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
        if(msg.ID == the_conf.CANID || msg.ID == 0) parseCANcommand(&msg);
        if(cansniffer && CAN_messagebuf_push(&msg)) return; // error: buffer is full, try later
        *RFxR |= CAN_RF0R_RFOM0; // release fifo for access to next message
    }
    //if(*RFxR & CAN_RF0R_FULL0) *RFxR &= ~CAN_RF0R_FULL0;
    *RFxR = 0; // clear FOVR & FULL
}

void usb_lp_can1_rx0_isr(){ // Rx FIFO0 (overrun)
    if(CAN->RF0R & CAN_RF0R_FOVR0){ // FIFO overrun
        CAN->RF0R &= ~CAN_RF0R_FOVR0;
        can_status = CAN_FIFO_OVERRUN;
    }
}

void can1_rx1_isr(){ // Rx FIFO1 (overrun)
    if(CAN->RF1R & CAN_RF1R_FOVR1){
        CAN->RF1R &= ~CAN_RF1R_FOVR1;
        can_status = CAN_FIFO_OVERRUN;
    }
}

void can1_sce_isr(){ // status changed
    if(CAN->MSR & CAN_MSR_ERRI){ // Error
#ifdef EBUG
        last_err_code = CAN->ESR;
#endif
        CAN->MSR &= ~CAN_MSR_ERRI;
        // request abort for problem mailbox
        if(CAN->TSR & CAN_TSR_TERR0) CAN->TSR |= CAN_TSR_ABRQ0;
        if(CAN->TSR & CAN_TSR_TERR1) CAN->TSR |= CAN_TSR_ABRQ1;
        if(CAN->TSR & CAN_TSR_TERR2) CAN->TSR |= CAN_TSR_ABRQ2;
        can_status = CAN_ERR;
    }
}
