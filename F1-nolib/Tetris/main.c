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

#include "adcrandom.h"
#include "arkanoid.h"
#include "balls.h"
#include "buttons.h"
#include "fonts.h"
#include "hardware.h"
#include "menu.h"
#include "proto.h"
#include "screen.h"
#include "snake.h"
#include "tetris.h"
#include "usb.h"
#include "usb_lib.h"

// timeout for autosleep (30s)
#define AUTOSLEEP_TMOUT     (30000)

volatile uint32_t Tms = 0;
uint8_t balls = 0;

enum{
    STATE_MENU,
    STATE_SNAKE,
    STATE_TETRIS,
    STATE_ARKANOID,
    STATE_SLEEP,
    STATE_GAMEOVER
} curstate = STATE_SLEEP;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

#define USBBUFSZ    (127)
// usb getline
static char *get_USB(){
    static char tmpbuf[USBBUFSZ+1], *curptr = tmpbuf;
    static int rest = USBBUFSZ;
    int x = USB_receive(curptr);
    curptr[x] = 0;
    if(!x) return NULL;
    if(curptr[x-1] == '\n'){
        curptr = tmpbuf;
        rest = USBBUFSZ;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = USBBUFSZ;
        USB_send("USB buffer overflow\n");
    }
    return NULL;
}

static void process_menu(){
    switch(menu_activated()){
        case MENU_SLEEP:
            USB_send("Select 'Sleep'\n");
            ScreenOFF();
            curstate = STATE_SLEEP;
        break;
        case MENU_BALLS:
            USB_send("Select 'Balls'\n");
            if(balls){
                balls = 0;
            }else{
                curstate = STATE_SLEEP;
                balls_init();
                balls = 1;
            }
        break;
        case MENU_SNAKE:
            USB_send("Select 'Snake'\n");
            snake_init();
            curstate = STATE_SNAKE;
        break;
        case MENU_TETRIS:
            USB_send("Select 'Tetris'\n");
            tetris_init();
            curstate = STATE_TETRIS;
        break;
        case MENU_ARKANOID:
            USB_send("Select 'Arkanoid'\n");
            arkanoid_init();
            curstate = STATE_ARKANOID;
        default:
        break;
    }
}

static void gotomenu(){
    curstate = STATE_MENU;
    clear_events();
    show_menu();
}

int main(void){
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    SysTick_Config(72000);
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    hw_setup();
    USBPU_OFF();
    adc_setup();
    USB_setup();
    //iwdg_setup();
    USBPU_ON();

    keyevent evt;
    while(1){
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        IWDG->KR = IWDG_REFRESH;
        if(balls) process_balls();
        process_keys();
        switch(curstate){
            case STATE_SLEEP:
                if(keystate(KEY_M, &evt) && evt == EVT_RELEASE){
                    USB_send("Activate menu\n");
                    gotomenu();
                }
            break;
            case STATE_MENU:
                process_menu();
                if(Tms - lastUnsleep > AUTOSLEEP_TMOUT){
                    USB_send("Autosleep\n");
                    ScreenOFF();
                    curstate = STATE_SLEEP;
                }
            break;
            case STATE_SNAKE:
                if(!snake_proces()){
                    show_gameover();
                    curstate = STATE_GAMEOVER;
                }
            break;
            case STATE_TETRIS:
                if(!tetris_process()){
                    show_gameover();
                    curstate = STATE_GAMEOVER;
                }
            break;
            case STATE_ARKANOID:
                if(!arkanoid_process()){
                    show_gameover();
                    curstate = STATE_GAMEOVER;
                }
            break;
            case STATE_GAMEOVER: // show gameover screen
                if(keystate(KEY_M, &evt) && evt == EVT_RELEASE){
                    gotomenu();
                }else if(Tms - lastUnsleep > AUTOSLEEP_TMOUT){
                    USB_send("Autosleep\n");
                    ScreenOFF();
                    curstate = STATE_SLEEP;
                }
            break;
        }

        usb_proc();
        char *txt; const char *ans;
        if((txt = get_USB())){
            ans = parse_cmd(txt);
            if(ans) USB_send(ans);
        }
    }
    return 0;
}

