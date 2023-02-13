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

#include "commonproto.h"
#include "hdr.h"
#include "proto.h"
#include "usb.h"


#define NOPARCHK(par) do{if(PARBASE(par) != CANMESG_NOPAR) return ERR_BADPAR;}while(0)

extern volatile uint32_t Tms;

// common functions for CAN and USB (or CAN only functions)

static errcodes cu_nosuchfn(_U_ uint8_t par, _U_ int32_t *val){ return ERR_BADCMD; }

errcodes cu_ping(_U_ uint8_t par, _U_ int32_t *val){
    return ERR_OK;
}

static errcodes cu_reset(uint8_t par, _U_ int32_t *val){
    NOPARCHK(par);
    NVIC_SystemReset();
    return ERR_OK;
}
static errcodes cu_time(uint8_t par, int32_t *val){
    NOPARCHK(par);
    *val = Tms;
    return ERR_OK;
}


errcodes cu_abspos(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_accel(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_adc(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_button(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_canid(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_diagn(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_emstop(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_eraseflash(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_esw(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_eswreact(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_goto(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_gotoz(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_gpio(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_gpioconf(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_maxspeed(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_maxsteps(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_mcut(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_mcuvdd(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_microsteps(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_minspeed(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_motflags(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_motmul(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_motreinit(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_relpos(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_relslow(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_saveconf(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_screen(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_speedlimit(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_state(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_stop(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_tmcbus(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_udata(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}
errcodes cu_usartstatus(_U_ uint8_t par, _U_ int32_t *val){return ERR_BADCMD;}


const fpointer cmdlist[CCMD_AMOUNT] = {
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
    [CCMD_GOTOZERO] = "gotoz"
};
