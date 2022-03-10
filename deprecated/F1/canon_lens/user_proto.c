/*
 * user_proto.c
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "cdcacm.h"
#include "main.h"
#include "hardware_ini.h"
#include "spi.h"

void help(){
	P("c\tmax close diaphragm\n");
	P("D\tdiaphragm 10+\n");
	P("d\tdiaphragm 10-\n");
	P("F\tmove F to 127 steps+\n");
	P("f\tmove F to 127 steps-\n");
	P("h\tshow this help\n");
	P("o\tmax open diaphragm\n");
	P("r\t(or empty line) repeat last command sequence\n");
	P("v\tshow focus value (in steps)\n");
	P("+\tgo to infinity\n");
	P("-\tgo to minimal F\n");
}

#define UBUFF_LEN (64)
uint8_t ubuff[UBUFF_LEN];
int ubuff_sz = 0;

// read unsigned char in oct,dec or hex format; if success, return 0; otherwice return 1
// modify `buf` to point at last non-digit symbol
int read_uchar(uint8_t **buf, uint8_t *eol){
	uint8_t *start = *buf;
	uint32_t readed = 0;
	if(*start == '0'){ // hex or oct
		if(start[1] == 'x'){ // hex
			start += 2;
			while(start < eol){
				char ch = *start, m = 0;
				if(ch >= '0' && ch <= '9') m = '0';
				else if(ch >= 'a' && ch <= 'f') m = 'a' - 10;
				else if(ch >= 'A' && ch <= 'F') m = 'A' - 10;
				else break;
				if(m){
					readed <<= 4;
					readed |= ch - m;
				}
				++start;
			}
		}else{ // oct
			++start;
			while(start < eol){
				char ch = *start;
				if(ch >= '0' && ch < '8'){
					readed <<= 3;
					readed |= ch - '0';
				}else break;
				++start;
			}
		}
	}else{ // dec
		while(start < eol){
				char ch = *start;
				if(ch >= '0' && ch <= '9'){
					readed *= 10;
					readed += ch - '0';
				}else break;
				++start;
			}
	}
	if(readed > 255){
		P("bad value!");
		return 1;
	}
	ubuff[ubuff_sz++] = (uint8_t) readed;
	*buf = start;
	return 0;
}

static const uint8_t gotoplus[]  = {0, 10, 7, 5, 0};
static const uint8_t gotominus[] = {0, 10, 7, 6, 0};
static const uint8_t diaopen[] = {0, 10, 19, 128, 0};
static const uint8_t diaclose[] = {0, 10, 19, 127, 0};
static const uint8_t diap[] = {0, 10, 19, ~10, 8, 0};
static const uint8_t diam[] = {0, 10, 19, 10, 8, 0};
static const uint8_t fmovp[] = {0, 10, 7, 68, 0, 127, 0};
static const uint8_t fmovm[] = {0, 10, 7, 68, 255, 128, 0};


void write_buff(uint8_t *buf, int len){
	int i;
	for(i = 0; i < len; ++i){
		uint8_t rd = spi_write_byte(buf[i]);
		print_int(i); P(": written "); print_int(buf[i]);
		P(" readed "); print_int(rd); newline();
		Delay(2);
	}
}

void showFval(){
	int16_t val;
	uint8_t rd = 0, *b = (uint8_t*)&val;
	int i;
	for(i = 0; i < 3 && rd != 0xaa; ++i){
		spi_write_byte(10);
		Delay(2);
		rd = spi_write_byte(0);
		Delay(5);
	}
	if(rd != 0xaa){
		P("Something wrong: lens don't answer!\n");
		return;
	}
	spi_write_byte(192);
	Delay(2);
	b[1] = spi_write_byte(0);
	Delay(2);
	b[0] = spi_write_byte(0);
	P("focus motor position: ");
	print_int(val);
	newline();
}

/**
 * parse command buffer buf with length len
 * fill uint8_t data buffer with readed data
 */
void parse_incoming_buf(){
	uint8_t *curch = usbdatabuf, *eol = usbdatabuf + usbdatalen;
	uint8_t ch = *curch, data = 0;
	if(usbdatalen == 2){
		switch(ch){
				case 'h': // help
					help();
				break;
				case 'r': // repeat last command
					write_buff(ubuff, ubuff_sz);
				break;
#define WRBUF(x)  do{write_buff((uint8_t*)x, sizeof(x));}while(0)
				case '+': // goto infty
					WRBUF(gotoplus);
				break;
				case '-': // goto 2.5m
					WRBUF(gotominus);
				break;
				case 'o': // open d
					WRBUF(diaopen);
				break;
				case 'c': // close d
					WRBUF(diaclose);
				break;
				case 'F': // f127+
					WRBUF(fmovp);
				break;
				case 'f': // f127-
					WRBUF(fmovm);
				break;
				case 'd':
					WRBUF(diam);
				break;
				case 'D':
					WRBUF(diap);
				break;
				case 'v':
					showFval();
				break;
			}
			empty_buf();
			return;
	}
	while(curch < eol){
		ch = *curch;
		if(ch >= '0' && ch <= '9'){
			if(!data){data = 1; ubuff_sz = 0;}
			if(read_uchar(&curch, eol)){
				ubuff_sz = 0;
				break;
			}
			continue;
		}
		++curch;
	}
	write_buff(ubuff, ubuff_sz);
	if(eol[-1] != '\n') P("Warning! Buffer overflow, some data may be corrupt!!!");
	empty_buf();
}

/**
 * Send char array wrd thru USB
 */
void prnt(uint8_t *wrd){
	if(!wrd) return;
	while(*wrd) usb_send(*wrd++);
}

/**
 * Print buff as hex values
 * @param buf - buffer to print
 * @param l   - buf length
 * @param s   - function to send a byte
 */
void print_hex(uint8_t *buff, uint8_t l){
	void putc(uint8_t c){
		if(c < 10)
			usb_send(c + '0');
		else
			usb_send(c + 'a' - 10);
	}
	usb_send('0'); usb_send('x'); // prefix 0x
	while(l--){
		putc(buff[l] >> 4);
		putc(buff[l] & 0x0f);
	}
}

/**
 * Print decimal integer value
 * @param N - value to print
 * @param s - function to send a byte
 */
void print_int(int32_t N){
	uint8_t buf[10], L = 0;
	if(N < 0){
		usb_send('-');
		N = -N;
	}
	if(N){
		while(N){
			buf[L++] = N % 10 + '0';
			N /= 10;
		}
		while(L--) usb_send(buf[L]);
	}else usb_send('0');
}

