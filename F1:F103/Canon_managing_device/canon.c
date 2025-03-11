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
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "spi.h"
#include "usb_dev.h"

#define CU(a)   ((const uint8_t*)a)

// common state machine, init state machine
static lens_state state = LENS_DISCONNECTED;
static lensinit_state inistate = INI_ERR;
// Freal = F - Fdelta, F - F counts, Fdelta - F@0; Forig - when F was @ start; Fmax is F@1
static uint16_t Fdelta = 0, Forig = BADFOCVAL, Fmax = BADFOCVAL;

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

// command buffer
static uint8_t buf[MAXCMDLEN] = {0}, ready = 0;

// return 1 if Tms - T0 < MOVING_PAUSE
static int waitstill(uint32_t T0){
    if(Tms - T0 < MOVING_PAUSE) return 1;
    return 0;
}

/**
 * @brief canon_read - read some data
 * @param cmd
 * @param zeroz
 * @return
 */
static int canon_read(uint8_t cmd, uint8_t zeroz){
    if(zeroz > MAXCMDLEN - 1) return FALSE;
    ++zeroz;
    for(int i = 0; i < MAXCMDLEN/4; ++i) ((uint32_t*)buf)[i] = 0;
    buf[0] = cmd;
    if(SPI_transmit(buf, zeroz) != zeroz) return FALSE;
    return TRUE;
}

// get current F value by stepper pos
static uint16_t getfocval(){
    if(!canon_read(CANON_GETSTPPOS, 3)) return BADFOCVAL;
    uint16_t F = (buf[1] << 8) | buf[2];
    return (F - Fdelta);
}

typedef enum{
    F_MOVING,
    F_STOPPED,
    F_ERR
} f_movstate;

// return F_STOPPED if current F differs from old; modify old
static f_movstate Fstopped(uint16_t *oldF){
    uint16_t f = getfocval();
#ifdef EBUG
    USB_sendstr("Curpos="); USB_sendstr(u2str(f)); USB_sendstr("\n");
#endif
    if(BADFOCVAL == f) return F_ERR;
    if(*oldF == f){
        DBG("F stopped");
        return F_STOPPED;
    }
    *oldF = f;
    return F_MOVING;
}

/*
static void canon_writeu8(uint8_t cmd, uint8_t u){
    *((uint32_t*)buf) = 0;
    buf[0] = cmd; buf[1] = u;
    SPI_transmit(buf, 2);
}*/
int canon_writeu16(uint8_t cmd, uint16_t u){
    *((uint32_t*)buf) = 0;
    buf[0] = cmd; buf[1] = u >> 8; buf[2] = u & 0xff; buf[3] = 0;
    if(4 != SPI_transmit(buf, 4)) return FALSE;
    return TRUE;
}

static void canon_poll(){
    DBG("CANON_POLL");
    ready = 0;
    // wait no more than 1s
    uint32_t Tstart = Tms;
    while(Tms - Tstart < POLLING_TIMEOUT){
        IWDG->KR = IWDG_REFRESH;
        if(!canon_read(CANON_POLL, 1)){
            continue;
        }
        if(buf[1] == CANON_POLLANS){// && canon_read(CANON_LONGID, 0)){
            ready = 1;
            DBG("Ready!");
            return;
        }
    }
    // set state to error
    DBG("Not poll answer or no 0\n");
    state = LENS_ERR;
}

// turn on power and send ack
void canon_init(){
    ready = 0;
    inistate = INI_ERR;
    state = LENS_ERR;
    if(!canon_read(CANON_ID, 31)){
        DBG("Error sending starting sequence\n");
        return;
    }
    DBG("turn on power\n");
    if(!canon_read(CANON_POWERON, 1)){
        DBG("OOps\n");
        return;
    }
    canon_poll();
    if(ready){
        inistate = INI_START;
        state = LENS_INITIALIZED;
        Fdelta = 0; Forig = BADFOCVAL; Fmax = BADFOCVAL;
    }
}

/**
 * @brief canon_proc - check incoming SPI messages
 */
