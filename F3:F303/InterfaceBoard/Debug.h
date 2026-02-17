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

#pragma once

#include "usb_dev.h"

// debugging messages on config interface
#ifdef EBUG
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)
#define DBG(str)        do{debug_message_text(__FILE__ " (L" STR(__LINE__) "): " str); debug_newline_only();}while(0)
#define DBGs(str)       debug_message_text(str)
#define DBGch(ch)       debug_message_char(ch)
#define DBGn()          debug_newline_only()
#define DBGpri()        print_debug_messages();
#else
#define DBG(str)
#define DBGs(str)
#define DBGch(ch)
#define DBGn()
#define DBGpri()
#endif

void debug_message_text(const char *str);
void debug_message_char(char ch);
void debug_newline_only();
void print_debug_messages();
