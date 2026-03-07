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

#include <string.h>
#include <stm32f0.h>
#include "ringbuffer.h"

#define CHK(b)  do{if(!b) return -1;}while(0)

static int datalen(ringbuffer *b){
    if(b->tail >= b->head) return (b->tail - b->head);
    else return (b->length - b->head + b->tail);
}

// stored data length
int RB_datalen(ringbuffer *b){
    CHK(b);
    if(0 == datalen(b)) return 0; // don't block for empty RO operations
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
    CHK(b);
    if(b->busy) return -1;
    b->busy = 1;
    int ret = hasbyte(b, byte);
    b->busy = 0;
    return ret;
}

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
    memcpy(s, b->data + b->head, _1st);
    if(_1st < len && l > _1st){
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
    CHK(b);
    if(!s || len < 1) return -1;
    if(0 == datalen(b)) return 0;
    if(b->busy) return -1;
    b->busy = 1;
    int r = read(b, s, len);
    b->busy = 0;
    return r;
}

// length of data from current position to `byte` (including byte)
static int lento(ringbuffer *b, uint8_t byte){
    int idx = hasbyte(b, byte);
    if(idx < 0) return 0;
    int partlen = idx + 1 - b->head;
    // now calculate length of new data portion
    if(idx < b->head) partlen += b->length;
    return partlen;
}

static int readto(ringbuffer *b, uint8_t byte, uint8_t *s, int len){
    int partlen = lento(b, byte);
    if(!partlen) return 0;
    if(partlen > len) return -1;
    return read(b, s, partlen);
}

/**
 * @brief RB_readto fill array `s` with data until byte `byte` (with it)
 * @param b - ringbuffer
 * @param byte - check byte
 * @param s - buffer to write data or NULL to clear data
 * @param len - length of `s` or 0 to clear data
 * @return amount of bytes written (negative, if len<data in buffer or buffer is busy)
 */
int RB_readto(ringbuffer *b, uint8_t byte, uint8_t *s, int len){
    CHK(b);
    if(!s || len < 1) return -1;
    if(0 == datalen(b)) return 0;
    if(b->busy) return -1;
    b->busy = 1;
    int n = 0;
    if(s && len > 0){
        n = readto(b, byte, s, len);
    }else{
        incr(b, &b->head, lento(b, byte)); // just throw data out
    }
    b->busy = 0;
    return n;
}

int RB_datalento(ringbuffer *b, uint8_t byte){
    CHK(b);
    if(0 == datalen(b)) return 0;
    if(b->busy) return -1;
    b->busy = 1;
    int n = lento(b, byte);
    b->busy = 0;
    return n;
}

// if l < rest of buffer, truncate and return actually written bytes
static int write(ringbuffer *b, const uint8_t *str, int l){
    int r = b->length - 1 - datalen(b); // rest length
    if(r < 1) return 0;
    if(l > r) l = r;
    int _1st = b->length - b->tail;
    if(_1st > l) _1st = l;
    memcpy(b->data + b->tail, str, _1st);
    if(_1st < l){ // add another piece from start
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
    CHK(b);
    if(!str || l < 1) return -1;
    if(b->length - datalen(b) < 2) return 0;
    if(b->busy) return -1;
    b->busy = 1;
    int w = write(b, str, l);
    b->busy = 0;
    return w;
}

// just delete all information in buffer `b`
int RB_clearbuf(ringbuffer *b){
    CHK(b);
    if(b->busy) return -1;
    b->busy = 1;
    b->head = 0;
    b->tail = 0;
    bzero(b->data, b->length);
    b->busy = 0;
    return 1;
}
