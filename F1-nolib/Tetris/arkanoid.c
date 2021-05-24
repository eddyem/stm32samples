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
#include "buttons.h"
#include "debug.h"
#include "fonts.h"
#include "proto.h"
#include "screen.h"

// fraction of 1 for fixed point
#define FIXPOINT            (1000)

// time (in ms) for keeping racket old X speed
#define KEEPXSPEEDTIME      (300)

// max amount of balls
#define NBALLSMAX           (8)
// fixed point macros:
// int to fixed
#define i2fixed(x)          ((x)*FIXPOINT)
// fixed to int
#define fixed2i(x)          ((x)/FIXPOINT)

// field parameters (should be multiple of BRICK_WIDTH)
#define FIELD_WIDTH         (39)
// bottom height
#define FIELD_BOTTOM        (12)
// X coordinate of field start
#define X0                  (1)
// active zone (with bricks)
#define FIELD_ACTIVE        (SCREEN_HEIGHT - FIELD_BOTTOM)
// Y-coordinate of racket
#define RACKET_Y            (SCREEN_HEIGHT - 2)
// floor (should be seen when bonus activates)
#define FLOOR_Y             (SCREEN_HEIGHT - 1)

// starting Y speed of the ball
#define BALL_NORMAL_SPEED   (0.007 * FIXPOINT)
#define BALL_MIN_Y_SPEED    (0.005 * FIXPOINT)
// maximal speed of the ball
#define BALL_MAX_X_SPEED    (0.015 * FIXPOINT)
#define BALL_MAX_Y_SPEED    (0.010 * FIXPOINT)
// racket speed delta per millisecond
#define D_RACKET_SPEED      (0.002 * FIXPOINT)

// part of racket's speed to add to ball's speed (1/frac)
#define XSPDFRAC            (200)
#define YSPDFRAC            (1000)

#define BRICK_WIDTH         (3)

// colors
#define BACKGROUND_COLOR    (COLOR_BLACK)
#define FOREGROUND_COLOR    (COLOR_LYELLOW)
#define CUP_COLOR           (COLOR_LRED)
#define RACKET_COLOR        (COLOR_LGREEN)
#define BALL_COLOR          (COLOR_GREEN)

// racket widths
#define RACKET_MEDIUM       (5)
#define RACKET_NARROW       (3)
#define RACKET_WIDE         (9)

// starting amount of lives
#define LIVES_AT_START      (3)

// probability of bonus appears (percent)
#define BONUS_PROBABILITY   (35)

// there's a bonus inside this brick
#define BRICK_IS_BONUS      (1<<7)
// and there's another free flag @ 1<<6 + reserved @ 1<<5
// mask to get color from brick type
#define COLOR_MASK          (0x3f)
typedef enum{ // brick types
    BRICK_NONE = 0,     // no brick
    BRICK_WALL,         // non-breakable brick
    BRICK_GRAY,         // two bricks without bonuses
    BRICK_WHITE,
    BRICK_FLOOR,        // add floor under the racket
    BRICK_ENLARGE,      // enlarge the rocket (don't forget to move it's center when positioned at the edge)
    BRICK_CATCH,        // sticky balls
    BRICK_SLOW,         // slow ball
    BRICK_DISRUPTION    // each ball divides to three
} bricktype;

static const uint8_t brickcolors[] = {
    [BRICK_NONE] = 0,
    [BRICK_WALL] = CUP_COLOR,
    [BRICK_GRAY] = 0b00100101,
    [BRICK_WHITE] = COLOR_WHITE,
    [BRICK_FLOOR] = COLOR_LBLUE,
    [BRICK_ENLARGE] = COLOR_LGREEN,
    [BRICK_CATCH] = COLOR_LCYAN,
    [BRICK_SLOW] = COLOR_LYELLOW,
    [BRICK_DISRUPTION] = COLOR_LPURPLE,
};

// empty cell
#define ____                (0)
// simple bricks with no bonuses
#define _VV_                (BRICK_GRAY)
#define _WW_                (BRICK_WHITE)
// bricks with bonuses
#define _FL_                (BRICK_FLOOR)
#define _EN_                (BRICK_ENLARGE)
#define _CA_                (BRICK_CATCH)
#define _SL_                (BRICK_SLOW)
#define _DI_                (BRICK_DISRUPTION)
// wall
#define XXXX                (BRICK_WALL)

