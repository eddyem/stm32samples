/*
 *                                                                                                  geany_encoding=koi8-r
 * can_process.c
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "adc.h"
#include "can.h"
#include "can_process.h"
#include "proto.h"

extern volatile uint32_t Tms; // timestamp data

// v==0 - send V12 & V5
static void senduival(){
    uint8_t buf[5];
    uint16_t *vals = getUval();
    buf[0] = CMD_GETUVAL; // V12 and V5
    buf[1] = vals[0] >> 8;  // H
    buf[2] = vals[0] & 0xff;// L
    buf[3] = vals[1] >> 8; // -//-
    buf[4] = vals[1] & 0xff;
    SEND_CAN(buf, 5);
}

static void sendu16(uint8_t cmd, uint16_t data){
    uint8_t buf[3];
    buf[0] = cmd;
    buf[1] = data >> 8;
    buf[2] = data & 0xff;
    SEND_CAN(buf, 3);
}

void can_messages_proc(){
    CAN_message *can_mesg = CAN_messagebuf_pop();
    if(!can_mesg) return; // no data in buffer
    uint8_t len = can_mesg->length;
#ifndef EBUG
    if(can_mesg->fifoNum == 1){ // not my data - just show it
#endif
        if(monitCAN){
            printu(Tms);
            SEND(" #");
            printuhex(can_mesg->ID);
            SEND(" (F#"); printu(can_mesg->fifoNum); SEND(")");
            for(uint8_t ctr = 0; ctr < len; ++ctr){
                SEND(" ");
                printuhex(can_mesg->data[ctr]);
            }
            IWDG->KR = IWDG_REFRESH;
            newline(); sendbuf();
        }
#ifndef EBUG
        return;
    }
#endif
    IWDG->KR = IWDG_REFRESH;
    if(!len) return; // no data in message
    uint8_t *data = can_mesg->data;
    switch(data[0]){
        case CMD_PING: // pong
            SEND_CAN(data, 1);
        break;
        case CMD_GETMCUTEMP:
            sendu16(CMD_GETMCUTEMP, (int16_t)getMCUtemp());
        break;
        case CMD_GETUVAL:
            senduival();
        break;
        case CMD_GETU3V3:
            sendu16(CMD_GETU3V3, (uint16_t)getVdd());
        break;
    }
}

// try to send messages, wait no more than 100ms
CAN_status try2send(uint8_t *buf, uint8_t len, uint16_t id){
    uint32_t Tstart = Tms;
    while(Tms - Tstart < SEND_TIMEOUT_MS){
        if(CAN_OK == can_send(buf, len, id)) return CAN_OK;
        IWDG->KR = IWDG_REFRESH;
    }
    SEND("CAN_BUSY\n");
    return CAN_BUSY;
}
