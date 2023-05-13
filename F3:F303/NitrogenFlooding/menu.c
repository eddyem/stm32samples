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

// integer parameter to change
typedef struct{
    uint32_t min;
    uint32_t max;
    uint32_t cur;
    uint32_t *val;
} u32par;

// current parameter of uint32_t setter
static u32par u32parameter = {0};

static const char *parname = NULL;

// upper level menu to return from subwindow functions
static menu *uplevel = NULL;

static void showadc(menu *m);
static void htr1(menu *m);
static void htr2(menu *m);

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
static void stest5(menu _U_ *m){testx("sub 5");}


static menuitem submenu1items[] = {
    {"test 1", NULL, stest1},
    {"test 2", NULL, stest2},
    {"test 3", NULL, stest3},
    {"test 4", NULL, stest4},
    {"test 5", NULL, stest5},
};

static menu submenu1 = {
    .parent = &mainmenu,
    .nitems = 5,
    .selected = 0,
    .items = submenu1items
};

static menuitem mainmenuitems[] = {
    {"ADC raw data", NULL, showadc},
    {"Heater1 power", NULL, htr1},
    {"Heater2 power", NULL, htr2},
    {"Submenu1", &submenu1, NULL},
    {"Test3", NULL, test3},
    {"Test4", NULL, test4},
};

menu mainmenu = {
    .parent = NULL,
    .nitems = 6,
    .selected = 0,
    .items = mainmenuitems
};

static void refresh_adcwin(uint8_t evtmask){
    if(evtmask & BTN_ESC_MASK || evtmask & BTN_SEL_MASK){ // switch to menu
        init_menu(uplevel);
        return;
    }
    if(evtmask) return;
    cls();
    SetFontScale(1);
    int Y = fontheight + 1, wmax = 0;
    setFGcolor(COLOR_CYAN);
    for(int i = 0; i < NUMBER_OF_AIN_CHANNELS; ++i, Y+=fontheight+2){
        int X = 20;
        X = PutStringAt(X, Y, "ADC input ");
        X = PutStringAt(X, Y, u2str(i));
        if(X > wmax) wmax = X;
    }
    wmax += 20;
    setFGcolor(COLOR_WHITE);
    Y = fontheight + 1;
    for(int i = 0; i < NUMBER_OF_AIN_CHANNELS; ++i, Y+=fontheight+2){
        PutStringAt(wmax, Y, u2str(ADCvals[i]));
    }
    setFGcolor(COLOR_LIGHTGREEN);
    Y += 5;
    PutStringAt(20, Y, "ADC ext"); PutStringAt(wmax, Y, u2str(ADCvals[ADC_EXT]));
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
static void refresh_u32setter(uint8_t evtmask){
    int selected = 0;
    if(evtmask & BTN_ESC_MASK){ // return to main menu
        init_menu(uplevel);
        return;
    }
    cls();
    if(evtmask & BTN_SEL_MASK){ // change value
        *u32parameter.val = u32parameter.cur;
        selected = 1;
    } else if(evtmask & BTN_LEFT_MASK){ // decrement
        if(u32parameter.cur > u32parameter.min) --u32parameter.cur;
    } else if(evtmask & BTN_RIGHT_MASK){ // increment
        if(u32parameter.cur < u32parameter.max) ++u32parameter.cur;
    }
    setFGcolor(COLOR_CHOCOLATE);
    SetFontScale(2);
    int Y = fontheight + fontheight/2;
    CenterStringAt(Y, parname);
    SetFontScale(3);
    Y += fontheight + fontheight/2;
    if(selected) setFGcolor(COLOR_GREEN);
    else setFGcolor(COLOR_BLUE);
    CenterStringAt(Y, u2str(u32parameter.cur));
}

// init pwm setter
static void showpwm(menu *m, int nccr){
    u32parameter.max = 100;
    u32parameter.min = 0;
    u32parameter.cur = (nccr == 3) ? TIM3->CCR3 : TIM3->CCR4;
    u32parameter.val = (nccr == 3) ? (uint32_t*) &TIM3->CCR3 : (uint32_t*) &TIM3->CCR4;
    parname = m->items[m->selected].text;
    uplevel = m;
    init_window(refresh_u32setter);
}
static void htr1(menu *m){showpwm(m, 3);}
static void htr2(menu *m){showpwm(m, 4);}
