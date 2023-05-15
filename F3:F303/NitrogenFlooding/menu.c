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

#include "adc.h"
#include "hardware.h"
#include "indication.h"
#include "menu.h"
#include "screen.h"
#include "strfunc.h"
#include "usb.h"

#include <string.h>

// integer parameter to change
typedef struct{
    int32_t min;
    int32_t max;
    int32_t cur;
    int32_t *val;
} i32par_t;

// function to run after selecting "yes" in yesno window
typedef void (*yesfun_t)();
yesfun_t yesfunction = NULL;

// current parameter of uint32_t setter
static i32par_t i32parameter = {0};

// string buffer for non-constant parname
#define PARNMBUFSZ  (63)
static char parnmbuff[PARNMBUFSZ+1];
static const char *parname = NULL;

// upper level menu to return from subwindow functions
static menu *uplevel = NULL;

static void showadc(menu *m);
static void htr1(menu *m);
static void htr2(menu *m);
static void savesettings(menu *m);

static void testx(const char *text){
    USB_sendstr(text);
    USB_sendstr(" pressed\n");
}

static void test3(menu _U_ *m){testx("main 3");}
static void test4(menu _U_ *m){testx("main 4");}
static void stest1(menu _U_ *m){testx("sub 1");}
static void stest2(menu _U_ *m){testx("sub 2");}
static void stest3(menu _U_ *m){testx("sub 3");}
static void stest4(menu _U_ *m){testx("sub 4");}


static menuitem settingsmenuitems[] = {
    {"test 1", NULL, stest1},
    {"test 2", NULL, stest2},
    {"test 3", NULL, stest3},
    {"test 4", NULL, stest4},
    {"Save", NULL, savesettings},
};

static menu settingsmenu = {
    .parent = &mainmenu,
    .nitems = 5,
    .selected = 0,
    .items = settingsmenuitems
};

static menuitem mainmenuitems[] = {
    {"ADC raw data", NULL, showadc},
    {"Heater1 power", NULL, htr1},
    {"Heater2 power", NULL, htr2},
    {"Settings", &settingsmenu, NULL},
    {"Test3", NULL, test3},
    {"Test4", NULL, test4},
};

menu mainmenu = {
    .parent = NULL,
    .nitems = 6,
    .selected = 0,
    .items = mainmenuitems
};

static void refresh_adcwin(btnevtmask evtmask){
    if(BTN_PRESS(evtmask, BTN_ESC_MASK | BTN_SEL_MASK)){ // switch to menu
        init_menu(uplevel);
        return;
    }
    if(evtmask) return;
    cls();
    SetFontScale(1);
    int Y0 = fontheight*2 + 5; // output
    int X, Y = Y0, Xstart = 20, Xraw = 0, XR = 0, XK = 0;
    setFGcolor(COLOR_CYAN);
    for(int i = 0; i < NUMBER_OF_AIN_CHANNELS; ++i, Y+=fontheight+2){
        //X = PutStringAt(X, Y, "ADC input ");
        X = PutStringAt(Xstart, Y, u2str(i));
        if(X > Xraw) Xraw = X;
    }
    Xraw += 20; // Xraw - starting X for text 'RAW'
    Y = Y0;
    setFGcolor(COLOR_LIGHTBLUE);
    // RAW values
    for(int i = 0; i < NUMBER_OF_AIN_CHANNELS; ++i, Y+=fontheight+2){
        X = PutStringAt(Xraw, Y, u2str(ADCvals[i]));
        if(X > XR) XR = X;
    }
    XR += 20; // XR - starting X for text 'R, Ohm'
    Y = Y0;
    setFGcolor(COLOR_LIGHTGREEN);
    // R in Ohms
    for(int i = 0; i < NUMBER_OF_AIN_CHANNELS; ++i, Y+=fontheight+2){
        if(ADCvals[i] > ADC_NOCONN) X = PutStringAt(XR, Y, "NC");
        else X = PutStringAt(XR, Y, float2str(calcR(ADCvals[i]), 3));
        if(X > XK) XK = X;
    }
    Y = Y0;
    XK += 20; // XK - starting X for text 'T, K'
    // T in K
    for(int i = 0; i < NUMBER_OF_AIN_CHANNELS; ++i, Y+=fontheight+2){
        if(ADCvals[i] < ADC_NOCONN){
            float K = calcT(ADCvals[i]);
            setFGcolor(COLOR_YELLOW);
            PutStringAt(XK, Y, float2str(K, 1));
            setFGcolor(COLOR_GOLD);
            PutStringAt(XK+80, Y, float2str(K-273.15, 1));
        }
    }
    setFGcolor(COLOR_BLUE);
    Y0 = fontheight + 1;
    // print header
    PutStringAt(Xstart, Y0, "#");
    PutStringAt(Xraw, Y0, "Raw");
    PutStringAt(XR, Y0, "R, Ohm");
    PutStringAt(XK, Y0, "T, K");
    PutStringAt(XK+80, Y0, "degC");
    setFGcolor(COLOR_CHOCOLATE);
    Y += 5;
    PutStringAt(Xstart, Y, "P"); PutStringAt(Xraw, Y, u2str(ADCvals[ADC_EXT]));
    float P = calcPres5050();
    if(P > 30) setFGcolor(COLOR_RED);
    else if(P > 10) setFGcolor(COLOR_YELLOW);
    else setFGcolor(COLOR_GREEN);
    X = PutStringAt(XR, Y, float2str(P, 1));
    PutStringAt(X+16, Y, "kPa");
    //setFGcolor(COLOR_LIGHTYELLOW);
    //Y += 2 + fontheight;
    //PutStringAt(20, Y, "T MCU"); PutStringAt(wmax, Y, float2str(getMCUtemp(), 0));
    UpdateScreen(0, -1);
}

