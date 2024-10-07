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
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "shutter.h"
#include "usb.h"

static const char *states[SHUTTER_STATE_AMOUNT] = {
    [SHUTTER_ERROR] = "error",
    [SHUTTER_OPENED] = "opened",
    [SHUTTER_CLOSED] = "closed",
    [SHUTTER_PROCESS] = "process",
    [SHUTTER_EXPOSE] = "exposing",
};

// hall states
static const char *opcl[2] = {"closed", "opened"};

shutter_state shutterstate = SHUTTER_CLOSED;
static shutter_state nextstate = SHUTTER_CLOSED;

static uint32_t Tstart = 0, Texp = 0, Topened = 0;

/**
 * @brief changestate - open/close shutter and set next state to nxt
 * @return TRUE if success
 */
static int changestate(shutter_state state){
    int hall = CHKHALL();
    // check current state: change only if `state`==`shutterstate` but real position differs
    if(state == SHUTTER_CLOSED && shutterstate == state){
        DBG("Close -> close");
        if(!the_conf.chkhall) return TRUE;
        else if(hall == 0) return TRUE;
    }
    if(state == SHUTTER_OPENED && shutterstate == state){
        DBG("Open->open");
        if(!the_conf.chkhall) return TRUE;
        else if(hall == 1) return TRUE;
    }
    // wait while exposition ends or close shutter manually
    if(state == SHUTTER_EXPOSE && shutterstate == state){
        DBG("Exposed!");
        return FALSE;
    }
    if(getShutterVoltage() < the_conf.workvoltage){
        DBG("Undervoltage!");
        return FALSE; // insufficient voltage
    }
    if(state == SHUTTER_EXPOSE || state == SHUTTER_OPENED){
        DBG("Start opening");
        set_pwm(100);
    }else if(state == SHUTTER_CLOSED){
        DBG("Close");
        set_pwm(0);
    }else{
        shutterstate = state;
        return TRUE;
    }
    shutterstate = SHUTTER_PROCESS;
    nextstate = state;
    Tstart = Tms;
    return TRUE;
}

/**
 * @brief open_shutter, close shutter - change shutter state
 * @return false if can't work due to error (no shutter) or insufficient voltage
 */
int open_shutter(){
    return changestate(SHUTTER_OPENED);
}

int close_shutter(){
    return changestate(SHUTTER_CLOSED);
}

int expose_shutter(uint32_t exptime){
    if(!changestate(SHUTTER_EXPOSE)) return FALSE;
    Texp = exptime;
    return TRUE;
}

void process_shutter(){
    static uint32_t T = 0;
    uint32_t V = getShutterVoltage();
    int hall = CHKHALL();
    if(V < the_conf.workvoltage){
        DBG("Undervoltage -> err");
        shutterstate = SHUTTER_ERROR;
        return;
    }
    switch(shutterstate){
        case SHUTTER_ERROR: // error state: undervoltage or wrong hall state
            changestate(nextstate);
            return;
        break;
        case SHUTTER_PROCESS: // process opening or closing: set holding voltage when opene
            if(the_conf.chkhall){ // what if hall is active?
                switch(nextstate){
                    case SHUTTER_OPENED:
                    case SHUTTER_EXPOSE:
                        if(hall){ // set holding voltage
                            set_pwm(the_conf.holdingPWM);
                            shutterstate = nextstate;
                            Topened = Tstart = Tms;
                            print_shutter_state();
                            return;
                        }
                    break;
                    case SHUTTER_CLOSED:
                        if(!hall){
                            shutterstate = nextstate;
                            print_shutter_state();
                            return;
                        }
                    break;
                    default: // impossible state
                        nextstate = SHUTTER_CLOSED;
                        shutterstate = SHUTTER_ERROR;
                        print_shutter_state();
                        return;
                }
            }
            if(Tms - Tstart >= the_conf.shuttertime){
                DBG("time");
                if(the_conf.chkhall && nextstate != SHUTTER_CLOSED){ // error when open
                    nextstate = SHUTTER_CLOSED;
                    shutterstate = SHUTTER_ERROR;
                    print_shutter_state();
                    return;
                }
                if(nextstate == SHUTTER_OPENED || nextstate == SHUTTER_EXPOSE){
                    DBG("Holding PWM");
                    set_pwm(the_conf.holdingPWM);
                    Topened = Tms;
                }
                shutterstate = nextstate;
                Tstart = Tms;
                print_shutter_state();
            }
        break;
        case SHUTTER_EXPOSE: // wait for exposition ends to close shutter
            // now Tstart is time when shutter was opened; wait until Tms - Tstart >= Texp
            if(Tms - Tstart < Texp || T == Tms) break; // once per 1ms
            T = Tms;
            if(!close_shutter()){
                if(Tms - Tstart >= Texp + the_conf.shuttertime){ // try to close not more than `waitingtime` ms
                    USB_sendstr("exp=cantclose\n");
                    shutterstate = SHUTTER_ERROR;
                    nextstate = SHUTTER_CLOSED;
                }
            }else{ DBG("Exp end -> close"); }
        break;
        default:
            if(the_conf.chkhall){
                if(hall){
                    if(shutterstate == SHUTTER_CLOSED) shutterstate = SHUTTER_ERROR;
                }else if(shutterstate == SHUTTER_OPENED){
                    USB_sendstr("exp=errclosed\n");
                    shutterstate = SHUTTER_ERROR;
                    nextstate = SHUTTER_CLOSED;
                }
            }
        break;
    }
    if(shutterstate == SHUTTER_ERROR) return;
    static uint8_t oldbtnstate = 0;
    uint8_t s = CHKCCD();
    if(oldbtnstate == s) return; // button's state not changed
    // check button only when can open/close & shutter operations done
    if(s){ // CCD active - open shutter
        if(!open_shutter()){ oldbtnstate = 0; return; }
    }else{ // close
        if(!close_shutter()){ oldbtnstate = 1; return; }
    }
    oldbtnstate = s;
}

void print_shutter_state(){
    USB_sendstr("shutter=");
    USB_sendstr(states[shutterstate]);
    USB_putbyte('\n');
    if(the_conf.chkhall){
        USB_sendstr("sensor=");
        USB_sendstr(opcl[CHKHALL()]);
        USB_putbyte('\n');
    }
    if(shutterstate == SHUTTER_EXPOSE){
        USB_sendstr("expfor=");
        USB_sendstr(u2str(Texp));
        USB_putbyte('\n');
    }
    if(Tms - Topened > the_conf.shuttertime){
        USB_sendstr("exptime=");
        USB_sendstr(u2str(Tms - Topened - the_conf.shuttertime));
        USB_putbyte('\n');
    }
}
