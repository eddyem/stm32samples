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
#include "balls.h"
#include "screen.h"

#include "proto.h"
#include "usb.h"

extern uint32_t volatile Tms;

// fraction of 1 for fixed point
#define FIXPOINT    (10000)
// max pause in ms to add next ball
#define MAXPAUSEADDBALL (5000)
// max speed in fractions per millisecond
#define SPEEDMAX    (300)
// max balls amount
#define NBALLSMAX   (20)

// coordinates/speeds in fractions
static int
    x[NBALLSMAX], y[NBALLSMAX],
    xnew[NBALLSMAX], ynew[NBALLSMAX],
    xspeed[NBALLSMAX], yspeed[NBALLSMAX];
static uint8_t colr[NBALLSMAX];
static int Nballs = 0;
static uint32_t Tlast, Taddb;

static void clear_balls(){
    for(int i = 0; i < Nballs; ++i)
        DrawPix(x[i]/FIXPOINT, y[i]/FIXPOINT, 0);
}

static void draw_balls(){
    for(int i = 0; i < Nballs; ++i){
        x[i] = xnew[i]; y[i] = ynew[i];
        DrawPix(x[i]/FIXPOINT, y[i]/FIXPOINT, colr[i]);
    }
}

static void add_ball(){
    if(Nballs == NBALLSMAX) return;
    uint8_t c = getRand() & 0xff;
    if(c == 0) c = 1;
    colr[Nballs] = c;
    x[Nballs] = SCREEN_WIDTH*FIXPOINT/2;
    y[Nballs] = SCREEN_HEIGHT*FIXPOINT/2;
    xspeed[Nballs] = ((int)getRand()) % SPEEDMAX;
    yspeed[Nballs] = ((int)getRand()) % SPEEDMAX;
    ++Nballs;
    Taddb = Tms + getRand() % MAXPAUSEADDBALL;
}

void balls_init(){
    Nballs = 0;
    setBGcolor(0);
    ClearScreen();
    ScreenON();
    Tlast = Tms;
    add_ball();
}

//#define abs(x)  ((x<0) ? -x : x)

void process_balls(){
    if(Tms == Tlast) return;
    if(Nballs < NBALLSMAX && Taddb < Tms){
        add_ball();
    }
    int diff = (int)(Tms - Tlast);
    for(int i = 0; i < Nballs; ++i){
        xnew[i] = x[i] + xspeed[i]*diff;
        ynew[i] = y[i] + yspeed[i]*diff;
        inline void xrev(){xspeed[i] = -xspeed[i]; xnew[i] = x[i] + xspeed[i]*diff;}
        inline void yrev(){yspeed[i] = -yspeed[i]; ynew[i] = y[i] + yspeed[i]*diff;}
        if(xnew[i] < 0) xrev();
        else if(xnew[i] > SCREEN_WIDTH*FIXPOINT) xrev();
        if(ynew[i] < 0) yrev();
        else if(ynew[i] > SCREEN_HEIGHT*FIXPOINT) yrev();
        int nx = xnew[i]/FIXPOINT, ny = ynew[i]/FIXPOINT;
        int dx = nx - x[i]/FIXPOINT, dy = ny - y[i]/FIXPOINT;
        if((dx || dy) && GetPix(nx, ny)){ // change direction in case of collisions
            xrev(); yrev();
            xspeed[i] += getRand() & 3; // and randomly change speed
            yspeed[i] += getRand() & 3;
        }
    }
    clear_balls();
    draw_balls();
    Tlast = Tms;
}