// show RAW ADC data
static void showadc(menu *m){
    init_window(refresh_adcwin);
    uplevel = m;
}

// window of uint32_t setter
static void refresh_i32setter(btnevtmask evtmask){
    int selected = 0;
    if(BTN_PRESS(evtmask, BTN_ESC_MASK)){ // return to main menu
        init_menu(uplevel);
        return;
    }
    cls();
    int incrdecr = 0;
    if(BTN_PRESS(evtmask, BTN_SEL_MASK)){ // change value
        *i32parameter.val = i32parameter.cur;
        selected = 1;
    } else if(BTN_PRESSHOLD(evtmask, BTN_LEFT_MASK)){ // decrement
        if(BTN_HOLD(evtmask, BTN_LEFT_MASK)) incrdecr = -3;
        else incrdecr = -1;
    } else if(BTN_PRESSHOLD(evtmask, BTN_RIGHT_MASK)){ // increment
        if(BTN_HOLD(evtmask, BTN_RIGHT_MASK)) incrdecr = 3;
        else incrdecr = 1;
    }
    if(incrdecr){
        i32parameter.cur += incrdecr;
        if(i32parameter.cur < i32parameter.min) i32parameter.cur = i32parameter.min;
        if(i32parameter.cur > i32parameter.max) i32parameter.cur = i32parameter.max;
    }
    setFGcolor(COLOR_CHOCOLATE);
    SetFontScale(2);
    int Y = fontheight + fontheight/2;
    CenterStringAt(Y, parname);
    SetFontScale(3);
    Y += fontheight + fontheight/2;
    if(selected) setFGcolor(COLOR_GREEN);
    else setFGcolor(COLOR_BLUE);
    CenterStringAt(Y, u2str(i32parameter.cur));
}

// init pwm setter
static void showpwm(menu *m, int nccr){
    i32parameter.max = PWM_CCR_MAX;
    i32parameter.min = 0;
    i32parameter.cur = (nccr == 3) ? TIM3->CCR3 : TIM3->CCR4;
    i32parameter.val = (nccr == 3) ? (int32_t*) &TIM3->CCR3 : (int32_t*) &TIM3->CCR4;
    parname = m->items[m->selected].text;
    uplevel = m;
    init_window(refresh_i32setter);
}
static void htr1(menu *m){showpwm(m, 3);}
static void htr2(menu *m){showpwm(m, 4);}


static void refresh_yesno(btnevtmask evtmask){
    if(BTN_PRESS(evtmask, BTN_ESC_MASK)){ // return to main menu
        init_menu(uplevel);
        return;
    }
    cls();
    const char *msgs[] = {"No", "Yes"};
    static int current = 0; // current item: "NO"
    int selected = 0; // not selected
    if(BTN_PRESS(evtmask, BTN_SEL_MASK)){ // change value
        selected = 1;
    } else if(BTN_PRESS(evtmask, BTN_LEFT_MASK | BTN_RIGHT_MASK)){
        current = !current;
    }
    setFGcolor(COLOR_CHOCOLATE);
    SetFontScale(2);
    int Y = fontheight + fontheight/2;
    CenterStringAt(Y, parname);
    SetFontScale(3);
    Y += fontheight + fontheight/2;
    if(selected) setFGcolor(COLOR_GREEN);
    else setFGcolor(COLOR_BLUE);
    CenterStringAt(Y, msgs[current]);
    if(selected && current == 1) yesfunction();
}

static void savesdummy(){
    USB_sendstr("Settings saved\n");
}
// save current settings
static void savesettings(menu *m){
    yesfunction = savesdummy;
    uplevel = m;
    const char *p = m->items[m->selected].text;
    int l = strlen(p);
    if(l > PARNMBUFSZ-1) l = PARNMBUFSZ-1;
    strncpy(parnmbuff, p, l);
    parnmbuff[l] = '?';
    parnmbuff[l+1] = 0;
    parname = parnmbuff;
    init_window(refresh_yesno);
}
