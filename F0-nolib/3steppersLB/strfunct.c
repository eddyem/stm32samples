/*
 * This file is part of the 3steppers project.
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

#include "adc.h"
#include "buttons.h"
#include "can.h"
#include "commonproto.h"
#include "flash.h"
#include "hardware.h"
#include "strfunct.h"
#include "usb.h"
#include "version.inc"

#include <string.h> // strlen

extern volatile uint8_t canerror;

uint8_t ShowMsgs = 0;
uint16_t Ignore_IDs[IGN_SIZE];
uint8_t IgnSz = 0;
static char buff[BUFSZ+1], *bptr = buff;
static uint8_t blen = 0;

void sendbuf(){
    if(blen == 0) return;
    *bptr = 0;
    USB_sendstr(buff);
    bptr = buff;
    blen = 0;
}

void bufputchar(char ch){
    if(blen > BUFSZ-1){
        sendbuf();
    }
    *bptr++ = ch;
    ++blen;
}

void addtobuf(const char *txt){
    while(*txt) bufputchar(*txt++);
}


/**
 * @brief cmpstr - the same as strncmp
 * @param s1,s2 - strings to compare
 * @return 0 if strings equal or 1/-1
 */
int cmpstr(const char *s1, const char *s2){
    int ret = 0;
    do{
        ret = *s1 - *s2;
        if(ret == 0 && *s1 && *s2){
            ++s1; ++s2;
            continue;
        }
        break;
    }while(1);
    return ret;
}

/**
 * @brief getchr - analog of strchr
 * @param str - string to search
 * @param symbol - searching symbol
 * @return pointer to symbol found or NULL
 */
char *getchr(const char *str, char symbol){
    do{
        if(*str == symbol) return (char*)str;
    }while(*(++str));
    return NULL;
}

