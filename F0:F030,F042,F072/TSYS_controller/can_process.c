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
#include "sensors_manage.h"
#include "version.inc"

extern volatile uint32_t Tms; // timestamp data
// id of master - all data will be sent to it
static uint16_t master_id = MASTER_ID;

static inline void sendmcut(uint8_t *data){
    uint8_t t[3];
    uint16_t T = getMCUtemp();
    t[0] = data[1]; // command itself
    t[1] = (T >> 8) & 0xff; // H
    t[2] = T & 0xff;        // L
    can_send_data(t,3);
}

static inline void senduival(){
    uint8_t buf[5];
    uint16_t *vals = getUval();
    buf[0] = CMD_GETUIVAL0; // V12 and V5
    buf[1] = vals[0] >> 8;  // H
    buf[2] = vals[0] & 0xff;// L
    buf[3] = vals[1] >> 8; // -//-
    buf[4] = vals[1] & 0xff;
    can_send_data(buf, 5);
    buf[0] = CMD_GETUIVAL1; // I12 and V3.3
    buf[1] = vals[2] >> 8;  // H
    buf[2] = vals[2] & 0xff;// L
    buf[3] = vals[3] >> 8; // -//-
    buf[4] = vals[3] & 0xff;
    can_send_data(buf, 5);
}

static inline void showui(char *v1, char *v2, uint8_t *data){
    char N = '0' + data[1];
    addtobuf(v1);
    bufputchar(N);
    bufputchar('=');
    uint16_t v = data[3]<<8 | data[4];
    printu(v);
    newline();
    addtobuf(v2);
    bufputchar(N);
    bufputchar('=');
    v = data[5]<<8 | data[6];
    printu(v);
}

void can_messages_proc(){
    CAN_message *can_mesg = CAN_messagebuf_pop();
    if(!can_mesg) return; // no data in buffer
    uint8_t len = can_mesg->length;
    IWDG->KR = IWDG_REFRESH;
#ifdef EBUG
    SEND("got message, len: "); bufputchar('0' + len);
    SEND(", data: ");
    for(int ctr = 0; ctr < len; ++ctr){
        printuhex(can_mesg->data[ctr]);
        bufputchar(' ');
    }
    newline();
#endif
    uint8_t *data = can_mesg->data, b[6];
    b[0] = data[1];
    // show received message in sniffer mode
    if(cansniffer){
        printu(Tms);
        SEND(" #");
        printuhex(can_mesg->ID);
        for(int ctr = 0; ctr < len; ++ctr){
            SEND(" ");
            printuhex(can_mesg->data[ctr]);
        }
        newline();
    }
    // don't process alien messages
    if(can_mesg->ID != CANID && can_mesg->ID != BCAST_ID) return;
    int16_t t;
    uint32_t U32;
    if(data[0] == COMMAND_MARK){   // process commands
        if(len < 2) return;
        // master shouldn't react to broadcast commands!
        if(can_mesg->ID == BCAST_ID && CANID == MASTER_ID) return;
        switch(data[1]){
            case CMD_DUMMY0:
            case CMD_DUMMY1:
                SEND("DUMMY");
                bufputchar('0' + (data[1]==CMD_DUMMY0 ? 0 : 1));
                newline();
            break;
            case CMD_PING: // pong
                can_send_data(b, 1);
            break;
            case CMD_SENSORS_STATE:
                b[1] = Sstate;
                b[2] = sens_present[0];
                b[3] = sens_present[1];
                b[4] = Nsens_present;
                b[5] = Ntemp_measured;
                can_send_data(b, 6);
            break;
            case CMD_START_MEASUREMENT:
                sensors_start();
            break;
            case CMD_START_SCAN:
                sensors_scan_mode = 1;
            break;
            case CMD_STOP_SCAN:
                sensors_scan_mode = 0;
            break;
            case CMD_SENSORS_OFF:
                sensors_off();
            break;
            case CMD_LOWEST_SPEED:
                i2c_setup(VERYLOW_SPEED);
            break;
            case CMD_LOW_SPEED:
                i2c_setup(LOW_SPEED);
            break;
            case CMD_HIGH_SPEED:
                i2c_setup(HIGH_SPEED);
            break;
            case CMD_REINIT_I2C:
                i2c_setup(CURRENT_SPEED);
            break;
            case CMD_CHANGE_MASTER_B:
                master_id = BCAST_ID;
            break;
            case CMD_CHANGE_MASTER:
                master_id = MASTER_ID;
            break;
            case CMD_GETMCUTEMP:
                sendmcut(data);
            break;
            case CMD_GETUIVAL:
                senduival();
            break;
            case CMD_REINIT_SENSORS:
                sensors_init();
            break;
            case CMD_GETBUILDNO:
                b[1] = 0;
                *((uint32_t*)&b[2]) = BUILDNO;
                can_send_data(b, 6);
            break;
            case CMD_SYSTIME:
                b[1] = 0;
                *((uint32_t*)&b[2]) = Tms;
                can_send_data(b, 6);
            break;
        }
    }else if(data[0] == DATA_MARK){ // process received data
        char Ns = '0' + data[1];
        if(len < 3) return;
        switch(data[2]){
            case CMD_PING:
                mesg("CMD_PING");
                SEND("PONG");
                bufputchar(Ns);
            break;
            case CMD_SENSORS_STATE:
                mesg("CMD_SENSORS_STATE");
                SEND("SSTATE");
                bufputchar(Ns);
                bufputchar('=');
                SEND(sensors_get_statename(data[3]));
                SEND("\nNSENS");
                bufputchar(Ns);
                bufputchar('=');
                printu(data[6]);
                SEND("\nSENSPRESENT");
                bufputchar(Ns);
                bufputchar('=');
                printu(data[4] | (data[5]<<8));
                SEND("\nNTEMP");
                bufputchar(Ns);
                bufputchar('=');
                printu(data[7]);
            break;
            case CMD_START_MEASUREMENT: // temperature
                mesg("CMD_START_MEASUREMENT");
                if(len != 6) return;
                bufputchar('T');
                bufputchar(Ns);
                bufputchar('_');
                printu(data[3]);
                bufputchar('=');
                t = data[4]<<8 | data[5];
                if(t < 0){
                    t = -t;
                    bufputchar('-');
                }
                printu(t);
            break;
            case CMD_GETMCUTEMP:
                mesg("CMD_GETMCUTEMP");
                addtobuf("TMCU");
                bufputchar(Ns);
                bufputchar('=');
                t = data[3]<<8 | data[4];
                if(t < 0){
                    bufputchar('-');
                    t = -t;
                }
                printu(t);
            break;
            case CMD_GETUIVAL0: // V12 and V5
                mesg("CMD_GETUIVAL0");
                showui("V12_", "V5_", data);
            break;
            case CMD_GETUIVAL1: // I12 and V3.3
                mesg("CMD_GETUIVAL1");
                showui("I12_", "V33_", data);
            break;
            case CMD_GETBUILDNO:
                mesg("CMD_GETBUILDNO");
                addtobuf("BUILDNO");
                bufputchar(Ns);
                bufputchar('=');
                U32 = *((uint32_t*)&data[4]);
                printu(U32);
            break;
            case CMD_SYSTIME:
                mesg("CMD_SYSTIME");
                addtobuf("SYSTIME");
                bufputchar(Ns);
                bufputchar('=');
                U32 = *((uint32_t*)&data[4]);
                printu(U32);
            break;
            default:
                SEND("UNKNOWN_DATA");
        }
        newline();
    }
}

