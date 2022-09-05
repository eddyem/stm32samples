/*
 * This file is part of the pl2303 project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f1.h>

#include "ringbuffer.h"

// ring buffer
static char ringbuffer[RBSIZE];
// head - position of first data byte
// tail - position of last data byte + 1
// head == tail - empty! So, buffer can't store more than RBSIZE-1 bytes of data!
static volatile int head = 0, tail = 0;

static int datalen(){
    if(tail >= head) return (tail - head);
    else return (RBSIZE - head + tail);
}
static int restlen(){
    return (RBSIZE - 1 - datalen());
}

static void mcpy(char *targ, const char *src, int l){
    while(l--) *targ++ = *src++;
}

TRUE_INLINE void incr(volatile int *what, int n){
    *what += n;
    if(*what >= RBSIZE) *what -= RBSIZE;
}

int RB_read(char s[BLOCKSIZE]){
    int l = datalen();
    if(!l) return 0;
    if(l > BLOCKSIZE) l = BLOCKSIZE;
    int _1st = RBSIZE - head;
    if(_1st > l) _1st = l;
    if(_1st > BLOCKSIZE) _1st = BLOCKSIZE;
    mcpy(s, ringbuffer+head, _1st);
    if(_1st < BLOCKSIZE && l > _1st){
        mcpy(s+_1st, ringbuffer, l-_1st);
        incr(&head, l);
        return l;
    }
    incr(&head ,_1st);
    return _1st;
}

int RB_write(const char *str, int l){
    int r = restlen();
    if(l > r) l = r;
    if(!l) return 0;
    int _1st = RBSIZE - tail;
    if(_1st > l) _1st = l;
    mcpy(ringbuffer+tail, str, _1st);
    if(_1st < l){ // add another piece from start
        mcpy(ringbuffer, str+_1st, l-_1st);
    }
    incr(&tail, l);
    return l;
}