#define FIELD_SZ            (FIELD_ACTIVE * FIELD_WIDTH / BRICK_WIDTH)

#define LEVELS_AMOUNT       (3)

// empty level
#if 0
static uint8_t levelX[FIELD_SZ] = {
/*00*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*01*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*02*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*03*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*04*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*05*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*06*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*07*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*08*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*09*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*10*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*11*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*12*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*13*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*14*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*15*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*16*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*17*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*18*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*19*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
};
#endif

static uint8_t level1[FIELD_SZ] = {
/*00*/  _VV_,____,____,____,____,_WW_,____,_WW_,____,____,____,____,_VV_,
/*01*/  ____,_VV_,____,____,____,____,_WW_,____,____,____,____,_VV_,____,
/*02*/  ____,____,_VV_,____,____,____,____,____,____,____,_VV_,____,____,
/*03*/  ____,_SL_,____,_VV_,____,____,____,____,____,_VV_,____,_SL_,____,
/*04*/  ____,____,____,____,_VV_,_DI_,_DI_,_DI_,_VV_,____,____,____,____,
/*05*/  ____,____,____,____,____,_VV_,_DI_,_VV_,____,____,____,____,____,
/*06*/  ____,____,____,____,_EN_,____,_VV_,____,_EN_,____,____,____,____,
/*07*/  ____,____,____,____,____,_CA_,_FL_,_CA_,____,____,____,____,____,
/*08*/  ____,____,____,____,____,____,_CA_,____,____,____,____,____,____,
/*09*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*10*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*11*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*12*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*13*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*14*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*15*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*16*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*17*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*18*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*19*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
};
static uint8_t level2[FIELD_SZ] = {
/*00*/  ____,____,____,_WW_,____,_WW_,____,_WW_,____,_WW_,____,____,____,
/*01*/  ____,____,_WW_,_VV_,____,____,_WW_,____,____,_VV_,_WW_,____,____,
/*02*/  ____,_WW_,_WW_,____,_VV_,____,____,____,_VV_,____,_WW_,_WW_,____,
/*03*/  _WW_,____,____,_WW_,____,_VV_,____,_VV_,____,_WW_,____,____,_WW_,
/*04*/  ____,____,_VV_,____,_WW_,____,_VV_,____,_WW_,____,_VV_,____,____,
/*05*/  ____,____,____,____,____,_WW_,____,_WW_,____,____,____,____,____,
/*06*/  ____,____,____,____,____,____,_WW_,____,____,____,____,____,____,
/*07*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*08*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*09*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*10*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*11*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*12*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*13*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*14*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*15*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*16*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*17*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*18*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*19*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
};
static uint8_t level3[FIELD_SZ] = { // w=13, h=20
/*00*/  _WW_,_VV_,_WW_,____,____,_VV_,____,____,____,____,_VV_,____,____,
/*01*/  _VV_,_WW_,_VV_,____,_VV_,____,_VV_,____,____,____,_VV_,____,____,
/*02*/  ____,____,_WW_,_VV_,____,____,____,_VV_,____,____,_VV_,____,____,
/*03*/  ____,____,____,_WW_,_VV_,____,____,____,_VV_,____,_VV_,____,_VV_,
/*04*/  ____,____,____,____,_WW_,_VV_,____,____,____,_VV_,_VV_,_VV_,____,
/*05*/  ____,____,_WW_,____,____,_WW_,_VV_,____,____,____,_VV_,_WW_,_WW_,
/*06*/  ____,____,____,____,____,____,_WW_,_VV_,____,____,_VV_,____,____,
/*07*/  ____,____,____,_WW_,_WW_,_WW_,_WW_,_WW_,_VV_,____,_VV_,____,_VV_,
/*08*/  ____,____,____,____,____,____,____,____,_WW_,_VV_,_VV_,_VV_,_WW_,
/*09*/  ____,____,_VV_,____,____,____,____,____,____,_WW_,_VV_,_WW_,____,
/*10*/  ____,____,_VV_,_VV_,_VV_,_VV_,_VV_,_VV_,_VV_,_VV_,_VV_,_VV_,_VV_,
/*11*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*12*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*13*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*14*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*15*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*16*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*17*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*18*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
/*19*/  ____,____,____,____,____,____,____,____,____,____,____,____,____,
};
static uint8_t *levels[LEVELS_AMOUNT] = {level1, level2, level3};
static uint8_t currentLevel[FIELD_SZ]; // data of current level's bricks
static uint8_t paused = 0;

