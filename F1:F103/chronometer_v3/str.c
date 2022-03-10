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

// Commands parser

#include "adc.h"
#include "GPS.h"
#include "fonts.h"
#include "hardware.h"
#include "lidar.h"
#include "screen.h"
#include "str.h"
#include "time.h"
#include "usart.h"
#include "usb.h"

// flag to show new GPS message over USB
uint8_t showGPSstr = 0;
// check/not check shutter
uint8_t chkshtr = 1;
// show/not show shutter events
uint8_t showshtr = 1;
// show time or message; starting types; time to next start (once/auto)
uint8_t showtime = 0, startflags = 0, timetostartO = 0, timetostartA = 0;

extern uint32_t shotms[];

/**
 * @brief cmpstr - the same as strncmp
 * @param s1,s2 - strings to compare
 * @param n - max symbols amount + 1 (!!!!)
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

#define sendu(x) do{sendstring(u2str(x));}while(0)

/*
static void sendi(int32_t I){
    if(I < 0){
        sendchar('-');
        I = -I;
    }
    sendstring(u2str((uint32_t)I));
}*/

// echo '1' if true or '0' if false
static void checkflag(uint8_t f){
    if(f) sendchar('1');
    else sendchar('0');
}

/**
 * @brief showuserconf - show configuration over USB
 */
static void showuserconf(){
    sendstring("DISTMIN="); sendu(the_conf.dist_min);
    sendstring("\nDISTMAX="); sendu(the_conf.dist_max);
    sendstring("\nTRIGLVL="); sendu(the_conf.trigstate);
    sendstring("\nTRIGPAUSE={");
    for(int i = 0; i < TRIGGERS_AMOUNT; ++i){
        if(i) sendstring(", ");
        sendu(the_conf.trigpause[i]);
    }
    sendstring("}\nUSART1SPD="); sendu(the_conf.USART_speed);
    sendstring("\nLIDARSPD="); sendu(the_conf.LIDAR_speed);
    sendstring("\nNFREE=");
    sendu(the_conf.NLfreeWarn);
    sendstring("\nSTREND=");
    if(the_conf.defflags & FLAG_STRENDRN) sendstring("RN");
    else sendchar('N');
    uint8_t f = the_conf.defflags;
    sendstring("\nSAVE_EVENTS=");
    checkflag(f & FLAG_SAVE_EVENTS);
    sendstring("\nGPSPROXY=");
    checkflag(f & FLAG_GPSPROXY);
    sendstring("\nLIDAR=");
    checkflag(!(f & FLAG_NOLIDAR));
    sendstring("\nEVTLEN="); sendu(the_conf.ledshow_time);
    sendstring("\n"); // <-- sendstring @ the end to initialize data transmission
}

extern uint8_t USB_connected; // need to reset USB
/**
 * @brief parse_USBCMD - parsing of string buffer got by USB
 * @param cmd - buffer with commands
 * @return 0 if got command, 1 if command not recognized
 */
