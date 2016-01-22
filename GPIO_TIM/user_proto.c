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
#include "timgpio.h"

/**
 * parce command buffer buf with length len
 * return 0 if buffer processed or len if there's not enough data in buffer
 */
void parse_incoming_buf(uint8_t *buf, int *len){
	static int lastidx = 0;
	int l = *len;
	for(; lastidx < l; ++lastidx){
		uint8_t cmd = buf[lastidx];
		usb_send(cmd);
		if(cmd == '\n'){
			timgpio_transfer(buf, *len);
			*len = 0;
			lastidx = 0;
			return;
		}
	}
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

