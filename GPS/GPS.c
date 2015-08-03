/*
 * GPS.c
 *
 * Copyright 2015 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "main.h"
#include "GPS.h"
#include "uart.h"

#define GPS_endline() do{GPS_send_string((uint8_t*)"\r\n");}while(0)
#define U(arg)  ((uint8_t*)arg)

gps_status GPS_status = GPS_WAIT;

void GPS_send_string(uint8_t *str){
	while(*str)
		fill_uart_buff(USART2, *str++);
}

int strncmp(const uint8_t *one, const uint8_t *two, int n){
	int diff = 0;
	do{
		diff = (int)(*one++) - (int)(*two++);
	}while(--n && diff == 0);
	return diff;
}

uint8_t *ustrchr(uint8_t *str, uint8_t symbol){
//	uint8_t *ptr = str;
	do{
		if(*str == symbol) return str;
	}while(*(++str));
	return NULL;
}

uint8_t hex(uint8_t n){
	return ((n < 10) ? (n+'0') : (n+'A'-10));
}

/**
 * Check checksum
 */
int checksum_true(uint8_t *buf){
	uint8_t *eol;
	uint8_t checksum = 0, cs[3];
	if(*buf != '$' || !(eol = ustrchr(buf, '*'))){
		DBG("Wrong data: ");
		DBG(buf);
		DBG("\n");
		return 0;
	}
	while(++buf != eol)
		checksum ^= *buf;
	++buf;
	cs[0] = hex(checksum >> 4);
	cs[1] = hex(checksum & 0x0f);
	if(buf[0] == cs[0] && buf[1] == cs[1])
		return 1;
#ifdef EBUG
	cs[2] = 0;
	P("CHS, get ");
	P(buf);
	P(" need ");
	P(cs);
	usb_send('\n');
#endif
	return 0;
}

void send_chksum(uint8_t chs){
	fill_uart_buff(USART2, hex(chs >> 4));
	fill_uart_buff(USART2, hex(chs & 0x0f));
}
/**
 * Calculate checksum & write message to port
 * @param  buf - command to write (with leading $ and trailing *)
 * return 0 if fails
 */
void write_with_checksum(uint8_t *buf){
	uint8_t checksum = 0;
	GPS_send_string(buf);
	++buf; // skip leaders
	do{
		checksum ^= *buf++;
	}while(*buf && *buf != '*');
	send_chksum(checksum);
	GPS_endline();
}

/**
 * set rate for given NMEA field
 * @param field - name of NMEA field
 * @param rate  - rate in seconds (0 disables field)
 * @return -1 if fails, rate if OK
 */
void block_field(const uint8_t *field){
	uint8_t buf[22];
	memcpy(buf, U("$PUBX,40,"), 9);
	memcpy(buf+9, field, 3);
	memcpy(buf+12, U(",0,0,0,0*"), 9);
	buf[21] = 0;
	write_with_checksum(buf);
}


/**
 * Send starting sequences (get only RMC messages)
 */
void GPS_send_start_seq(){
	const uint8_t *GPmsgs[5] = {U("GSV"), U("GSA"), U("GGA"), U("GLL"), U("VTG")};
	int i;
	for(i = 0; i < 5; ++i)
		block_field(GPmsgs[i]);
}
/*
uint8_t *nextpos(uint8_t **buf, int pos){
	int i;
	if(pos < 1) pos = 1;
	for(i = 0; i < pos; ++i){
		*buf = ustrchr(*buf, ',');
		if(!*buf) break;
		++(*buf);
	}
	return *buf;
}
#define NEXT()      do{if(!nextpos(&buf, 1)) goto ret;}while(0)
#define SKIP(NPOS)  do{if(!nextpos(&buf, NPOS)) goto ret;}while(0)
*/
/**
 * Parse answer from GPS module
 *
 * Recommended minimum specific GPS/Transit data
 * $GPRMC,hhmmss,status,latitude,N,longitude,E,spd,cog,ddmmyy,mv,mvE,mode*cs
 * 1    = UTC of position fix
 * 2    = Data status (V=navigation receiver warning)
 * 3    = Latitude of fix
 * 4    = N or S
 * 5    = Longitude of fix
 * 6    = E or W
 * 7    = Speed over ground in knots
 * 8    = Cource over ground in degrees
 * 9    = UT date
 * 10   = Magnetic variation degrees (Easterly var. subtracts from true course)
 * 11   = E or W
 * 12   = Mode: N(bad), E(approx), A(auto), D(diff)
 * 213457.00,A,4340.59415,N,04127.47560,E,2.494,,290615,,,A*7B
 */
