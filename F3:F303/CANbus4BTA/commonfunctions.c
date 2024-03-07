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

#include "adc.h"
#include "can.h"
#include "commonfunctions.h"
#include "encoder.h"
#include "flash.h"
#include "gpio.h"
#include "proto.h"
#include "spi.h"
#include "usb.h"

#define FIXDL(m)     do{m->length = 8;}while(0)
/*********** START of all common functions list (for `funclist`) ***********/
// reset MCU
static errcodes reset(CAN_message _U_ *msg){
    USB_sendstr("Soft reset\n");
    USB_sendall();
    NVIC_SystemReset();
    return ERR_OK; // never reached
}
// get/set Tms
static errcodes time_getset(CAN_message *msg){
    if(ISSETTER(msg->data)){
        Tms = *(uint32_t*)&msg->data[4];
    }else FIXDL(msg);
    *(uint32_t*)&msg->data[4] = Tms;
    return ERR_OK;
}
// get MCU T
static errcodes mcut(CAN_message *msg){
    FIXDL(msg);
    *(uint32_t*)&msg->data[4] = (uint32_t)(10.f * getMCUtemp());
    return ERR_OK;
}
// get ADC raw values
static errcodes adcraw(CAN_message *msg){
    FIXDL(msg);
    uint8_t no = msg->data[2] & ~SETTER_FLAG;
    if(no >= NUMBER_OF_ADC_CHANNELS) return ERR_BADPAR;
    *(uint32_t*)&msg->data[4] = getADCval(no);
    return ERR_OK;
}
// get ADC voltage
static errcodes adcv(CAN_message *msg){
    FIXDL(msg);
    uint8_t no = msg->data[2] & ~SETTER_FLAG;
    if(no >= ADC_TSENS) return ERR_BADPAR;
    float v = getADCvoltage(no) * the_conf.adcmul[no] * 100.f;
    *(uint32_t*)&msg->data[4] = (uint32_t) v; // or float??
    return ERR_OK;
}
// get/set CAN ID
static errcodes canid(CAN_message *msg){
    if(ISSETTER(msg->data)){
        the_conf.CANID = *(uint32_t*)&msg->data[4];
        CAN_reinit(0); // setup with new ID
    }else FIXDL(msg);
    *(uint32_t*)&msg->data[4] = the_conf.CANID;
    return ERR_OK;
}
// get/set ADC multiplier
static errcodes adcmul(CAN_message *msg){
    uint8_t no = msg->data[2] & ~SETTER_FLAG;
    if(no >= ADC_TSENS) return ERR_BADPAR;
    if(ISSETTER(msg->data)){
        the_conf.adcmul[no] = ((float)*(uint32_t*)&msg->data[4])/1000.f;
    }else FIXDL(msg);
    *(uint32_t*)&msg->data[4] = (uint32_t)(1000.f * the_conf.adcmul[no]);
    return ERR_OK;
}
// save config
static errcodes saveconf(CAN_message _U_ *msg){
    if(0 == store_userconf()) return ERR_OK;
    return ERR_CANTRUN;
}
// erase storage
static errcodes erasestor(CAN_message _U_ *msg){
    if(0 == erase_storage(-1)) return ERR_OK;
    return ERR_CANTRUN;
}
// relay management
static errcodes relay(CAN_message *msg){
    if(ISSETTER(msg->data)){
        if(msg->data[4] == 1) RELAY_ON();
        else RELAY_OFF();
    }else FIXDL(msg);
    *(uint32_t*)&msg->data[4] = RELAY_GET();
    return ERR_OK;
}
// blocking ESW get status
static errcodes eswblk(CAN_message *msg){
    uint8_t no = msg->data[2] & ~SETTER_FLAG;
    if(no > 1) return ERR_BADPAR;
    FIXDL(msg);
    *(uint32_t*)&msg->data[4] = getSwitches(no);
    return ERR_OK;
}
// bounce-free ESW get status
static errcodes esw(CAN_message *msg){
    uint8_t no = msg->data[2] & ~SETTER_FLAG;
    if(no > 1) return ERR_BADPAR;
    FIXDL(msg);
    *(uint32_t*)&msg->data[4] = getESW(no);
    return ERR_OK;
}
// init SPI2
static errcodes initspi2(CAN_message _U_ *msg){
    spi_setup(2);
    return ERR_OK;
}
// send/read 1..4 bytes
static errcodes sendspi2(CAN_message *msg){
    int n = msg->length - 4;
    if(n < 1) return ERR_BADVAL;
    if(spi_writeread(2, msg->data + 4, n)) return ERR_OK;
    return ERR_CANTRUN;
}
// read encoder value and send over CAN
static errcodes encget(CAN_message *msg){
    FIXDL(msg);
    if(read_encoder(msg->data + 4)) return ERR_OK;
    return ERR_CANTRUN;
}

