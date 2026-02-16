/*
 * This file is part of the multiiface project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "Debug.h"
#include "hardware.h" // Config_mode
#include "ringbuffer.h"
#include "usb_dev.h"

// asinchronous debugging
// DBG(str): add filename, line, string message `str` and '\n'
// DBGs(str): only add `str`, without '\n'
// DBGn(): add just '\n'

#ifdef EBUG

#define DBGBUFSZ   (1024)

static uint8_t buf[DBGBUFSZ];
static ringbuffer dbgrb = {.data = buf, .length = DBGBUFSZ};

void debug_message_text(const char *str){
    if(!Config_mode) return;
    RB_write(&dbgrb, (const uint8_t*)str, strlen(str));
}

void debug_message_char(char ch){
    if(!Config_mode) return;
     RB_write(&dbgrb, (const uint8_t*)&ch, 1);
}

void debug_newline_only(){
    if(!Config_mode) return;
    char nl = '\n';
    RB_write(&dbgrb, (const uint8_t*)&nl, 1);
}

void print_debug_messages(){
    if(!Config_mode) return;
    uint8_t rcvbuf[256];
    do{
        int n = RB_readto(&dbgrb, '\n', rcvbuf, 256);
        if(n == 0) break;
        else if(n < 0) n = -n; // partial string: longer than 256 bytes
        USB_send(ICFG, rcvbuf, n);
    }while(1);
}

#endif