// flags for theball.flags
#define ACTIVE_BALL         (1<<0)
#define STICKY_BALL         (1<<1)

// !!! BALL AND RACKET COORDINATES ARE IN FIELD SYSTEM !!!
typedef struct{ // all coordinates and speeds are fixed point
    int x; int y;           // old/actual center coordinates
    int xspeed; int yspeed; // speed
    uint8_t flags;
} theball;

extern uint32_t volatile Tms;

// current balls amount (level ends when last ball falls)
static uint8_t Nballs;
static theball balls[NBALLSMAX];
static uint8_t racketW; // current racket width
// all coordinates and speeds are fixed point
static int racketX;
static int racketspeed, oldracketspeed;
// level number
static uint8_t curLevNo;
// lifes amount
static uint16_t Nlives;
// total bricks amount
static uint16_t Nbricks;

// bonuses
static uint8_t floor_activated; // ==1 if floor is active

static void showText(){ // show score/level etc
    int y = curfont->height - curfont->baseline;
    PutStringAt(X0+FIELD_WIDTH+2, y, "         ");
    PutStringAt(X0+FIELD_WIDTH+2, y, u2str(score));
    y += curfont->height;
    int w = PutStringAt(X0+FIELD_WIDTH+2, y, u2str(Nlives));
    PutStringAt(X0+FIELD_WIDTH+2+w, y, "L ");
    y += curfont->height;
    if(paused){
        PutStringAt(X0+FIELD_WIDTH+2, y, "PAUSE");
    }else
        PutStringAt(X0+FIELD_WIDTH+2, y, "          ");
}

#ifdef EBUG
static void showcur(){
    uint8_t *targ = currentLevel;
    const char *brk;
    for(int y = 0; y < FIELD_ACTIVE; ++y){
        for(int x = 0; x < (FIELD_WIDTH / BRICK_WIDTH); ++x){
            uint8_t b = *targ++;
            switch(b & COLOR_MASK){
                case 0:
                    brk = "___";
                break;
                case 1:
                    brk = "XXX";
                break;
                case 2:
                    brk = "_VV";
                break;
                case 3:
                    brk = "_WW";
                break;
                case 4:
                    brk = "_FL";
                break;
                case 5:
                    brk = "_EN";
                break;
                case 6:
                    brk = "_CA";
                break;
                case 7:
                    brk = "_SL";
                break;
                case 8:
                    brk = "_DI";
                break;
                default:
                    brk = "????";
                break;
            }
            USB_send(brk);
            if(b & BRICK_IS_BONUS) USB_send("+");
            else USB_send("_");
            USB_send(" ");
        }
        USB_send("\n");
    }
}
#else
#define showcur()
#endif

static void draw_level(){
    if(curLevNo > LEVELS_AMOUNT) curLevNo = LEVELS_AMOUNT;
    uint8_t *brickz = levels[curLevNo], *targ = currentLevel;
    for(int y = 0; y < FIELD_ACTIVE; ++y){
        for(int x = 0; x < (FIELD_WIDTH / BRICK_WIDTH); ++x){
            uint8_t cur = *brickz++, curcol = cur & COLOR_MASK;
            if((curcol > BRICK_WHITE) && getRand() % 100 < BONUS_PROBABILITY) cur |= BRICK_IS_BONUS;
            *targ++ = cur; // copy brick data into level
            if(!cur) continue;
            uint8_t curclolr = brickcolors[curcol];
            int X = X0 + x*BRICK_WIDTH;
            DrawPix(X+0, y, curclolr);
            DrawPix(X+1, y, curclolr);
            DrawPix(X+2, y, curclolr);
            DBG("Brick "); DBGU(cur); DBG(" @ "); DBGU(x); DBG(" , "); DBGU(y); NL();
            if(cur != BRICK_WALL) ++Nbricks;
        }
    }
    showcur();
    for(int y = 0; y < SCREEN_HEIGHT; ++y){
        DrawPix(X0-1, y, CUP_COLOR);
        DrawPix(X0+FIELD_WIDTH, y, CUP_COLOR);
    }
    showText();
}