// try to send messages, wait no more than 100ms
static CAN_status try2send(uint8_t *buf, uint8_t len, uint16_t id){
    uint32_t Tstart = Tms;
    while(Tms - Tstart < SEND_TIMEOUT_MS){
        if(CAN_OK == can_send(buf, len, id)) return CAN_OK;
        IWDG->KR = IWDG_REFRESH;
    }
    SEND("CAN_BUSY\n");
    return CAN_BUSY;
}


/**
 * Send command over CAN bus (works only if controller number is 0 - master mode)
 * @param targetID - target identifier
 * @param cmd - command to send
 */
CAN_status can_send_cmd(uint16_t targetID, uint8_t cmd){
    //if(Controller_address != 0 && cmd != CMD_DUMMY0 && cmd != CMD_DUMMY1) return CAN_NOTMASTER;
    uint8_t buf[2];
    buf[0] = COMMAND_MARK;
    buf[1] = cmd;
    return try2send(buf, 2, targetID);
}

// send data over CAN bus to MASTER_ID (not more than 6 bytes)
CAN_status can_send_data(uint8_t *data, uint8_t len){
    if(len > 6) return CAN_OK;
    uint8_t buf[8];
    buf[0] = DATA_MARK;
    buf[1] = Controller_address;
    int i;
    for(i = 0; i < len; ++i) buf[i+2] = *data++;
    return try2send(buf, len+2, master_id);
}

/**
 * send temperature data over CAN bus once per call
 * @return next number or -1 if all data sent
 */
int8_t send_temperatures(int8_t N){
    if(N < 0 || Controller_address == 0) return -1; // don't need to send Master's data over CAN bus
    int a, p;
    uint8_t can_data[4];
    int8_t retn = N;
    can_data[0] = CMD_START_MEASUREMENT;
    a = N / 10;
    p = N - a*10;
    if(p == 2){ // next sensor
        if(++a > MUL_MAX_ADDRESS) return -1;
        p = 0;
    }
    do{
        if(!(sens_present[p] & (1<<a))){
            if(p == 0) p = 1;
            else{
                p = 0;
                ++a;
            }
        } else break;
    }while(a <= MUL_MAX_ADDRESS);
    if(a > MUL_MAX_ADDRESS) return -1; // done
    retn = a*10 + p; // current temperature sensor number
    can_data[1] = a*10 + p;
    //char b[] = {'T', a+'0', p+'0', '=', '+'};
    int16_t t = Temperatures[a][p];
    if(t == BAD_TEMPERATURE || t == NO_SENSOR){ // don't send data if it's absent on current measurement
        ++retn;
    }else{
        can_data[2] = t>>8;   // H byte
        can_data[3] = t&0xff; // L byte
        if(CAN_OK == can_send_data(can_data, 4)){ // OK, calculate next address
            ++retn;
        }
    }
    return retn;
}
