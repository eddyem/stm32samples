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
#include "debug.h"
#include "fonts.h"
#include "screen.h"
#include "tetris.h"

// backround color
#define BACKGROUND_COLOR    (COLOR_BLACK)
#define FOREGROUND_COLOR    (COLOR_LYELLOW)
#define CUP_COLOR           (COLOR_LRED)

// height of cup
#define CUPHEIGHT       (31)
#define CUPWIDTH        (14)
// screen coordinate of x=0 @ cup
#define CUPX0           (7)
// screen coordinate of y=0 @ cup (Y grows upside down)
#define CUPY0           (0)
// coordinates of "NEXT figure"
#define NEXTX0          (-5)
#define NEXTY0          (SCREEN_HEIGHT/2-2)
#define xmax            (CUPWIDTH-1)
#define ymax            (CUPHEIGHT-1)

// min speed
#define MAXSTEPMS       (299)
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
    .color = 0b00000001
};
// J: 00, 01, 02, -10 + 2 = 0x22, 0x23, 0x24, 0x12
static const figure J = {
    .f = {0x22, 0x23, 0x24, 0x12},
    .color = 0b00000001
};
// O: 00, 01, 10, 11 + 2 = 0x22, 0x23, 0x32, 0x33
static const figure O = {
    .f = {0x22, 0x23, 0x32, 0x33},
    .color = 0b00000101
};

// I: 0-1, 00, 01, 02 + 2 = 0x21, 0x22, 0x23, 0x24
static const figure I = {
    .f = {0x21, 0x22, 0x23, 0x24},
    .color = 0b00100000
};

// S: -10, 00, 01, 11 + 2 = 0x12, 0x22, 0x23, 0x33
static const figure S = {
    .f = {0x12, 0x22, 0x23, 0x33},
    .color = 0b00000100
};

// Z: -11, 01, 00, 10 + 2 = 0x13, 0x23, 0x22, 0x32
static const figure Z = {
    .f = {0x13, 0x23, 0x22, 0x32},
    .color = 0b00000100
};

// T: -10, 01, 00, 10 + 2 = 0x12, 0x23, 0x22, 0x32
static const figure T = {
    .f = {0x12, 0x23, 0x22, 0x32},
    .color = 0b00100101
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
    DBG("CHKFIG: x="); DBGU(x); DBG(", y="); DBGU(y); DBG(" ... ");
    if(x < 0 || x > xmax){
        DBG("x<0 || >xmax\n");
        return 0;
    }
    if(y < 0 || y > ymax){
        DBG("y<0 || > ymax\n");
        return 0;
    }
    for(int i = 0; i < 4; ++i){
        int xn = x + GETX(F->f[i]), yn = y + GETY(F->f[i]);
        if(yn > ymax){
            DBG("y > ymax\n");
            return 0;
        }
        if(xn < 0 || xn > xmax){ // border
            DBG("x out of borders\n");
            return 0;
        }
        if(GetPix(CUPX0 + xn, CUPY0 + yn) != BACKGROUND_COLOR){ // occupied
            DBG("occupied\n");
            return 0;
        }
    }
    DBG("good\n");
    return 1;
}

// clear figure from old location
static void clearfigure(int x, int y){
    DBG("Clear @ "); DBGU(x); DBG(", "); DBGU(y); NL();
    for(int i = 0; i < 4; ++i){
        int xn = x + GETX(curfigure.f[i]), yn = y + GETY(curfigure.f[i]);
        //if(xn < 0 || xn > xmax || yn < 0 || yn > ymax) continue; // out of cup
        DrawPix(CUPX0 + xn, CUPY0 + yn, BACKGROUND_COLOR);
    }
}

// put figure into new location
static void putfigure(int x, int y, figure *F){
    DBG("Put @ "); DBGU(x); DBG(", "); DBGU(y); NL();
    for(int i = 0; i < 4; ++i){
        int xn = x + GETX(F->f[i]), yn = y + GETY(F->f[i]);
        //if(xn < 0 || xn > xmax || yn < 0 || yn > ymax) continue; // out of cup
        DrawPix(CUPX0 + xn, CUPY0 + yn, F->color);
    }
}

