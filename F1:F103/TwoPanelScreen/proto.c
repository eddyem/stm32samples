/*
 * This file is part of the TETRIS project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "adcrandom.h"
#include "fonts.h"
#include "proto.h"
#include "screen.h"
#include "usb.h"
#include "version.inc"

#define USBBUFSZ            (127)
#define SCROLL_STEP_MS      (50)   // мс между сдвигами на 1 пиксель
#define TEXTBUF_SZ          (256)

extern uint32_t Tms;

// scrolling/still
static char     text_buf[TEXTBUF_SZ] = {0};
static uint16_t text_scroll = 0;         // scrolling amount in pixels
static uint8_t  text_mode   = 0;         // 0 - one string, 1 - two strings
static uint8_t  display_mode = 0;        // 0 - text, 1 - pixels blob
static uint8_t  text_fg = COLOR_WHITE;
static uint8_t  text_bg = COLOR_BLACK;
static uint32_t Tlast_scroll = 0;
static uint8_t  text_needs_redraw = 1;

// blob receiving
static int32_t  blob_remaining = 0;
static uint8_t *blob_ptr = NULL;

// received string
static char usb_line[USBBUFSZ + 1];
static int  usb_line_len = 0;

char *omit_spaces(const char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return (char*)buf;
}

// In case of overflow return `buf` and N==0xffffffff
// read decimal number & return pointer to next non-number symbol
static char *getdec(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '9'){
            break;
        }
        if(num > 429496729 || (num == 429496729 && c > '5')){ // overflow
            *N = 0xffffff;
            return start;
        }
        num *= 10;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read hexadecimal number (without 0x prefix!)
static char *gethex(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        uint8_t M = 0;
        if(c >= '0' && c <= '9'){
            M = '0';
        }else if(c >= 'A' && c <= 'F'){
            M = 'A' - 10;
        }else if(c >= 'a' && c <= 'f'){
            M = 'a' - 10;
        }
        if(M){
            if(num & 0xf0000000){ // overflow
                *N = 0xffffff;
                return start;
            }
            num <<= 4;
            num += c - M;
        }else{
            break;
        }
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read octal number (without 0 prefix!)
static char *getoct(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '7'){
            break;
        }
        if(num & 0xe0000000){ // overflow
            *N = 0xffffff;
            return start;
        }
        num <<= 3;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read binary number (without b prefix!)
static char *getbin(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '1'){
            break;
        }
        if(num & 0x80000000){ // overflow
            *N = 0xffffff;
            return start;
        }
        num <<= 1;
        if(c == '1') num |= 1;
        ++buf;
    }
    *N = num;
    return (char*)buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf
 *      (if it is == buf, there's no number or if *N==0xffffffff there was overflow)
 */
char *getnum(const char *txt, uint32_t *N){
    char *nxt = NULL;
    char *s = omit_spaces(txt);
    if(*s == '0'){ // hex, oct or 0
        if(s[1] == 'x' || s[1] == 'X'){ // hex
            nxt = gethex(s+2, N);
            if(nxt == s+2) nxt = (char*)txt;
        }else if(s[1] > '0'-1 && s[1] < '8'){ // oct
            nxt = getoct(s+1, N);
            if(nxt == s+1) nxt = (char*)txt;
        }else{ // 0
            nxt = s+1;
            *N = 0;
        }
    }else if(*s == 'b' || *s == 'B'){
        nxt = getbin(s+1, N);
        if(nxt == s+1) nxt = (char*)txt;
    }else{
        nxt = getdec(s, N);
        if(nxt == s) nxt = (char*)txt;
    }
    return nxt;
}

