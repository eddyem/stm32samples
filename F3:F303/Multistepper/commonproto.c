/*
 * This file is part of the multistepper project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "commonproto.h"
#include "flash.h"
#include "hardware.h"
#include "hdr.h"
#include "pdnuart.h"
#include "proto.h"
#include "steppers.h"
#include "usb.h"


#define NOPARCHK(par) do{if(PARBASE(par) != CANMESG_NOPAR) return ERR_BADPAR;}while(0)

#define CHECKN(val, par) do{val = PARBASE(par); \
    if(val > MOTORSNO-1) return ERR_BADPAR;}while(0)

extern volatile uint32_t Tms;

// common functions for CAN and USB (or CAN only functions)

static errcodes cu_nosuchfn(uint8_t _U_ par, int32_t _U_ *val){
    return ERR_BADCMD;
}

errcodes cu_abspos(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    errcodes ret = ERR_OK;
    if(ISSETTER(par)){
        ret = setmotpos(n, *val);
    }
    getpos(n, val);
    return ret;
}

errcodes cu_accel(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    if(ISSETTER(par)){
        if(*val/the_conf.microsteps[n] > ACCELMAXSTEPS || *val < 1) return ERR_BADVAL;
        the_conf.accel[n] = *val;
        update_stepper(n);
    }
    *val = the_conf.accel[n];
    return ERR_OK;
}

static const uint8_t extADCchnl[NUMBER_OF_EXT_ADC_CHANNELS] = {ADC_AIN0, ADC_AIN1, ADC_AIN2, ADC_AIN3};
// V*100
errcodes cu_adc(uint8_t par, int32_t *val){
    uint8_t n = PARBASE(par);
    if(n > NUMBER_OF_EXT_ADC_CHANNELS - 1) return ERR_BADPAR;
    float v = getADCvoltage(extADCchnl[n])*100.f;
    *val = (int32_t)v;
    return ERR_OK;
}

// NON-STANDARD COMMAND!!!!!!!
// errcode == keystate, value = last time!!!!
errcodes cu_button(uint8_t par, int32_t *val){
    uint8_t n = PARBASE(par);
    if(n > BTNSNO-1){
        *val = CANMESG_NOPAR; // the only chance to understand error
        return ERR_BADPAR;
    }
    return (uint8_t) keystate(n, (uint32_t*)val);
}

errcodes cu_diagn(uint8_t _U_ par, int32_t _U_ *val){
    return ERR_BADCMD;
}

errcodes cu_emstop(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    emstopmotor(n);
    return ERR_OK;
}

errcodes cu_eraseflash(uint8_t _U_ par, int32_t _U_ *val){
    NOPARCHK(par);
    if(ISSETTER(par)){
        if(erase_storage(*val)) return ERR_BADVAL;
    }else if(erase_storage(-1)) return ERR_CANTRUN;
    return ERR_OK;
}

// par - motor number
errcodes cu_esw(uint8_t par, int32_t *val){
    uint8_t n; CHECKN(n, par);
    *val = ESW_state(n);
    return ERR_OK;
}

errcodes cu_eswreact(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    if(ISSETTER(par)){
        if(*val < 0 || *val > ESW_AMOUNT-1) return ERR_BADVAL;
        the_conf.ESW_reaction[n] = *val;
        update_stepper(n);
    }
    *val = geteswreact(n);
    return ERR_OK;
}

errcodes cu_goto(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    if(ISSETTER(par)) return motor_absmove(n, *val);
    return getpos(n, val);
}

errcodes cu_gotoz(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    return motor_goto0(n);
}

TRUE_INLINE void setextpar(uint8_t val, uint8_t i){
    switch(val){
        case 0:
            EXT_CLEAR(i);
        break;
        case 1:
            EXT_SET(i);
        break;
        default:
            EXT_TOGGLE(i);
    }
}

// TODO: do it right
errcodes cu_gpio(uint8_t _U_ par, int32_t _U_ *val){
#if EXTNO > 4
#error "change the code!!!"
#endif
    uint8_t n = PARBASE(par);
#ifdef EBUG
    USND("par="); printu(par);
    USND(", n="); USB_putbyte('0'+n); newline();
#endif
    if(n == CANMESG_NOPAR){ // all
#ifdef EBUG
        USND("ALL\n");
#endif
        uint8_t *arr = (uint8_t*)val;
        if(ISSETTER(par)){
            for(int i = 0; i < EXTNO; ++i)
                setextpar(arr[i], i);
        }
        for(int i = 0; i < EXTNO; ++i){
            arr[i] = EXT_CHK(i);
        }
        return ERR_OK;
    }else if(n > EXTNO-1) return ERR_BADPAR;
    if(ISSETTER(par))
        setextpar((uint8_t)*val, n);
    *val = (int32_t) EXT_CHK(n);
    return ERR_OK;
}

// TODO: configure PU/PD, IN/OUT
errcodes cu_gpioconf(uint8_t _U_ par, int32_t _U_ *val){
    return ERR_BADCMD;
}

// calculate ARR value for given speed, return nearest possible speed
static uint16_t getSPD(uint8_t n, int32_t speed){
    uint32_t ARR = PCLK/(MOTORTIM_PSC+1) / the_conf.microsteps[n] / speed - 1;
    if(ARR < MOTORTIM_ARRMIN) ARR = MOTORTIM_ARRMIN;
    else if(ARR > 0xffff) ARR = 0xffff;
    speed = PCLK/(MOTORTIM_PSC+1) / the_conf.microsteps[n] / (ARR + 1);
    if(speed > 0xffff) speed = 0xffff;
    return (uint16_t)speed;
}

errcodes cu_maxspeed(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    if(ISSETTER(par)){
        if(*val <= the_conf.minspd[n]) return ERR_BADVAL;
        the_conf.maxspd[n] = getSPD(n, *val);
        update_stepper(n);
    }
    *val = the_conf.maxspd[n];
    return ERR_OK;
}

errcodes cu_maxsteps(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    if(ISSETTER(par)){
        if(*val < 1) return ERR_BADVAL;
        the_conf.maxsteps[n] = *val;
    }
    *val = the_conf.maxsteps[n];
    return ERR_OK;
}

errcodes cu_mcut(uint8_t par, int32_t *val){
    NOPARCHK(par);
    float f = getMCUtemp();
    *val = (uint32_t)(f*10.f);
    return ERR_OK;
}

errcodes cu_mcuvdd(uint8_t par, int32_t *val){
    NOPARCHK(par);
    float f = getVdd();
    *val = (uint32_t)(f*10.f);
    return ERR_OK;
}

errcodes cu_microsteps(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    if(ISSETTER(par)){
#if MICROSTEPSMAX > 512
#error "Change the code anywhere!"
#endif
        uint16_t m = (uint16_t)*val;
        if(m < 1 || m > MICROSTEPSMAX) return ERR_BADVAL;
        // find most significant bit
        if(m != 1<<MSB(m)) return ERR_BADVAL;
        if(the_conf.maxspd[n] * m > PCLK/(MOTORTIM_PSC+1)/(MOTORTIM_ARRMIN+1)) return ERR_BADVAL;
        the_conf.microsteps[n] = m;
        update_stepper(n);
    }
    *val = the_conf.microsteps[n];
    return ERR_OK;
}

errcodes cu_minspeed(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    if(ISSETTER(par)){
        if(*val >= the_conf.maxspd[n]) return ERR_BADVAL;
        the_conf.minspd[n] = getSPD(n, *val);
        update_stepper(n);
    }
    *val = the_conf.minspd[n];
    return ERR_OK;
}

errcodes cu_motflags(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    if(ISSETTER(par)){
        the_conf.motflags[n] = *((motflags_t*)val);
        update_stepper(n);
    }
    *(motflags_t*)val = the_conf.motflags[n];
    return ERR_OK;
}

errcodes cu_motmul(uint8_t _U_ par, int32_t _U_ *val){
    return ERR_BADCMD;
}

errcodes cu_motreinit(uint8_t _U_ par, int32_t _U_ *val){
    NOPARCHK(par);
    init_steppers();
    return ERR_OK;
}

errcodes cu_pdn(uint8_t par, int32_t *val){
    uint8_t n = PARBASE(par);
    if(ISSETTER(par)){
        if(!pdnuart_writereg(0, n, *val)) return ERR_CANTRUN;
    }
    if(!pdnuart_readreg(0, n, (uint32_t*)val)) return ERR_CANTRUN;
    return ERR_OK;
}

errcodes cu_ping(uint8_t _U_ par, int32_t _U_ *val){
    return ERR_OK;
}

errcodes cu_relpos(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    if(ISSETTER(par)) return motor_relmove(n, *val);
    return getremainsteps(n, val);
}

errcodes cu_relslow(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    if(ISSETTER(par)) return motor_relslow(n, *val);
    return getremainsteps(n, val);
}

static errcodes cu_reset(uint8_t par, int32_t _U_ *val){
    NOPARCHK(par);
    NVIC_SystemReset();
    return ERR_OK;
}

errcodes cu_saveconf(uint8_t _U_ par, int32_t _U_ *val){
    NOPARCHK(par);
    if(store_userconf()) return ERR_CANTRUN;
    return ERR_OK;
}

errcodes cu_screen(uint8_t _U_ par, int32_t _U_ *val){
    return ERR_BADCMD;
}

errcodes cu_speedlimit(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    *val = getSPD(n, 0xffff);
    return ERR_OK;
}

errcodes cu_state(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    *val = getmotstate(n);
    return ERR_OK;
}

errcodes cu_stop(uint8_t _U_ par, int32_t _U_ *val){
    uint8_t n; CHECKN(n, par);
    stopmotor(n);
    return ERR_OK;
}

static errcodes cu_time(uint8_t par, int32_t *val){
    NOPARCHK(par);
    *val = Tms;
    return ERR_OK;
}

errcodes cu_tmcbus(uint8_t _U_ par, int32_t _U_ *val){
    return ERR_BADCMD;
}

errcodes cu_udata(uint8_t _U_ par, int32_t _U_ *val){
    return ERR_BADCMD;
}

errcodes cu_usartstatus(uint8_t _U_ par, int32_t _U_ *val){
    return ERR_BADCMD;
}

// V*10
errcodes cu_vdrive(uint8_t par, int32_t _U_ *val){
    NOPARCHK(par);
    float v = getADCvoltage(ADC_VDRIVE)*1000.f;
    *val = (int32_t)v;
    return ERR_OK;
}

errcodes cu_vfive(uint8_t par, int32_t *val){
    NOPARCHK(par);
    float v = getADCvoltage(ADC_VFIVE)*200.f;
    *val = (int32_t)v;
    return ERR_OK;
}

const fpointer cancmdlist[CCMD_AMOUNT] = {
    // different commands
    [CCMD_PING] = cu_ping,
    [CCMD_RELAY] = cu_nosuchfn,
    [CCMD_BUZZER] = cu_nosuchfn,
    [CCMD_ADC] = cu_adc,
    [CCMD_BUTTONS] = cu_button,
    [CCMD_ESWSTATE] = cu_esw,
    [CCMD_MCUT] = cu_mcut,
    [CCMD_MCUVDD] = cu_mcuvdd,
    [CCMD_RESET] = cu_reset,
    [CCMD_TIMEFROMSTART] = cu_time,
    [CCMD_PWM] = cu_nosuchfn,
    [CCMD_EXT] = cu_gpio,
    // configuration
    [CCMD_SAVECONF] = cu_saveconf,
    [CCMD_ENCSTEPMIN] = cu_nosuchfn,
    [CCMD_ENCSTEPMAX] = cu_nosuchfn,
    [CCMD_MICROSTEPS] = cu_microsteps,
    [CCMD_ACCEL] = cu_accel,
    [CCMD_MAXSPEED] = cu_maxspeed,
    [CCMD_MINSPEED] = cu_minspeed,
    [CCMD_SPEEDLIMIT] = cu_speedlimit,
    [CCMD_MAXSTEPS] = cu_maxsteps,
    [CCMD_ENCREV] = cu_nosuchfn,
    [CCMD_MOTFLAGS] = cu_motflags,
    [CCMD_ESWREACT] = cu_eswreact,
    // motor's commands
    [CCMD_ABSPOS] = cu_goto,
    [CCMD_RELPOS] = cu_relpos,
    [CCMD_RELSLOW] = cu_relslow,
    [CCMD_EMERGSTOP] = cu_emstop,
    [CCMD_EMERGSTOPALL] = cu_emstop, // without args
    [CCMD_STOP] = cu_stop,
    [CCMD_REINITMOTORS] = cu_motreinit,
    [CCMD_MOTORSTATE] = cu_state,
    [CCMD_ENCPOS] = cu_nosuchfn,
    [CCMD_SETPOS] = cu_abspos,
    [CCMD_GOTOZERO] = cu_gotoz,
    [CCMD_MOTMUL] = cu_motmul,
    [CCMD_DIAGN] = cu_diagn,
    [CCMD_ERASEFLASH] = cu_eraseflash,
    [CCMD_UDATA] = cu_udata,
    [CCMD_USARTSTATUS] = cu_usartstatus,
    [CCMD_VDRIVE] = cu_vdrive,
    [CCMD_VFIVE] = cu_vfive
    // Leave all commands upper for back-compatability with 3steppers
};

const char* cancmds[CCMD_AMOUNT] = {
    [CCMD_PING] = "ping",
    [CCMD_ADC] = "adc",
    [CCMD_BUTTONS] = "button",
    [CCMD_ESWSTATE] = "esw",
    [CCMD_MCUT] = "mcut",
    [CCMD_MCUVDD] = "mcuvdd",
    [CCMD_RESET] = "reset",
    [CCMD_TIMEFROMSTART] = "time",
    [CCMD_EXT] = "gpio",
    [CCMD_SAVECONF] = "saveconf",
    [CCMD_MICROSTEPS] = "microsteps",
    [CCMD_ACCEL] = "accel",
    [CCMD_MAXSPEED] = "maxspeed",
    [CCMD_MINSPEED] = "minspeed",
    [CCMD_SPEEDLIMIT] = "speedlimit",
    [CCMD_MAXSTEPS] = "maxsteps",
    [CCMD_MOTFLAGS] = "motflags",
    [CCMD_ESWREACT] = "eswreact",
    [CCMD_ABSPOS] = "goto",
    [CCMD_RELPOS] = "relpos",
    [CCMD_RELSLOW] = "relslow",
    [CCMD_EMERGSTOP] = "emstop N",
    [CCMD_EMERGSTOPALL] = "emstop",
    [CCMD_STOP] = "stop",
    [CCMD_REINITMOTORS] = "motreinit",
    [CCMD_MOTORSTATE] = "state",
    [CCMD_SETPOS] = "abspos",
    [CCMD_GOTOZERO] = "gotoz",
    [CCMD_MOTMUL] = "motmul",
    [CCMD_DIAGN] = "diagn",
    [CCMD_ERASEFLASH] = "eraseflash",
    [CCMD_UDATA] = "udata",
    [CCMD_USARTSTATUS] = "usartstatus",
    [CCMD_VDRIVE] = "vdrive",
    [CCMD_VFIVE] = "vfive",
    [CCMD_PDN] = "pdn"
};