// parse `txt` to CAN_message
static CAN_message *parseCANmsg(char *txt){
    static CAN_message canmsg;
    //SEND("CAN command with arguments:\n");
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
            //SEND("ID="); printuhex(canmsg.ID); newline();
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
    sendbuf();
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

static void CANini(char *txt){
    txt = omit_spaces(txt);
    int32_t N;
    if(!txt) goto eofunc;
    char *n = getnum(txt, &N);
    if(txt == n)  goto eofunc;
    if(N < 50){
        SEND("Lowest speed is 50kbps");
        return;
    }else if(N > 3000){
        SEND("Highest speed is 3000kbps");
        return;
    }
    the_conf.CANspeed = (uint16_t)N;
    CAN_reinit((uint16_t)N);
eofunc:
    SEND("canspeed="); printu(the_conf.CANspeed); NL();
}

static void addIGN(char *txt){
    if(IgnSz == IGN_SIZE){
        DBG("Ignore buffer is full");
        return;
    }
    txt = omit_spaces(txt);
    int32_t N;
    char *n = getnum(txt, &N);
    if(txt == n){
        SEND("No ID given");
        return;
    }
    if(N == the_conf.CANID){
        SEND("You can't ignore self ID!");
        return;
    }
    if(N > 0x7ff){
        SEND("ID should be 11-bit number!");
        return;
    }
    Ignore_IDs[IgnSz++] = (uint16_t)(N & 0x7ff);
    SEND("Added ID "); printu(N);
    SEND("\nIgn buffer size: "); printu(IgnSz);
}

static void print_ign_buf(_U_ char *txt){
    if(IgnSz == 0){
        SEND("Ignore buffer is empty");
        return;
    }
    SEND("Ignored IDs:\n");
    for(int i = 0; i < IgnSz; ++i){
        if(i) newline();
        printu(i);
        SEND(": ");
        printuhex(Ignore_IDs[i]);
    }
}

// print ID/mask of CAN->sFilterRegister[x] half
static void printID(uint16_t FRn){
    if(FRn & 0x1f) return; // trash
    printuhex(FRn >> 5);
}
/*
Can filtering: FSCx=0 (CAN->FS1R) -> 16-bit identifiers
CAN->FMR = (sb)<<8 | FINIT - init filter in starting bank sb
CAN->FFA1R FFAx = 1 -> FIFO1, 0 -> FIFO0
CAN->FA1R FACTx=1 - filter active
MASK: FBMx=0 (CAN->FM1R), two filters (n in FR1 and n+1 in FR2)
    ID:   CAN->sFilterRegister[x].FRn[0..15]
    MASK: CAN->sFilterRegister[x].FRn[16..31]
    FR bits:  STID[10:0] RTR IDE EXID[17:15]
LIST: FBMx=1, four filters (n&n+1 in FR1, n+2&n+3 in FR2)
    IDn:   CAN->sFilterRegister[x].FRn[0..15]
    IDn+1: CAN->sFilterRegister[x].FRn[16..31]
*/
static void list_filters(_U_ char *txt){
    uint32_t fa = CAN->FA1R, ctr = 0, mask = 1;
    while(fa){
        if(fa & 1){
            SEND("Filter "); printu(ctr); SEND(", FIFO");
            if(CAN->FFA1R & mask) SEND("1");
            else SEND("0");
            SEND(" in ");
            if(CAN->FM1R & mask){ // up to 4 filters in LIST mode
                SEND("LIST mode, IDs: ");
                printID(CAN->sFilterRegister[ctr].FR1 & 0xffff);
                SEND(" ");
                printID(CAN->sFilterRegister[ctr].FR1 >> 16);
                SEND(" ");
                printID(CAN->sFilterRegister[ctr].FR2 & 0xffff);
                SEND(" ");
                printID(CAN->sFilterRegister[ctr].FR2 >> 16);
            }else{ // up to 2 filters in MASK mode
                SEND("MASK mode: ");
                if(!(CAN->sFilterRegister[ctr].FR1&0x1f)){
                    SEND("ID="); printID(CAN->sFilterRegister[ctr].FR1 & 0xffff);
                    SEND(", MASK="); printID(CAN->sFilterRegister[ctr].FR1 >> 16);
                    SEND(" ");
                }
                if(!(CAN->sFilterRegister[ctr].FR2&0x1f)){
                    SEND("ID="); printID(CAN->sFilterRegister[ctr].FR2 & 0xffff);
                    SEND(", MASK="); printID(CAN->sFilterRegister[ctr].FR2 >> 16);
                }
            }
            newline();
        }
        fa >>= 1;
        ++ctr;
        mask <<= 1;
    }
}

/**
 * @brief add_filter - add/modify filter
 * @param str - string in format "bank# FIFO# mode num0 .. num3"
 * where bank# - 0..27
 * if there's nothing after bank# - delete filter
 * FIFO# - 0,1
 * mode - 'I' for ID, 'M' for mask
 * num0..num3 - IDs in ID mode, ID/MASK for mask mode
 */
static void add_filter(char *str){
    int32_t N;
    str = omit_spaces(str);
    char *n = getnum(str, &N);
    if(n == str){
        SEND("No bank# given");
        return;
    }
    if(N == 0 || N > STM32F0FBANKNO-1){
        SEND("0 (reserved for self) < bank# < 28 (max bank# is 27)!!!");
        return;
    }
    uint8_t bankno = (uint8_t)N;
    str = omit_spaces(n);
    if(!*str){ // deactivate filter
        SEND("Deactivate filters in bank ");
        printu(bankno);
        CAN->FMR = CAN_FMR_FINIT;
        CAN->FA1R &= ~(1<<bankno);
        CAN->FMR &=~ CAN_FMR_FINIT;
        return;
    }
    uint8_t fifono = 0;
    if(*str == '1') fifono = 1;
    else if(*str != '0'){
        SEND("FIFO# is 0 or 1");
        return;
    }
    str = omit_spaces(str + 1);
    char c = *str;
    uint8_t mode = 0; // ID
    if(c == 'M' || c == 'm') mode = 1;
    else if(c != 'I' && c != 'i'){
        SEND("mode is 'M/m' for MASK and 'I/i' for IDLIST");
        return;
    }
    str = omit_spaces(str + 1);
    uint32_t filters[4];
    uint32_t nfilt;
    for(nfilt = 0; nfilt < 4; ++nfilt){
        n = getnum(str, &N);
        if(n == str) break;
        filters[nfilt] = N;
        str = omit_spaces(n);
    }
    if(nfilt == 0){
        SEND("You should add at least one filter!");
        return;
    }
    if(mode && (nfilt&1)){
        SEND("In MASK mode you should point pairs of ID/MASK");
        return;
    }
    CAN->FMR = CAN_FMR_FINIT;
    uint32_t mask = 1<<bankno;
    CAN->FA1R |= mask; // activate given filter
    if(fifono) CAN->FFA1R |= mask; // set FIFO number
    else CAN->FFA1R &= ~mask;
    if(mode) CAN->FM1R &= ~mask; // MASK
    else CAN->FM1R |= mask; // LIST
    uint32_t F1 = (0x8f<<16);
    uint32_t F2 = (0x8f<<16);
    // reset filter registers to wrong value
    CAN->sFilterRegister[bankno].FR1 = (0x8f<<16) | 0x8f;
    CAN->sFilterRegister[bankno].FR2 = (0x8f<<16) | 0x8f;
    switch(nfilt){
        case 4:
            F2 = filters[3] << 21;
            // fallthrough
        case 3:
            CAN->sFilterRegister[bankno].FR2 = (F2 & 0xffff0000) | (filters[2] << 5);
            // fallthrough
        case 2:
            F1 = filters[1] << 21;
            // fallthrough
        case 1:
            CAN->sFilterRegister[bankno].FR1 = (F1 & 0xffff0000) | (filters[0] << 5);
    }
    CAN->FMR &=~ CAN_FMR_FINIT;
    SEND("Added filter with ");
    printu(nfilt); SEND(" parameters");
}

void canid(char *txt){
    if(txt && *txt){
        int good = FALSE;
        char *eq = getchr(txt, '=');
        if(eq){
            eq = omit_spaces(eq+1);
            if(eq){
                int32_t N;
                if(eq != getnum(eq, &N) && N > -1 && N < 0xfff){
                    the_conf.CANID = (uint16_t)N;
                    good = TRUE;
                }
            }
        }
        if(!good) SEND("CANID setter format: `canid=ID`, ID is 11bit\n");
    }
    SEND("canid="); printuhex(the_conf.CANID);
}

void inpause(_U_ char *txt){
    ShowMsgs = FALSE;
}

void inresume(_U_ char *txt){
    ShowMsgs = TRUE;
}

void delignlist(_U_ char *txt){
    IgnSz = 0;
}

void bootldr(_U_ char *txt){
    SEND("Go into DFU mode\n");
    sendbuf();
    Jump2Boot();
}

void getcounter(_U_ char *txt){
    SEND("CR1="); printu(TIM1->CR1);
    SEND("\nCR2="); printu(TIM2->CR1);
    SEND("\nCR3="); printu(TIM3->CR1);
    SEND("\nCNT1="); printu(TIM1->CNT);
    SEND("\nCNT2="); printu(TIM2->CNT);
    SEND("\nCNT3="); printu(TIM3->CNT);
    NL();
}

void wdcheck(_U_ char *txt){
    while(1){nop();}
}
/*
void stp_check(char *txt){
    uint8_t N = *txt - '0';
    if(N < 3){
        MOTOR_EN(N);
        MOTOR_CW(N);
        mottimers[N]->ARR = 300;
        mottimers[N]->CR1 |= TIM_CR1_CEN;
    }else{
        for(N = 0; N < 3; ++N){
            MOTOR_DIS(N);
            MOTOR_CCW(N);
            mottimers[N]->CR1 &= ~TIM_CR1_CEN;
        }
    }
}*/


typedef void(*specfpointer)(char *arg);

enum{
    SCMD_IGNORE,
    SCMD_DELIGNLIST,
    SCMD_DFU,
    SCMD_FILTER,
    SCMD_CANSPEED,
    SCMD_CANID,
    SCMD_LISTFILTERS,
    SCMD_IGNBUF,
    SCMD_PAUSE,
    SCMD_RESUME,
    SCMD_SEND,
    SCMD_DUMPCONF,
    SCMD_GETCTR,
    SCMD_WD,
    //SCMD_ST,
    SCMD_AMOUNT
};

typedef struct{
    int cmd_code;           // CMD_... or <0 for usb-only commands
    const char *command;    // text command (up to 65536 commands)
    const char *help;       // help message for text protocol
} commands;

// the main commands list, index is CAN command code
static const commands textcommands[] = {
    // different commands
    {0, "", "Different commands:"}, // DELIMETERS
    {CMD_ADC, "adc", "get ADC values"},
    {CMD_BUTTONS, "button", "get buttons state"},
    {CMD_BUZZER, "buzzer", "change buzzer state (1/0)"},
    {CMD_ESWSTATE, "esw", "get end switches state"},
    {CMD_EXT, "ext", "external outputs"},
    {CMD_MCUT, "mcut", "get MCU T"},
    {CMD_MCUVDD, "mcuvdd", "get MCU Vdd"},
    {CMD_PING, "ping", "echo given command back"},
    {CMD_PWM, "pwm", "pwm value"},
    {CMD_RELAY, "relay", "change relay state (1/0)"},
    {CMD_RESET, "reset", "reset MCU"},
    {CMD_TIMEFROMSTART, "time", "get time from start"},
    // configuration
    {0, "", "Confuguration:"},
    {CMD_ACCEL, "accel", "set/get accel/decel (steps/s^2)"},
    {CMD_ENCREV, "encrev", "set/get max encoder's pulses per revolution"},
    {CMD_ENCSTEPMAX, "encstepmax", "maximal encoder ticks per step"},
    {CMD_ENCSTEPMIN, "encstepmin", "minimal encoder ticks per step"},
    {CMD_ESWREACT, "eswreact", "end-switches reaction"},
    {CMD_MAXSPEED, "maxspeed", "set/get max speed (steps per sec)"},
    {CMD_MAXSTEPS, "maxsteps", "set/get max steps (from zero)"},
    {CMD_MICROSTEPS, "microsteps", "set/get microsteps settings"},
    {CMD_MINSPEED, "minspeed", "set/get min speed (steps per sec)"},
    {CMD_MOTFLAGS, "motflags", "set/get motorN flags"},
    {CMD_SAVECONF, "saveconf", "save current configuration"},
    {CMD_SPEEDLIMIT, "speedlimit", "get limiting speed for current microsteps"},
    // motors' commands
    {0, "", "Motors' commands:"},
    {CMD_ABSPOS, "abspos", "set/get position (in steps)"},
    {CMD_EMERGSTOPALL, "emerg", "emergency stop all motors"},
    {CMD_EMERGSTOP, "emstop", "emergency stop motor (right now)"},
    {CMD_ENCPOS, "encpos", "set/get encoder's position"},
    {CMD_GOTOZERO, "gotoz", "find zero position & refresh counters"},
    {CMD_REINITMOTORS, "motreinit", "re-init motors after configuration changed"},
    {CMD_RELPOS, "relpos", "set relative steps, get remaining"},
    {CMD_RELSLOW, "relslow", "set relative steps @ lowest speed"},
    {CMD_MOTORSTATE, "state", "get motor state"},
    {CMD_STOP, "stop", "smooth motor stopping"},
    // USB-only commands
    {0, "", "USB-only commands:"},
    {-SCMD_CANID, "canid", "get/set CAN ID"},
    {-SCMD_CANSPEED, "canspeed", "CAN bus speed"},
    {-SCMD_DELIGNLIST, "delignlist", "delete ignore list"},
    {-SCMD_DFU, "dfu", "activate DFU mode"},
    {-SCMD_DUMPCONF, "dumpconf", "dump current configuration"},
    {-SCMD_FILTER, "filter", "add/modify filter, format: bank# FIFO# mode(M/I) num0 [num1 [num2 [num3]]]"},
    {-SCMD_GETCTR, "getctr", "get TIM1/2/3 counters"},
    {-SCMD_IGNBUF, "ignbuf", "print ignore buffer"},
    {-SCMD_IGNORE, "ignore", "add ID to ignore list (max 10 IDs)"},
    {-SCMD_LISTFILTERS, "listfilters", "list all active filters"},
    {-SCMD_PAUSE, "pause", "pause IN packets displaying"},
    {-SCMD_RESUME, "resume", "resume IN packets displaying"},
    {-SCMD_SEND, "send", "send data over CAN: send ID byte0 .. byteN"},
    //{-SCMD_ST, "st", "check steppers"},
    {-SCMD_WD, "wd", "check watchdog"},
    {0, NULL, NULL}
};


static specfpointer speccmdlist[SCMD_AMOUNT] = {
    [SCMD_IGNORE] = addIGN,
    [SCMD_DELIGNLIST] =  delignlist,
    [SCMD_DFU] =  bootldr,
    [SCMD_FILTER] =  add_filter,
    [SCMD_CANSPEED] =  CANini,
    [SCMD_CANID] =  canid,
    [SCMD_LISTFILTERS] =  list_filters,
    [SCMD_IGNBUF] =  print_ign_buf,
    [SCMD_PAUSE] =  inpause,
    [SCMD_RESUME] =  inresume,
    [SCMD_SEND] =  sendCANcommand,
    [SCMD_DUMPCONF] =  dump_userconf,
    [SCMD_GETCTR] =  getcounter,
    [SCMD_WD] = wdcheck,
    //[SCMD_ST] = stp_check,
};

static void showHelp(){
    SEND("https://github.com/eddyem/stm32samples/tree/master/F0-nolib/3steppersLB build#" BUILD_NUMBER " @ " BUILD_DATE "\n");
    SEND("Common commands format is cmd[ N[ = val]]\n\twhere N is command argument (0..127), val is its value\n");
    //SEND("Commands list:\n");
    const commands *cmd = textcommands;
    while(cmd->command){
        if(*cmd->command){
            bufputchar('\t');
            SEND(cmd->command); /*SEND(" (");
            if(cmd->cmd_code < 0) bufputchar('u');
            else bufputchar('c');
            SEND(") - ");*/
            SEND(" - ");
        }
        SEND(cmd->help); newline();
        ++cmd;
    }
    sendbuf();
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data (will be broken by this function!)
 * @param isUSB - == 1 if data got from USB
 * Common commands format: command [[N]=I], where
 *      command - one of `command` from `cmdlist`
 *      N - optional parameter (0..255)
 *      I - value (int32_t), need for setter
 * Special commands format: s_command [text], where
 *      s_command - one of `spec_cmdlist`
 *      text - optional list of arguments
 * The space after command name is mandatory (it will be substituted by \0)
 */
void cmd_parser(char *txt){
    char cmd[32], *pcmd = cmd;
    int i = 0;
    char *eptr = omit_spaces(txt);
    if(!*eptr) return;
    while(*eptr && i < 30){
        if(*eptr < 'a' || *eptr > 'z') break;
        *pcmd++ = *eptr++;
        ++i;
    }
    *pcmd = 0;
    if(cmd[0] == 0){ // empty command
        showHelp();
        return;
    }
    if(eptr && *eptr){
        eptr = omit_spaces(eptr);
    }
    // find command
    const commands *c = textcommands;
    while(c->command){
        if(0 == cmpstr(c->command, cmd)){
#ifdef EBUG
        SEND("Find known command: "); SEND(cmd);
        if(eptr && *eptr) SEND(", args: "); SEND(eptr);
        NL();
#endif
            if(c->cmd_code < 0){ // USB-only command
                speccmdlist[-(c->cmd_code)](eptr);
            }else{ // common command
                uint8_t par = CANMESG_NOPAR;
                int32_t val = 0;
                if(eptr && *eptr){
                    char *nxt = getnum(eptr, &val);
                    if(nxt && nxt != eptr){ // command has parameter?
                        if(val < 0 || val >= CANMESG_NOPAR){
                            SEND("Command parameter should be 0..126!"); NL();
                            return;
                        }
                        par = (uint8_t)val;
                    }else nxt = eptr;
                    eptr = getchr(nxt, '=');
                    if(eptr){ // command has value?
                        eptr = omit_spaces(eptr + 1);
                        nxt = getnum(eptr, &val);
                        if(nxt != eptr){
                            par |= 0x80; // setter
                        }
                    }
                }
                // here we got command & ppar/pval -> call CMD
                errcodes retcode = cmdlist[c->cmd_code](par, &val);
                SEND(cmd);
                par &= 0x7f;
                if(par != CANMESG_NOPAR) printu(par);
                bufputchar('='); printi(val);
                SEND(" ("); printuhex((uint32_t)val); bufputchar(')');
                if(ERR_OK != retcode){
                    SEND("\nERRCODE=");
                    printu(retcode);
                }
            }
            NL();
            return;
        }
        ++c;
    }
    showHelp();
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
void printi(int32_t val){
    if(val < 0){
        val = -val;
        bufputchar('-');
    }
    printu((uint32_t)val);
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

// check Ignore_IDs & return 1 if ID isn't in list
uint8_t isgood(uint16_t ID){
    for(int i = 0; i < IgnSz; ++i)
        if(Ignore_IDs[i] == ID) return 0;
    return 1;
}

char *omit_spaces(char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return buf;
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