// split long text into two lines in two-line display mode
static void split_two_lines(char *l1, uint32_t n1, char *l2, uint32_t n2){
    int len = strlen(text_buf);
    int split = len / 2;
    for(int i = 0; i < len; ++i){
        if(text_buf[i] == ' '){
            char tmp[TEXTBUF_SZ];
            int tlen = i;
            if(tlen > (int)sizeof(tmp)-1) tlen = sizeof(tmp)-1;
            memcpy(tmp, text_buf, tlen);
            tmp[tlen] = 0;
            if(strpixlen(tmp) <= SCREEN_WIDTH) split = i;
            else break;
        }
    }
    int n = split;
    if(n > (int)n1 - 1) n = n1 - 1;
    memcpy(l1, text_buf, n);
    l1[n] = 0;
    const char *rest = text_buf + split;
    while(*rest == ' ') ++rest;
    strncpy(l2, rest, n2 - 1);
    l2[n2 - 1] = 0;
}

static int max_line_width(){
    if(text_mode == 0) return strpixlen(text_buf);
    char l1[TEXTBUF_SZ], l2[TEXTBUF_SZ];
    split_two_lines(l1, sizeof(l1), l2, sizeof(l2));
    int w1 = strpixlen(l1), w2 = strpixlen(l2);
    return (w1 > w2) ? w1 : w2;
}

static void render_text(){
    if(display_mode != 0) return;
    if(!text_buf[0]) return;
    for(int i = 0; i < SCREENBUF_SZ; ++i) screenbuf[i] = text_bg;

    setFGcolor(text_fg);
    int charH = curfont->height;
    int base_off = charH - curfont->baseline - 1; // Y-координата базовой линии при top=0

    if(text_mode == 0){
        int top = (SCREEN_HEIGHT - charH) / 2;
        if(top < 0) top = 0;
        PutStringAt(-(int)text_scroll, top + base_off, text_buf);
    }else{
        char l1[TEXTBUF_SZ], l2[TEXTBUF_SZ];
        split_two_lines(l1, sizeof(l1), l2, sizeof(l2));
        int total = charH * 2;
        int top = (SCREEN_HEIGHT - total) / 2;
        if(top < 0) top = 0;
        PutStringAt(-(int)text_scroll, top + base_off, l1);
        PutStringAt(-(int)text_scroll, top + charH + base_off, l2);
    }
}

static void update_scroll(void){
    if(display_mode != 0) return;
    if(!text_buf[0]) return;
    int tw = max_line_width();
    if(tw <= SCREEN_WIDTH){
        if(text_scroll){
            text_scroll = 0;
            text_needs_redraw = 1;
        }
        return;
    }
    if(Tms - Tlast_scroll < SCROLL_STEP_MS) return;
    Tlast_scroll = Tms;
    if(++text_scroll > (uint16_t)tw) text_scroll = 0;
    text_needs_redraw = 1;
}



const char* helpmsg =
    "https://github.com/eddyem/stm32samples/tree/master/F1:F103/TwoPanelScreen "
#ifdef EBUG
    "debug "
#else
    "release "
#endif
    "build #" BUILD_NUMBER " @ " BUILD_DATE
    "'0/1' - screen off/on\n"
    "'A' - get ADC values\n"
    "'B <color>' - set background color\n"
    "'C <color>' - clear screen with given color\n"
    "'F <color>' - set text color\n"
    "'f' - get FPS\n"
    "'G' - get 100 random numbers\n"
    "'L <0|1>' - single/double line text mode\n"
    "'P' - receive 128x32 pixel blob (4096 bytes RRRGGGBB)\n"
    "'R' - software reset\n"
    "'S <0|1>' - font size: 0=small(10px), 1=big(16px)\n"
    "'T <text>' - set display text\n"
;



