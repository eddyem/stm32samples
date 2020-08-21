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
#pragma once
#ifndef STR_H__
#define STR_H__

#include "stm32f1.h"
#include "flash.h"
#include "hardware.h"

// local buffer size (chars)
#define LOCBUFSZ    128

// TTY commands
#define CMD_BTNSTATE    "btnstate"
#define CMD_BUZZER      "buzzer"
#define CMD_CLEARSCRN   "cls"
#define CMD_CURDIST     "curdist"
#define CMD_DELLOGS     "deletelogs"
#define CMD_DISTMAX     "distmax"
#define CMD_DISTMIN     "distmin"
#define CMD_DUMP        "dump"
#define CMD_DUMPN       "ndump"
#define CMD_EVTLEN      "evtlen"
#define CMD_FLASH       "flash"
#define CMD_GETMCUTEMP  "mcutemp"
#define CMD_GETVDD      "vdd"
#define CMD_GATE        "gate"
#define CMD_GPSPROXY    "gpsproxy"
#define CMD_GPSRESTART  "gpsrestart"
#define CMD_GPSSTAT     "gpsstat"
#define CMD_GPSSTR      "gpsstring"
#define CMD_LEDS        "leds"
#define CMD_LIDAR       "lidar"
#define CMD_LIDARSPEED  "lidspd"
#define CMD_MESG        "mesg"
#define CMD_NFREE       "nfree"
#define CMD_PRINTTIME   "time"
#define CMD_RESET       "reset"
#define CMD_SAVEEVTS    "se"
#define CMD_SHOWCONF    "showconf"
#define CMD_SHOWSHTR    "showshutter"
#define CMD_SHOWTIME    "showtime"
#define CMD_STARTAUTO   "stauto"
#define CMD_STARTONCE   "stonce"
#define CMD_STORECONF   "store"
#define CMD_STORTEST    "stortest"
#define CMD_STREND      "strend"
#define CMD_SQUEAK      "squeak"
#define CMD_TRGPAUSE    "trigpause"
#define CMD_TRGTIME     "trigtime"
#define CMD_TRIGLVL     "triglevel"
#define CMD_USARTSPD    "usartspd"
#define CMD_USBRST      "usbreset"

// starting flags: once, every N seconds
#define ST_FLAG_ONCE    (1<<0)
#define ST_FLAG_AUTO    (1<<1)
// max delayed time = 5 min (2..9)
#define MAX_TIMETOSTART 5
extern uint8_t showGPSstr, showtime, startflags, timetostartO, timetostartA, chkshtr, showshtr;
extern uint32_t lastTtrig;
extern event_log lastLog;

int getnum(const char *buf, int32_t *N);
char *u2str(uint32_t val);
char *u2hex(uint32_t val);

int strln(const char *s);
char *strcp(char* dst, const char *src);
int cmpstr(const char *s1, const char *s2, int n);
char *getchr(const char *str, char symbol);
void parse_CMD(char *cmd);
char *get_trigger_shot(int number, const event_log *logdata);
void show_trigger_shot(uint8_t trigger_shot);
void sendstring(const char *str);
void sendchar(char ch);
#endif // STR_H__
