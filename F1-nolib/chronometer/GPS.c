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

#include "GPS.h"
#include "hardware.h"
#include "time.h"
#include "usart.h"
#include "str.h"
#include <string.h> // memcpy

#define GPS_endline() do{usart_send(GPS_USART, "\r\n"); transmit_tbuf(GPS_USART); }while(0)
#define GPS_send_string(str) do{usart_send(GPS_USART, str);}while(0)

gps_status GPS_status = GPS_NOTFOUND;
int need2startseq = 1;

static uint8_t hex(uint8_t n){
    return ((n < 10) ? (n+'0') : (n+'A'-10));
}

/**
 * Check checksum
 */
static int checksum_true(const char *buf){
    char *eol;
    uint8_t checksum = 0, cs[3];
    if(*buf != '$' || !(eol = getchr(buf, '*'))){
        return 0;
    }
    while(++buf != eol)
        checksum ^= (uint8_t)*buf;
    ++buf;
    cs[0] = hex(checksum >> 4);
    cs[1] = hex(checksum & 0x0f);
    if(buf[0] == cs[0] && buf[1] == cs[1])
        return 1;
    return 0;
}

static void send_chksum(uint8_t chs){
    usart_putchar(GPS_USART, hex(chs >> 4));
    usart_putchar(1, hex(chs >> 4));
    usart_putchar(GPS_USART, hex(chs & 0x0f));
    usart_putchar(1, hex(chs & 0x0f));
}
/**
 * Calculate checksum & write message to port
 * @param  buf - command to write (without leading $ and trailing *)
 * return 0 if fails
 */
static void write_with_checksum(const char *buf){
    char *txt = NULL;
     // clear old buffer data
    for(int i = 0; i < 10000; ++i){
        if(usartrx(GPS_USART)){
            usart_getline(GPS_USART, &txt);
            DBG("Old data");
            GPS_parse_answer(txt);
            break;
        }
    }
    DBG("Send:");
    uint8_t checksum = 0;
    usart_putchar(GPS_USART, '$');
    usart_putchar(1, '$');
    GPS_send_string(buf);
    SEND(buf);
    do{
        checksum ^= *buf++;
    }while(*buf);
    usart_putchar(GPS_USART, '*');
    usart_putchar(1, '*');
    send_chksum(checksum);
    newline();
    GPS_endline();
}


/*
 * MTK fields format:
 *      $PMTKxxx,yyy,zzz*2E
 *          P - proprietary, MTK - always this, xxx - packet type, yyy,zzz - packet data
 * Packet types:
 * 220 - PMTK_SET_POS_FIX, data - position fix interval (msec, > 200)
 * 255 - PMTK_SET_SYNC_PPS_NMEA - turn on/off (def - off) PPS, data = 0/1 ->  "$PMTK255,1" turn ON
 * 285 - PMTK_SET_PPS_CONFIG - set PPS configuration, data fields:
 *      1st - 0-disable, 1-after 1st fix, 2-3D only, 3-2D/3D only, 4-always
 *      2nd - 2..998 - pulse width
 * 314 - PMTK_API_SET_NMEA_OUTPUT - set output messages, N== N fixes per output,
 *      order of messages: GLL,RMC,VTG,GGA,GSA,GSV,GRS,GST, only RMC per every pos fix:
 *      $PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
 * 386 - PMTK_API_SET_STATIC_NAV_THD speed threshold (m/s) for static navigation
 *      $PMTK386,1.5
 * ;
 */

/**
 * Send starting sequences (get only RMC messages)
 */
void GPS_send_start_seq(){
    DBG("Send start seq");
    // turn ON PPS:
    write_with_checksum("PMTK255,1");
    // set pulse width to 10ms with working after 1st fix
    write_with_checksum("PMTK285,1,10");
    // set only RMC:
    write_with_checksum("PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    // set static speed threshold
    write_with_checksum("PMTK386,1.5");
    need2startseq = 0;
}

/**
 * Parse answer from GPS module
 *
 * Recommended minimum specific GPS/Transit data
 * $GPRMC,hhmmss.sss,status,latitude,N,longitude,E,spd,cog,ddmmyy,mv,mvE,mode*cs
 * 1    = UTC of position fix
 * 2    = Data status (V=valid, A=invalid)
 * 3    = Latitude (ddmm.mmmm)
 * 4    = N or S
 * 5    = Longitude (dddmm.mmmm)
 * 6    = E or W
 * 7    = Speed over ground in knots
 * 8    = Cource over ground in degrees
 * 9    = UT date (ddmmyy)
 * 10   = Magnetic variation degrees (Easterly var. subtracts from true course)
 * 11   = E or W
 * 12   = Mode: N(bad), E(approx), A(auto), D(diff)
 * 213457.00,A,4340.59415,N,04127.47560,E,2.494,,290615,,,A*7B
 */
void GPS_parse_answer(const char *buf){
    char *ptr;
#if defined USART1PROXY
    usart_send(1, buf); newline();
#endif
    if(buf[1] == 'P') return; // answers to proprietary messages
    if(cmpstr(buf+3, "RMC", 3)){ // not RMC message
        need2startseq = 1;
        return;
    }
    if(!checksum_true(buf)){
        return; // wrong checksum
    }
    buf += 7; // skip header
    if(*buf == ','){ // time unknown
        GPS_status = GPS_WAIT;
        return;
    }
    ptr = getchr(buf, ',');
    if(!ptr ) return;
    *ptr++ = 0;
    if(*ptr == 'A'){
        GPS_status = GPS_VALID;
        set_time(buf);
    }else{
        GPS_status = GPS_NOT_VALID;
    }
}