// dir=-1 - right, =1 - left
static void rotatefigure(int dir, figure *F){
    figure nF = {.color = F->color};
    for(int i = 0; i < 4; ++i){
        int x = GETX(F->f[i]), y = GETY(F->f[i]), xn, yn;
        if(dir > 0){ // CW
            xn = y; yn = -x;
            DBG("Rotate CW\n");
        }else if(dir < 0){ // CCW
            xn = -y; yn = x;
            DBG("Rotate CCW\n");
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
            if(GetPix(CUPX0 + x, CUPY0 + y) != BACKGROUND_COLOR) ++N;
        if(N == 0) upper = y;
        else if(N == CUPWIDTH){ // full line - roll all upper
            DBG("========= Full row! ========= "); DBG("y="); DBGU(y); DBG(", upper="); DBGU(upper); NL();
            ++ret;
            for(int yy = y; yy > upper; --yy){
                uint8_t *ptr = &screenbuf[(yy+CUPY0)*SCREEN_WIDTH + CUPX0];
                for(int x = 0; x < CUPWIDTH; ++x, ++ptr)
                    *ptr = ptr[-SCREEN_WIDTH]; // copy upper row into the current
            }
            ++upper;
        }
    }
    DBG("Roll="); DBGU(ret); NL();
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

// return 0 if cannot move by y
static int mvfig(int *x, int *y, int dx){
    register int xx = *x, yy = *y;
    DBG("MVFIG: x="); DBGU(*x); DBG(", y="); DBGU(*y); DBG(", dx="); DBGU(dx); NL();
    int ret = 1;
    clearfigure(xx, yy);
    // check dx:
    if(dx){
        int step = 1;
        if(dx < 0){step = -1; dx = -dx;}
        while(dx){
            if(chkfigure(xx + step, yy, &curfigure)){
                xx = xx + step; *x = xx;
            }else break;
            --dx;
        }
    }
    if(chkfigure(xx, yy+1, &curfigure)){
        ++yy; *y = yy;
    }else ret = 0;
    putfigure(xx, yy, &curfigure);
    return ret;
}

// dir == -1 - left, 1 - right
static void rotfig(int x, int y, int dir){
    if(!dir) return;
    DBG("Rotate "); DBGU(dir); NL();
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
    DBG("\n\nDraw next figure\n");
    curfigure = nextfigure;
    clearfigure(NEXTX0, NEXTY0); // clear NEXT
    nextfigure = *figures[getrand()];
    putfigure(NEXTX0, NEXTY0, &nextfigure);
    xf = xmax/2; yf = 1;
    if(!chkfigure(xf, yf, &curfigure)) return 0; // can't draw next
    putfigure(xf, yf, &curfigure);
    Tlast = Tms;
    incSpd = StepMS;
    return 1;
}

void tetris_init(){
    setBGcolor(BACKGROUND_COLOR);
    setFGcolor(FOREGROUND_COLOR);
    choose_font(FONTN8);
    ClearScreen();
    ScreenON();
    score = 0;
    nextSpeedScore = NXTSPEEDSCORE;
    StepMS = MAXSTEPMS;
    // draw cup
    for(int y = 0; y < CUPHEIGHT; ++y){
        DrawPix(CUPX0-1, CUPY0 + y, CUP_COLOR);
        DrawPix(CUPX0 + CUPWIDTH, CUPY0 + y, CUP_COLOR);
    }
    for(int x = 0; x < CUPWIDTH + 2; ++x)
        DrawPix(CUPX0 - 1 + x, CUPY0 + CUPHEIGHT, CUP_COLOR);
    nextfigure = *figures[getrand()];
    PutStringAt(CUPX0 + CUPWIDTH + 3, CUPY0 + 3 + curfont->height, "SCORE:");
    PutStringAt(CUPX0 + CUPWIDTH + 3, CUPY0 + 3 + 2*curfont->height, "0       ");
    drawnext();
}

// process tetris game; @return 0 if game is over
int tetris_process(){
    static int moveX = 0, rot = 0;
    static uint8_t paused = 0;
    keyevent evt;
    if(Tms == Tlast) return 1;
#define PRESS(x)    (keystate(x, &evt) && evt == EVT_PRESS)
#define HOLD(x)     (keyevt(x) == EVT_HOLD)
    // change moving direction
    if(keystate(KEY_U, &evt) && (evt == EVT_PRESS || evt == EVT_HOLD)){ // UP - pause
        if(paused){
            DBG("---->  Tetris resume\n");
            paused = 0;
        }else{
            DBG("---->  Tetris paused\n");
            paused = 1;
        }
    }
    if(!paused){
        if(keystate(KEY_D, &evt) && evt == EVT_PRESS){ // Down - drop
            incSpd = MINSTEPMS;
        }
        if(PRESS(KEY_L)) // L - left
            moveX = -1;
        else if(HOLD(KEY_L))
            moveX = -2;
        if(PRESS(KEY_R)) // Right
            moveX = 1;
        else if(HOLD(KEY_R))
            moveX = 2;
        if(PRESS(KEY_M)){ // Menu - rotate CCW
            rot = 1;
        }
        if(PRESS(KEY_S)){ // Set - rotate CW
            rot = -1;
        }
    }
#undef PRESS
#undef HOLD
    clear_events();
    if(Tms - Tlast > incSpd){
        Tlast = Tms;
        if(paused) return 1;
        int xnew = xf, ynew = yf;
        if(rot){
            rotfig(xf, yf, rot);
            rot = 0;
        }
        DBG("Move down 1px\n");
        if(!mvfig(&xnew, &ynew, moveX)){ // can't move: end of moving?
            moveX = 0;
            int s = checkandroll();
            switch(s){ // add score
                case 1:
                    score += 1;
                    DBG("One line!\n");
                break;
                case 2:
                    score += 3;
                    DBG("Two lines!\n");
                break;
                case 3:
                    score += 7;
                    DBG("Three lines!\n");
                break;
                case 4:
                    score += 15;
                    DBG("Four lines!\n");
                break;
                default:
                break;
            }
            DBG("Score: "); DBG(u2str(score)); DBG("\n");
            if(s){
                setBGcolor(BACKGROUND_COLOR);
                setFGcolor(FOREGROUND_COLOR);
                choose_font(FONTN8);
                uint8_t l = PutStringAt(CUPX0 + CUPWIDTH + 3, CUPY0 + 3 + 2*curfont->height, u2str(score));
                PutStringAt(l+CUPX0 + CUPWIDTH + 3, CUPY0 + 3 + 2*curfont->height, "   ");
            }
            if(StepMS > MINSTEPMS && score > nextSpeedScore){ // increase speed
                --StepMS;
                nextSpeedScore += NXTSPEEDSCORE;
                DBG("Increase speed, StepMS="); DBG(u2str(StepMS)); DBG("\n");
            }
            if(!drawnext()) return 0;
        }else{
            moveX = 0;
            xf = xnew; yf = ynew;
        }
    }
    return 1;
}
