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
#include "fonts.h"
#include "hardware.h"
#include "ili9341.h"
#include "indication.h"
#include "menu.h"
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

static void refresh_mainwin(btnevtmask evtmask);
static void refresh_menu(btnevtmask evtmask);

// current menu
static menu *curmenu = &mainmenu;
// window handler
window_handler swhandler = refresh_mainwin;

// Display state: window or menu
typedef enum{
    DISP_WINDOW,    // show window
    DISP_MENU       // show menu
} display_state;

static display_state dispstate = DISP_WINDOW;
static uint32_t lastTupd = 0; // last main window update time

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

static void refresh_mainwin(btnevtmask evtmask){ // ask all parameters and refresh main window with new values
    if(BTN_PRESS(evtmask, BTN_ESC_MASK)){ // force refresh
        Sens_measured_time = Tms - SENSORS_DATA_TIMEOUT*2; // force refresh
        lastTupd = Tms;
        return; // just start measurements
    }
    if(BTN_PRESS(evtmask, BTN_SEL_MASK)){ // switch to menu
        init_menu(&mainmenu);
        return;
    }
    if(evtmask) return; // left/right buttons press, any hold - do nothing
    cls();
    SetFontScale(1); // small menu items labels
    setBGcolor(COLOR_BLACK); setFGcolor(COLOR_LIGHTGREEN);
    PutStringAt(4, 16, "Temperature Pressure Humidity Dew point");
    int y = 37;
    uint16_t fgcolor;
    if(Tms - Sens_measured_time < 2*SENSORS_DATA_TIMEOUT){ // data was recently refreshed
        if(Temperature < T_MIN || Temperature > T_MAX) fgcolor = COLOR_RED;
        else if(Temperature < 0) fgcolor = COLOR_BLUE;
        else fgcolor = COLOR_GREEN;
        setFGcolor(fgcolor); PutStringAt(32, y, float2str(Temperature, 2));
        if(Pressure > P_MAX) fgcolor = COLOR_RED;
        else fgcolor = COLOR_YELLOW;
        setFGcolor(fgcolor); PutStringAt(104, y, float2str(Pressure, 1));
        if(Humidity > H_MAX) fgcolor = COLOR_RED;
        else fgcolor = COLOR_CHOCOLATE;
        setFGcolor(fgcolor); PutStringAt(184, y, float2str(Humidity, 1));
        if(Temperature - Dewpoint < DEW_MIN) fgcolor = COLOR_RED;
        else fgcolor = COLOR_LIGHTBLUE;
        setFGcolor(fgcolor); PutStringAt(248, y, float2str(Dewpoint, 1));
    }else{ // show "errr"
        setBGcolor(COLOR_RED); setFGcolor(COLOR_CYAN);
        CenterStringAt(y, "No signal");
    }
    // display all other data
    SetFontScale(3); setBGcolor(COLOR_BLACK);
    // TODO: show current level
    setFGcolor(COLOR_RED); CenterStringAt(130, "Level: NULL");
    if(getPWM(2) || getPWM(3)){
        setFGcolor(COLOR_GREEN);
        CenterStringAt(220, "Processing");
    }
    UpdateScreen(0, SCRNH-1);
}

#define refresh_window(e)   swhandler(e)

void init_window(window_handler h){
    dispstate = DISP_WINDOW;
    swhandler = h ? h : refresh_mainwin;
    refresh_window(0);
}

void init_menu(menu *m){
    dispstate = DISP_MENU;
    curmenu = m;
    refresh_menu(0);
}

static void refresh_menu(btnevtmask evtmask){ // refresh menu with changed selection
    DBG("REFRESH menu");
    if(!curmenu){
        init_window(refresh_mainwin);
        return;
    }
    if(BTN_PRESS(evtmask, BTN_ESC_MASK)){ // escape to level upper or to main window
        if(curmenu) curmenu = curmenu->parent;
        if(!curmenu){
            init_window(refresh_mainwin);
            return;
        }
    } else if(BTN_PRESS(evtmask, BTN_SEL_MASK)){
        menuitem *selitem = &curmenu->items[curmenu->selected];
        menu *sub = selitem->submenu;
        void (*action)() = selitem->action;
        if(action) action(curmenu);
        if(sub){ // change to submenu
            curmenu = sub;
        } else return;
    } else if(BTN_PRESSHOLD(evtmask, BTN_LEFT_MASK)){ // up
        if(curmenu->selected) --curmenu->selected;
    } else if(BTN_PRESSHOLD(evtmask, BTN_RIGHT_MASK)){ // down
        if(curmenu->selected < curmenu->nitems - 1) ++curmenu->selected;
    }
    cls();
    // check number of first menu item to display
    int firstitem = 0, nitemsonscreen = SCRNH / fontheight, half = nitemsonscreen/2;
    if(curmenu->nitems > nitemsonscreen){ // menu is too large
        if(curmenu->nitems - curmenu->selected <= half) firstitem = curmenu->nitems - nitemsonscreen;
        else firstitem = curmenu->selected - half;
    } else nitemsonscreen = curmenu->nitems;
    if(firstitem < 0) firstitem = 0;
    SetFontScale(3);
    int Y = fontheight - fontbase + 1;
    for(int i = 0; i < nitemsonscreen; ++i, Y += fontheight){
        int idx = firstitem + i;
        if(idx >= curmenu->nitems) break;
        if(idx == curmenu->selected){ // invert fg/bg
            setFGcolor(COLOR_BLACK);
            setBGcolor(COLOR_DARKGREEN);
        }else{
            setFGcolor(COLOR_DARKGREEN);
            setBGcolor(COLOR_BLACK);
        }
        CenterStringAt(Y, curmenu->items[idx].text);
    }
    UpdateScreen(0, SCRNH-1);
}

/*
 * Custom keys:
 * 0 - main screen
 * 1 - up
 * 2 - down
 * 3 - select/menu
 */
#if BTNSNO > 8
#pragma error "Change this code!"
#endif
TRUE_INLINE btnevtmask btns_proc(){
    static uint32_t lastT = 0;
    btnevtmask evtmask = 0; // bitmask for active buttons (==1)
    static keyevent lastevent[BTNSNO] = {0};
    if(lastUnsleep == lastT) return 0; // no buttons activity
    lastT = lastUnsleep;
    for(int i = 0; i < BTNSNO; ++i){
        keyevent evt = keystate(i, NULL); // T may be used for doubleclick detection
        if(evt == EVT_PRESS && lastevent[i] != EVT_PRESS) evtmask |= 1<<i;
        if(evt == EVT_HOLD){
            evtmask |= (0x100 << i);
        }
        lastevent[i] = evt;
    }
    if(!evtmask) return 0;
    if(dispstate == DISP_WINDOW) refresh_window(evtmask);
    else refresh_menu(evtmask);
    return evtmask;
}

void indication_process(){
    if(!LEDsON) return;
    leds_proc();
    if(ScrnState != SCREEN_RELAX) return; // dont process buttons when screen in updating state
    btnevtmask e = btns_proc();
    if(dispstate == DISP_WINDOW){ // send refresh by timeout event
        if(e) lastTupd = Tms;
        else if(Tms - lastTupd > WINDOW_REFRESH_TIMEOUT){
            lastTupd = Tms;
            refresh_window(0);
        }
    }
}
