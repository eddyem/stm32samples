/*
 * This file is part of the shutter project.
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

#include "adc.h"
#include "hardware.h"
#include "proto.h"
#include "shutter.h"
#include "usb.h"

static const char *states[SHUTTER_STATE_AMOUNT] = {
    [SHUTTER_ERROR] = "error",
    [SHUTTER_RELAX] = "relax",
    [SHUTTER_PROCESS] = "process",
    [SHUTTER_WAIT] = "wait",
};

static const char *opcl[2] = {"closed", "opened"};

shutter_state shutterstate = SHUTTER_ERROR;

static uint32_t Tstart = 0;

static int changestate(int open){
    if(open == CHKHALL()) return TRUE; // already opened or closed
    if(getADCvoltage(CHSHTR) < SHTR_WORK_VOLTAGE / SHTRVMUL) return FALSE;
    if(shutterstate == SHUTTER_ERROR) return FALSE;
    if(open) SHTROPEN();
    else SHTRCLOSE();
    shutterstate = SHUTTER_PROCESS;
    Tstart = Tms;
    return TRUE;
}

/**
 * @brief open_shutter, close shutter - change shutter state
 * @return false if can't work due to error (no shutter) or insufficient voltage
 */
int open_shutter(){
    return changestate(1);
}

int close_shutter(){
    return changestate(0);
}

void process_shutter(){
#ifdef EBUG
    static uint32_t T = 0;
#endif
    uint32_t V = getADCvoltage(CHSHTR)*SHTRVMUL;
    switch(shutterstate){
        case SHUTTER_ERROR:
            SHTROFF();
            shutterstate = SHUTTER_WAIT;
            Tstart = Tms;
        break;
        case SHUTTER_PROCESS:
#ifdef EBUG
            if(T != Tms){
                T = Tms;
                USB_sendstr(u2str(V));
                USB_putbyte('\n');
            }
#endif
            if(Tms - Tstart > SHUTTER_TIME || V < SHTR_MIN_VOLTAGE){
                SHTROFF();
                shutterstate = SHUTTER_WAIT;
                Tstart = Tms;
            }
        break;
        case SHUTTER_WAIT:
            if(Tms - Tstart > WAITING_TIME){
                SHTRHIZ();
                shutterstate = SHUTTER_RELAX;
                USB_sendstr("shutter=");
                USB_sendstr(opcl[CHKHALL()]);
                USB_putbyte('\n');
            }
        break;
        default:
            if(CHKFB()) shutterstate = SHUTTER_ERROR;
        break;
    }
    static uint8_t oldbtnstate = 0;
    uint8_t s = CHKCCD();
    if(oldbtnstate == s) return; // button's state not changed
    // check button only when can open/close & shutter operations done
    if(V >= SHTR_WORK_VOLTAGE && shutterstate == SHUTTER_RELAX){ // shutter state allows to open/close
        if(s){ // pressed
            if(!CHKHALL()){ if(open_shutter()){oldbtnstate = s; USB_sendstr(" open, old->1\n");}}
            else{USB_sendstr("pressed when CHKHALL(), old->1\n"); oldbtnstate = s;}
        }else{ // released
            if(CHKHALL()){ if(close_shutter()){oldbtnstate = s; USB_sendstr(" close, old->0\n");}}
            else{USB_sendstr("released when !CHKHALL(), old->0\n");  oldbtnstate = s;}
        }
    }
}

void print_shutter_state(){
    add2buf("shutter=");
    if(shutterstate != SHUTTER_RELAX) add2buf(states[shutterstate]);
    else add2buf(opcl[CHKHALL()]);
    add2buf("\nregstate=");
    bufputchar('0' + SHTRSTATE());
    add2buf("\nfbstate=");
    bufputchar('0' + CHKFB());
    bufputchar('\n');
}
