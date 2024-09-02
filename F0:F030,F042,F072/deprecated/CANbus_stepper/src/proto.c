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
#include "adc.h"
#include "can.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "steppers.h"
#include "usart.h"
#include "usb.h"
#include <string.h> // strlen, strcpy(

extern volatile uint8_t canerror;
uint8_t monitCAN = 0; // ==1 to show CAN messages
#define BUFSZ UARTBUFSZ

static char buff[BUFSZ+1], *bptr = buff;
static uint8_t blen = 0, // length of data in `buff`
    USBcmd = 0; // ==1 if buffer prepared for USB

char *omit_spaces(char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return buf;
}

void buftgt(uint8_t isUSB){
    USBcmd = isUSB;
}

void sendbuf(){
    IWDG->KR = IWDG_REFRESH;
    if(blen == 0) return;
    if(USBcmd){
        *bptr = 0;
        USB_sendstr(buff);
    }else while(LINE_BUSY == usart_send(buff, blen)){IWDG->KR = IWDG_REFRESH;}
    bptr = buff;
    blen = 0;
}

void addtobuf(const char *txt){
    IWDG->KR = IWDG_REFRESH;
    int l = strlen(txt);
    if(l > BUFSZ){
        sendbuf();
        if(USBcmd) USB_sendstr(txt);
        else usart_send_blocking(txt, l);
    }else{
        if(blen+l > BUFSZ){
            sendbuf();
        }
        strcpy(bptr, txt);
        bptr += l;
    }
    blen += l;
}

void bufputchar(char ch){
    if(blen > BUFSZ-1){
        sendbuf();
    }
    *bptr++ = ch;
    ++blen;
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
    SEND("\nV33="); printu(getVdd());
    newline();
}

// check address & return 0 if wrong or roll to next non-digit
static char *chk485addr(char *txt){
    uint32_t N;
    char *nxt = getnum(txt, &N);
    if(nxt == txt) return NULL;
    if(N == getBRDaddr()){
        return nxt;
    }
    return NULL;
}

// parse `txt` to CAN_message
static CAN_message *parseCANmsg(char *txt){
    static CAN_message canmsg;
    uint32_t N;
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
    canmsg.length = (uint8_t) ctr;
    return &canmsg;
}

// send command, format: ID (hex/bin/dec) data bytes (up to 8 bytes, space-delimeted)
TRUE_INLINE void sendCANcommand(char *txt){
    CAN_message *msg = parseCANmsg(txt);
    if(!msg) return;
    uint32_t N = 1000000;
    while(CAN_BUSY == can_send(msg->data, msg->length, msg->ID)){
        if(--N == 0) break;
    }
}

static uint8_t userconf_changed = 0; // ==1 if user_conf was changed
TRUE_INLINE void userconf_manip(char *txt){
    txt = omit_spaces(txt);
    switch(*txt){
        case 'd': // dump
            dump_userconf();
        break;
        case 's': // store
            if(userconf_changed){
                if(!store_userconf()){
                    userconf_changed = 0;
                    SEND("Stored!");
                }else SEND("Error when storing!");
            }
        break;
        default:
            SEND("\nUserconf commands:\n"
                 "d - userconf dump\n"
                 "s - userconf store\n"
                 );
    }
}

TRUE_INLINE void setdefflags(char *txt){
    const char *needar = "Need argument 0 or 1 for flag";
    txt = omit_spaces(txt);
    char ch = *txt;
    ++txt;
    uint32_t U;
    if(txt == getnum(txt, &U)){
        SEND(needar);
        return;
    }
    switch(ch){
        case 'r':
            if(U > 1){
                SEND(needar);
                return;
            }
            the_conf.defflags.reverse = U&1;
        break;
        default:
            SEND("\nFlag commands:\n"
                 "r - set/clear reverse\n"
                 );
    }
}

