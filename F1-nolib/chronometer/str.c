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

#include "str.h"
#include "usart.h"
/**
 * @brief cmpstr - the same as strncmp
 * @param s1,s2 - strings to compare
 * @param n - max symbols amount
 * @return 0 if strings equal or 1/-1
 */
int cmpstr(const char *s1, const char *s2, int n){
    int ret = 0;
    do{
        ret = *s1 - *s2;
    }while(*s1++ && *s2++ && --n);
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
