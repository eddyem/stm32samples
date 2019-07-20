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
#include "flash.h"
#include "GPS.h"
#include "str.h"
#include "time.h"
#include "usart.h"
#include "usb.h"

// flag to show new GPS message over USB
uint8_t showGPSstr = 0;

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
    USB_send("\nCONFIG:\nDISTMIN="); sendu(the_conf.dist_min);
    USB_send("\nDISTMAX="); sendu(the_conf.dist_max);
    USB_send("\nADCMIN="); sendi(the_conf.ADC_min);
    USB_send("\nADCMAX="); sendi(the_conf.ADC_max);
    USB_send("\nPULLUPS="); sendu(the_conf.trig_pullups);
    USB_send("\nTRIGLVL="); sendu(the_conf.trigstate);
    USB_send("\nTRIGPAUSE={");
    for(int i = 0; i < TRIGGERS_AMOUNT; ++i){
        if(i) USB_send(", ");
        sendu(the_conf.trigpause[i]);
    }
    USB_send("}");
    USB_send("\nENDCONFIG\n");
}

/**
 * @brief parse_USBCMD - parsing of string buffer got by USB
 * @param cmd - buffer with commands
 * @return 0 if got command, 1 if command not recognized
 */
int parse_USBCMD(char *cmd){
#define CMP(a,b)  cmpstr(a, b, sizeof(b)-1)
#define GETNUM(x) if(getnum(cmd+sizeof(x)-1, &N)) goto bad_number;
    static uint8_t conf_modified = 0;
    uint8_t succeed = 0;
    int32_t N;
    if(!cmd || !*cmd) return 0;
    IWDG->KR = IWDG_REFRESH;
    if(*cmd == '?'){ // help
        USB_send("Commands:\n"
                 CMD_ADCMAX    " - max ADC value treshold for trigger\n"
                 CMD_ADCMIN    " - min -//- (triggered when ADval>min & <max\n"
                 CMD_GETADCVAL " - get ADC value\n"
                 CMD_DISTMIN   " - min distance threshold (cm)\n"
                 CMD_DISTMAX   " - max distance threshold (cm)\n"
                 CMD_GPSRESTART " - send Full Cold Restart to GPS\n"
                 CMD_GPSSTR    " - current GPS data string\n"
                 CMD_LEDS      "S - turn leds on/off (1/0)\n"
                 CMD_GETMCUTEMP " - MCU temperature\n"
                 CMD_PULLUP    "NS - triggers pullups state (N - trigger No, S - 0/1 for off/on)\n"
                 CMD_SHOWCONF  " - show current configuration\n"
                 CMD_PRINTTIME " - print time\n"
                 CMD_STORECONF " - store new configuration in flash\n"
                 CMD_TRIGLVL   "NS - working trigger N level S\n"
                 CMD_TRGPAUSE  "NP - pause (P, ms) after trigger N shots\n"
                 CMD_TRGTIME   "N - show last trigger N time\n"
                 CMD_GETVDD    " - Vdd value\n"
                 );
    }else if(CMP(cmd, CMD_PRINTTIME) == 0){
        USB_send(get_time(&current_time, get_millis()));
    }else if(CMP(cmd, CMD_DISTMIN) == 0){ // set low limit
        DBG("CMD_DISTMIN");
        GETNUM(CMD_DISTMIN);
        if(N < 0 || N > 0xffff) goto bad_number;
        if(the_conf.dist_min != (uint16_t)N){
            conf_modified = 1;
            the_conf.dist_min = (uint16_t) N;
            succeed = 1;
        }
    }else if(CMP(cmd, CMD_DISTMAX) == 0){ // set low limit
        DBG("CMD_DISTMAX");
        GETNUM(CMD_DISTMAX);
        if(N < 0 || N > 0xffff) goto bad_number;
        if(the_conf.dist_max != (uint16_t)N){
            conf_modified = 1;
            the_conf.dist_max = (uint16_t) N;
            succeed = 1;
        }
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
    }else if(CMP(cmd, CMD_PULLUP) == 0){
        DBG("Pullups");
        cmd += sizeof(CMD_PULLUP) - 1;
        uint8_t Nt = *cmd++ - '0';
        if(Nt > TRIGGERS_AMOUNT - 1) goto bad_number;
        uint8_t state = *cmd -'0';
        if(state > 1) goto bad_number;
        uint8_t oldval = the_conf.trig_pullups;
        if(!state) the_conf.trig_pullups = oldval & ~(1<<Nt);
        else the_conf.trig_pullups = oldval | (1<<Nt);
        if(oldval != the_conf.trig_pullups) conf_modified = 1;
        succeed = 1;
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
        if(the_conf.trigpause[Nt] != N) conf_modified = 1;
        the_conf.trigpause[Nt] = N;
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
            LED_off();
            LED1_off();
            LEDSon = 0;
            USB_send("OFF\n");
        }
    }else if(CMP(cmd, CMD_ADCMAX) == 0){ // set low limit
        GETNUM(CMD_ADCMAX);
        if(N < -4096 || N > 4096) goto bad_number;
        if(the_conf.ADC_max != (int16_t)N){
            conf_modified = 1;
            the_conf.ADC_max = (int16_t) N;
            succeed = 1;
        }
    }else if(CMP(cmd, CMD_ADCMIN) == 0){ // set low limit
        GETNUM(CMD_ADCMIN);
        if(N < -4096 || N > 4096) goto bad_number;
        if(the_conf.ADC_min != (int16_t)N){
            conf_modified = 1;
            the_conf.ADC_min = (int16_t) N;
            succeed = 1;
        }
    }else if(CMP(cmd, CMD_GPSRESTART) == 0){
        USB_send("Send full cold restart to GPS\n");
        GPS_send_FullColdStart();
    }else return 1;
    IWDG->KR = IWDG_REFRESH;
    if(succeed) USB_send("Success!\n");
    return 0;
  bad_number:
    USB_send("Error: bad number!\n");
    return 0;
}

/**
 * @brief show_trigger_shot - print on USB message about last trigger shot time
 * @param tshot - each bit consists information about trigger
 */
void show_trigger_shot(uint8_t tshot){
    uint8_t X = 1;
    for(int i = 0; i < TRIGGERS_AMOUNT && tshot; ++i, X <<= 1){
        IWDG->KR = IWDG_REFRESH;
        if(tshot & X) tshot &= ~X;
        else continue;
        if(trigger_shot & X) trigger_shot &= ~X;
        USB_send("TRIG");
        sendu(i);
        USB_send("=");
        USB_send(get_time(&shottime[i].Time, shottime[i].millis));
        USB_send("\n");
    }
}
