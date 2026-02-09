/*
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

#include <string.h> // memcpy
#include "ringbuffer.h"

static int datalen(ringbuffer *b){
    if(b->tail >= b->head) return (b->tail - b->head);
    else return (b->length - b->head + b->tail);
}

// stored data length
int RB_datalen(ringbuffer *b){
    if(b->busy) return -1;
    b->busy = 1;
    int l = datalen(b);
    b->busy = 0;
    return l;
}

static int hasbyte(ringbuffer *b, uint8_t byte){
    if(b->head == b->tail) return -1; // no data in buffer
    int startidx = b->head;
    if(b->head > b->tail){ //
        for(int found = b->head; found < b->length; ++found)
            if(b->data[found] == byte) return found;
        startidx = 0;
    }
    for(int found = startidx; found < b->tail; ++found)
        if(b->data[found] == byte) return found;
    return -1;
}

/**
 * @brief RB_hasbyte - check if buffer has given byte stored
 * @param b - buffer
 * @param byte - byte to find
 * @return index if found, -1 if none or busy
 */
int RB_hasbyte(ringbuffer *b, uint8_t byte){
    if(b->busy) return -1;
    b->busy = 1;
    int ret = hasbyte(b, byte);
    b->busy = 0;
    return ret;
}
/*
// poor memcpy
static void mcpy(uint8_t *targ, const uint8_t *src, int l){
    while(l--) *targ++ = *src++;
}*/

// increment head or tail
TRUE_INLINE void incr(ringbuffer *b, volatile int *what, int n){
    *what += n;
    if(*what >= b->length) *what -= b->length;
}

static int read(ringbuffer *b, uint8_t *s, int len){
    int l = datalen(b);
    if(!l) return 0;
    if(l > len) l = len;
    int _1st = b->length - b->head;
    if(_1st > l) _1st = l;
    if(_1st > len) _1st = len;
    //mcpy(s, b->data + b->head, _1st);
    memcpy(s, b->data + b->head, _1st);
    if(_1st < len && l > _1st){
        //mcpy(s+_1st, b->data, l - _1st);
        memcpy(s+_1st, b->data, l - _1st);
        incr(b, &b->head, l);
        return l;
    }
    incr(b, &b->head, _1st);
    return _1st;
}

/**
 * @brief RB_read - read data from ringbuffer
 * @param b - buffer
 * @param s - array to write data
 * @param len - max len of `s`
 * @return bytes read or -1 if busy
 */
int RB_read(ringbuffer *b, uint8_t *s, int len){
    if(b->busy) return -1;
    b->busy = 1;
    int r = read(b, s, len);
    b->busy = 0;
    return r;
}

static int readto(ringbuffer *b, uint8_t byte, uint8_t *s, int len){
    int idx = hasbyte(b, byte);
    if(idx < 0) return 0;
    int partlen = idx + 1 - b->head;
    // now calculate length of new data portion
    if(idx < b->head) partlen += b->length;
    if(partlen > len) return -read(b, s, len);
    return read(b, s, partlen);
}

/**
 * @brief RB_readto fill array `s` with data until byte `byte` (with it)
 * @param b - ringbuffer
 * @param byte - check byte
 * @param s - buffer to write data
 * @param len - length of `s`
 * @return amount of bytes written (negative, if len<data in buffer or buffer is busy)
 */
int RB_readto(ringbuffer *b, uint8_t byte, uint8_t *s, int len){
    if(b->busy) return -1;
    b->busy = 1;
    int n = readto(b, byte, s, len);
    b->busy = 0;
    return n;
}

static int write(ringbuffer *b, const uint8_t *str, int l){
    int r = b->length - 1 - datalen(b); // rest length
    if(l > r || !l) return 0;
    int _1st = b->length - b->tail;
    if(_1st > l) _1st = l;
    //mcpy(b->data + b->tail, str, _1st);
    memcpy(b->data + b->tail, str, _1st);
    if(_1st < l){ // add another piece from start
        //mcpy(b->data, str+_1st, l-_1st);
        memcpy(b->data, str+_1st, l-_1st);
    }
    incr(b, &b->tail, l);
    return l;
}

/**
 * @brief RB_write - write some data to ringbuffer
 * @param b - buffer
 * @param str - data
 * @param l - length
 * @return amount of bytes written or -1 if busy
 */
int RB_write(ringbuffer *b, const uint8_t *str, int l){
    if(b->busy) return -1;
    b->busy = 1;
    int w = write(b, str, l);
    b->busy = 0;
    return w;
}

// just delete all information in buffer `b`
int RB_clearbuf(ringbuffer *b){
    if(b->busy) return -1;
    b->busy = 1;
    b->head = 0;
    b->tail = 0;
    b->busy = 0;
    return 1;
}
