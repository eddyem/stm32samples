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

#include "adc.h"
#include "canproto.h"
#include "flash.h"
#include "hardware.h"
#include "modbusrtu.h"
#include "strfunc.h"
#include "usart.h"

#define FIXDL(m)     do{m->length = 8;}while(0)

/*********** START of all common functions list (for `funclist`) ***********/
static errcodes ping(CAN_message _U_ *m){
    return ERR_OK; // send same message
}
// reset MCU
static errcodes reset(CAN_message _U_ *msg){
    usart_send("Soft reset\n");
    usart_transmit();
    NVIC_SystemReset();
    return ERR_OK; // never reached
}
// get/set Tms
static errcodes time_getset(CAN_message *msg){
    if(ISSETTER(msg->data)){
        Tms = MSGP_GET_U32(msg);
    }
    FIXDL(msg);
    MSGP_SET_U32(msg, Tms);
    return ERR_OK;
}
// get MCU T
static errcodes mcut(CAN_message *msg){
    FIXDL(msg);
    MSGP_SET_U32(msg, getMCUtemp());
    return ERR_OK;
}
// get ADC raw values
static errcodes adcraw(CAN_message *msg){
    FIXDL(msg);
    uint8_t no = PARVAL(msg->data);
    if(no >= ADC_CHANNELS) return ERR_BADPAR;
    MSGP_SET_U32(msg, getADCval(no));
    return ERR_OK;
}
// set common CAN ID / get CAN IN in
static errcodes canid(CAN_message *msg){
    if(ISSETTER(msg->data)){
        the_conf.CANIDin = the_conf.CANIDout = (uint16_t)MSGP_GET_U32(msg);
        CAN_reinit(0); // setup with new ID
    }
    FIXDL(msg);
    MSGP_SET_U32(msg, the_conf.CANIDin);
    return ERR_OK;
}
// get/set input CAN ID
static errcodes canidin(CAN_message *msg){
    if(ISSETTER(msg->data)){
        the_conf.CANIDin = (uint16_t)MSGP_GET_U32(msg);
        CAN_reinit(0); // setup with new ID
    }
    FIXDL(msg);
    MSGP_SET_U32(msg, the_conf.CANIDin);
    return ERR_OK;
}
// get/set output CAN ID
static errcodes canidout(CAN_message *msg){
    if(ISSETTER(msg->data)){
        the_conf.CANIDout = (uint16_t)MSGP_GET_U32(msg);
    }
    FIXDL(msg);
    MSGP_SET_U32(msg, the_conf.CANIDout);
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
    uint8_t no = OUTMAX+1;
    if(msg->length > 2){
        uint8_t chnl = PARVAL(msg->data);
        if(chnl != NO_PARNO) no = chnl;
    }
    if(ISSETTER(msg->data)){
        if(set_relay(no, MSGP_GET_U32(msg)) < 0) return ERR_BADPAR;
    }
    FIXDL(msg);
    int rval = get_relay(no);
    if(rval < 0) return ERR_BADPAR;
    MSGP_SET_U32(msg, (uint32_t)rval);
    return ERR_OK;
}
// get current ESW status
static errcodes esw(CAN_message *msg){
    uint8_t no = INMAX+1;
    if(msg->length > 2){
        uint8_t chnl = PARVAL(msg->data);
        if(chnl != NO_PARNO) no = chnl;
    }
    int val = get_esw(no);
    if(val < 0) return ERR_BADPAR;
    MSGP_SET_U32(msg, (uint32_t)val);
    FIXDL(msg);
    return ERR_OK;
}
// bounce-free ESW get status
static errcodes eswg(CAN_message *msg){
    uint8_t no = INMAX+1;
    if(msg->length > 2){
        uint8_t chnl = PARVAL(msg->data);
        if(chnl != NO_PARNO) no = chnl;
    }
    uint32_t curval = get_ab_esw();
    if(no > INMAX) MSGP_SET_U32(msg, curval);
    else MSGP_SET_U32(msg, (curval & (1<<no)) ? 0 : 1);
    FIXDL(msg);
    return ERR_OK;
}
// onboard LED
static errcodes led(CAN_message *m){
    if(m->length > 4 && ISSETTER(m->data)) LED(m->data[4]);
    m->data[4] = LED(-1);
    FIXDL(m);
    return ERR_OK;
}

