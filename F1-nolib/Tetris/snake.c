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
#include "snake.h"

#include "usb.h"
#include "proto.h"

#define SNAKE_MAXLEN        (128)
// colors                      RRRGGGBB
#define SNAKE_BGCOLOR       (0)
#define SNAKE_HEADCOLOR     (0b00001100)
#define SNAKE_COLOR         (0b00000100)
// food: +1 to size
#define FOOD_COLOR          (0b00001101)
// cut - -1 to size
#define CUT_COLOR           (0b00011100)
// chance of CUT appears  when drawing food (/1000)
#define CUT_PROMILLE        (80)
// score - +10 to score
#define SCORE_COLOR         (0b00100100)
// add this to score after each SCORE_COLOR eating
#define ADDSCORE            (25)
// chance of SCORE appears when doing move (1/1000)
#define SCORE_PROMILLE      (15)
// border
#define BORDER_COLOR        (0b00100000)
// max and min pause between steps
#define MAXSTEPMS           (199)
#define MINSTEPMS           (4)

extern uint32_t Tms;

typedef struct{
    uint8_t x;
    uint8_t y;
} point;

static point snake[SNAKE_MAXLEN];
static int8_t dirX, dirY;
static uint8_t snakelen;
static point Food;

static point RandCoords(){
    point pt = {0,0};
    for(int i = 0; i < 10000; ++i){ // not more than 10000 tries
        pt.x = 1 + getRand() % (SCREEN_WIDTH-2);
        pt.y = 1 + getRand() % (SCREEN_HEIGHT-2);
        if(GetPix(pt.x, pt.y) == SNAKE_BGCOLOR) break;
    };
    return pt;
}

// return 0 if failed
static int ThrowFood(){
    Food = RandCoords();
    if(Food.x == 0 && Food.y == 0) return 0;
    DrawPix(Food.x, Food.y, FOOD_COLOR);
    if(getRand() % 1000 < CUT_PROMILLE){
        point p = RandCoords();
        if(p.x == 0 && p.y == 0) return 1;
        DrawPix(p.x, p.y, CUT_COLOR);
    }
    return 1;
}
/*
static void draw_snake(){
    USB_send("\n\n\nDraw snake, len="); USB_send(u2str(snakelen)); USB_send("\n");
    DrawPix(snake[0].x, snake[0].y, SNAKE_HEADCOLOR);
    USB_send(u2str(snake[0].x)); USB_send("-"); USB_send(u2str(snake[0].y)); USB_send(" ");
    for(int i = 1; i < snakelen; ++i){
        DrawPix(snake[i].x, snake[i].y, SNAKE_COLOR);
        USB_send(u2str(snake[i].x)); USB_send("-"); USB_send(u2str(snake[i].y)); USB_send(" ");
    }
    USB_send("\n");

}*/

void snake_init(){
    setBGcolor(SNAKE_BGCOLOR);
    ClearScreen();
    ScreenON();
    snakelen = 1;
    snake[0].x = SCREEN_HW;
    snake[0].y = SCREEN_HH;
    // draw border
    for(int i = 0; i < SCREEN_WIDTH; ++i){
        DrawPix(i, 0, BORDER_COLOR);
        DrawPix(i, SCREEN_HEIGHT-1, BORDER_COLOR);
    }
    for(int i = 1; i < SCREEN_HEIGHT-1; ++i){
        DrawPix(0, i, BORDER_COLOR);
        DrawPix(SCREEN_WIDTH-1, i, BORDER_COLOR);
    }
    ThrowFood();
    // starting moving direction
    switch(getRand() % 4){
        case 0:
            dirX = 1; dirY = 0;
        break;
        case 1:
            dirX = -1; dirY = 0;
        break;
        case 2:
            dirX = 0; dirY = 1;
        break;
        case 3:
            dirX = 0; dirY = -1;
        break;
    }
    StepMS = MAXSTEPMS;
    Tlast = Tms; incSpd = 0;
    score = 0;
    DrawPix(snake[0].x, snake[0].y, SNAKE_HEADCOLOR);
    USB_send("Snake inited\n");
}