// x is pixel coordinate in field system
static void draw_racket(int x, uint8_t color){
    int xl = X0 + x - racketW / 2;
    for(int i = 0; i < racketW; ++i){
        DrawPix(xl + i, RACKET_Y, color);
    }
    //DBG("Racket @"); DBGU(x); DBG(" colr "); DBGU(color); DBG(" spd "); DBGU(racketspeed); NL();
}

static void process_racket(int dt){
    if(!racketspeed) return;
    oldracketspeed = racketspeed;
    int xnew = racketX + racketspeed * dt;
    int X = fixed2i(xnew), Xo = fixed2i(racketX);
    if(X == Xo){
        racketX = xnew;
        return;
    }
    int xmax = FIELD_WIDTH - 1 - racketW/2;
    if(X < racketW/2){
        X = racketW/2;
        xnew = i2fixed(X);
        racketspeed = 0;
        //DBG("Left border\n");
    }else if(X > xmax){
        X = xmax;
        xnew = i2fixed(X);
        racketspeed = 0;
        //DBG("Right border\n");
    }else{
        draw_racket(Xo, BACKGROUND_COLOR); // clear racket
        draw_racket(X, RACKET_COLOR);
    }
    racketX = xnew;
}

static void startlevl(){
    racketW = RACKET_MEDIUM;
    DBG("wid: "); DBGU(racketW); NL();
    racketX = i2fixed(FIELD_WIDTH / 2);
    racketspeed = 0;
    oldracketspeed = 0;
    floor_activated = 0;
    Nballs = 1;
    for(int i = 1; i < NBALLSMAX; ++i) balls[i].flags = 0;
    int rx = fixed2i(racketX);
    balls[0] = (theball){.x = racketX, .y = i2fixed(RACKET_Y-1), .flags = STICKY_BALL|ACTIVE_BALL};
    draw_racket(rx, RACKET_COLOR);
    DrawPix(X0+rx, RACKET_Y-1, BALL_COLOR);
    Tlast = Tms;
    showText(); // refresh Nlives text
    clear_events();
}

