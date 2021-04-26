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
#include "buttons.h"
#include "screen.h"
#include "tetris.h"

#include "usb.h"
#include "proto.h"

// backround color
#define BACKGROUND_COLOR    (COLOR_BLACK)
#define CUP_COLOR           (COLOR_LRED)

// height of cup
#define CUPHEIGHT       (30)
#define CUPWIDTH        (20)
// screen coordinate of x=0 @ cup
#define CUPX0           (1)
// screen coordinate of y=0 @ cup (Y grows upside down)
#define CUPY0           (0)
// coordinates of "NEXT figure"
#define NEXTX0          (30)
#define NEXTY0          (4)
#define xmax            (CUPWIDTH-1)
#define ymax            (CUPHEIGHT-1)

// min speed
#define MAXSTEPMS       (199)
// max speed / drop speed
#define MINSTEPMS       (19)
// each NXTSPEEDSCORE of score the speed will be increased
#define NXTSPEEDSCORE   (50)

extern uint32_t Tms;

// get coordinates by figure member
#define GETX(u) (((u) >> 4) - 2)
#define GETY(u) (((u) & 0xf) - 2)

typedef struct{
    uint8_t f[4];
    uint8_t color;
} figure;

// L: 00, 01, 02, 10 + 2 = 0x22, 0x23, 0x24, 0x32
static const figure L = {
    .f = {0x22, 0x23, 0x24, 0x32},
    .color = COLOR_LBLUE
};
// J: 00, 01, 02, -10 + 2 = 0x22, 0x23, 0x24, 0x12
static const figure J = {
    .f = {0x22, 0x23, 0x24, 0x12},
    .color = COLOR_BLUE
};
// O: 00, 01, 10, 11 + 2 = 0x22, 0x23, 0x32, 0x33
static const figure O = {
    .f = {0x22, 0x23, 0x32, 0x33},
    .color = COLOR_YELLOW
};

// I: 0-1, 00, 01, 02 + 2 = 0x21, 0x22, 0x23, 0x24
static const figure I = {
    .f = {0x21, 0x22, 0x23, 0x24},
    .color = COLOR_CYAN
};

// S: -10, 00, 01, 11 + 2 = 0x12, 0x22, 0x23, 0x33
static const figure S = {
    .f = {0x12, 0x22, 0x23, 0x33},
    .color = COLOR_LGREEN
};

// Z: -11, 01, 00, 10 + 2 = 0x13, 0x23, 0x23, 0x32
static const figure Z = {
    .f = {0x13, 0x23, 0x23, 0x32},
    .color = COLOR_GREEN
};

// T: -10, 01, 00, 10 + 2 = 0x12, 0x23, 0x22, 0x32
static const figure T = {
    .f = {0x12, 0x23, 0x22, 0x32},
    .color = COLOR_PURPLE
};

#define FIGURESNUM  (7)
static const figure *figures[FIGURESNUM] = {&L, &J, &O, &I, &S, &Z, &T};
static figure curfigure, nextfigure;

// current figure coordinates
static int xf, yf;
// next score the speed will be encreased
static uint32_t nextSpeedScore = 0;


// chk if figure can be put to (x,y); @return 1 if empty
static int chkfigure(int x, int y, const figure *F){
    if(x < 0 || x > xmax) return 0;
    if(y < 0 || y > ymax) return 0;
    for(int i = 0; i < 4; ++i){
        int xn = x + GETX(F->f[i]), yn = y + GETY(F->f[i]);
        if(yn > ymax || yn < 0) return 0;
        if(xn < 0 || xn > xmax || yn < 0) return 0; // border
        if(GetPix(xn ,yn) != BACKGROUND_COLOR) return 0; // occupied
    }
    return 1;
}

// clear figure from old location
static void clearfigure(int x, int y){
    USB_send("Clear @ "); USB_send(u2str(x)); USB_send(", "); USB_send(u2str(y)); USB_send("\n");
    for(int i = 0; i < 4; ++i){
        int xn = x + GETX(curfigure.f[i]), yn = y + GETY(curfigure.f[i]);
        if(xn < 0 || xn > xmax || yn < 0 || yn > ymax) continue; // out of cup
        DrawPix(xn, yn, BACKGROUND_COLOR);
    }
}

// put figure into new location
static void putfigure(int x, int y, figure *F){
    USB_send("Put @ "); USB_send(u2str(x)); USB_send(", "); USB_send(u2str(y)); USB_send("\n");
    for(int i = 0; i < 4; ++i){
        int xn = x + GETX(F->f[i]), yn = y + GETY(F->f[i]);
        if(xn < 0 || xn > xmax || yn < 0 || yn > ymax) continue; // out of cup
        DrawPix(xn, yn, F->color);
    }
}

// dir=-1 - right, =1 - left
static void rotatefigure(int dir, figure *F){
    figure nF = {.color = F->color};
    for(int i = 0; i < 4; ++i){
        int x = GETX(F->f[i]), y = GETY(F->f[i]), xn, yn;
        if(dir > 0){ // CW
            xn = y; yn = -x;
            USB_send("Rotate CW\n");
        }else if(dir < 0){ // CCW
            xn = -y; yn = x;
            USB_send("Rotate CCW\n");
        }
        nF.f[i] = ((xn + 2) << 4) | (yn + 2);
    }
    *F = nF;
}

