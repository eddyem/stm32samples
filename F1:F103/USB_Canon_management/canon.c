/*
 * This file is part of the canonmanage project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "canon.h"
#include "hardware.h"
#include "proto.h"
#include "spi.h"
#include "usb.h"

#define CU(a)   ((const uint8_t*)a)

#if 0
typedef struct{
    canon_commands cmd; // command code
    int zeros;          // amount of zeros after command
} command;

static command commands[] = {
    {CANON_LONGID, 11},
    {CANON_ID, 6},
    {CANON_REPDIA, 1},
    {CANON_FSTOP, 1},
    {CANON_FMIN, 1},
    {CANON_FMAX, 1},
    {CANON_POWERON, 1},
    {CANON_POWEROFF, 1},
    {CANON_POLL, 1},
    {CANON_DIAPHRAGM, 2},
    {CANON_FOCMOVE, 2},
    {CANON_FOCBYHANDS, 1},
    {CANON_GETINFO, 15},
    {CANON_GETREG, 2},
    {CANON_GETFOCLIM, 2},
    {CANON_GETDIAL, 2},
    {CANON_GETFOCM, 4},
};
#endif

// command buffer (buf[0] == last command sent)
static uint8_t buf[SPIBUFSZ] = {0}, ready = 0;

static void canon_read(uint8_t cmd, uint8_t zeroz){
    if(zeroz > MAXCMDLEN - 1) return;
    ++zeroz;
    *((uint32_t*)buf) = 0;
    buf[0] = cmd;
    SPI_transmit(buf, zeroz);
}/*
static void canon_writeu8(uint8_t cmd, uint8_t u){
    *((uint32_t*)buf) = 0;
    buf[0] = cmd; buf[1] = u;
    SPI_transmit(buf, 2);
}*/
static void canon_writeu16(uint8_t cmd, uint16_t u){
    *((uint32_t*)buf) = 0;
    buf[0] = cmd; buf[1] = u >> 8; buf[2] = u & 0xff;
    SPI_transmit(buf, 3);
}

static void canon_poll(){
    ready = 0;
    canon_read(CANON_POLL, 0);
}

// turn on power and send ack
void canon_init(){
    ready = 0;
    canon_read(CANON_ID, 31);
}

// send over USB 16-bit unsigned
static void printu16(uint8_t *b){
    USB_send(u2str((b[1] << 8) | b[2]));
}

/**
 * @brief canon_proc - check incoming SPI messages
 */
void canon_proc(){
    uint8_t lastcmd = buf[0]; // last command sent
    uint32_t uval;
    uint8_t x;
    uint8_t *rbuf = SPI_receive(&x);
    if(!rbuf) return;
#ifdef EBUG
    //if(lastcmd != CANON_POLL){
        USB_send("SPI receive: ");
        for(uint8_t i = 0; i < x; ++i){
            if(i) USB_send(", ");
            USB_send(u2hexstr(rbuf[i]));
        }
        USB_send("\n");
    //}
#endif
    int need2poll = 0;
    switch (lastcmd){
        case CANON_LONGID: // something
//            need2poll = 0;
        break;
        case CANON_ID: // got ID -> turn on power
            canon_read(CANON_POWERON, 1);
        break;
        /*case CANON_POWERON:
            ;
        break;*/
        case CANON_POLL:
            if(rbuf[0] == CANON_POLLANS){
                canon_read(CANON_LONGID, 0);
                ready = 1;
#ifdef EBUG
                USB_send("Ready!\n");
#endif
            }else need2poll = 1;
        break;
        case CANON_GETINFO:
            USB_send("Info="); for(int i = 1; i < 7; ++i){
                USB_send(u2hexstr(rbuf[i])); USB_send(" ");
            }
            USB_send("\n");
        break;
        case CANON_GETREG:
            USB_send("Reg="); USB_send(u2hexstr((rbuf[1] << 8) | rbuf[2])); USB_send("\n");
        break;
        case CANON_GETMODEL:
            USB_send("Lens="); printu16(rbuf); USB_send("\n");
        break;
        case CANON_GETDIAL:
            USB_send("Fsteps="); printu16(rbuf); USB_send("\n");
            //canon_read(CANON_GETFOCM, 4);
        break;
        case CANON_GETFOCM: // don't work @EF200
            uval = (rbuf[1] << 24) | (rbuf[2] << 16) | (rbuf[3] << 8) | rbuf[4];
            USB_send("Fval="); USB_send(u2str(uval)); USB_send("\n");
        break;
        default:
            need2poll = 1; // poll after any other command
        break;
    }
    if(need2poll) canon_poll();
}

/**
 * @brief canon_diaphragm - run comands
 * @param command: open/close diaphragm by 1 step (+/-), open/close fully (o/c)
 * @return 0 if success or error code (1 - not ready, 2 - bad command)
 */
int canon_diaphragm(char command){
    if(!ready) return 1;
    int16_t val = 0;
    switch(command){
        case '+':
            val = -1;
        break;
        case '-':
            val = 1;
        break;
        case 'o':
        case 'O':
            val = 128;
        break;
        case 'c':
        case 'C':
            val = 127;
        break;
        default:
            return 2; // unknown command
    }
    canon_writeu16(CANON_DIAPHRAGM, (uint16_t)(val << 8));
    return 0;
}

int canon_focus(int16_t val){
    if(!ready) return 1;
    if(val == 0) canon_read(CANON_GETDIAL, 2);
    else canon_writeu16(CANON_FOCMOVE, val);
    return 0;
}

int canon_sendcmd(uint8_t cmd){
    if(!ready) return 1;
    canon_read(cmd, 0);
    return 0;
}

void canon_setlastcmd(uint8_t x){
    buf[0] = x;
}

// acquire 16bit value
int canon_asku16(uint8_t cmd){
    if(!ready) return 1;
    canon_read(cmd, 2);
    return 0;
}

int canon_getinfo(){
    if(!ready) return 1;
    canon_read(CANON_GETINFO, 6);
    return 0;
}
