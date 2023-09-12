/*
 *                                                                                                  geany_encoding=koi8-r
 * proto.c
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */

#include <string.h>

#include "adc.h"
#include "can.h"
#include "can_process.h"
#include "hardware.h"
#include "proto.h"
#include "sensors_manage.h"
#include "usart.h"
#include "usb.h"
#include "version.inc"

extern volatile uint8_t canerror;
extern volatile uint32_t Tms;

static char buff[UARTBUFSZ+1], /* +1 - for USB send (it receive \0-terminated line) */ *bptr = buff;
static int blen = 0, USBcmd = 0, debugmode =
#ifdef EBUG
        1
#else
    0
#endif
;
// LEDs are OFF by default
uint8_t noLED =
#ifdef EBUG
        0
#else
        1
#endif
;

void sendbuf(){
    IWDG->KR = IWDG_REFRESH;
    if(blen == 0) return;
    *bptr = 0;
    if(USBcmd) USB_send(buff);
    else for(int i = 0; (i < 9999) && (LINE_BUSY == usart_send(buff, blen)); ++i){IWDG->KR = IWDG_REFRESH;}
    bptr = buff;
    blen = 0;
}

void addtobuf(const char *txt){
    IWDG->KR = IWDG_REFRESH;
    int l = strlen(txt);
    if(l > UARTBUFSZ){
        sendbuf(); // send prevoius data in buffer
        if(USBcmd) USB_send(txt);
        else for(int i = 0; (i < 9999) && (LINE_BUSY == usart_send_blocking(txt, l)); ++i){IWDG->KR = IWDG_REFRESH;}
    }else{
        if(blen+l > UARTBUFSZ){
            sendbuf();
        }
        strcpy(bptr, txt);
        bptr += l;
        *bptr = 0;
        blen += l;
    }
}

void bufputchar(char ch){
    if(blen > UARTBUFSZ-1){
        sendbuf();
    }
    *bptr++ = ch;
    ++blen;
}

static void CANsend(uint16_t targetID, uint8_t cmd, char echo){
    if(CAN_OK == can_send_cmd(targetID, cmd)){
        bufputchar(echo);
        bufputchar('\n');
    }
}

// show all ADC values
static inline void showADCvals(){
    char msg[] = "ADCn=";
    for(int n = 0; n < NUMBER_OF_ADC_CHANNELS; ++n){
        msg[3] = n + '0';
        addtobuf(msg);
        printu(getADCval(n));
        newline();
    }
}

static inline void printmcut(){
    SEND("MCUT=");
    int32_t T = getMCUtemp();
    if(T < 0){
        bufputchar('-');
        T = -T;
    }
    printu(T);
    newline();
}

static inline void showUIvals(){
    uint16_t *vals = getUval();
    SEND("V12="); printu(vals[0]);
    SEND("\nV5="); printu(vals[1]);
    SEND("\nV33="); printu(vals[3]);
    SEND("\nI12="); printu(vals[2]);
    newline();
}

static char *omit_spaces(char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return buf;
}

static inline void setCANbrate(char *str){
    if(!str || !*str) return;
    int32_t spd = 0;
    str = omit_spaces(str);
    char *e = getnum(str, &spd);
    if(e == str){
        SEND("BAUDRATE=");
        printu(curcanspeed);
        newline();
        return;
    }
    if(spd < CAN_SPEED_MIN || spd > CAN_SPEED_MAX){
        SEND("Wrong speed\n");
        return;
    }
    CAN_setup(spd);
    SEND("OK\n");
}

// parse `txt` to CAN_message
static CAN_message *parseCANmsg(char *txt){
    static CAN_message canmsg;
    int32_t N;
    char *n;
    int ctr = -1;
    canmsg.ID = 0xffff;
    do{
        txt = omit_spaces(txt);
        n = getnum(txt, &N);
        if(txt == n) break;
        txt = n;
        if(ctr == -1){
            if(N > 0x7ff){
                SEND("ID should be 11-bit number!\n");
                return NULL;
            }
            canmsg.ID = (uint16_t)(N&0x7ff);
            ctr = 0;
            continue;
        }
        if(ctr > 7){
            SEND("ONLY 8 data bytes allowed!\n");
            return NULL;
        }
        if(N > 0xff){
            SEND("Every data portion is a byte!\n");
            return NULL;
        }
        canmsg.data[ctr++] = (uint8_t)(N&0xff);
    }while(1);
    if(canmsg.ID == 0xffff){
        SEND("NO ID given, send nothing!\n");
        return NULL;
    }
    SEND("Message parsed OK\n");
    canmsg.length = (uint8_t) ctr;
    return &canmsg;
}

