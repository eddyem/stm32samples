#pragma once

#include <stm32f3.h>

typedef struct{
    uint8_t *data;      // data buffer
    const int length;   // its length
    int head;           // head index
    int tail;           // tail index
} ringbuffer;

int RB_read(ringbuffer *b, uint8_t *s, int len);
int RB_readto(ringbuffer *b, uint8_t byte, uint8_t *s, int len);
int RB_hasbyte(ringbuffer *b, uint8_t byte);
int RB_write(ringbuffer *b, const uint8_t *str, int l);
int RB_datalen(ringbuffer *b);
void RB_clearbuf(ringbuffer *b);