// a set of setters for user_conf
TRUE_INLINE void setters(char *txt){
    uint32_t U;
    uint8_t u8;
    const char *drvshould = "Driver type should be one of: 2130, 4988, 8825";
    const char *usshould = "Microsteps amount is a power of two: 1..512";
    const char *motspdshould = "Motor speed should be from 1 to " STR(10000);
    txt = omit_spaces(txt);
    if(!*txt){
        SEND("Setters need more arguments");
        return;
    }
    char *nxt = getnum(txt + 1, &U);
    switch(*txt){
        case 'a': // accdecsteps
            if(nxt == txt + 1){
                SEND("No accdecsteps value given");
                return;
            }
            if(U < ACCDECSTEPS_MIN || U > ACCDECSTEPS_MAX){
                SEND("The value should be from" STR(ACCDECSTEPS_MIN) " to " STR(ACCDECSTEPS_MAX));
                return;
            }
            if(the_conf.accdecsteps != (uint16_t) U){
                the_conf.accdecsteps = (uint16_t) U;
                userconf_changed = 1;
                SEND("Set accdecsteps to "); printu(U);
            }
        break;
        case 'c': // set CAN speed
            if(nxt == txt + 1){
                SEND("No CAN speed given");
                return;
            }
            if(U < 50){
                SEND("Speed should be not less than 50kbps");
                return;
            }
            if(U > 3000){
                SEND("Speed should be not greater than 3000kbps");
                return;
            }
            if(the_conf.CANspeed != (uint16_t)U){
                the_conf.CANspeed = (uint16_t)U;
                SEND("Set CAN speed to "); printu(U);
                userconf_changed = 1;
            }
        break;
        case 'd': // set driver type
            if(nxt == txt+1){
                SEND(drvshould);
                break;
            }
            u8 = DRV_NONE;
            switch(U){
                case 2130:
                    u8 = DRV_2130;
                    SEND("TMC2130");
                break;
                case 4988:
                    u8 = DRV_4988;
                    SEND("A4988");
                break;
                case 8825:
                    u8 = DRV_8825;
                    SEND("DRV8825");
                break;
                default:
                    SEND(drvshould);
            }
            if(the_conf.driver_type != u8){
                the_conf.driver_type = u8;
                userconf_changed = 1;
            }
        break;
        case 'F':
            setdefflags(txt+1);
        break;
        case 'm': // microsteps
            if(nxt == txt + 1){ // no number
                SEND(usshould); break;
            }
            if(U < 1 || U > getMaxUsteps() || (U & (U-1)) != 0){ // U over of range or not power of two
                SEND(usshould); break;
            }
            if(the_conf.microsteps != (uint16_t)U){
                the_conf.microsteps = (uint16_t)U;
                userconf_changed = 1;
                SEND("Set microsteps to "); printu(U);
            }
        break;
        case 'M': // maxsteps
            if(nxt == txt + 1 || U > INT32_MAX){
                SEND("Enter number from 0 (infinity) to INT32_MAX"); break;
            }
            if(U != the_conf.maxsteps){
                the_conf.maxsteps = U;
                userconf_changed = 1;
                SEND("Set maxsteps to "); printu(U);
            }
        break;
        case 's': // motor speed
            if(nxt == txt + 1 || U < 1 || U > MAX_SPEED){ // no number
                SEND(motspdshould); break;
            }
            if(the_conf.motspd != (uint16_t)U){
                the_conf.motspd = (uint16_t)U;
                userconf_changed = 1;
                SEND("Set motspd to "); printu(U);
            }
        break;
        default:
            SEND("\nSetters commands:\n"
                 "a - set accdecsteps\n"
                 "c - set default CAN speed\n"
                 "d - set driver type\n"
                 "F - set flags\n"
                 "m - set microsteps\n"
                 "M - set maxsteps\n"
                 "s - set motspd\n"
                );
    }
}

