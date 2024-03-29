/*
 * This file is part of the F303usart project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include <math.h>
#include <stm32f3.h>
#include <string.h>

#include "hardware.h"
#include "usart.h"

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

// be careful: if pow10 would be bigger you should change str[] size!
static const float pwr10[] = {1.f, 10.f, 100.f, 1000.f, 10000.f};
static const float rounds[] = {0.5f, 0.05f, 0.005f, 0.0005f, 0.00005f};
#define P10L  (sizeof(pwr10)/sizeof(uint32_t) - 1)
static char * float2str(float x, uint8_t prec){
    static char str[16] = {0}; // -117.5494E-36\0 - 14 symbols max!
    if(prec > P10L) prec = P10L;
    if(isnan(x)){ memcpy(str, "NAN", 4); return str;}
    else{
        int i = isinf(x);
        if(i){memcpy(str, "-INF", 5); if(i == 1) return str+1; else return str;}
    }
    char *s = str + 14; // go to end of buffer
    uint8_t minus = 0;
    if(x < 0){
        x = -x;
        minus = 1;
    }
    int pow = 0; // xxxEpow
    // now convert float to 1.xxxE3y
    while(x > 1000.f){
        x /= 1000.f;
        pow += 3;
    }
    if(x > 0.) while(x < 1.){
        x *= 1000.f;
        pow -= 3;
    }
    // print Eyy
    if(pow){
        uint8_t m = 0;
        if(pow < 0){pow = -pow; m = 1;}
        while(pow){
            register int p10 = pow/10;
            *s-- = '0' + (pow - 10*p10);
            pow = p10;
        }
        if(m) *s-- = '-';
        *s-- = 'E';
    }
    // now our number is in [1, 1000]
    uint32_t units;
    if(prec){
        units = (uint32_t) x;
        uint32_t decimals = (uint32_t)((x-units+rounds[prec])*pwr10[prec]);
        // print decimals
        while(prec){
            register int d10 = decimals / 10;
            *s-- = '0' + (decimals - 10*d10);
            decimals = d10;
            --prec;
        }
        // decimal point
        *s-- = '.';
    }else{ // without decimal part
        units = (uint32_t) (x + 0.5);
    }
    // print main units
    if(units == 0) *s-- = '0';
    else while(units){
        register uint32_t u10 = units / 10;
        *s-- = '0' + (units - 10*u10);
        units = u10;
    }
    if(minus) *s-- = '-';
    return s+1;
}

static const float tests[] = {-1.23456789e-37f, -3.14159265e-2f, -1234.56789f, -1.2345678f, 0.f, 1e-40f, 0.1234567f, 123.456789f, 2.473829e31f, 
NAN, INFINITY, -INFINITY};
#define TESTN 12

int main(void){
//    sysreset();
    if(!StartHSE()) StartHSI();
    SysTick_Config((uint32_t)72000); // 1ms
    hw_setup();
    usart_setup();
    usart_send("Start\n");
    uint32_t ctr = Tms;
    float more = 5.123f, ang = 0.f;
    while(1){
        if(Tms - ctr > 499){
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
            /**/
            more += 0.1234567f;
            ang += 0.5f;
        }
        if(bufovr){
            bufovr = 0;
            usart_send("bufovr\n");
        }
        char *txt = NULL;
        if(usart_getline(&txt)){
            if(*txt == '?'){
                for(int i = 0; i < TESTN; ++i) for(int j = 0; j <= (int)P10L; ++j){
                    usart_send(float2str(tests[i], j));
                    usart_putchar('\n');
                }
                usart_send("\nValue of 'more': ");
                usart_send(float2str(more, 4));
                usart_send(", sqrt of 'more': ");
                usart_send(float2str(sqrtf(more), 4));
                usart_send("\nValue of 'ang': ");
                usart_send(float2str(ang, 4));
                usart_send(", tan of 'ang' degr: ");
                usart_send(float2str(tan(ang/180.f*M_PI), 4));
                usart_send("\ninf and nan: "); usart_send(float2str(ang/0.f, 4)); usart_putchar(' '); usart_send(float2str(sqrtf(-ang), 4));
                usart_putchar('\n');
            }else{
                usart_send("Get by USART1: ");
                usart_send(txt);
                usart_putchar('\n');
            }
        }
    }
}
