/*
 * This file is part of the Chiller project.
 * Copyright 2018 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "effects.h"
#include "hardware.h"
#include "protocol.h"
#include "usart.h"



extern uint8_t crit_error;

#ifdef EBUG
static void putADC(int n){
    put_string("ADC");
    put_char('0' + n);
    put_string(" value: ");
    put_uint(getADCval(n));
    put_char('\n');
//    while(LINE_BUSY == usart1_sendbuf());
}

/**
 * @brief debugging_proc - debugging functions
 * @param command - rest of cmd
 */
static void debugging_proc(const char *command){
    char ch;
    int i;
    switch(*command++){
        case 'w':
            SEND("Test watchdog\n");
            while(1){nop();}
        break;
        case 'A': // raw ADC values depending on next symbol
            ch = *command;
            if(ch == 'A' || ch == 'B'){
                for(i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i){
                    putADC(i);
                }
            }else{
            i = ch - '0';
            if(i < 0 || i >= NUMBER_OF_ADC_CHANNELS){
                SEND("Wrong channel nuber!\n");
            }else putADC(i);
            }
        break;
        case 'B':
            SEND("B command\n");
        break;
        default:
            SEND("wrong command\n");
        break;
    }
    while(LINE_BUSY == usart1_sendbuf()){nop();};
}
#endif

static void chPWM(const char *command){
    int n = *command++ - '1';
    if(n < 0 || n > 2){
        put_string("Wrong PWM channel number");
        return;
    }
    int32_t CCR = getPWM(n);
    if((command = getnum(command, &CCR))){
        set_effect(n, EFF_NONE);
        int32_t speed = SG90_STEP;
        if(*command++ == ','){
            getnum(command, &speed);
        }
        CCR = setPWM(n, CCR, speed);
    }
    put_string("pulse");
    put_char('1' + n);
    put_char('=');
    put_int(CCR);
    put_char('\n');
}

static void set_servoT(const char *buf){
    int32_t T;
    if(!getnum(buf, &T) || T < 1000 || T > 65535){
        put_string("Bad period value\n");
        usart1_sendbuf();
        return;
    }
    put_string("Set period to ");
    put_int(T);
    put_string(" us\n");
    setTIM3T(T);
    usart1_sendbuf();
}

static void chk_effect(const char *cmd, effect_t eff, const char *name){
    if(set_effect(*(++cmd)-'1', eff) == eff){
        put_string("Turn on ");
        put_string(name);
        put_string(" effect\n");
    }else put_string("err\n");
}

/**
 * @brief process_command - command parser
 * @param command - command text (all inside [] without spaces)
 * @return text to send over terminal or NULL
 */
char *process_command(const char *command){
    char *ret = NULL;
    usart1_sendbuf(); // send buffer (if it is already filled)
    switch(*command){
        case '?': // help
            SEND_BLK(
                "1-3[pos[,speed]]- set/get xth pulse length (us) (0,1,2 - min, max, mid)\n"
                "fx - servo period (us)"
                "Mn - set Mad Wipe effect\n"
                "Pn - set Pendulum effect\n"
                "R  - reset\n"
                "Sn - set Small Pendulum effect\n"
                "t  - get MCU temperature (approx.)\n"
                "V  - get Vdd\n"
                "Wn - set Wipe effect\n"
                );
#ifdef EBUG
            SEND_BLK("d -> goto debug:\n"
                 "\tAx - get raw ADCx value (A for all)\n"
                 "\tw - test watchdog\n"
                 );
#endif
        break;
        case '1':
        case '2':
        case '3':
            chPWM(command);
        break;
        case 'f':
            set_servoT(++command);
        break;
        case 'M':
            chk_effect(command, EFF_MADWIPE, "mad wipe");
        break;
        case 'P':
            chk_effect(command, EFF_PENDULUM, "pendulum");
        break;
        case 'R': // reset MCU
            NVIC_SystemReset();
        break;
        case 'S':
            chk_effect(command, EFF_SMPENDULUM, "small pendulum");
        break;
        case 't': // get mcu T
            put_string("MCUTEMP10=");
            put_int(getMCUtemp());
            put_char('\n');
        break;
        case 'V': // get Vdd
            put_string("VDD100=");
            put_uint(getVdd());
            put_char('\n');
        break;
        case 'W':
            chk_effect(command, EFF_WIPE, "wipe");
        break;
#ifdef EBUG
        case 'd':
            debugging_proc(++command);
            return NULL;
        break;
#endif
    }
    usart1_sendbuf();
    return ret;
}
