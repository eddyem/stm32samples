/*
 * This file is part of the chronometer project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "GPS.h"
#include "lidar.h"
#include "str.h"
#include "time.h"
#include "usart.h"
#include "usb.h"

// flag to show new GPS message over USB
uint8_t showGPSstr = 0;

extern uint32_t shotms[];

/**
 * @brief cmpstr - the same as strncmp
 * @param s1,s2 - strings to compare
 * @param n - max symbols amount
 * @return 0 if strings equal or 1/-1
 */
int cmpstr(const char *s1, const char *s2, int n){
    int ret = 0;
    while(--n){
        ret = *s1 - *s2;
        if(ret == 0 && *s1 && *s2){
            ++s1; ++s2;
            continue;
        }
        break;
    }
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

#define sendu(x) do{USB_send(u2str(x));}while(0)

static void sendi(int32_t I){
    if(I < 0){
        USB_send("-");
        I = -I;
    }
    USB_send(u2str((uint32_t)I));
}

/**
 * @brief showuserconf - show configuration over USB
 */
static void showuserconf(){
    USB_send("DISTMIN="); sendu(the_conf.dist_min);
    USB_send("\nDISTMAX="); sendu(the_conf.dist_max);
    USB_send("\nADCMIN="); sendi(the_conf.ADC_min);
    USB_send("\nADCMAX="); sendi(the_conf.ADC_max);
    USB_send("\nTRIGLVL="); sendu(the_conf.trigstate);
    USB_send("\nTRIGPAUSE={");
    for(int i = 0; i < TRIGGERS_AMOUNT; ++i){
        if(i) USB_send(", ");
        sendu(the_conf.trigpause[i]);
    }
    USB_send("}");
    USB_send("\nUSART1SPD="); sendu(the_conf.USART_speed);
    USB_send("\nSTREND=");
    if(the_conf.defflags & FLAG_STRENDRN) USB_send("RN");
    else USB_send("N");
    uint8_t f = the_conf.defflags;
    USB_send("\nSAVE_EVENTS=");
    if(f & FLAG_SAVE_EVENTS) USB_send("1");
    else USB_send("0");
    USB_send("\nGPSPROXY=");
    if(f & FLAG_GPSPROXY) USB_send("1");
    else USB_send("0");
    USB_send("\nNFREE=");
    sendu(the_conf.NLfreeWarn);
    USB_send("\n");
}

/**
 * @brief parse_USBCMD - parsing of string buffer got by USB
 * @param cmd - buffer with commands
 * @return 0 if got command, 1 if command not recognized
 */
int parse_USBCMD(char *cmd){
#define CMP(a,b)  cmpstr(a, b, sizeof(b))
#define GETNUM(x) if(getnum(cmd+sizeof(x)-1, &N)) goto bad_number;
    static uint8_t conf_modified = 0;
    uint8_t succeed = 0;
    int32_t N;
    if(!cmd || !*cmd) return 0;
    IWDG->KR = IWDG_REFRESH;
    if(*cmd == '?'){ // help
        USB_send("Commands:\n"
                 CMD_ADCMAX    " - max ADC value treshold for trigger\n"
                 CMD_ADCMIN    " - min -//- (triggered when ADval>min & <max)\n"
                 CMD_GETADCVAL " - get ADC value\n"
                 CMD_BUZZER    "S - turn buzzer ON/OFF\n"
                 CMD_CURDIST   " - show current LIDAR distance\n"
                 CMD_DELLOGS   " - delete logs from flash memory\n"
                 CMD_DISTMIN   " - min distance threshold (cm)\n"
                 CMD_DISTMAX   " - max distance threshold (cm)\n"
                 CMD_DUMP      "N - dump 20 last stored events (no x), all (x<1) or x\n"
                 CMD_FLASH     " - FLASH info\n"
                 CMD_GPSPROXY  "S - GPS proxy over USART1 on/off\n"
                 CMD_GPSRESTART " - send Full Cold Restart to GPS\n"
                 CMD_GPSSTAT   " - get GPS status\n"
                 CMD_GPSSTR    " - current GPS data string\n"
                 CMD_LEDS      "S - turn leds on/off (1/0)\n"
                 CMD_GETMCUTEMP " - MCU temperature\n"
                 CMD_NFREE     " - warn when free logs space less than this number (0 - not warn)"
                 CMD_PRINTTIME " - print time\n"
                 CMD_RESET     " - reset MCU\n"
                 CMD_SAVEEVTS  "S - save/don't save (1/0) trigger events into flash\n"
                 CMD_SHOWCONF  " - show current configuration\n"
                 CMD_STORECONF " - store new configuration in flash\n"
                 CMD_STREND    "C - string ends with \\n (C=n) or \\r\\n (C=r)\n"
                 CMD_PRINTTIME " - print current time\n"
                 CMD_TRIGLVL   "NS - working trigger N level S\n"
                 CMD_TRGPAUSE  "NP - pause (P, ms) after trigger N shots\n"
                 CMD_TRGTIME   "N - show last trigger N time\n"
                 CMD_USARTSPD  "N - set USART1 speed to N\n"
                 CMD_GETVDD    " - Vdd value\n"
                 );
    }else if(CMP(cmd, CMD_PRINTTIME) == 0){
        USB_send(get_time(&current_time, get_millis()));
        USB_send("\n");
    }else if(CMP(cmd, CMD_DISTMIN) == 0){ // set low limit
        DBG("CMD_DISTMIN");
        GETNUM(CMD_DISTMIN);
        if(N < 0 || N > 0xffff) goto bad_number;
        if(the_conf.dist_min != (uint16_t)N){
            conf_modified = 1;
            the_conf.dist_min = (uint16_t) N;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_DISTMAX) == 0){ // set low limit
        DBG("CMD_DISTMAX");
        GETNUM(CMD_DISTMAX);
        if(N < 0 || N > 0xffff) goto bad_number;
        if(the_conf.dist_max != (uint16_t)N){
            conf_modified = 1;
            the_conf.dist_max = (uint16_t) N;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_STORECONF) == 0){ // store everything
        DBG("Store");
        if(conf_modified){
            if(store_userconf()){
                USB_send("Error: can't save data!\n");
            }else{
                conf_modified = 0;
                succeed = 1;
            }
        }
    }else if(CMP(cmd, CMD_GPSSTR) == 0){ // show GPS status string
        showGPSstr = 1;
    }else if(CMP(cmd, CMD_TRIGLVL) == 0){
        DBG("Trig levels");
        cmd += sizeof(CMD_TRIGLVL) - 1;
        uint8_t Nt = *cmd++ - '0';
        if(Nt > TRIGGERS_AMOUNT - 1) goto bad_number;
        uint8_t state = *cmd -'0';
        if(state > 1) goto bad_number;
        uint8_t oldval = the_conf.trigstate;
        if(!state) the_conf.trigstate = oldval & ~(1<<Nt);
        else the_conf.trigstate = oldval | (1<<Nt);
        if(oldval != the_conf.trigstate) conf_modified = 1;
        succeed = 1;
    }else if(CMP(cmd, CMD_SHOWCONF) == 0){
        showuserconf();
    }else if(CMP(cmd, CMD_TRGPAUSE) == 0){
        DBG("Trigger pause");
        cmd += sizeof(CMD_TRGPAUSE) - 1;
        uint8_t Nt = *cmd++ - '0';
        if(Nt > TRIGGERS_AMOUNT - 1) goto bad_number;
        if(getnum(cmd, &N)) goto bad_number;
        if(N < 0 || N > 10000) goto bad_number;
        if(the_conf.trigpause[Nt] != N){
            conf_modified = 1;
            the_conf.trigpause[Nt] = N;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_TRGTIME) == 0){
        DBG("Trigger time");
        cmd += sizeof(CMD_TRGTIME) - 1;
        uint8_t Nt = *cmd++ - '0';
        if(Nt > TRIGGERS_AMOUNT - 1) goto bad_number;
        show_trigger_shot((uint8_t)1<<Nt);
    }else if(CMP(cmd, CMD_GETVDD) == 0){
        USB_send("VDD=");
        uint32_t vdd = getVdd();
        sendu(vdd/100);
        vdd %= 100;
        if(vdd < 10) USB_send(".0");
        else USB_send(".");
        sendu(vdd);
        USB_send("\n");
    }else if(CMP(cmd, CMD_GETMCUTEMP) == 0){
        int32_t t = getMCUtemp();
        USB_send("MCUTEMP=");
        if(t < 0){
            t = -t;
            USB_send("-");
        }
        sendu(t/10);
        USB_send(".");
        sendu(t%10);
        USB_send("\n");
    }else if(CMP(cmd, CMD_GETADCVAL) == 0){
        USB_send("ADCVAL=");
        sendu(getADCval(0));
        USB_send("\n");
    }else if(CMP(cmd, CMD_LEDS) == 0){
        uint8_t Nt = cmd[sizeof(CMD_LEDS) - 1] - '0';
        if(Nt > 1) goto bad_number;
        USB_send("LEDS=");
        if(Nt){
            LEDSon = 1;
            USB_send("ON\n");
        }else{
            LED_off();  // turn off LEDS
            LED1_off(); // by user request
            LEDSon = 0;
            USB_send("OFF\n");
        }
    }else if(CMP(cmd, CMD_ADCMAX) == 0){ // set low limit
        GETNUM(CMD_ADCMAX);
        if(N < -4096 || N > 4096) goto bad_number;
        if(the_conf.ADC_max != (int16_t)N){
            conf_modified = 1;
            the_conf.ADC_max = (int16_t) N;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_ADCMIN) == 0){ // set low limit
        GETNUM(CMD_ADCMIN);
        if(N < -4096 || N > 4096) goto bad_number;
        if(the_conf.ADC_min != (int16_t)N){
            conf_modified = 1;
            the_conf.ADC_min = (int16_t) N;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_GPSRESTART) == 0){
        USB_send("Send full cold restart to GPS\n");
        GPS_send_FullColdStart();
    }else if(CMP(cmd, CMD_BUZZER) == 0){
        uint8_t Nt = cmd[sizeof(CMD_BUZZER) - 1] - '0';
        if(Nt > 1) goto bad_number;
        USB_send("BUZZER=");
        if(Nt){
            buzzer_on = 1;
            USB_send("ON\n");
        }else{
            buzzer_on = 0;
            USB_send("OFF\n");
        }
    }else if(CMP(cmd, CMD_GPSSTAT) == 0){
        USB_send("GPS status: ");
        const char *str = "unknown";
        switch(GPS_status){
            case GPS_NOTFOUND:
                str = "not found";
            break;
            case GPS_WAIT:
                str = "waiting";
            break;
            case GPS_NOT_VALID:
                str = "no satellites";
            break;
            case GPS_VALID:
                str = "valid time";
            break;
        }
        USB_send(str);
        if(Tms - last_corr_time < 1500)
            USB_send(", PPS working\n");
        else
            USB_send(", no PPS\n");
    }else if(CMP(cmd, CMD_USARTSPD) == 0){
        GETNUM(CMD_USARTSPD);
        if(N < 400 || N > 3000000) goto bad_number;
        if(the_conf.USART_speed != (uint32_t)N){
            the_conf.USART_speed = (uint32_t)N;
            conf_modified = 1;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_RESET) == 0){
        USB_send("Soft reset\n");
        NVIC_SystemReset();
    }else if(CMP(cmd, CMD_STREND) == 0){
        char c = cmd[sizeof(CMD_STREND) - 1];
        succeed = 1;
        if(c == 'n' || c == 'N'){
            if(the_conf.defflags & FLAG_STRENDRN){
                conf_modified = 1;
                the_conf.defflags &= ~FLAG_STRENDRN;
            }
        }else if(c == 'r' || c == 'R'){
            if(!(the_conf.defflags & FLAG_STRENDRN)){
                conf_modified = 1;
                the_conf.defflags |= FLAG_STRENDRN;
            }
        }else{
            succeed = 0;
            USB_send("Bad letter, should be 'n' or 'r'\n");
        }
    }else if(CMP(cmd, CMD_FLASH) == 0){ // show flash size
        USB_send("FLASHSIZE=");
        sendu(FLASH_SIZE);
        USB_send("kB\nFLASH_BASE=");
        USB_send(u2hex(FLASH_BASE));
        USB_send("\nFlash_Data=");
        USB_send(u2hex((uint32_t)Flash_Data));
        USB_send("\nvarslen=");
        sendu((uint32_t)&_varslen);
        USB_send("\nCONFsize=");
        sendu(sizeof(user_conf));
        USB_send("\nNconf_records=");
        sendu(maxCnum - 1);
        USB_send("\nlogsstart=");
        USB_send(u2hex((uint32_t)logsstart));
        USB_send("\nLOGsize=");
        sendu(sizeof(event_log));
        USB_send("\nNlogs_records=");
        sendu(maxLnum - 1);
        USB_send("\n");
    }else if(CMP(cmd, CMD_SAVEEVTS) == 0){
        if('0' == cmd[sizeof(CMD_SAVEEVTS) - 1]){
            if(the_conf.defflags & FLAG_SAVE_EVENTS){
                conf_modified = 1;
                the_conf.defflags &= ~FLAG_SAVE_EVENTS;
            }
        }else{
            if(!(the_conf.defflags & FLAG_SAVE_EVENTS)){
                conf_modified = 1;
                the_conf.defflags |= FLAG_SAVE_EVENTS;
            }
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_DUMP) == 0){
        if(getnum(cmd+sizeof(CMD_DUMP)-1, &N)) N = -20; // default - without N
        else N = -N;
        if(N > 0) N = 0;
        if(dump_log(N, -1)) USB_send("Event log empty!\n");
    }else if(CMP(cmd, CMD_NFREE) == 0){
        GETNUM(CMD_NFREE);
        if(N < 0 || N > 0xffff) goto bad_number;
        if(the_conf.NLfreeWarn != (uint16_t)N){
            conf_modified = 1;
            the_conf.NLfreeWarn = (uint16_t)N;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_DELLOGS) == 0){
        if(store_log(NULL)) USB_send("Error during erasing flash\n");
        else USB_send("All logs erased\n");
    }else if(CMP(cmd, CMD_GPSPROXY) == 0){
        if(cmd[sizeof(CMD_GPSPROXY) - 1] == '0'){
            if(the_conf.defflags & FLAG_GPSPROXY){
                conf_modified = 1;
                the_conf.defflags &= ~FLAG_GPSPROXY;
            }
        }else{
            if(!(the_conf.defflags & FLAG_GPSPROXY)){
                conf_modified = 1;
                the_conf.defflags |= FLAG_GPSPROXY;
            }
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_CURDIST) == 0){
        USB_send("DIST=");
        sendu(last_lidar_dist);
        USB_send("\nSTREN=");
        sendu(last_lidar_stren);
        USB_send("\nTRIGDIST=");
        sendu(lidar_triggered_dist);
        USB_send("\nTms=");
        sendu(Tms);
        USB_send("\nshotms=");
        sendu(shotms[LIDAR_TRIGGER]);
        USB_send("\n");
    }else return 1;
    /*else if(CMP(cmd, CMD_) == 0){
        ;
    }*/

    IWDG->KR = IWDG_REFRESH;
    if(succeed) USB_send("Success!\n");
    return 0;
  bad_number:
    USB_send("Error: bad number!\n");
    return 0;
}

/**
 * @brief get_trigger_shot - print on USB message about last trigger shot time
 * @param number  - number of event (if > -1)
 * @param logdata - record from event log
 * @return string with data
 */
char *get_trigger_shot(int number, const event_log *logdata){
    static char buf[64];
    char *bptr = buf;
    if(number > -1){
        bptr = strcp(bptr, u2str(number));
        bptr = strcp(bptr, ": ");
    }
    if(logdata->trigno == LIDAR_TRIGGER){
        bptr = strcp(bptr, "LIDAR, dist=");
        bptr = strcp(bptr, u2str(logdata->lidar_dist));
        bptr = strcp(bptr, ", TRIG" STR(LIDAR_TRIGGER));
    }else{
        bptr = strcp(bptr, "TRIG");
        *bptr++ = '0' + logdata->trigno;
    }
    *bptr++ = '=';
    IWDG->KR = IWDG_REFRESH;
    bptr = strcp(bptr, get_time(&logdata->shottime.Time, logdata->shottime.millis));
    bptr = strcp(bptr, ", len=");
    if(logdata->triglen < 0) bptr = strcp(bptr, ">1s");
    else bptr = strcp(bptr, u2str((uint32_t) logdata->triglen));
    *bptr++ = '\n'; *bptr++ = 0;
    return buf;
}

/**
 * @brief show_trigger_shot printout @ USB data with all triggers shot recently (+ save it in flash)
 * @param tshot - each bit consists information about trigger
 */
void show_trigger_shot(uint8_t tshot){
    uint8_t X = 1;
    for(int i = 0; i < TRIGGERS_AMOUNT && tshot; ++i, X <<= 1){
        IWDG->KR = IWDG_REFRESH;
        if(tshot & X) tshot &= ~X;
        else continue;
        event_log l;
        l.elog_sz = sizeof(event_log);
        l.trigno = i;
        if(i == LIDAR_TRIGGER) l.lidar_dist = lidar_triggered_dist;
        l.shottime = shottime[i];
        l.triglen = triglen[i];
        USB_send(get_trigger_shot(-1, &l));
        if(the_conf.defflags & FLAG_SAVE_EVENTS){
            if(store_log(&l)) USB_send("\n\nError saving event!\n\n");
        }
    }
}

/**
 * @brief strln == strlen
 * @param s - string
 * @return length
 */
int strln(const char *s){
    int i = 0;
    while(*s++) ++i;
    return i;
}

/**
 * @brief strcp - strcpy (be carefull: it doesn't checks destination length!)
 * @param dst - destination
 * @param src - source
 * @return pointer to '\0' @ dst`s end
 */
char *strcp(char* dst, const char *src){
    int l = strln(src);
    if(l < 1) return dst;
    while((*dst++ = *src++));
    return dst - 1;
}


// read `buf` and get first integer `N` in it
// @return 0 if all OK or 1 if there's not a number; omit spaces and '='
int getnum(const char *buf, int32_t *N){
    char c;
    int positive = -1;
    int32_t val = 0;
    while((c = *buf++)){
        if(c == '\t' || c == ' ' || c == '='){
            if(positive < 0) continue; // beginning spaces
            else break; // spaces after number
        }
        if(c == '-'){
            if(positive < 0){
                positive = 0;
                continue;
            }else break; // there already was `-` or number
        }
        if(c < '0' || c > '9') break;
        if(positive < 0) positive = 1;
        val = val * 10 + (int32_t)(c - '0');
    }
    if(positive != -1){
        if(positive == 0){
            if(val == 0) return 1; // single '-'
            val = -val;
        }
        *N = val;
    }else return 1;
    return 0;
}

static char strbuf[11];
// return string buffer (strbuf) with val
char *u2str(uint32_t val){
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

// return strbuf filled with hex
char *u2hex(uint32_t val){
    char *bufptr = strbuf;
    *bufptr++ = '0';
    *bufptr++ = 'x';
    uint8_t *ptr = (uint8_t*)&val + 3;
    int i, j;
    IWDG->KR = IWDG_REFRESH;
    for(i = 0; i < 4; ++i, --ptr){
        for(j = 1; j > -1; --j){
            register uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) *bufptr++ = half + '0';
            else *bufptr++ = half - 10 + 'a';
        }
    }
    *bufptr = 0;
    return strbuf;
}
