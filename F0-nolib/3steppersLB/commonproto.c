/*
 * This file is part of the 3steppers project.
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

#include "adc.h"
#include "buttons.h"
#include "can.h"
#include "commonproto.h"
#include "hardware.h"

/******* All functions from cmdlist[i].function *******/

static errcodes pingparser(uint8_t _U_ *par, int32_t _U_ *val){
    return ERR_OK; // just echo all input data over CAN (or return OK to USB)
}

static errcodes relayparser(uint8_t _U_ *par, int32_t _U_ *val){
    if(ISSETTER(*par)){
        if(*val) ON(RELAY);
        else OFF(RELAY);
    }
    *val = CHK(RELAY);
    return ERR_OK;
}

static errcodes buzzerparser(uint8_t _U_ *par, int32_t _U_ *val){
    if(ISSETTER(*par)){
        if(*val) ON(BUZZER);
        else OFF(BUZZER);
    }
    *val = CHK(BUZZER);
    return ERR_OK;
}

static errcodes adcparser(uint8_t _U_ *par, int32_t _U_ *val){
    uint8_t n = PARBASE(*par);
    if(n > NUMBER_OF_ADC_CHANNELS) return ERR_BADPAR;
    *val = (int32_t) getADCval(n);
    return ERR_OK;
}

// NON-STANDARD COMMAND!!!!!!!
// errcode == keystate, value = last time!!!!
static errcodes buttonsparser(uint8_t _U_ *par, int32_t _U_ *val){
#if BTNSNO > 4
#error "change the code!!!"
#endif
    uint8_t n = PARBASE(*par);
    if(n >= BTNSNO){
        *par = CANMESG_NOPAR; // the only chance to understand error
        return ERR_BADPAR;
    }
    return (uint8_t) keystate(n, (uint32_t*)val);
}

static errcodes eswparser(uint8_t _U_ *par, int32_t _U_ *val){
    uint8_t n = PARBASE(*par);
    if(n > ESWNO-1) return ERR_BADPAR;
    *val = (int32_t)ESW_state(n);
    return ERR_OK;
}

static errcodes mcutparser(uint8_t _U_ *par, int32_t _U_ *val){
    *val = getMCUtemp();
    return ERR_OK;
}

static errcodes mcuvddparser(uint8_t _U_ *par, int32_t _U_ *val){
    *val = getVdd();
    return ERR_OK;
}

static errcodes resetparser(uint8_t _U_ *par, int32_t _U_ *val){
    NVIC_SystemReset();
    return ERR_OK;
}

static errcodes timeparser(uint8_t _U_ *par, int32_t _U_ *val){
    *val = Tms;
    return ERR_OK;
}

static errcodes pwmparser(uint8_t _U_ *par, int32_t _U_ *val){
    if(PARBASE(*par) > PWMCHMAX && *par != CANMESG_NOPAR) return ERR_BADPAR;
#if PWMCHMAX != 0
#error "change the code!!!"
#endif
    if(ISSETTER(*par)){
        if(*val < 0 || *val > PWMMAX) return ERR_BADVAL;
        PWMset((uint32_t)*val);
    }
    *val = (int32_t) PWMget();
    return ERR_OK;
}

static errcodes extparser(uint8_t _U_ *par, int32_t _U_ *val){
    return ERR_OK;
}

/*
static CAN_errcodes parser(const uint8_t _U_ *par, const int32_t _U_ *val){
    return CANERR_OK;
}*/

// the main commands list, index is CAN command code
const commands cmdlist[CMD_AMOUNT] = {
    [CMD_PING] = {"ping", pingparser, "echo given command back"},
    [CMD_RELAY] = {"relay", relayparser, "change relay state (1/0)"},
    [CMD_BUZZER] = {"buzzer", buzzerparser, "change buzzer state (1/0)"},
    [CMD_ADC] = {"adc", adcparser, "get ADC values"},
    [CMD_BUTTONS] = {"button", buttonsparser, "get buttons state"},
    [CMD_ESWSTATE] = {"esw", eswparser, "get end switches state"},
    [CMD_MCUT] = {"mcut", mcutparser, "get MCU T"},
    [CMD_MCUVDD] = {"mcuvdd", mcuvddparser, "get MCU Vdd"},
    [CMD_RESET] = {"reset", resetparser, "reset MCU"},
    [CMD_TIMEFROMSTART] = {"time", timeparser, "get time from start"},
    [CMD_PWM] = {"pwm", pwmparser, "pwm value"},
    [CMD_EXT] = {"ext", extparser, "external outputs"},
};