// The ball gets the active field cell?
// @return 0 if ball falled out
static int process_ball(theball *ball, int dt){ // dt is time since last check, ms
    if(ball->flags & STICKY_BALL){ // ball follows the racket
        int Xold = fixed2i(ball->x), Xnew = fixed2i(racketX);
        ball->y = i2fixed(RACKET_Y - 1);
        ball->x = racketX;
        if(Xold != Xnew){ // redraw ball
            DrawPix(X0 + Xold, RACKET_Y - 1, BACKGROUND_COLOR);
            DrawPix(X0 + Xnew, RACKET_Y - 1, BALL_COLOR);
        }
        return 1;
    }
    int DX = ball->xspeed * dt, DY = ball->yspeed * dt;
    int xnew = ball->x + DX;
    int ynew = ball->y + DY;
    // get integer coordinates (in field's system!)
    int X = fixed2i(xnew), Y = fixed2i(ynew);
    int Xo = fixed2i(ball->x), Yo = fixed2i(ball->y);
    if(Xo == X && Yo == Y){ // still the old cell
        ball->x = xnew;
        ball->y = ynew;
        return 1;
    }
    // ball is in new cell; delete old
    DrawPix(X0 + Xo, Yo, BACKGROUND_COLOR);
    //DBG("Del ball @ "); DBGU(Xo); DBG(", "); DBGU(Yo); NL();
    if(X < 0){ // left border
        DBG("Ball: left border\n");
        xnew = DX; X = 0;
        ball->xspeed = -ball->xspeed;
    }else if(X > FIELD_WIDTH - 1){ // right border
        DBG("Ball: right border\n");
        X = FIELD_WIDTH-1;
        xnew = i2fixed(FIELD_WIDTH) - DX;
        ball->xspeed = -ball->xspeed;
    }else if(Y < 0){ // upper border
        DBG("Ball: upper border\n");
        ynew = DY; Y = 0;
        ball->yspeed = -ball->yspeed;
    }else if(Y > RACKET_Y){ // fall into the depth
        if(!floor_activated){
            DBG("Ball: drop down\n");
            return 0;
        }
        ynew = i2fixed(RACKET_Y) - DY; Y = RACKET_Y;
        ball->yspeed = -ball->yspeed;
        DBG("Ball: floor\n");
    }else if(Y == RACKET_Y){ // check the racket
        DBG("check the racket\n");
        int w2 = i2fixed(racketW) / 2;
        if(xnew >= racketX - w2 && xnew <= racketX + w2){ // got the racket
            int absrs = (oldracketspeed > 0) ? oldracketspeed : -oldracketspeed;
            ynew = i2fixed(RACKET_Y) - DY; Y = RACKET_Y - 1;
            ball->xspeed += oldracketspeed / XSPDFRAC;
            if(ball->xspeed > BALL_MAX_X_SPEED) ball->xspeed = BALL_MAX_X_SPEED;
            else if(ball->xspeed < -BALL_MAX_X_SPEED) ball->xspeed = -BALL_MAX_X_SPEED;
            if(absrs){
                ball->yspeed += absrs/YSPDFRAC;
                if(ball->yspeed > BALL_MAX_Y_SPEED) ball->yspeed = BALL_MAX_Y_SPEED;
            }/* else{
                --ball->yspeed;
                if(ball->yspeed < BALL_MIN_Y_SPEED) ball->yspeed = BALL_MIN_Y_SPEED;
            }*/
            DBG("Ball: got racket, racketspeed="); DBGU(absrs);
            DBG(" ballspeed: "); DBGU(ball->xspeed); DBG(", "); DBGU(ball->yspeed); NL();
            ball->yspeed = -ball->yspeed; // change ball direction
        }
    }else if(Y < FIELD_ACTIVE){ // ball is in brick zone, check hits
        int idx = (FIELD_WIDTH * Y + X) / BRICK_WIDTH;
        uint8_t cur = currentLevel[idx];
        if(cur){
            DBG("brick y="); DBGU(Y); DBG(", x="); DBGU(X); DBG(", idx="); DBGU(idx); DBG(", cur="); DBGU(cur); NL();
        /*
         * Ball hits the brick, process this
         * TODO: DONT FORGET ABOUT BONUSES!!!
         */
            if(cur != BRICK_WALL){ // brick can be broken
                // delete brick
                DBG("Now delete ["); DBGU(idx); DBG("]="); DBGU(currentLevel[idx]); NL();
                currentLevel[idx] = 0;
                //showcur();
                int sX = X/BRICK_WIDTH; sX *= BRICK_WIDTH; sX += X0;
                DBG("sx="); DBGU(sX); NL();
                DrawPix(sX+0, Y, BACKGROUND_COLOR);
                DrawPix(sX+1, Y, BACKGROUND_COLOR);
                DrawPix(sX+2, Y, BACKGROUND_COLOR);
                /* process bonuses here */
                score += (cur & COLOR_MASK) + 1;
                if(--Nbricks == 0){
                    if(++curLevNo >= LEVELS_AMOUNT){
                        DBG("No more levels, you win!\n");
                        return 0; // no more levels
                    }
                    // init next level
                    ClearScreen();
                    draw_level();
                    startlevl();
                    return 1;
                }
                showText();
            }
            // check from which side the ball came and change direction
            if(Xo != X && Yo == Y) ball->xspeed = -ball->xspeed;
            else if(Yo != Y && Xo == X) ball->yspeed = -ball->yspeed;
            else{ball->xspeed = -ball->xspeed; ball->yspeed = -ball->yspeed; }
            X = Xo; Y = Yo; xnew = ball->x; ynew = ball->y;
        }
    }
    ball->x = xnew; ball->y = ynew;
    DrawPix(X0 + X, Y, BALL_COLOR);
    //DBG("Draw ball @ "); DBGU(X); DBG(", "); DBGU(Y); NL();
    return 1;
}

