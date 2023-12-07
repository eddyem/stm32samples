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
    [SHUTTER_EXPOSE] = "exposing",
};

static const char *regstates[4] = {
    [REG_OPEN] = "open",
    [REG_CLOSE] = "close",
    [REG_OFF] = "off",
    [REG_HIZ] = "hiZ"
};

static const char *opcl[2] = {"closed", "opened"};

shutter_state shutterstate = SHUTTER_ERROR;
static shutter_state nextstate = SHUTTER_RELAX;

static uint32_t Tstart = 0, Texp = 0, Topened = 0;

/**
 * @brief changestate - open/close shutter and set next state to nxt
 * @return TRUE if success
 */
static int changestate(int open, shutter_state nxt){
    if(shutterstate != SHUTTER_RELAX && shutterstate != SHUTTER_EXPOSE) return FALSE; // don't ready
    if(open == CHKHALL()){
        //shutterstate = SHUTTER_RELAX;
        return TRUE; // already opened or closed
    }
    if(getShutterVoltage() < the_conf.workvoltage) return FALSE;
    //if(shutterstate == SHUTTER_ERROR) return FALSE;
    if(open) SHTROPEN();
    else SHTRCLOSE();
    shutterstate = SHUTTER_PROCESS;
    nextstate = nxt;
    Tstart = Tms;
    return TRUE;
}

/**
 * @brief open_shutter, close shutter - change shutter state
 * @return false if can't work due to error (no shutter) or insufficient voltage
 */
int open_shutter(){
    return changestate(1, SHUTTER_RELAX);
}

int close_shutter(){
    return changestate(0, SHUTTER_RELAX);
}

int expose_shutter(uint32_t exptime){
    if(!changestate(1, SHUTTER_EXPOSE)) return FALSE;
    Texp = exptime;
    return TRUE;
}

void process_shutter(){
    static uint32_t T = 0;
    uint32_t V = getShutterVoltage();
    switch(shutterstate){
        case SHUTTER_ERROR: // error state: no shutter? - switch HIZ/OFF each 10ms
            if(SHTRSTATE() == REG_HIZ){ // check in HiZ state: if the error still occurs?
                if(!CHKFB()){
                    shutterstate = SHUTTER_RELAX;
                    nextstate = SHUTTER_RELAX;
                }else if(Tms - T > 10){
                    T = Tms;
                    SHTROFF(); // turn off for 10ms
                }
            }else{
                if(Tms - T > 10){
                    T = Tms;
                    SHTRHIZ(); // and check error again
                }
            }
        break;
        case SHUTTER_PROCESS: // process opening or closing
#ifdef EBUG
            if(T != Tms){
                T = Tms;
                USB_sendstr(u2str(V));
                USB_putbyte('\n');
            }
#endif
            if(Tms - Tstart > the_conf.shutterrime || V < the_conf.minvoltage){
                SHTROFF();
                shutterstate = SHUTTER_WAIT;
                Tstart = Tms;
            }
        break;
        case SHUTTER_WAIT: // wait for mechanical work done
            if(Tms - Tstart > the_conf.waitingtime){
                SHTRHIZ();
                shutterstate = nextstate;
                int h = CHKHALL();
                if(h) Topened = Tms;
                else{
                    USB_sendstr("exptime=");
                    USB_sendstr(u2str(Tms - Topened - the_conf.shutterrime));
                    USB_putbyte('\n');
                }
                USB_sendstr("shutter=");
                USB_sendstr(opcl[h]);
                USB_putbyte('\n');
            }
        break;
        case SHUTTER_EXPOSE: // wait for exposition ends to close shutter
            // now Tstart is time when shutter was opened; wait until Tms - Tstart >= Texp
            if(Tms - Tstart < Texp || T == Tms) break; // once per 1ms
            T = Tms;
            if(!close_shutter()){
                if(Tms - Tstart > Texp + the_conf.waitingtime){ // try to close not more than `waitingtime` ms
                    USB_sendstr("exp=cantclose\n");
                    shutterstate = SHUTTER_ERROR;
                }
            }
        break;
        default:
            if(CHKFB()){
                shutterstate = SHUTTER_ERROR;
                T = Tms;
            }
        break;
    }
    static uint8_t oldbtnstate = 0;
    uint8_t s = CHKCCD();
    if(oldbtnstate == s) return; // button's state not changed
    // check button only when can open/close & shutter operations done
    if(V >= the_conf.workvoltage && shutterstate == SHUTTER_RELAX){ // shutter state allows to open/close
        if(s){ // pressed
            if(!CHKHALL()){ if(open_shutter()){oldbtnstate = s; /*USB_sendstr(" open, old->1\n");*/}}
            else{/*USB_sendstr("pressed when CHKHALL(), old->1\n");*/ oldbtnstate = s;}
        }else{ // released
            if(CHKHALL()){ if(close_shutter()){oldbtnstate = s; /*USB_sendstr(" close, old->0\n");*/}}
            else{/*USB_sendstr("released when !CHKHALL(), old->0\n");*/  oldbtnstate = s;}
        }
    }
}

void print_shutter_state(){
    add2buf("shutter=");
    if(shutterstate != SHUTTER_RELAX) add2buf(states[shutterstate]);
    else add2buf(opcl[CHKHALL()]);
    if(CHKHALL()){
        if(shutterstate == SHUTTER_EXPOSE){
            add2buf("\nexpfor="); add2buf(u2str(Texp));
        }
        add2buf("\nexptime=");
        add2buf(u2str(Tms - Topened));
    }
    add2buf("\nregstate=");
    add2buf(regstates[SHTRSTATE()]);
    add2buf("\nfbstate=");
    bufputchar('0' + CHKFB());
    bufputchar('\n');
}
