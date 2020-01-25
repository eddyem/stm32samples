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

#include "flash.h"
#include "str.h"
#include "time.h"
#include "usart.h"
#include "usb.h"

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
    if(*cmd == '?'){ // help
        USB_send("Commands:\n"
                 CMD_DISTMIN   " - min distance threshold (cm)\n"
                 CMD_DISTMAX   " - max distance threshold (cm)\n"
                 CMD_PRINTTIME " - print time\n"
                 CMD_STORECONF " - store new configuration in flash\n"
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
    }else return 1;
    if(succeed) USB_send("Success!\n");
    return 0;
  bad_number:
    USB_send("Error: bad number!\n");
    return 0;
}
