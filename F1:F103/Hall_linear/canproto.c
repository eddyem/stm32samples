/*
 * This file is part of the hallinear project.
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
#include "canproto.h"
#include "flash.h"
#include "hardware.h"
#include "strfunc.h"

#define FIXDL(m)     do{m->length = 8;}while(0)

/*********** START of all common functions list (for `funclist`) ***********/
static errcodes ping(CAN_message _U_ *m){
    return ERR_OK; // send same message
}
// reset MCU
static errcodes reset(CAN_message _U_ *msg){
    NVIC_SystemReset();
    return ERR_OK; // never reached
}
// get/set Tms
static errcodes time_get(CAN_message *msg){
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
// vdd
static errcodes vdd(CAN_message *msg){
    FIXDL(msg);
    MSGP_SET_U32(msg, getVdd());
    return ERR_OK;
}
// get ADC value from AD0
static errcodes adcval(CAN_message *msg){
    FIXDL(msg);
    uint32_t U = getADCval(ADC_CH_0) * the_conf.Mul;
    U /= the_conf.Div;
    MSGP_SET_U32(msg, U);
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
// get ADC voltage from AD0
static errcodes adcvoltage(CAN_message *msg){
    FIXDL(msg);
    MSGP_SET_U32(msg, getADCvoltage(ADC_CH_0));
    return ERR_OK;
}
// set common CAN ID / get CAN IN in
static errcodes canid(CAN_message *msg){
    if(ISSETTER(msg->data)){
        the_conf.canID = (uint16_t)MSGP_GET_U32(msg);
        CAN_reinit(); // setup with new ID
    }
    FIXDL(msg);
    MSGP_SET_U32(msg, the_conf.canID);
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

// common uint32_t setter/getter
static errcodes u32setget(CAN_message *msg){
    uint16_t cmd = *(uint16_t*)msg->data;
    uint32_t *ptr = NULL;
    switch(cmd){
        case CMD_CANSPEED:
            ptr = &the_conf.canspeed;
            if(ISSETTER(msg->data)){
               *ptr = MSGP_GET_U32(msg);
                CAN_reinit();
                FIXDL(msg);
                MSGP_SET_U32(msg, *ptr);
                return ERR_OK;
            }
        break;
        case CMD_DIV:
            ptr = &the_conf.Div;
        break;
        case CMD_MUL:
            ptr = &the_conf.Mul;
        break;
        default: break;
    }
    if(!ptr) return ERR_CANTRUN; // unknown error
    if(ISSETTER(msg->data)){
        *ptr = MSGP_GET_U32(msg);
    }
    FIXDL(msg);
    MSGP_SET_U32(msg, *ptr);
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
    [CMD_TIME] = {time_get, 0, 0, 0},
    [CMD_MCUTEMP] = {mcut, 0, 0, 0},
    [CMD_VDD] = {vdd, 0, 0, 0},
    [CMD_ADC] = {adcval, 0, 0, 0},
    [CMD_ADCRAW] = {adcraw, 0, 0, 3}, // need parno: 0..3
    [CMD_ADCV] = {adcvoltage, 0, 0, 0},
    [CMD_CANSPEED] = {u32setget, CAN_MIN_SPEED, CAN_MAX_SPEED, 0},
    [CMD_CANID] = {canid, 1, 0x7ff, 0},
    [CMD_DIV] = {u32setget, 1, INT32_MAX, 0},
    [CMD_MUL] = {u32setget, 1, 1>>20, 0},
    [CMD_SAVECONF] = {saveconf, 0, 0, 0},
    [CMD_ERASESTOR] = {erasestor, 0, 0, 0},
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
}