// send command, format: ID (hex/bin/dec) data bytes (up to 8 bytes, space-delimeted)
static void sendCANcommand(char *txt){
    CAN_message *msg = parseCANmsg(txt);
    if(!msg) return;
    uint32_t N = 1000;
    while(CAN_BUSY == can_send(msg->data, msg->length, msg->ID)){
        if(--N == 0) break;
    }
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
 */
void cmd_parser(char *txt, uint8_t isUSB){
    USBcmd = isUSB;
    int16_t L = strlen(txt), ID = BCAST_ID;
    char _1st = txt[0];
    if(_1st >= '0' && _1st < '8'){ // send command to Nth controller, not broadcast
        if(L == 3){ // with '\n' at end!
            ID = (CAN_ID_PREFIX & CAN_ID_MASK) | (_1st - '0');
            _1st = txt[1];
        }else{
            _1st = '?'; // show help
        }
    }
    switch(_1st){
        case '#':
            if(ID == BCAST_ID) NVIC_SystemReset();
            CANsend(ID, CMD_RESET_MCU, _1st);
        break;
        case '@':
            debugmode = !debugmode;
            SEND("DEBUG mode ");
            if(debugmode) SEND("ON");
            else SEND("OFF");
            newline();
        break;
        case 'a':
            showADCvals();
        break;
        case 'B':
            CANsend(ID, CMD_DUMMY0, _1st);
        break;
        case 'b':
            setCANbrate(txt + 1);
        break;
        case 'c':
            showcoeffs();
        break;
        case 'D':
            CANsend(MASTER_ID, CMD_DUMMY1, _1st);
        break;
        case 'd':
            SEND("Can address: ");
            printuhex(CANID);
            newline();
        break;
        case 'E':
            CANsend(ID, CMD_STOP_SCAN, _1st);
        break;
        case 'e':
            sensors_scan_mode = 0;
        break;
        case 'F':
            CANsend(ID, CMD_SENSORS_OFF, _1st);
        break;
        case 'f':
            sensors_off();
        break;
        case 'g':
            SEND("Group ID (sniffer) CAN mode\n");
            CAN_listenall();
        break;
        case 'H':
            CANsend(ID, CMD_HIGH_SPEED, _1st);
        break;
        case 'h':
            i2c_setup(HIGH_SPEED);
        break;
        case 'I':
            CANsend(ID, CMD_REINIT_SENSORS, _1st);
        break;
        case 'i':
            sensors_init();
        break;
        case 'J':
            CANsend(ID, CMD_GETMCUTEMP, _1st);
        break;
        case 'j':
            printmcut();
        break;
        case 'K':
            CANsend(ID, CMD_GETUIVAL, _1st);
        break;
        case 'k':
            showUIvals();
        break;
        case 'L':
            CANsend(ID, CMD_LOW_SPEED, _1st);
        break;
        case 'l':
            i2c_setup(LOW_SPEED);
        break;
        case 'M':
            CANsend(ID, CMD_CHANGE_MASTER_B, _1st);
        break;
        case 'm':
            CANsend(ID, CMD_CHANGE_MASTER, _1st);
        break;
        case 'N':
            CANsend(ID, CMD_GETBUILDNO, _1st);
        break;
        case 'O':
            noLED = 0;
            SEND("LED on\n");
        break;
        case 'o':
            noLED = 1;
            LED_off(LED0);
            LED_off(LED1);
            SEND("LED off\n");
        break;
        case 'P':
            CANsend(ID, CMD_PING, _1st);
        break;
        case 'Q':
            CANsend(ID, CMD_SYSTIME, _1st);
        break;
        case 'q':
            SEND("SYSTIME0="); printu(Tms); newline();
        break;
        case 'R':
            CANsend(ID, CMD_REINIT_I2C, _1st);
        break;
        case 'r':
            i2c_setup(CURRENT_SPEED);
        break;
        case 's':
            sendCANcommand(txt+1);
        break;
        case 'T':
            CANsend(ID, CMD_START_MEASUREMENT, _1st);
        break;
        case 't':
            sensors_start();
        break;
        case 'u':
            SEND("Unique ID CAN mode\n");
            CAN_listenone();
        break;
        case 'V':
            CANsend(ID, CMD_LOWEST_SPEED, _1st);
        break;
        case 'v':
            i2c_setup(VERYLOW_SPEED);
        break;
        case 'X':
            CANsend(ID, CMD_START_SCAN, _1st);
        break;
        case 'x':
            sensors_scan_mode = 1;
        break;
        case 'Y':
            CANsend(ID, CMD_SENSORS_STATE, _1st);
        break;
        case 'y':
            SEND("SSTATE0=");
            SEND(sensors_get_statename(Sstate));
            SEND("\nNSENS0=");
            printu(Nsens_present);
            SEND("\nSENSPRESENT0=");
            printu(sens_present[0] | (sens_present[1]<<8));
            SEND("\nNTEMP0=");
            printu(Ntemp_measured);
            newline();
        break;
        case 'z':
            SEND("CANERROR=");
            if(canerror){
                canerror = 0;
                bufputchar('1');
            }else bufputchar('0');
            newline();
        break;
        default: // help
            SEND("https://github.com/eddyem/tsys01/tree/master/STM32/TSYS_controller build#" BUILD_NUMBER " @ " BUILD_DATE "\n");
            SEND(
            "ALL little letters - without CAN messaging\n"
            "# - reset MCU (self or with given ID like '1#')\n"
            "0..7 - send command to given controller (0 - this) instead of broadcast\n"
            "@ - set/clear debug mode\n"
            "a - get raw ADC values\n"
            "B - send broadcast CAN dummy message\n"
            "b - get/set CAN bus baudrate\n"
            "c - show coefficients (current)\n"
            "d - get last CAN address\n"
            "D - send CAN dummy message to master\n"
            "Ee- end themperature scan\n"
            "Ff- turn oFf sensors\n"
            "g - group (sniffer) CAN mode\n"
            "Hh- high I2C speed\n"
            "Ii- (re)init sensors\n"
            "Jj- get MCU temperature\n"
            "Kk- get U/I values\n"
            "Ll- low I2C speed\n"
            "Mm- change master id to 0 (m) / broadcast (M)\n"
            "N - get build number\n"
            "Oo- turn onboard diagnostic LEDs *O*n or *o*ff (both commands are local)\n"
            "P - ping everyone over CAN\n"
            "Qq- get system time\n"
            "Rr- reinit I2C\n"
            "s - send CAN message\n"
            "Tt- start temperature measurement\n"
            "u - unique ID (default) CAN mode\n"
            "Vv- very low I2C speed\n"
            "Xx- Start themperature scan\n"
            "Yy- get sensors state\n"
            "z - check CAN status for errors\n"
            );
        break;
    }
}

// print 32bit unsigned int
void printu(uint32_t val){
    char buf[11], *bufptr = &buf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            register uint32_t o = val;
            val /= 10;
            *(--bufptr) = (o - 10*val) + '0';
        }
    }
    addtobuf(bufptr);
}