void GPS_parse_answer(uint8_t *buf){
	uint8_t *ptr;
	if(strncmp(buf+3, U("RMC"), 3)){ // not RMC message
		GPS_send_start_seq();
		return;
	}
	if(!checksum_true(buf)){
		DBG("Wrong chs\n");
		return; // wrong checksum
	}
	buf += 7; // skip header
	if(*buf == ','){ // time unknown
		GPS_status = GPS_WAIT;
		return;
	}
	//P("time: ");
	ptr = ustrchr(buf, ',');
	*ptr++ = 0;
	//P(buf);
	if(*ptr == 'A')
		GPS_status = GPS_VALID;
	else
		GPS_status = GPS_NOT_VALID;
	print_curtime();
	set_time(buf);
//	buf = ustrchr(ptr, ',');
//	P(" ");
	P("\n");
}

/*

void rmc(uint8_t *buf){
	//DBG("rmc: %s\n", buf);
	int H, M, LO, LA, d, m, y, getdate = 0;
	double S, longitude, lattitude, speed, track, mag;
	char varn = 'V', north = '0', east = '0', mageast = '0', mode = 'N';
	sscanf((char*)buf, "%2d%2d%lf", &H, &M, &S);
	NEXT();
	if(*buf != ',') varn = *buf;
	if(varn != 'A')
		PRINT("(data could be wrong)");
	else{
		PRINT("(data valid)");
		if(GP->date) getdate = 1; // as only we have valid data we show it to user
	}
	PRINT(" time: %02d:%02d:%05.2f", H, M, S);
	PRINT(" timediff: %g", timediff(H, M, S));
	NEXT();
	sscanf((char*)buf, "%2d%lf", &LA, &lattitude);
	NEXT();
	if(*buf != ','){
		north = *buf;
		lattitude = (double)LA + lattitude / 60.;
		if(north == 'S') lattitude = -lattitude;
		PRINT(" latt: %g", lattitude);
		Latt_mean += lattitude;
		Latt_sq += lattitude*lattitude;
		++Latt_N;
	}
	NEXT();
	sscanf((char*)buf, "%3d%lf", &LO, &longitude);
	NEXT();
	if(*buf != ','){
		east = *buf;
		longitude = (double)LO + longitude / 60.;
		if(east == 'W') longitude = -longitude;
		PRINT(" long: %g", longitude);
		Long_mean += longitude;
		Long_sq += longitude*longitude;
		++Long_N;
	}
	NEXT();
	if(*buf != ','){
		sscanf((char*)buf, "%lf", &speed);
		PRINT(" speed: %gknots", speed);
	}
	NEXT();
	if(*buf != ','){
		sscanf((char*)buf, "%lf", &track);
		PRINT(" track: %gdeg,True", track);
	}
	NEXT();
	if(sscanf((char*)buf, "%2d%2d%2d", &d, &m, &y) == 3)
		PRINT(" date(dd/mm/yy): %02d/%02d/%02d", d, m, y);
	if(getdate) show_date(H,M,S,d,m,y); // show date & exit
	NEXT();
	sscanf((char*)buf, "%lf,%c", &mag, &mageast);
	if(mageast == 'E' || mageast == 'W'){
		if(mageast == 'W') mag = -mag;
		PRINT(" magnetic var: %g", mag);
	}
	SKIP(2);
	if(*buf != ','){
		mode = *buf;
		PRINT(" mode: %c", mode);
	}
ret:
	PRINT("\n");
}
*/