// common uint32_t setter/getter
static errcodes u32setget(CAN_message *msg){
    uint16_t idx = *(uint16_t*)msg->data;
    uint32_t *ptr = NULL;
    switch(idx){
        case CMD_CANSPEED: ptr = &the_conf.CANspeed; break;
        case CMD_BOUNCE: ptr = &the_conf.bounce_ms; break;
        case CMD_USARTSPEED: ptr = &the_conf.usartspeed; break;
        default: break;
    }
    if(!ptr) return ERR_CANTRUN; // unknown error
    if(ISSETTER(msg->data)){
        *ptr = *(uint32_t*)&msg->data[4];
    }else FIXDL(msg);
    *(uint32_t*)&msg->data[4] = *ptr;
    return ERR_OK;
}
// common bitflag setter/getter
static errcodes flagsetget(CAN_message *msg){
    uint16_t idx = *(uint16_t*)msg->data;
    uint8_t bit = 32;
    switch(idx){
        case CMD_ENCISSSI: bit = FLAGBIT(ENC_IS_SSI); break;
        default: break;
    }
    if(bit > 31) return ERR_CANTRUN; // unknown error
    if(ISSETTER(msg->data)){
        if(msg->data[4])  the_conf.flags |= 1<<bit;
        else the_conf.flags &= ~(1<<bit);
    }else FIXDL(msg);
    *(uint32_t*)&msg->data[4] = (the_conf.flags & (1<<bit)) ? 1 : 0;
    return ERR_OK;
}
/************ END of all common functions list (for `funclist`) ************/


typedef struct{
    errcodes (*fn)(CAN_message *msg); // function to run with can packet `msg`
    int32_t minval;  // minimal/maximal values of *(int32_t*)(&data[4]) - if minval != maxval
    int32_t maxval;
    uint8_t datalen; // minimal data length (full CAN packet, bytes)
} commonfunction;

// list of common (CAN/USB) functions
// !!!!!!!!! Getters should set message length to 8 !!!!!!!!!!!
static const commonfunction funclist[CMD_AMOUNT] = {
    [CMD_RESET] = {reset, 0, 0, 0},
    [CMD_TIME] = {time_getset, 0, 0, 0},
    [CMD_MCUTEMP] = {mcut, 0, 0, 0},
    [CMD_ADCRAW] = {adcraw, 0, 0, 3}, // need parno: 0..4
    [CMD_ADCV] = {adcv, 0, 0, 3}, // need parno: 0..3
    [CMD_CANSPEED] = {u32setget, CAN_MIN_SPEED, CAN_MAX_SPEED, 0},
    [CMD_CANID] = {canid, 1, 0x7ff, 0},
    [CMD_ADCMUL] = {adcmul, 0, 0, 3}, // at least parno
    [CMD_SAVECONF] = {saveconf, 0, 0, 0},
    [CMD_ERASESTOR] = {erasestor, 0, 0, 0},
    [CMD_RELAY] = {relay, 0, 1, 0},
    [CMD_GETESW_BLK] = {eswblk, 0, 0, 3},
    [CMD_GETESW] = {esw, 0, 0, 3},
    [CMD_BOUNCE] = {u32setget, 0, 300, 0},
    [CMD_USARTSPEED] = {u32setget, 1200, 3000000, 0},
    [CMD_ENCISSSI] = {flagsetget, 0, 0, 0},
    [CMD_SPIINIT] = {initspi2, 0, 0, 0},
    [CMD_SPISEND] = {sendspi2, 0, 0, 5},
    [CMD_ENCGET] = {encget, 0, 0, 0},
};


/**
 * FORMAT:
 *  0 1   2      3    4 5 6 7
 * [CMD][PAR][errcode][VALUE]
 * CMD - uint16_t, PAR - uint8_t, errcode - one of `errcodes`, VALUE - int32_t
 */

/**
 * @brief run_can_cmd - run common CAN/USB commands with limits checking
 * @param msg - incoming message
 */
void run_can_cmd(CAN_message *msg){
    uint8_t datalen = msg->length;
    uint8_t *data = msg->data;
#ifdef EBUG
    DBG("Get data: ");
    for(int i = 0; i < msg->length; ++i){
        USB_sendstr(uhex2str(msg->data[i])); USB_putbyte(' ');
    }
    //for(int i = msg->length-1; i < 8; ++i) msg->data[i] = 0;
    newline();
#endif
    if(datalen < 2){
        FORMERR(msg, ERR_WRONGLEN);
        return;
    }
    uint16_t idx = *(uint16_t*)data;
    if(idx >= CMD_AMOUNT || funclist[idx].fn == NULL){ // bad command index
        FORMERR(msg, ERR_BADCMD); return;
    }
    // check minimal length
    if(funclist[idx].datalen > datalen){
        FORMERR(msg, ERR_WRONGLEN); return;
    }
    if(datalen > 3 && (data[2] & SETTER_FLAG)){
        // check setter's length
        if(datalen != 8){ FORMERR(msg, ERR_WRONGLEN); return; }
        // check setter's values
        if(funclist[idx].maxval != funclist[idx].minval){
            int32_t newval = *(int32_t*)&data[4];
            if(newval < funclist[idx].minval || newval > funclist[idx].maxval){
                FORMERR(msg, ERR_BADVAL); return;
            }
        }
    }
    data[3] = funclist[idx].fn(msg); // set error field as result of function
    data[2] &= ~SETTER_FLAG; // and clear setter flag
#ifdef EBUG
    DBG("Return data: ");
    for(int i = 0; i < msg->length; ++i){
        USB_sendstr(uhex2str(msg->data[i])); USB_putbyte(' ');
    }
    newline();
#endif
}