static const char *parse_cmd(const char *buf){
    uint32_t N;
    char cmd = buf[0];
    const char *arg = buf + 1;
    while(*arg == ' ') ++arg;

    switch(cmd){
    case '0':
        ScreenOFF();
        return "OFF\n";
    case '1':
        ScreenON();
        if(display_mode == 0) text_needs_redraw = 1;
        return "ON\n";
    case 'A':
        USB_send("Tsens="); USB_send(u2str(getADCval(0)));
        USB_send("\nVref="); USB_send(u2str(getADCval(1)));
        USB_send("\nRand="); USB_send(u2str(getRand()));
        USB_send("\n");
        return NULL;
    case 'B':
        if(getnum(arg, &N) && N <= 0xff){
            text_bg = N & 0xff;
            setBGcolor(text_bg);
            text_needs_redraw = 1;
            return "BG color set\n";
        }
        return "Wrong color\n";
    case 'C':
        if(getnum(arg, &N)){
            ScreenOFF();
            setBGcolor(N & 0xff);
            ClearScreen();
            ScreenON();
            display_mode = 0;
            text_needs_redraw = 0;
            return "Screen cleared\n";
        }
        return "Wrong color\n";
    case 'F':
        if(getnum(arg, &N)){
            text_fg = N & 0xff;
            setFGcolor(text_fg);
            text_needs_redraw = 1;
            return "FG color set\n";
        }
        return "Wrong color\n";
    case 'f':
        if(SCREEN_RELAX == getScreenState()) return "Screen is inactive\n";
        USB_send("FPS="); USB_send(u2str(getFPS())); USB_send("\n");
        return NULL;
    case 'G':
        for(int i = 0; i < 100; ++i){
            USB_send(u2str(getRand()));
            USB_send("\n");
        }
        return NULL;
    case 'L':
        if(getnum(arg, &N) && N <= 1){
            text_mode = N & 1;
            text_scroll = 0;
            Tlast_scroll = Tms;
            text_needs_redraw = 1;
            return "Line mode set\n";
        }
        return "Wrong mode\n";
    case 'P':
        // start receiving of blob
        display_mode = 1;
        blob_remaining = SCREENBUF_SZ;
        blob_ptr = screenbuf;
        return "Send blob\n";
    case 'R':
        USB_send("Soft reset\n");
        NVIC_SystemReset();
        break;
    case 'S':
        if(getnum(arg, &N) && N <= 1){
            choose_font(N ? FONT14 : FONTN8);
            text_scroll = 0;
            Tlast_scroll = Tms;
            text_needs_redraw = 1;
            return "Font set\n";
        }
        return "Wrong font\n";
    case 'T':{
        int i;
        for(i = 0; i < TEXTBUF_SZ - 1 && *arg && *arg != '\n'; ++i)
            text_buf[i] = *arg++;
        text_buf[i] = 0;
        text_scroll = 0;
        Tlast_scroll = Tms;
        display_mode = 0;
        text_needs_redraw = 1;
        return "Text set\n";
    }
    default:
        return helpmsg;
    }
    return NULL;
}

// process blob receiving
void proto_process(){
    if(blob_remaining > 0){
        uint8_t tmp[64];
        int x = USB_receive((char*)tmp);
        if(x > 0){
            int n = (x < blob_remaining) ? x : (int)blob_remaining;
            memcpy(blob_ptr, tmp, n);
            blob_ptr += n;
            blob_remaining -= n;
        }
        return;
    }
    // read commands
    if(usb_line_len < USBBUFSZ){
        int x = USB_receive(usb_line + usb_line_len);
        if(x > 0){
            usb_line_len += x;
            int start = 0;
            for(int i = 0; i < usb_line_len; ++i){
                if(usb_line[i] == '\n'){
                    usb_line[i + 1] = 0;
                    const char *ans = parse_cmd(usb_line + start);
                    if(ans) USB_send(ans);
                    start = i + 1;
                }
            }
            if(start){
                int rest = usb_line_len - start;
                if(rest > 0) memmove(usb_line, usb_line + start, rest);
                usb_line_len = rest;
            }
        }
    }

    // 3. refresh/scroll text
    update_scroll();
    if(text_needs_redraw && display_mode == 0){
        text_needs_redraw = 0;
        render_text();
    }
}

// return string with number `val`
char *u2str(uint32_t val){
    static char strbuf[11];
    char *bufptr = &strbuf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    return bufptr;
}
