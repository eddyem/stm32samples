/*
 * This file is part of the test project.
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

// debugging messages on config interface
#ifdef EBUG
#include "usart.h"
#include "strfunc.h"

#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)
#define DBGl(str)       do{usart_sendstr(__FILE__ " (L" STR(__LINE__) "): " str); usart_send("\n", 1);}while(0)
#define DBGs(str)       usart_sendstr(str)
#define DBGch(ch)       usart_send(ch, 1)
#define DBGn()          usart_send("\n", 1)
#else
#define DBGl(str)
#define DBGs(str)
#define DBGch(ch)
#define DBGn()
#define DBGpri()
#endif