void parse_CMD(char *cmd){
    char *oldcmd = cmd;
#define CMP(a,b)  cmpstr(a, b, sizeof(b))
#define GETNUM(x) do{if(getnum(cmd+sizeof(x)-1, &N)) goto bad_number;}while(0)
    static uint8_t conf_modified = 0;
    uint8_t succeed = 0;
    int32_t N;
    if(!cmd || !*cmd) return;
    IWDG->KR = IWDG_REFRESH;
    if(*cmd == '?' || CMP(cmd, "help") == 0){ // help
        sendstring("Commands:\n"
                    CMD_BTNSTATE    " - show triggers state\n"
                    CMD_BUZZER      "S - turn buzzer ON/OFF\n"
                    CMD_CLEARSCRN   " - turn LED display off\n"
                    CMD_CURDIST     " - show current LIDAR distance\n"
                    CMD_DELLOGS     " - delete logs from flash memory\n"
                    CMD_DISTMIN     " - min distance threshold (cm)\n"
                    CMD_DISTMAX     " - max distance threshold (cm)\n"
                    CMD_DUMP        "N - dump 20 last stored events (no x), all (x<1) or x\n"
                    CMD_EVTLEN      "N - duration of the trigger event display (ms)\n"
                    CMD_FLASH       " - FLASH info\n"
                    CMD_GATE        "S - check/not check triggers (1/0)\n"
                    CMD_GPSPROXY    "S - GPS proxy over USART1 on/off\n"
                    CMD_GPSRESTART  " - send Full Cold Restart to GPS\n"
                    CMD_GPSSTAT     " - get GPS status\n"
                    CMD_GPSSTR      " - current GPS data string\n"
                    CMD_LEDS        "S - turn leds on/off (1/0)\n"
                    CMD_LIDAR       "S - switch between LIDAR (1) or command TTY (0)\n"
                    CMD_LIDARSPEED  "N - set LIDAR speed to N\n"
                    CMD_GETMCUTEMP  " - MCU temperature\n"
                    CMD_MESG        " str - show 'str' at display (no more than 7 chars)\n"
                    CMD_DUMPN       "N - dump Nth log & show on screen (-N - Nth from last)\n"
                    CMD_NFREE       " - warn when free logs space less than this number (0 - not warn)\n"
                    CMD_RESET       " - reset MCU\n"
                    CMD_SAVEEVTS    "S - save/don't save (1/0) trigger events into flash\n"
                    CMD_SHOWCONF    " - show current configuration\n"
                    CMD_SHOWSHTR    "S - show/not show trigger events\n"
                    CMD_SHOWTIME    " - show current time\n"
                    CMD_SQUEAK      " - make a short buzzer chirp\n"
                    CMD_STARTAUTO   "X - auto start every X minutes (0 or absent - cancel, +/- - increase/decrease by 1min)\n"
                    CMD_STARTONCE   "X - delayed start after X minutes (like auto)\n"
                    CMD_STORECONF   " - store new configuration in flash\n"
                    CMD_STORTEST    " - add test trigger event record into flash\n"
                    CMD_STREND      "C - string ends with \\n (C=n) or \\r\\n (C=r)\n"
                    CMD_PRINTTIME   " - print current time\n"
                    CMD_TRIGLVL     "NS - working trigger N level S\n"
                    CMD_TRGPAUSE    "NP - pause (P, ms) after trigger N shots\n"
                    CMD_TRGTIME     "N - show last trigger N time\n"
                    CMD_USARTSPD    "N - set USART1 speed to N\n"
                    CMD_USBRST      " - reset USB connectioin\n"
                    CMD_GETVDD      " - Vdd value\n"
                 );
    }else if(CMP(cmd, CMD_PRINTTIME) == 0){ // print current time
        sendstring(get_time(&current_time, get_millis()));
        sendstring("\n");
    }else if(CMP(cmd, CMD_DISTMIN) == 0){ // set low LIDAR limit
        GETNUM(CMD_DISTMIN);
        if(N < 0 || N > 0xffff) goto bad_number;
        if(the_conf.dist_min != (uint16_t)N){
            conf_modified = 1;
            the_conf.dist_min = (uint16_t) N;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_DISTMAX) == 0){ // set high LIDAR limit
        GETNUM(CMD_DISTMAX);
        if(N < 0 || N > 0xffff) goto bad_number;
        if(the_conf.dist_max != (uint16_t)N){
            conf_modified = 1;
            the_conf.dist_max = (uint16_t) N;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_STORECONF) == 0){ // store everything in flash
        if(conf_modified){
            if(store_userconf()){
                sendstring("Error: can't save data!\n");
            }else{
                conf_modified = 0;
                succeed = 1;
            }
        }
    }else if(CMP(cmd, CMD_GPSSTR) == 0){ // show GPS status string
        showGPSstr = 1;
    }else if(CMP(cmd, CMD_TRIGLVL) == 0){ // trigger levels: 0->1 or 1->0
        cmd += sizeof(CMD_TRIGLVL) - 1;
        uint8_t Nt = (uint8_t)(*cmd++ - '0');
        if(Nt > TRIGGERS_AMOUNT - 1) goto bad_number;
        uint8_t state = (uint8_t)(*cmd -'0');
        if(state > 1) goto bad_number;
        uint8_t oldval = the_conf.trigstate;
        if(!state) the_conf.trigstate = oldval & ~(1<<Nt);
        else the_conf.trigstate = (uint8_t)(oldval | (1<<Nt));
        if(oldval != the_conf.trigstate) conf_modified = 1;
        succeed = 1;
    }else if(CMP(cmd, CMD_SHOWCONF) == 0){ // print current configuration
        showuserconf();
    }else if(CMP(cmd, CMD_TRGPAUSE) == 0){ // pause after Nth trigger
        cmd += sizeof(CMD_TRGPAUSE) - 1;
        uint8_t Nt = (uint8_t)(*cmd++ - '0');
        if(Nt > TRIGGERS_AMOUNT - 1) goto bad_number;
        if(getnum(cmd, &N)) goto bad_number;
        if(N < 0 || N > 10000) goto bad_number;
        if(the_conf.trigpause[Nt] != (uint16_t)N){
            conf_modified = 1;
            the_conf.trigpause[Nt] = (uint16_t)N;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_TRGTIME) == 0){ // last trigger time
        cmd += sizeof(CMD_TRGTIME) - 1;
        uint8_t Nt = (uint8_t)(*cmd++ - '0');
        if(Nt > TRIGGERS_AMOUNT - 1) goto bad_number;
        show_trigger_shot((uint8_t)(1<<Nt));
    }else if(CMP(cmd, CMD_GETVDD) == 0){ // Vdd
        sendstring("VDD=");
        uint32_t vdd = getVdd();
        sendu(vdd/100);
        vdd %= 100;
        if(vdd < 10) sendstring(".0");
        else sendstring(".");
        sendu(vdd);
        sendstring("\n");
    }else if(CMP(cmd, CMD_GETMCUTEMP) == 0){ // ~Tmcu
        int32_t t = getMCUtemp();
        sendstring("MCUTEMP=");
        if(t < 0){
            t = -t;
            sendstring("-");
        }
        sendu(t/10);
        sendstring(".");
        sendu(t%10);
        sendstring("\n");
    }else if(CMP(cmd, CMD_LEDS) == 0){ // turn LEDs on/off
        uint8_t Nt = (uint8_t)(cmd[sizeof(CMD_LEDS) - 1] - '0');
        if(Nt > 1) goto bad_number;
        sendstring("LEDS=");
        if(Nt){
            LEDSon = 1;
            sendstring("ON\n");
        }else{
            LED_off();  // turn off LEDS
            LED1_off(); // by user request
            LEDSon = 0;
            sendstring("OFF\n");
        }
    }else if(CMP(cmd, CMD_GPSRESTART) == 0){ // restart GPS
        sendstring("Send full cold restart to GPS\n");
        GPS_send_FullColdStart();
    }else if(CMP(cmd, CMD_BUZZER) == 0){
        uint8_t Nt = (uint8_t)(cmd[sizeof(CMD_BUZZER) - 1] - '0');
        if(Nt > 1) goto bad_number;
        sendstring("BUZZER=");
        if(Nt){
            buzzer_on = 1;
            sendstring("ON\n");
        }else{
            buzzer_on = 0;
            sendstring("OFF\n");
        }
    }else if(CMP(cmd, CMD_GPSSTAT) == 0){ // GPS status
        sendstring("GPS status: ");
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
        sendstring(str);
        if(Tms - last_corr_time < 1500)
            sendstring(", PPS working\n");
        else
            sendstring(", no PPS\n");
    }else if(CMP(cmd, CMD_USARTSPD) == 0){ // USART speed
        GETNUM(CMD_USARTSPD);
        if(N < 400 || N > 3000000) goto bad_number;
        if(the_conf.USART_speed != (uint32_t)N){
            the_conf.USART_speed = (uint32_t)N;
            conf_modified = 1;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_LIDARSPEED) == 0){ // LIDAR speed
        GETNUM(CMD_LIDARSPEED);
        if(N < 400 || N > 3000000) goto bad_number;
        if(the_conf.LIDAR_speed != (uint32_t)N){
            the_conf.LIDAR_speed = (uint32_t)N;
            conf_modified = 1;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_RESET) == 0){ // Reset MCU
        sendstring("Soft reset\n");
        NVIC_SystemReset();
    }else if(CMP(cmd, CMD_STREND) == 0){ // string ends in '\n' or "\r\n"
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
            sendstring("Bad letter, should be 'n' or 'r'\n");
        }
    }else if(CMP(cmd, CMD_FLASH) == 0){ // show flash size
        sendstring("FLASHSIZE=");
        sendu(FLASH_SIZE);
        sendstring("kB\nFLASH_BASE=");
        sendstring(u2hex(FLASH_BASE));
        sendstring("\nFlash_Data=");
        sendstring(u2hex((uint32_t)Flash_Data));
        sendstring("\nvarslen=");
        sendu((uint32_t)&_varslen);
        sendstring("\nCONFsize=");
        sendu(sizeof(user_conf));
        sendstring("\nNconf_records=");
        sendu(maxCnum - 1);
        sendstring("\nlogsstart=");
        sendstring(u2hex((uint32_t)logsstart));
        sendstring("\nLOGsize=");
        sendu(sizeof(event_log));
        sendstring("\nNlogs_records=");
        sendu(maxLnum - 1);
        sendstring("\n");
    }else if(CMP(cmd, CMD_SAVEEVTS) == 0){ // save all events
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
    }else if(CMP(cmd, CMD_DUMP) == 0){ // dump N last events
        if(getnum(cmd+sizeof(CMD_DUMP)-1, &N)) N = -20; // default - without N
        else N = -N;
        if(N > 0) N = 0;
        if(dump_log(N, -1)) sendstring("Event log empty!\n");
    }else if(CMP(cmd, CMD_NFREE) == 0){ // warn if there's less than N free cells for logs in flash
        GETNUM(CMD_NFREE);
        if(N < 0 || N > 0xffff) goto bad_number;
        if(the_conf.NLfreeWarn != (uint16_t)N){
            conf_modified = 1;
            the_conf.NLfreeWarn = (uint16_t)N;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_DELLOGS) == 0){ // delete all logs
        if(store_log(NULL)) sendstring("Error during erasing flash\n");
        else sendstring("All logs erased\n");
    }else if(CMP(cmd, CMD_GPSPROXY) == 0){ // proxy GPS data over USART1
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
    }else if(CMP(cmd, CMD_CURDIST) == 0){ // current LIDAR distance
        sendstring("DIST=");
        sendu(last_lidar_dist);
        sendstring("\nSTREN=");
        sendu(last_lidar_stren);
        sendstring("\nTRIGDIST=");
        sendu(lidar_triggered_dist);
        sendstring("\nTms=");
        sendu(Tms);
        sendstring("\nshotms=");
        sendu(shotms[LIDAR_TRIGGER]);
        sendstring("\n");
    }else if(CMP(cmd, CMD_LIDAR) == 0){ // turn LIDAR on/off
        if(cmd[sizeof(CMD_LIDAR) - 1] == '0'){
            if(!(the_conf.defflags & FLAG_NOLIDAR)){
                conf_modified = 1;
                the_conf.defflags |= FLAG_NOLIDAR;
            }
        }else{
            if(the_conf.defflags & FLAG_NOLIDAR){
                conf_modified = 1;
                the_conf.defflags &= ~FLAG_NOLIDAR;
            }
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_CLEARSCRN) == 0){ // turn display off
        sendstring("Clear screen\n");
        showtime = 0;
        ScreenOFF();
    }else if(CMP(cmd, CMD_MESG) == 0){ // show short message instead of time
        char *m = &cmd[sizeof(CMD_MESG) - 1];
        while(*m == ' ' || *m == '\t') ++m;
        showtime = 0;
        ScreenOFF();
        choose_font(FONT14);
        uint16_t w = GetStrWidth(m);
        PutStringAt((SCREEN_WIDTH-w-1)/2, SCREEN_HEIGHT-1-curfont->baseline, m);
        ConvertScreenBuf();
        ShowScreen();
    }else if(CMP(cmd, CMD_SHOWTIME) == 0){ // show current time @ LED display
        ScreenOFF();
        showtime = 1;
    }else if(CMP(cmd, CMD_STORTEST) == 0){ // test storing event log
        event_log l;
        l.trigno = 1;
        l.shottime.Time = current_time;
        l.shottime.millis = Timer;
        l.triglen = 10;
        DBG("Try to store test");
        if(store_log(&l)) sendstring("Error storing\n");
        else succeed = 1;
    }else if(CMP(cmd, CMD_BTNSTATE) == 0){ // show gates/buttons state
        char btns[] = "BTN0=0, BTN1=0, BTN2=0, PPS=0\n";
        btns[5]  = gettrig(0) + '0';
        btns[13] = gettrig(1) + '0';
        btns[21] = gettrig(2) + '0';
        btns[28] = GET_PPS() + '0';
        sendstring(btns);
    }else if(CMP(cmd, CMD_USBRST) == 0){ // reset USB pullup
        USBPU_OFF();
        USB_connected = 0;
        sendstring("Reset USB\n");
        USBPU_ON();
    }else if(CMP(cmd, CMD_STARTAUTO) == 0){ // set/clear 'start auto' mode
        char c = cmd[sizeof(CMD_STARTAUTO) - 1];
        if(c == '+'){
            if(timetostartA && timetostartA < MAX_TIMETOSTART) ++timetostartA; // increase
        }else if(c == '-'){ // decrease or cancel
            if(timetostartA > 1) --timetostartA;
            else{ // cancel
                timetostartO = timetostartA = 0;
                startflags = 0;
            }
        }else if(c >= '1' && c <= MAX_TIMETOSTART + '0'){ // set time to start
            timetostartO = timetostartA = c - '0';
            startflags = ST_FLAG_AUTO | ST_FLAG_ONCE;
        }else{ // cancel
            timetostartO = timetostartA = 0;
            startflags = 0;
        }
        sendstring("Autostart ");
        if(startflags){
            sendstring("every ");
            sendchar('0' + timetostartA);
            sendstring(" minutes\n");
        }else{
            sendstring("canceled\n");
        }
    }else if(CMP(cmd, CMD_STARTONCE) == 0){ // set/clear 'start once' mode & modify time to next start (including nearest autostart)
        char c = cmd[sizeof(CMD_STARTAUTO) - 1];
        if(c == '+'){
             if(timetostartO && timetostartO < MAX_TIMETOSTART) ++timetostartO;
        }else if(c == '-'){
            if(timetostartO > 1) --timetostartO;
            else{
                timetostartO = timetostartA = 0;
                startflags = 0;
            }
        }else if(c >= '1' && c <= MAX_TIMETOSTART + '0'){ // set time to start
            timetostartO = c - '0';
            startflags |= ST_FLAG_ONCE;
        }else{ // cancel
            timetostartO = timetostartA = 0;
            startflags = 0;
        }
        sendstring("Single start ");
        if(startflags){
            sendstring("after ");
            sendchar('0' + timetostartO);
            sendstring(" minutes\n");
        }else{
            sendstring("canceled\n");
        }
    }else if(CMP(cmd, CMD_GATE) == 0){ // gate0 - don't check shutters; otherwise - check
        if(cmd[sizeof(CMD_GATE-1)] == '0'){
            sendstring("Don't ");
            chkshtr = 0;
        }else chkshtr = 1;
        sendstring("check gate\n");
    }else if(CMP(cmd, CMD_SHOWSHTR) == 0){ // showshutter0 - not show shutter events
        if(cmd[sizeof(CMD_SHOWSHTR)-1] == '0'){
            sendstring("Don't ");
            showshtr = 0;
        }else showshtr = 1;
        sendstring("show shutter events on LED\n");
    }else if(CMP(cmd, CMD_EVTLEN) == 0){ // USART speed
        GETNUM(CMD_EVTLEN);
        if(N < 1000 || N > 60000) goto bad_number;
        if(the_conf.ledshow_time != (uint16_t)N){
            the_conf.ledshow_time = (uint16_t)N;
            conf_modified = 1;
        }
        succeed = 1;
    }else if(CMP(cmd, CMD_DUMPN) == 0){ // dump Nth event
        if(getnum(cmd+sizeof(CMD_DUMPN)-1, &N)) N = -1; // default - last
        if(dump_log(N, 1)) sendstring("Wrong index!\n");
    }else if(CMP(cmd, CMD_SQUEAK) == 0){ // make a short squeak
        buzzer_squeak();
    }else{
        sendstring("Bad command: ");
        sendstring(oldcmd);
        sendstring("\n");
        return;
    }
    /*else if(CMP(cmd, CMD_) == 0){
        ;
    }*/

    IWDG->KR = IWDG_REFRESH;
    if(succeed) sendstring("Success!\n");
    return;
  bad_number:
    sendstring("Error: bad number!\n");
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