static void fire_a_balls(){ // run sticked balls
    for(int i = 0; i < NBALLSMAX; ++i){
        if(balls[i].flags & STICKY_BALL){ // this ball is alive
            DBG("run a ball, racket speed = "); DBGU(oldracketspeed); NL();
            balls[i].flags &= ~STICKY_BALL;
            balls[i].xspeed = oldracketspeed / XSPDFRAC;
            int absrs = (oldracketspeed > 0) ? oldracketspeed : -oldracketspeed;
            balls[i].yspeed = -BALL_NORMAL_SPEED - absrs/YSPDFRAC;
            if(balls[i].yspeed < -BALL_MAX_Y_SPEED) balls[i].yspeed = -BALL_MAX_Y_SPEED;
            if(balls[i].xspeed > BALL_MAX_X_SPEED) balls[i].xspeed = BALL_MAX_X_SPEED;
            else if(balls[i].xspeed < -BALL_MAX_X_SPEED) balls[i].xspeed = -BALL_MAX_X_SPEED;
        }
    }
}

void arkanoid_init(){
    setBGcolor(BACKGROUND_COLOR);
    setFGcolor(FOREGROUND_COLOR);
    choose_font(FONTN8);
    ClearScreen();
    ScreenON();
    score = 0;
    Nlives = LIVES_AT_START;
    curLevNo = 0;
    draw_level();
    startlevl();
}

/*
 * BUTTONS:
 * left/right - move racket
 * set - fire a ball
 * up - pause/resume
 * menu - go to menu
 */

int arkanoid_process(){
    static uint32_t Tracketspeed = 0;
    if(Tms == Tlast) return 1;
    int dt = (int)(Tms - Tlast);
    Tlast = Tms;
    // check buttons
    keyevent evt;
#define PRESS(x)    (keystate(x, &evt) && evt == EVT_PRESS)
#define HOLD(x)     (keyevt(x) == EVT_HOLD)
#define RELEASE(x)  (keyevt(x) == EVT_RELEASE)
    if(PRESS(KEY_U)){ // UP - pause
        if(paused){
            DBG("---->  Arkanoid resume\n");
            paused = 0;
            showText();
        }else{
            DBG("---->  Arkanoid paused\n");
            paused = 1;
            showcur();
            showText();
        }
    }
    if(paused){
        clear_events();
        return 1;
    }
    uint8_t released = 0;
    if(PRESS(KEY_L)) racketspeed = -FIXPOINT; // single move @1pt
    else if(HOLD(KEY_L)) racketspeed += -D_RACKET_SPEED;
    else released = 1;
    if(PRESS(KEY_R)) racketspeed = FIXPOINT; // single move @1pt
    else if(HOLD(KEY_R)) racketspeed += D_RACKET_SPEED;
    else ++released;
    if(2 == released){
        if(racketspeed){
            Tracketspeed = Tms;
            racketspeed = 0;
            DBG("Racket stops @"); DBGU(oldracketspeed); NL();
        }else{
            if(Tms - Tracketspeed > KEEPXSPEEDTIME){
#ifdef EBUG
                if(oldracketspeed) USB_send("oldracspd=0\n");
#endif
                oldracketspeed = 0;
            }
        }
    }
    if(PRESS(KEY_S)){
        fire_a_balls();
    }
    if(keystate(KEY_M, &evt) && evt == EVT_HOLD){ // break the game
        return 0;
    }
#undef PRESS
#undef HOLD
    process_racket(dt);
    for(int i = 0; i < NBALLSMAX; ++i){
        if(balls[i].flags & ACTIVE_BALL){ // this ball is alive
            if(!process_ball(&balls[i], dt)){ // ball falls out
                balls[i].flags = 0;
                --Nballs;
                DBG("--balls: "); DBGU(Nballs); NL();
            }
        }
    }
    if(Nballs == 0){ // --lives
        DBG("Ball falled\n");
        if(--Nlives == 0){
            DBG("no more lives\n");
            return 0; // game over
        }
        draw_racket(fixed2i(racketX), BACKGROUND_COLOR); // clear racket
        startlevl();
    }
    return 1;
}