// check cup for full lines & delete them; return number of deleted lines
static int checkandroll(){
    int upper = 0, ret = 0;
    for(int y = 0; y < CUPHEIGHT; ++y){
        int N = 0;
        for(int x = 0; x < CUPWIDTH; ++x)
            if(GetPix(x, y) != BACKGROUND_COLOR) ++N;
        if(N == 0) upper = y;
        else if(N == CUPWIDTH){ // full line - roll all upper
            ++ret;
            for(int yy = y; yy <= upper; --yy){
                uint8_t *ptr = &screenbuf[(yy+CUPY0)*SCREEN_WIDTH + CUPX0];
                for(int x = 0; x < CUPWIDTH; ++x, ++ptr)
                    *ptr = ptr[-CUPWIDTH]; // copy upper row into the current
            }
            ++upper;
        }
    }
    USB_send("Roll="); USB_send(u2str(ret)); USB_send("\n");
    return ret;
}

// get random number of next figure
static int getrand(){
    static int oldnum[2] = {-1, -1};
    int r = getRand() % FIGURESNUM;
    if(r == oldnum[1]) r = getRand() % FIGURESNUM;
    if(r == oldnum[0]) r = getRand() % FIGURESNUM;
    oldnum[0] = oldnum[1];
    oldnum[1] = r;
    return r;
}

// return 0 if cannot move
static int mvfig(int *x, int *y, int dx){
    register int xx = *x, yy = *y;
    int xnew = xx+dx, ynew = yy+1, ret = 1;
    clearfigure(xx, yy);
    if(chkfigure(xnew, ynew, &curfigure)){
        xx = xnew; yy = ynew;
        *x = xx; *y = yy;
    }else ret = 0;
    putfigure(xx, yy, &curfigure);
    return ret;
}

// dir == -1 - left, 1 - right
static void rotfig(int x, int y, int dir){
    if(!dir) return;
    figure newF = curfigure;
    rotatefigure(dir, &newF);
    clearfigure(x, y);
    if(chkfigure(x, y, &newF)){ // can rotate - substitute old figure with new
        curfigure = newF;
    }
    putfigure(x, y, &curfigure);
}

// draw current & next figures; return 0 if can't
static int drawnext(){
    curfigure = nextfigure;
    clearfigure(NEXTX0, NEXTY0); // clear NEXT
    nextfigure = *figures[getrand()];
    putfigure(NEXTX0, NEXTY0, &nextfigure);
    xf = xmax/2; yf = ymax/2;
    if(!chkfigure(xf, yf, &curfigure)) return 0; // can't draw next
    putfigure(xf, yf, &curfigure);
    return 1;
}

void tetris_init(){
    setBGcolor(BACKGROUND_COLOR);
    ClearScreen();
    ScreenON();
    StepMS = MAXSTEPMS;
    Tlast = Tms;
    incSpd = StepMS;
    score = 0;
    nextSpeedScore = NXTSPEEDSCORE;
    // draw cup
    for(int y = 0; y < CUPHEIGHT; ++y){
        DrawPix(CUPX0, CUPY0 + y, CUP_COLOR);
        DrawPix(CUPX0 + CUPWIDTH, CUPY0 + y, CUP_COLOR);
    }
    for(int x = 0; x < CUPWIDTH; ++x)
        DrawPix(CUPX0 + x, CUPY0 + CUPHEIGHT, CUP_COLOR);
    nextfigure = *figures[getrand()];
    drawnext();
}

// process tetris game; @return 0 if game is over
int tetris_process(){
    uint8_t moveX = 0, rot = 0;
    keyevent evt;
    static uint8_t paused = 0;
    // change moving direction
    if(keystate(KEY_U, &evt) && (evt == EVT_PRESS || evt == EVT_HOLD)){ // UP - pause
        if(paused){
            USB_send("Tetris resume\n");
            paused = 0;
        }else{
            USB_send("Tetris paused\n");
            paused = 1;
        }
    }
    if(keystate(KEY_D, &evt) && evt == EVT_PRESS){ // Down - drop
        incSpd = MINSTEPMS;
    }
    if(keystate(KEY_L, &evt) && evt == EVT_PRESS){ // L - left
         moveX = -1;
    }
    if(keystate(KEY_R, &evt) && evt == EVT_PRESS){ // Right
        moveX = 1;
    }
    if(keystate(KEY_M, &evt) && evt == EVT_PRESS){ // Menu - rotate CCW
        rot = -1;
    }
    if(keystate(KEY_S, &evt) && evt == EVT_PRESS){ // Set - rotate CW
        rot = 1;
    }
    clear_events();
    if(Tms - Tlast > incSpd){
        Tlast = Tms;
        if(paused) return 1;
        int xnew = xf, ynew = yf;
        if(rot) rotfig(xf, yf, rot);
        if(!mvfig(&xnew, &ynew, moveX) && ynew == yf){ // can't move: end of moving?
            int s = checkandroll();
            switch(s){ // add score
                case 1:
                    score += 1;
                break;
                case 2:
                    score += 3;
                break;
                case 3:
                    score += 7;
                break;
                case 4:
                    score += 15;
                break;
                default:
                break;
            }
            if(StepMS > MINSTEPMS && score > nextSpeedScore){ // increase speed
                --StepMS;
                nextSpeedScore += NXTSPEEDSCORE;
            }
            if(!drawnext()) return 0;
        }
    }
    return 1;
}