// time (Tms) of last trigger event
uint32_t lastTtrig;
// data of last trigger event
event_log lastLog;
/**
 * @brief show_trigger_shot printout @ USB data with all triggers shot recently (+ save it in flash)
 * @param tshot - each bit consists information about trigger
 */
void show_trigger_shot(uint8_t tshot){
    uint8_t X = 1;
    for(uint8_t i = 0; i < TRIGGERS_AMOUNT && tshot; ++i, X <<= 1){
        IWDG->KR = IWDG_REFRESH;
        if(tshot & X) tshot &= ~X;
        else continue;
        lastTtrig = Tms;
        lastLog.trigno = i;
        if(i == LIDAR_TRIGGER) lastLog.lidar_dist = lidar_triggered_dist;
        lastLog.shottime = shottime[i];
        lastLog.triglen = triglen[i];
        sendstring(get_trigger_shot(-1, &lastLog));
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

static char localbuffer[LOCBUFSZ];
static uint8_t bufidx = 0;
static void transmitlocbuf(){
    localbuffer[bufidx] = 0;
    USB_send(localbuffer);
    if(!(the_conf.defflags & FLAG_GPSPROXY)){ // USART1 isn't a GPS proxy
        usart_send(1, localbuffer);
        transmit_tbuf(1);
    }
    if(the_conf.defflags & FLAG_NOLIDAR){ // USART3 isn't a LIDAR
        usart_send(LIDAR_USART, localbuffer);
        transmit_tbuf(LIDAR_USART);
    }
    bufidx = 0;
}
// add char to buf
void sendchar(char ch){
    localbuffer[bufidx++] = ch;
    if(bufidx >= LOCBUFSZ-1) transmitlocbuf();
}
/**
 * @brief addtobuf - add to local buffer any zero-terminated substring
 * @param str - string to add
 *  it sends data to USB and (due to setup) USART1 when buffer will be full or when meet '\n' at the end of str
 */
void sendstring(const char *str){
    while(*str) sendchar(*str++);
    if(str[-1] == '\n') transmitlocbuf();
}