void canon_proc(){
    static uint32_t Tconn = 0;
    if(state == LENS_DISCONNECTED){
        if(!LENSCONNECTED()){
            Tconn = 0;
            return;
        }
        if(0 == Tconn){ // just connected
            DBG("Lens connected");
            Tconn = Tms ? Tms : 1;
            return;
        }
        if(Tms - Tconn < CONN_TIMEOUT) return;
        DBG("Connection timeout left, all OK");
        LENS_ON();
        if(the_conf.autoinit){
            canon_init();
        }else{
            state = LENS_SLEEPING; // wait until init
            ready = 1; // emulate ready flag for manual operations
        }
        return;
    }
    uint8_t OC = OVERCURRENT();
    if(!LENSCONNECTED() || OC){
        DBG("Disconnect or overcurrent");
        state = OC ? LENS_OVERCURRENT : LENS_DISCONNECTED;
        LENS_OFF();
        Tconn = 0;
        return;
    }
    if(state == LENS_ERR){
        if(0 == Tconn){
            DBG("Wait 5s till next recinit");
            Tconn = Tms ? Tms : 1;
            return;
        }
        if(Tms - Tconn < REINIT_PAUSE){
            DBG("5s left, try to reinit");
            Tconn = 0;
            canon_init();
        }
    }
    if(state != LENS_INITIALIZED) return;
    // initializing procedure: goto zero, check zeropoint and go back
    static uint8_t errctr = 0;
    if(errctr > 8){
        errctr = 0;
        inistate = INI_ERR;
    }
    static uint16_t oldF = BADFOCVAL; // old focus value (need to know that lens dont moving now)
    switch(inistate){
        case INI_START:
            DBG("INI_START");
            if(Forig != BADFOCVAL){
                if(canon_sendcmd(CANON_FMIN)){
                    ++errctr;
                    return;
                }
                inistate = INI_FGOTOZ;
                oldF = Forig;
                errctr = 0;
                Tconn = Tms;
            }else{
                Forig = getfocval(); // Fdelta == 0 -> Forig now is pure F counts
                ++errctr;
            }
        break;
        case INI_FGOTOZ: // wait until F changes stopped
            if(waitstill(Tconn)) return; // timeout
            DBG("INI_FGOTOZ");
            switch(Fstopped(&oldF)){
                case F_STOPPED:
                    Fdelta = oldF; // F@0
                    Forig -= oldF; // F in counts from 0
                    inistate = INI_FPREPMAX;
                    errctr = 0;
                break;
                case F_ERR:
                    ++errctr;
                break;
                default: // moving
                    return;
            }
        break;
        case INI_FPREPMAX:
            DBG("INI_FPREPMAX");
            if(canon_sendcmd(CANON_FMAX)){
                ++errctr;
                return;
            }
            inistate = INI_FGOTOMAX;
            errctr = 0;
            Tconn = Tms;
        break;
        case INI_FGOTOMAX:
            if(waitstill(Tconn)) return;
            DBG("INI_FGOTOMAX");
            switch(Fstopped(&oldF)){
                case F_STOPPED:
                    Fmax = oldF;
                    inistate = INI_FPREPOLD;
                    errctr = 0;
                break;
                case F_ERR:
                    ++errctr;
                break;
                default: // moving
                    return;
            }
        break;
        case INI_FPREPOLD:
            DBG("INI_FPREPOLD");
            if(!canon_writeu16(CANON_FOCMOVE, Forig - Fmax)){
                ++errctr;
            }else{
                errctr = 0;
                inistate = INI_FGOTOOLD;
                Tconn = Tms;
            }
        break;
        case INI_FGOTOOLD:
            if(waitstill(Tconn)) return;
            DBG("INI_FGOTOOLD");
            switch(Fstopped(&oldF)){
                case F_STOPPED:
                    DBG("ON position");
                    inistate = INI_READY;
                    state = LENS_READY;
                    errctr = 0;
                    canon_sendcmd(CANON_FOCBYHANDS);
                break;
                case F_ERR:
                    ++errctr;
                break;
                default: // moving
                    return;
            }
        break;
        default: // some error - change lens state to `ready` despite of errini
            DBG("ERROR in initialization -> ready without ini");
            state = LENS_READY;
    }
/*
        case CANON_GETFOCM: // don't work @EF200
            uval = (rbuf[1] << 24) | (rbuf[2] << 16) | (rbuf[3] << 8) | rbuf[4];
            USB_send("Fval="); USB_send(u2str(uval)); USB_send("\n");
        break;
        */
}

/**
 * @brief canon_diaphragm - run comands
 * @param command: open/close diaphragm by 1 step (+/-), open/close fully (o/c)
 * @return 0 if success or error code (1 - not ready, 2 - bad command, 3 - can't send data)
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
    if(!canon_writeu16(CANON_DIAPHRAGM, (uint16_t)(val << 8))) return 3;
    canon_poll();
    return 0;
}

// move focuser @ absolute position val (or show current F if val < 0)
int canon_focus(int16_t val){
    if(!ready || inistate != INI_READY) return 1;
    if(val < 0){
        USB_sendstr("Fsteps="); USB_sendstr(u2str(getfocval())); USB_sendstr("\n");
    }else{
        if(val > Fmax){
            USB_sendstr("Fmax="); USB_sendstr(u2str(Fmax)); USB_sendstr("\n");
            return 2;
        }
        uint16_t curF = getfocval();
        if(curF == BADFOCVAL){
            DBG("Unknown current F");
            return 1;
        }
        val -= curF;
        if(!canon_writeu16(CANON_FOCMOVE, val)) return 3;
    }
    canon_poll();
    return 0;
}

// send simplest command
int canon_sendcmd(uint8_t cmd){
    if(!ready || !canon_read(cmd, 1)) return 1;
    canon_poll();
    return 0;
}

// acquire 16bit value
int canon_asku16(uint8_t cmd){
    if(!ready || !canon_read(cmd, 3)) return 1;
    USB_sendstr("par=");
    USB_sendstr(u2str((buf[1] << 8) | buf[2]));
    USB_sendstr("\n");
    //canon_poll();
    return 0;
}

int canon_getinfo(){
    if(!ready || !canon_read(CANON_GETINFO, 6)) return 1;
    USB_sendstr("Info="); for(int i = 1; i < 7; ++i){
        USB_sendstr(u2hexstr(buf[i])); USB_sendstr(" ");
    }
    //canon_poll();
    USB_sendstr("\n");
    return 0;
}

uint16_t canon_getstate(){
    return state | (inistate << 8);
}