TRUE_INLINE void driver_commands(char *txt){
    txt = omit_spaces(txt);
    if(!*txt){
        SEND("Driver commands need more arguments");
        return;
    }
    char cmd = *txt++;
    txt = omit_spaces(txt);
    uint32_t U;
    int8_t sign = 1;
    if(*txt == '-'){
        ++txt;
        sign = -1;
    }
    char *nxt = getnum(txt, &U);
    stp_status st;
    switch(cmd){
        case '0':
            state = STP_MOVE0;
            stp_process();
        break;
        case '1':
            state = STP_MOVE1;
            stp_process();
        break;
        case 'd':
            SEND(stp_getdrvtype());
        break;
        case 'e':
            SEND("ESW=");
            printu(ESW_STATE());
        break;
        case 'i': // init
            initDriver();
        break;
        case 'l':
            SEND("Stepsleft=");
            printu(stpleft());
        break;
        case 'm':
            if(nxt == txt || U > (INT32_MAX-1)){
                SEND("Give right steps amount: from -INT32_MAX to INT32_MAX");
                return;
            }
            if(sign > 0) st = stp_move((int32_t)U);
            else st = stp_move(-(int32_t)U);
            switch(st){
                case STPS_ACTIVE:
                    SEND("IsMoving");
                break;
                case STPS_ONESW:
                    SEND("OnEndSwitch");
                break;
                case STPS_ZEROMOVE:
                    SEND("ZeroMove");
                break;
                case STPS_TOOBIG:
                    SEND("TooBigNumber");
                break;
                default:
                    SEND("Move to given steps amount");
            }
        break;
        case 'p':
            SEND("Motpos=");
            if(stppos() < 0){
                U = (uint32_t) -stppos();
                bufputchar('-');
            }else U = (uint32_t) stppos();
            printu(U);
        break;
        case 's':
            stp_stop();
            SEND("Stop motor");
        break;
        case 'S':
            SEND(stp_getstate());
        break;
        default:
            SEND("\nDriver commands:\n"
                 "0/1 - move to ESW0/ESW3 and stop there\n"
                 "d - current driver type\n"
                 "e - end-switches state\n"
                 "i - init stepper driver (8825, 4988, 2130)\n"
                 "l - steps left\n"
                 "m - move N steps\n"
                 "p - motor position\n"
                 "s - stop\n"
                 "S - state\n"
                );
    }
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
 */
void cmd_parser(char *txt, uint8_t isUSB){
    sendbuf();
    USBcmd = isUSB;
    // we can't simple use &txt[p] as variable: it can be non-aligned by 4!!!
    if(isUSB == TARGET_USART){ // check address and roll message to nearest non-space
        txt = chk485addr(txt);
        if(!txt) return;
    }
    txt = omit_spaces(txt);
    // long commands, commands with arguments
    switch(*txt){
        case 'D':
            driver_commands(txt + 1);
            goto eof;
        break;
        case 's':
            sendCANcommand(txt + 1);
            goto eof;
        break;
        case 'S': // setters
            setters(txt + 1);
            goto eof;
        break;
        case 'U':
            userconf_manip(txt + 1);
            goto eof;
        break;
    }
    if(txt[1] != '\n') *txt = '?'; // help for wrong message length
    switch(*txt){
        case '0':
            can_accept_one();
            SEND("Accept only my ID @CAN");
        break;
        case '1':
            timer_setup();
        break;
        case '@':
            can_accept_any();
            SEND("Accept any ID @CAN");
        break;
        case 'a':
            showADCvals();
        break;
        case 'b':
            SEND("Jump to bootloader.\n");
            sendbuf();
            Jump2Boot();
        break;
        case 'g':
            SEND("Board address: ");
            printuhex(refreshBRDaddr());
            SEND("\nCAN IN address (OUT=IN+1): ");
            printuhex(getCANID());
        break;
        case 'j':
            printmcut();
        break;
        case 'k':
            showUIvals();
        break;
        case 'm':
            monitCAN = !monitCAN;
            SEND("CAN monitoring ");
            if(monitCAN) SEND("ON");
            else SEND("OFF");
        break;
        case 't':
            if(ALL_OK != usart_send("TEST test\n", 10))
                SEND("Can't send data over RS485");
            else SEND("Sent");
        break;
        case 'T':
            SEND("Tms="); printu(Tms);
        break;
        case 'z':
            flashstorage_init();
        break;
        default: // help
            SEND(
            "0 - accept only data for this device\n"
            "@ - accept any IDs\n"
            "a - get raw ADC values\n"
            "b - switch to bootloader\n"
            "D? - stepper driver commands\n"
            "g - get board address\n"
            "j - get MCU temperature\n"
            "k - get U values\n"
            "m - start/stop monitoring CAN bus\n"
            "s - send data over CAN: s ID [byte0..7]\n"
            "S? - parameter setters\n"
            "t - send test sequence over RS-485\n"
            "T - print current time\n"
            "U? - options for user configuration\n"
            );
        break;
    }
eof:
    newline();
    sendbuf();
}

// print 32bit unsigned int
void printu(uint32_t val){
    char buf[11], *bufptr = &buf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    addtobuf(bufptr);
}

// print 32bit unsigned int as hex
void printuhex(uint32_t val){
    addtobuf("0x");
    uint8_t *ptr = (uint8_t*)&val + 3;
    int8_t i, j;
    for(i = 0; i < 4; ++i, --ptr){
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) bufputchar(half + '0');
            else bufputchar(half - 10 + 'a');
        }
    }
}

// THERE'S NO OVERFLOW PROTECTION IN NUMBER READ PROCEDURES!
// read decimal number
static char *getdec(char *buf, uint32_t *N){
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '9'){
            break;
        }
        num *= 10;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return buf;
}
// read hexadecimal number (without 0x prefix!)
static char *gethex(char *buf, uint32_t *N){
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
    *N = num;
    return buf;
}
// read binary number (without 0b prefix!)
static char *getbin(char *buf, uint32_t *N){
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
    *N = num;
    return buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf (if it is == buf, there's no number)
 */
char *getnum(char *txt, uint32_t *N){
    txt = omit_spaces(txt);
    if(*txt == '0'){
        if(txt[1] == 'x' || txt[1] == 'X') return gethex(txt+2, N);
        if(txt[1] == 'b' || txt[1] == 'B') return getbin(txt+2, N);
    }
    return getdec(txt, N);
}
