/*
 * This file is part of the multistepper project.
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
#include "commonproto.h"
#include "flash.h"
#include "hardware.h"
#include "strfunc.h"
#include "usb.h"

// PD1 - Tx, PD0 - Rx !!!

#include <string.h> // memcpy

// circular buffer for  received messages
static CAN_message messages[CAN_INMESSAGE_SIZE];
static uint8_t first_free_idx = 0;    // index of first empty cell
static int8_t first_nonfree_idx = -1; // index of first data cell
static uint16_t oldspeed = 100; // speed of last init
uint32_t floodT = FLOOD_PERIOD_MS; // flood period in ms
static uint8_t incrflood = 0; // ==1 for incremental flooding

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
    //MSG("Try to push\n");
#ifdef EBUG
        USB_sendstr("push\n");
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
// GPIO configured in hw_setup
void CAN_setup(uint16_t speed){
    if(speed == 0) speed = oldspeed;
    else if(speed < 50) speed = 50;
    else if(speed > 3000) speed = 3000;
    oldspeed = speed;
    uint32_t tmout = 10000;
    /* Enable the peripheral clock CAN */
    RCC->APB1ENR |= RCC_APB1ENR_CANEN;
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
    CAN->MCR |= CAN_MCR_INRQ; /* (1) */
    while((CAN->MSR & CAN_MSR_INAK) != CAN_MSR_INAK) /* (2) */
        if(--tmout == 0) break;
    if(tmout==0){ DBG("timeout!\n");}
    CAN->MCR &=~ CAN_MCR_SLEEP; /* (3) */
    CAN->MCR |= CAN_MCR_ABOM; /* allow automatically bus-off */

    CAN->BTR =  2 << 20 | 3 << 16 | (4500/speed - 1); //| CAN_BTR_SILM | CAN_BTR_LBKM; /* (4) */
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
        USB_sendstr("\nToo much errors, restarting CAN!\n");
        CAN_printerr();
        // request abort for all mailboxes
        CAN->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
        // reset CAN bus
        RCC->APB1RSTR |= RCC_APB1RSTR_CANRST;
        RCC->APB1RSTR &= ~RCC_APB1RSTR_CANRST;
        CAN_setup(0);
    }
    static uint32_t lastFloodTime = 0;
    static uint32_t incrmessagectr = 0;
    if(flood_msg && (Tms - lastFloodTime) >= floodT){ // flood every ~5ms
        lastFloodTime = Tms;
        CAN_send(flood_msg->data, flood_msg->length, flood_msg->ID);
    }else if(incrflood && (Tms - lastFloodTime) >= floodT){
        lastFloodTime = Tms;
        if(CAN_OK == CAN_send((uint8_t*)&incrmessagectr, 4, flood_msg->ID)) ++incrmessagectr;
    }
}

CAN_status CAN_send(uint8_t *msg, uint8_t len, uint16_t target_id){
    uint8_t mailbox = 0;
    // check first free mailbox
    if(CAN->TSR & (CAN_TSR_TME)){
        mailbox = (CAN->TSR & CAN_TSR_CODE) >> 24;
    }else{ // no free mailboxes
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

static void formerr(CAN_message *msg, errcodes err){
    if(msg->length < 4) msg->length = 4;
    msg->data[3] = (uint8_t)err;
}

/**
 * @brief parseCANcommand - parser
 * @param msg - incoming message @ my CANID
 * FORMAT:
 *  0 1   2      3    4 5 6 7
 * [CMD][PAR][errcode][VALUE]
 * CMD - uint16_t, PAR - uint8_t, errcode - one of CAN_errcodes, VALUE - int32_t
 * `errcode` of  incoming message doesn't matter
 */
TRUE_INLINE void parseCANcommand(CAN_message *msg){
    int N = 1000;
    // we don't check msg here as it cannot be NULL
#ifdef EBUG
    DBG("Get data");
    for(int i = 0; i < msg->length; ++i){
        USB_sendstr(uhex2str(msg->data[i])); USB_putbyte(' ');
    }
    newline();
#endif
    if(msg->length == 0) goto sendmessage; // PING
    uint16_t Index = *(uint16_t*)msg->data;
#ifdef EBUG
    USB_sendstr("Index = "); USB_sendstr(u2str(Index)); newline();
#endif
    if(Index >= CCMD_AMOUNT){
        formerr(msg, ERR_BADCMD);
        goto sendmessage;
    }
    msg->data[3] = ERR_OK;
    uint8_t par = msg->data[2];
    if(par & 0x80){
        formerr(msg, ERR_BADPAR);
        goto sendmessage;
    }
    int32_t *val = (int32_t *)(&msg->data[4]);
    if(msg->length == 8) par |= 0x80;
    else if(msg->length == 2) par = CANMESG_NOPAR; // no parameter
    else if(msg->length != 3){ // wrong length
        formerr(msg, ERR_WRONGLEN);
        goto sendmessage;
    }
#ifdef EBUG
    USB_sendstr("Run command\n");
#endif
    errcodes ec = cancmdlist[Index](par, val);
    if(ec != ERR_OK){
        formerr(msg, ec);
    }else{
        msg->length = 8;
    }
sendmessage:
    while(CAN_BUSY == CAN_send(msg->data, msg->length, the_conf.CANID))
        if(--N == 0) break;
}

static void can_process_fifo(uint8_t fifo_num){
    if(fifo_num > 1) return;
    CAN_FIFOMailBox_TypeDef *box = &CAN->sFIFOMailBox[fifo_num];
    volatile uint32_t *RFxR = (fifo_num) ? &CAN->RF1R : &CAN->RF0R;
#ifdef EBUG
        USB_sendstr(u2str(*RFxR & CAN_RF0R_FMP0)); USB_sendstr(" messages in FIFO\n");
#endif
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
        if(msg.ID == the_conf.CANID) parseCANcommand(&msg);
        if(CAN_messagebuf_push(&msg)) return; // error: buffer is full, try later
        *RFxR |= CAN_RF0R_RFOM0; // release fifo for access to next message
    }
    //if(*RFxR & CAN_RF0R_FULL0) *RFxR &= ~CAN_RF0R_FULL0;
    *RFxR = 0; // clear FOVR & FULL
}

;

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