// return 0 if failed
static int move_snake(){
    int last = snakelen - 1;
    point tail = {snake[last].x, snake[last].y};
    DrawPix(Food.x, Food.y, FOOD_COLOR); // redraw food
    DrawPix(snake[0].x, snake[0].y, SNAKE_COLOR); // redraw head with body color
    DrawPix(tail.x, tail.y, SNAKE_BGCOLOR); // delete tail
    for(int i = snakelen-1; i > 0; --i){
        snake[i] = snake[i-1];
    }
    snake[0].x += dirX; snake[0].y += dirY;
    uint8_t nxtcolr = GetPix(snake[0].x, snake[0].y);
    DrawPix(snake[0].x, snake[0].y, SNAKE_HEADCOLOR); // and draw head
    if(nxtcolr != SNAKE_BGCOLOR){ // barrier
        USB_send("barrier: "); USB_send(u2str(nxtcolr)); USB_send("\n");
        if(nxtcolr == SNAKE_COLOR || nxtcolr == BORDER_COLOR){ // border - game over
            if(nxtcolr == SNAKE_COLOR) USB_send("slum into yourself\n");
            else USB_send("crush into border\n");
            return 0;
        }else if(nxtcolr == CUT_COLOR && snakelen > 1){ // make snake shorter
            --snakelen;
            DrawPix(snake[snakelen].x, snake[snakelen].y, SNAKE_BGCOLOR);
        }else if(nxtcolr == SCORE_COLOR){ // add score
            score += ADDSCORE;
        }
        // not border or snake -> FOOD?!
        if(snake[0].x == Food.x && snake[0].y == Food.y){ // increase snake len & speed & score
            score += snakelen;
            if(StepMS > MINSTEPMS) --StepMS;
            if(snakelen < SNAKE_MAXLEN){
                snake[snakelen] = tail;
                ++snakelen;
                DrawPix(tail.x, tail.y, SNAKE_COLOR);
            }
            if(!ThrowFood()) return 0;
            USB_send("Got food, score="); USB_send(u2str(score));
            USB_send(", len="); USB_send(u2str(snakelen));
            USB_send(", speed="); USB_send(u2str(StepMS));
            USB_send("\n");
        }
    }
    // check borders: they could be broken when `balls` activated
    if(snake[0].x >= SCREEN_WIDTH || snake[0].y >= SCREEN_HEIGHT) return 0;
    if(getRand() % 1000 < SCORE_PROMILLE){ // add ++score
        point pt = RandCoords();
        if(pt.x && pt.y)
            DrawPix(pt.x, pt.y, SCORE_COLOR);
    }
    return 1;
}

#define SETDIR(dx, dy)      do{dirY = dy; dirX = dx; incSpd = 0;}while(0)
#define INCRSPD()           do{if(StepMS > 5*MINSTEPMS){if(incSpd == 0) incSpd = MAXSTEPMS/5; else incSpd += incSpd / 5; \
                            if(StepMS < incSpd + 5*MINSTEPMS) incSpd = StepMS - 5*MINSTEPMS;}}while(0)
#define DECRSPD()           do{incSpd /= 2;}while(0)

// return 0 if game over
int snake_proces(){
    keyevent evt;
    static uint8_t paused = 0;
    // change moving direction
    if(keystate(KEY_U, &evt)){ // UP
        if(evt == EVT_PRESS){ if(dirY != 1) SETDIR(0, -1);}
    }
    if(keystate(KEY_D, &evt)){ // Down
        if(evt == EVT_PRESS){ if(dirY != -1) SETDIR(0, 1);}
    }
    if(keystate(KEY_L, &evt)){ // Left
        if(evt == EVT_PRESS){ if(dirX != 1) SETDIR(-1, 0);}
    }
    if(keystate(KEY_R, &evt) && (evt == EVT_PRESS || evt == EVT_HOLD)){ // Right
        if(evt == EVT_PRESS){ if(dirX != -1) SETDIR(1, 0);}
    }
    if(keystate(KEY_M, &evt) && (evt == EVT_PRESS || evt == EVT_HOLD)){ // Menu - Pause or out
        if(3 == ++paused){
            USB_send("Exit snake\n");
            clear_events();
            paused = 0;
            return 0;
        }
    }
    if(keystate(KEY_S, &evt) && (evt == EVT_PRESS || evt == EVT_HOLD)){ // Set - Pause
        if(paused){
            USB_send("Snake resume\n");
            paused = 0;
        }else{
            USB_send("Snake paused\n");
            paused = 1;
        }
    }
    clear_events();
    if(Tms - Tlast > StepMS - incSpd){ // move @ redraw
        Tlast = Tms;
        if(paused) return 1;
        if(keyevt(KEY_U) == EVT_HOLD){
            if(dirY == -1) INCRSPD();
            else DECRSPD();
            USB_send(u2str(incSpd)); USB_send("\n");
        }else if(keyevt(KEY_D) == EVT_HOLD){
            if(dirY == 1) INCRSPD();
            else DECRSPD();
            USB_send(u2str(incSpd)); USB_send("\n");
        }else if(keyevt(KEY_L) == EVT_HOLD){
            if(dirX == -1) INCRSPD();
            else DECRSPD();
            USB_send(u2str(incSpd)); USB_send("\n");
        }else if(keyevt(KEY_R) == EVT_HOLD){
            if(dirX == 1) INCRSPD();
            else DECRSPD();
            USB_send(u2str(incSpd)); USB_send("\n");
        }
        if(!move_snake()){
            USB_send("Snake game over\n");
            paused = 0;
            return 0;
        }
    }
    return 1;
}