// common uint32_t setter/getter
static errcodes u32setget(CAN_message *msg){
    uint16_t cmd = *(uint16_t*)msg->data;
    uint32_t *ptr = NULL, val;
    switch(cmd){
        case CMD_CANSPEED: ptr = &the_conf.CANspeed; CAN_reinit(MSGP_GET_U32(msg)); break;
        case CMD_BOUNCE: ptr = &the_conf.bouncetime; break;
        case CMD_USARTSPEED: ptr = &the_conf.usartspeed; break;
        case CMD_INCHNLS: val = inchannels(); ptr = &val; break;
        case CMD_OUTCHNLS: val = outchannels(); ptr = &val; break;
        case CMD_MODBUSID: ptr = &the_conf.modbusID; break;
        case CMD_MODBUSIDOUT: ptr = &the_conf.modbusIDout; break;
        case CMD_MODBUSSPEED: ptr = &the_conf.modbusspeed; break;
    default: break;
    }
    if(!ptr) return ERR_CANTRUN; // unknown error
    if(ISSETTER(msg->data)){
        if(cmd == CMD_INCHNLS || cmd == CMD_OUTCHNLS) return ERR_CANTRUN; // can't set getter-only
        *ptr = MSGP_GET_U32(msg);
    }
    FIXDL(msg);
    MSGP_SET_U32(msg, *ptr);
    return ERR_OK;
}

// common bitflag setter/getter
// without parno - all flags, with - flag with number N
static errcodes flagsetget(CAN_message *msg){
    uint8_t idx = NO_PARNO;
    if(msg->length > 2){
        idx = PARVAL(msg->data);
        if(idx != NO_PARNO && idx > MAX_FLAG_BITNO) return ERR_BADPAR;
    }
    if(ISSETTER(msg->data)){
        uint32_t val = MSGP_GET_U32(msg);
        if(idx == NO_PARNO) the_conf.flags.u32 = val; // all bits
        else{ // only selected
            if(val) the_conf.flags.u32 |= 1 << idx;
            else the_conf.flags.u32 &= ~(1 << idx);
        }
    }
    FIXDL(msg);
    if(idx == NO_PARNO) MSGP_SET_U32(msg, the_conf.flags.u32);
    else MSGP_SET_U32(msg, (the_conf.flags.u32 & (1<<idx)) ? 1:0);
    return ERR_OK;
}
/************ END of all common functions list (for `funclist`) ************/

typedef struct{
    errcodes (*fn)(CAN_message *msg); // function to run with can packet `msg`
    int32_t minval;  // minimal/maximal values of *(int32_t*)(&data[4]) - if minval != maxval
    int32_t maxval;
    uint8_t datalen; // minimal data length (full CAN packet, bytes)
} commonfunction;

// list of common (CAN/RS-232) functions
// !!!!!!!!! Getters should set message length to 8 !!!!!!!!!!!
static const commonfunction funclist[CMD_AMOUNT] = {
    [CMD_PING] = {ping, 0, 0, 0},
    [CMD_RESET] = {reset, 0, 0, 0},
    [CMD_TIME] = {time_getset, 0, 0, 0},
    [CMD_MCUTEMP] = {mcut, 0, 0, 0},
    [CMD_ADCRAW] = {adcraw, 0, 0, 3}, // need parno: 0..4
    [CMD_CANSPEED] = {u32setget, CAN_MIN_SPEED, CAN_MAX_SPEED, 0},
    [CMD_CANID] = {canid, 1, 0x7ff, 0},
    [CMD_CANIDin] = {canidin, 1, 0x7ff, 0},
    [CMD_CANIDout] = {canidout, 1, 0x7ff, 0},
    [CMD_SAVECONF] = {saveconf, 0, 0, 0},
    [CMD_ERASESTOR] = {erasestor, 0, 0, 0},
    [CMD_RELAY] = {relay, 0, 0, 0},
    [CMD_GETESW] = {eswg, 0, 0, 0},
    [CMD_GETESWNOW] = {esw, 0, 0, 0},
    [CMD_BOUNCE] = {u32setget, 0, 1000, 0},
    [CMD_USARTSPEED] = {u32setget, 1200, 3000000, 0},
    [CMD_LED] = {led, 0, 0, 0},
    [CMD_FLAGS] = {flagsetget, 0, 0, 0},
    [CMD_INCHNLS] = {u32setget, 0, 0, 0},
    [CMD_OUTCHNLS] = {u32setget, 0, 0, 0},
    [CMD_MODBUSID] = {u32setget, 0, MODBUS_MAX_ID, 0}, // 0 - master!
    [CMD_MODBUSIDOUT] = {u32setget, 0, MODBUS_MAX_ID, 0},
    [CMD_MODBUSSPEED] = {u32setget, 1200, 115200, 0},
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
        usart_send(uhex2str(msg->data[i])); usart_putchar(' ');
    }
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
        usart_send(uhex2str(msg->data[i])); usart_putchar(' ');
    }
    newline();
#endif
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
void parseCANcommand(CAN_message *msg){
    // check PING
    if(msg->length != 0) run_can_cmd(msg);
    uint32_t Tstart = Tms;
    // send answer
    while(Tms - Tstart < SEND_TIMEOUT_MS){
        if(CAN_OK == CAN_send(msg)) return;
        IWDG->KR = IWDG_REFRESH;
    }
    usart_send("error=canbusy\n");
}
