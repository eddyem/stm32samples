#pragma once

#include <stdint.h>
#include <string.h>

void hexdump(int (*sendfun)(const char *s), uint8_t *arr, uint16_t len);
char *u2str(uint32_t val);
char *i2str(int32_t i);
char *uhex2str(uint32_t val);
const char *getnum(const char *txt, uint32_t *N);
const char *omit_spaces(const char *buf);
const char *getint(const char *txt, int32_t *I);
//void mymemcpy(char *dest, const char *src, int len);
