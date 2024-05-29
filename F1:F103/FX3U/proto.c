/*
 * This file is part of the fx3u project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "strfunc.h"
#include "usart.h"
#include "version.inc"

flags_t flags = {
    .can_monitor = 0
};

static void printans(int res){
    if(res) usart_send("OK");
    else usart_send("FAIL");
}

// setters of uint/int
static void usetter(int(*fn)(uint32_t), char* str){
    uint32_t d = 0;
    if(str == getnum(str, &d)) printans(FALSE);
    else printans(fn(d));
}
static void isetter(int(*fn)(int32_t), char* str){
    int32_t d = 0;
    if(str == getint(str, &d)) printans(FALSE);
    else printans(fn(d));
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 */
void cmd_parser(char *txt){
    (void)txt;
}
