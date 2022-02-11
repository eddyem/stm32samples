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

#include "stm32f3.h"
#include "hardware.h"
#include "usart.h"

//#include <arm_math.h>

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

# define IEEE_754_FLOAT_MANTISSA_BITS (23)
# define IEEE_754_FLOAT_EXPONENT_BITS (8)
# define IEEE_754_FLOAT_SIGN_BITS     (1)
# if (IS_BIG_ENDIAN == 1)
    typedef union {
        float value;
        struct {
            __int8_t   sign     : IEEE_754_FLOAT_SIGN_BITS;
            __int8_t   exponent : IEEE_754_FLOAT_EXPONENT_BITS;
            __uint32_t mantissa : IEEE_754_FLOAT_MANTISSA_BITS;
        };
    } IEEE_754_float;
# else
    typedef union {
        float value;
        struct {
            __uint32_t mantissa : IEEE_754_FLOAT_MANTISSA_BITS;
            __int8_t   exponent : IEEE_754_FLOAT_EXPONENT_BITS;
            __int8_t   sign     : IEEE_754_FLOAT_SIGN_BITS;
        };
    } IEEE_754_float;
# endif


#define P(x)  usart_putchar(x)
#define S(x) usart_send(x)

// be careful: if pow10 would be bigger you should change str[] size!
static const float pwr10[] = {1., 10., 100., 1000., 10000.};
static const float rounds[] = {0.5, 0.05, 0.005, 0.0005, 0.00005};
#define P10L  (sizeof(pwr10)/sizeof(uint32_t) - 1)
static char * float2str(float x, uint8_t prec){
    if(prec > P10L) prec = P10L;
    static char str[16] = {0}; // -117.5494E-36\0 - 14 symbols max!
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
    while(x < 1.){
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

static const float tests[] = {-1.23456789e-37, -3.14159265e-2, -1234.56789, -1.2345678, 0.1234567, 123.456789, 1.98765e7, 2.473829e31};
#define TESTN 8

int main(void){
    sysreset();
    if(!StartHSE()) StartHSI();
    SysTick_Config((uint32_t)72000); // 1ms
    hw_setup();
    usart_setup();
    usart_send("Start\n");
    uint32_t ctr = Tms;
    float x = 0.519, more = 5.123;
    while(1){
        if(Tms - ctr > 499){
            //usart_send("ping\n");
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
            /**/
            if((x += 0.519) > more){
                //usart_send("MORE!\n");
                more += 5.123;
                //__get_FPSCR();
            }
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
            }else{
                usart_send("Get by USART1: ");
                usart_send(txt);
                usart_putchar('\n');
            }
        }
    }
}
