/*
 * This file is part of the nitrogen project.
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

#include "BMP280.h"
#include "buttons.h"
#include "hardware.h"
#include "ili9341.h"
#include "incication.h"
#include "screen.h"
#include "strfunc.h"
#include "usb.h"

/*
 * LEDs:
 * 0 - blinks all the time
 * 1 - PWM2 state (blinks as longer as PWM larger)
 * 2 - PWM3 state (-//-)
 * 3 - ?
 */
// next state change time
static uint32_t ledT[LEDS_AMOUNT] = {0};
// arrays of high and low states' length
static uint32_t ledH[LEDS_AMOUNT] = {199, 0, 0, 0};
static uint32_t ledL[LEDS_AMOUNT] = {799, 1, 1, 1};

// led blinking
TRUE_INLINE void leds_proc(){
    uint32_t v = getPWM(2);
    ledH[1] = v*5; ledL[1] = (PWM_CCR_MAX - v) * 5;
    v = getPWM(3);
    ledH[2] = v*5; ledL[2] = (PWM_CCR_MAX - v) * 5;
    for(int i = 0; i < LEDS_AMOUNT; ++i){
        int state = LED_get(i);
        if(state){ // shining
            if(ledH[i] == 0) LED_off(i); // don't turn it on
            else if(ledL[i] && Tms > ledT[i]){
                LED_off(i);
                ledT[i] = Tms + ledL[i];
            }
        }else{
            if(ledL[i] == 0) LED_on(i); // don't turn it off
            else if(ledH[i] && Tms > ledT[i]){
                LED_on(i);
                ledT[i] = Tms + ledH[i];
            }
        }
    }
}

// Display state: main window or menu
typedef enum{
    DISP_MAINWIN,
    DISP_MENU
} display_state;

static display_state dispstate = DISP_MAINWIN;
static uint32_t lastTmeas = 0; // last measurement time

static void cls(){ // set default colors (bg=0, fg=0xffff) and clear screen
    setBGcolor(0);
    setFGcolor(0xffff);
    ClearScreen();
}

static void refresh_mainwin(){ // ask all parameters and refresh main window with new values
    DBG("REFRESH main window");
    cls();
    float T, P, H;
    BMP280_status s = BMP280_get_status();
    if(s == BMP280_NOTINIT || s == BMP280_ERR) BMP280_init();
    SetFontScale(1); // small menu items labels
    setBGcolor(COLOR_BLACK); setFGcolor(COLOR_LIGHTGREEN);
    PutStringAt(4, 16, "Temperature Pressure Humidity Dew point");
    int y = 37;
    uint16_t fgcolor;
    if(s == BMP280_RDY && BMP280_getdata(&T, &P, &H)){ // show data
        if(T < T_MIN || T > T_MAX) fgcolor = COLOR_RED;
        else if(T < 0) fgcolor = COLOR_BLUE;
        else fgcolor = COLOR_GREEN;
        setFGcolor(fgcolor); PutStringAt(32, y, float2str(T, 2));
        if(P > P_MAX) fgcolor = COLOR_RED;
        else fgcolor = COLOR_YELLOW;
        setFGcolor(fgcolor); PutStringAt(112, y, float2str(P, 1));
        if(H > H_MAX) fgcolor = COLOR_RED;
        else fgcolor = COLOR_CHOCOLATE;
        setFGcolor(fgcolor); PutStringAt(192, y, float2str(H, 1));
        float dew = Tdew(T, H);
        if(T - dew < DEW_MIN) fgcolor = COLOR_RED;
        else fgcolor = COLOR_LIGHTBLUE;
        setFGcolor(fgcolor); PutStringAt(248, y, float2str(dew, 1));
#ifdef EBUG
        USB_sendstr("T="); USB_sendstr(float2str(T, 2)); USB_sendstr("\nP=");
        USB_sendstr(float2str(P, 1));
        P *= 0.00750062f; USB_sendstr("\nPmm="); USB_sendstr(float2str(P, 1));
        USB_sendstr("\nH="); USB_sendstr(float2str(H, 1));
        USB_sendstr("\nTdew="); USB_sendstr(float2str(dew, 1));
        newline();
#endif
    }else{ // show "errr"
        setBGcolor(COLOR_RED); setFGcolor(COLOR_CYAN);
        CenterStringAt(y, "No signal");
    }
    // display all other data
    SetFontScale(3);
    // TODO: show current level
    setFGcolor(COLOR_RED); CenterStringAt(130, "Level: NULL");
    if(getPWM(2) || getPWM(3)){
        setFGcolor(COLOR_GREEN);
        CenterStringAt(220, "Processing");
    }
    UpdateScreen(0, SCRNH-1);
    if(!BMP280_start()) BMP280_init(); // start new measurement
}

static void refresh_menu(){ // refresh menu with changed selection
    DBG("REFRESH menu");
    cls();
}

/*
 * Custom keys:
 * 0 - main screen
 * 1 - up
 * 2 - down
 * 3 - select/menu
 */
TRUE_INLINE void btns_proc(){
    uint8_t evtmask = 0; // bitmask for active buttons (==1)
    for(int i = 0; i < BTNSNO; ++i){
        keyevent evt = keystate(i, NULL); // T may be used for doubleclick detection
        if(evt == EVT_PRESS || evt == EVT_HOLD) evtmask |= 1<<i;
    }
    // now check all buttons
    if(evtmask & 1<<0){ // escape to main window or force refresh
        if(dispstate == DISP_MENU){
            dispstate = DISP_MAINWIN;
        }
        lastTmeas = Tms - SENSORS_DATA_TIMEOUT*2; // force refresh
    }
    if(dispstate == DISP_MENU){ // buttons 'up'/'down' works only in menu mode
        if(evtmask & 1<<1){ // up
            ;
        }
        if(evtmask & 1<<2){ // down
            ;
        }
    }
    if(evtmask & 1<<3){ // select/menu
        if(dispstate == DISP_MAINWIN){ // switch to menu mode
            dispstate = DISP_MENU;
            refresh_menu();
        }else{ // select
            ;
        }
    }
}

void indication_process(){
    if(!LEDsON) return;
    leds_proc();
    btns_proc();
    switch(dispstate){
        case DISP_MAINWIN:
            if(Tms - lastTmeas > SENSORS_DATA_TIMEOUT){
                refresh_mainwin();
                lastTmeas = Tms;
            }
        break;
        case DISP_MENU: // do nothing
            ;
        break;
    }
}