// print 32bit unsigned int as hex
void printuhex(uint32_t val){
    addtobuf("0x");
    uint8_t *ptr = (uint8_t*)&val + 3;
    int i, j, z = 1;
    for(i = 0; i < 4; ++i, --ptr){
        if(*ptr == 0){ // omit leading zeros
            if(i == 3) z = 0;
            if(z) continue;
        }
        else z = 0;
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) bufputchar(half + '0');
            else bufputchar(half - 10 + 'a');
        }
    }
}

// THERE'S NO OVERFLOW PROTECTION IN NUMBER READ PROCEDURES!
// read decimal number
static char *getdec(const char *buf, int32_t *N){
    int32_t num = 0;
    int positive = TRUE;
    if(*buf == '-'){
        positive = FALSE;
        ++buf;
    }
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '9'){
            break;
        }
        num *= 10;
        num += c - '0';
        ++buf;
    }
    *N = (positive) ? num : -num;
    return (char *)buf;
}
// read hexadecimal number (without 0x prefix!)
static char *gethex(const char *buf, int32_t *N){
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
            num <<= 4;
            num += c - M;
        }else{
            break;
        }
        ++buf;
    }
    *N = (int32_t)num;
    return (char *)buf;
}
// read binary number (without 0b prefix!)
static char *getbin(const char *buf, int32_t *N){
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '1'){
            break;
        }
        num <<= 1;
        if(c == '1') num |= 1;
        ++buf;
    }
    *N = (int32_t)num;
    return (char *)buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf (if it is == buf, there's no number)
 */
char *getnum(char *txt, int32_t *N){
    if(*txt == '0'){
        if(txt[1] == 'x' || txt[1] == 'X') return gethex(txt+2, N);
        if(txt[1] == 'b' || txt[1] == 'B') return getbin(txt+2, N);
    }
    return getdec(txt, N);
}

// show message in debug mode
void mesg(char *txt){
    if(!debugmode) return;
    addtobuf("[DBG] ");
    addtobuf(txt);
    bufputchar('\n');
}
