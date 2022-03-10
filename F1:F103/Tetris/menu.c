/*
 * This file is part of the TETRIS project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "buttons.h"
#include "fonts.h"
#include "menu.h"
#include "screen.h"

#include "usb.h"
#include "proto.h"

// colors
#define MENUBGCOLOR     (0)
#define MENUFGCOLOR     (0b1100)
// menu item @ screen center
#define midNo  ((MENU_ITEMS - 1) / 2)
// Y coordinate of screen center
#define midY (SCREEN_HH - 1)

static const char* items[] = {
    [MENU_SLEEP]    = " SLEEP ",
    [MENU_BALLS]    = " BALLS ",
    [MENU_SNAKE]    = " SNAKE ",
    [MENU_TETRIS]   = " TETRIS ",
    [MENU_ARKANOID] = " ARKANOID ",
};

static menuitem curitem = MENU_SLEEP;

// show menu
static void _menu(){
    choose_font(FONTN8);
    setBGcolor(COLOR_BLACK);
    ClearScreen();
    int charH = curfont->height, charB = curfont->baseline;
    int curI = midNo;
    if(MENU_ITEMS*charH > SCREEN_HEIGHT){
        curI = curitem; charB -= charH/2;
    }
    for(int i = 0; i < MENU_ITEMS; ++i){
        if(i == curitem){
            setBGcolor(MENUFGCOLOR);
            setFGcolor(MENUBGCOLOR);
        }else{
            setBGcolor(MENUBGCOLOR);
            setFGcolor(MENUFGCOLOR);
        }
        CenterStringAt(midY + (i-curI)*charH - charB, items[i]);
    }
}

// initial menu
void show_menu(){
    curitem = MENU_SLEEP;
    ScreenON();
    _menu();
}

// process menu items and return its status
menuitem menu_activated(){
    keyevent evt;
    menuitem olditem = curitem;
    if(keystate(KEY_U, &evt) && evt == EVT_PRESS){ // dont' react @ HOLD events!
        if(curitem) --curitem;
    }
    if(keystate(KEY_D, &evt) && evt == EVT_PRESS){
        if(curitem < (MENU_ITEMS-1)) ++curitem;
    }
    if(curitem != olditem) _menu();
    if(keystate(KEY_S, &evt) && evt == EVT_HOLD){ // select by hold event
        return curitem;
    }
    clear_events(); // clear all other events
    return MENU_NONE;
}

// show "GAME OVER" screen
void show_gameover(){
    setBGcolor(COLOR_BLACK);
    setFGcolor(COLOR_RED);
    ClearScreen();
    choose_font(FONTN8);
    CenterStringAt(SCREEN_HEIGHT/2 - curfont->baseline - 1, "GAME OVER!");
    setFGcolor(COLOR_LYELLOW);
    CenterStringAt(SCREEN_HEIGHT - curfont->baseline - 2, u2str(score));
}
