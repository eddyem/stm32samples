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

// integer value given by user
static volatile int32_t User_value = 0;
enum{
	UVAL_START,		// user start to write integer value
	UVAL_ENTERED,	// value entered but not printed
	UVAL_BAD		// entered bad value
};
uint8_t Uval_ready = UVAL_BAD;

int read_int(char *buf, int cnt);

intfun I = NULL; // function to process entered integer

#define READINT() do{i += read_int(&buf[i+1], len-i-1);}while(0)

void help(){
	P("H\tshow this help\n");
	P("I\ttest entering integer value\n");
	P("T\tshow current approx. time\n");
	P("1\tswitch LED D1 state\n");
	P("2\tswitch LED D2 state\n");
}

/**
 * show entered integer value
 */
uint8_t show_int(int32_t v){
	newline();
	print_int(v);
	newline();
	return 0;
}

/**
 * parse command buffer buf with length len
 * return 0 if buffer processed or len if there's not enough data in buffer
 */
int parse_incoming_buf(char *buf, int len){
	uint8_t command;
	//uint32_t utmp;
	int i = 0;
	if(Uval_ready == UVAL_START){ // we are in process of user's value reading
		i += read_int(buf, len);
	}
	if(Uval_ready == UVAL_ENTERED){
		//print_int(User_value); // printout readed integer value for error control
		Uval_ready = UVAL_BAD; // clear Uval
		I(User_value);
		return 0;
	}
	for(; i < len; i++){
		command = buf[i];
		if(!command) continue; // omit zero
		switch (command){
			case '1':
				gpio_toggle(LEDS_PORT, LED_D1_PIN);
			break;
			case '2':
				gpio_toggle(LEDS_PORT, LED_D2_PIN);
			break;
			case 'H': // show help
				help();
			break;
			case 'I': // enter integer & show its value
				I = show_int;
				READINT();
			break;
			case 'T':
				newline();
				print_int(Timer); // be careful for Time >= 2^{31}!!!
				newline();
			break;
			case '\n': // show newline, space and tab as is
			case '\r':
			case ' ':
			case '\t':
			break;
			default:
				command = '?'; // echo '?' on unknown command in byte mode
		}
		usb_send(command); // echo readed byte
	}
	return 0; // all data processed - 0 bytes leave in buffer
}

/**
 * Send char array wrd thru USB or UART
 */
void prnt(uint8_t *wrd){
	if(!wrd) return;
	while(*wrd) usb_send(*wrd++);
}

/**
 * Read from TTY integer value given by user (in DEC).
 *    Reading stops on first non-numeric symbol.
 *    To work with symbol terminals reading don't stops on buffer's end,
 *    it waits for first non-numeric char.
 *    When working on string terminals, terminate string by '\n', 0 or any other symbol
 * @param buf - buffer to read from
 * @param cnt - buffer length
 * @return amount of readed symbols
 */
int read_int(char *buf, int cnt){
	int readed = 0, i;
	static int enteredDigits; // amount of entered digits
	static int sign; // sign of readed value
	if(Uval_ready){ // this is first run
		Uval_ready = UVAL_START; // clear flag
		enteredDigits = 0; // 0 digits entered
		User_value = 0; // clear value
		sign = 1; // clear sign
	}
	if(!cnt) return 0;
	for(i = 0; i < cnt; i++, readed++){
		uint8_t chr = buf[i];
		if(chr == '-'){
			if(enteredDigits == 0){ // sign should be first
				sign = -1;
				continue;
			}else{ // '-' after number - reject entered value
				Uval_ready = UVAL_BAD;
				break;
			}
		}
		if(chr < '0' || chr > '9'){
			if(enteredDigits)
				Uval_ready = UVAL_ENTERED;
			else
				Uval_ready = UVAL_BAD; // bad symbol
			break;
		}
		User_value = User_value * 10 + (int32_t)(chr - '0');
		enteredDigits++;
	}
	if(Uval_ready == UVAL_ENTERED) // reading has met an non-numeric character
		User_value *= sign;
	return readed;
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

