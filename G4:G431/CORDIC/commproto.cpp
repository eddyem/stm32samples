/*
 * This file is part of the cordic project.
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

#include <cstring>

extern "C"{
#include <math.h>
#include <stm32g4.h>

#include "astro.h"
#include "commproto.h"
#include "cordic.h"
#include "hardware.h"
#include "strfunc.h"
#include "test.h"
}

// sending function
static int (*SEND)(const char *str) = nullptr;

extern volatile uint32_t Tms;

// input coordinates (degrees) and meteo
static float phpa = 800.f, tc = 0.f, rh = 0.5f, az = 0.f, zd = 20.f, ra = 0.f, ha = 0.f, dec = 0.f;
// starting time parameters: UNIX-time value and Tms when setter called
static uint32_t unixt0 = 0, Tms0 = 0;

// get current UNIX time by Tms
static uint32_t curUNIXt(){
    return unixt0 + (Tms - Tms0 + 500) / 1000;
}

constexpr uint32_t hash(const char* str, uint32_t h = 0){
    return *str ? hash(str + 1, h + ((h << 7) ^ *str)) : h;
}

// structure for setters/getters of float parameters
typedef struct {
    const char* name;        // command name
    uint32_t    hash;        // its hash
    float*      var;         // pointer to variable
    float       min_val;     // minimal and maximal values
    float       max_val;
} VarEntry;

// table for variables
static const VarEntry var_table[] = {
    {"az",        hash("az"),        &az,  -180.0f,  180.0f},
    {"dec",       hash("dec"),       &dec,  -90.0f,   90.0f},
    {"ha",        hash("ha"),        &ha,  -180.0f,  180.0f},
    {"ra",        hash("ra"),        &ra,     0.0f,  360.0f},
    {"zd",        hash("zd"),        &zd,     0.0f,   90.0f},
    {"humid",     hash("humid"),     &rh,     0.0f,    1.0f},
    {"press",     hash("press"),     &phpa,   0.0f, 1200.0f},
    {"temp",      hash("temp"),      &tc, -273.15f,  100.0f},
};
static const size_t var_table_size = sizeof(var_table) / sizeof(var_table[0]);

//static uint8_t curbuf[MAXSTRLEN];

// COMMAND(USART,      "Read USART data or send (USART=hex)")

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// text delimeter for help function
#define DELIMETER(text)
// setters/getters for common `find_var_by_hash` and `handle_var`
#define FLOATVAR(cmd, desc)
// list of all commands and handlers
#define COMMAND_TABLE \
    COMMAND(help,       "show this help") \
    DELIMETER("Test functions") \
    COMMAND(testc,      "test CORDIC function: sincos, sin, cos, atan, sqrt, log") \
    COMMAND(testm,      "test math function: sin, cos, atan, sqrt, log") \
    COMMAND(sincos,     "calculate sin/cos for given angle in degrees") \
    DELIMETER("Coordinates") \
    FLOATVAR(az,        "azimuth, deg (-180..180)") \
    FLOATVAR(dec,       "DEC, deg (-90..90)") \
    FLOATVAR(ha,        "HA, deg (-180..180)") \
    FLOATVAR(ra,        "RA, deg (0..360)") \
    FLOATVAR(zd,        "zenith distance, deg (0..90)") \
    DELIMETER("Astro functions") \
    COMMAND(sets,       "sin-cos is cordic (1) or math (0)") \
    COMMAND(time,       "show MJD and LST for given UNIX-time (and set it as system)") \
    COMMAND(azhd,       "convert alt-az to ha-dec (0, default) or vise versa (1) and change input values") \
    COMMAND(hara,       "convert ha to ra (0, default) or vice versa (1) and change input values") \
    COMMAND(refr,       "calculate refraction for current zd") \
    DELIMETER("Meteo conditions") \
    FLOATVAR(humid,     "rel. humidity, 0..1") \
    FLOATVAR(press,     "atm. pressure (hpa)") \
    FLOATVAR(temp,      "temperature, degC") \

typedef struct {
    const char *name;
    const char *desc;
} CmdInfo;

// prototypes
#define COMMAND(name, desc)   static errcodes_t cmd_ ## name(const char*, char*);
COMMAND_TABLE
#undef COMMAND

static const CmdInfo cmdInfo[] = { // command name, description - for `help`
#undef DELIMETER
#undef FLOATVAR
#define DELIMETER(text)       { nullptr, text },
#define FLOATVAR(name, desc)  { #name,   desc },
#define COMMAND(name, desc)   { #name,   desc },
        COMMAND_TABLE
#undef COMMAND
#undef FLOATVAR
#undef DELIMETER
#define FLOATVAR(a, b)
#define DELIMETER(t)
};

static const char* errtxt[ERR_AMOUNT] = {
    [ERR_OK]        = "OK\n",
    [ERR_BADCMD]    = "BADCMD\n",
    [ERR_BADPAR]    = "BADPAR\n",
    [ERR_BADVAL]    = "BADVAL\n",
    [ERR_WRONGLEN]  = "WRONGLEN\n",
    [ERR_CANTRUN]   = "CANTRUN\n",
    [ERR_BUSY]      = "BUSY\n",
    [ERR_OVERFLOW]  = "OVERFLOW\n",
};

const char *EQ = " = "; // equal sign for getters

// send `command = `
#define CMDEQ()   do{SEND(cmd); SEND(EQ);}while(0)
// send `commandXXX = `
#define CMDEQP(x)   do{SEND(cmd); SEND(u2str((uint32_t)x)); SEND(EQ);}while(0)
// the same as last but with command as option and uint value
#define SHOWPARU(cmd, x, val)  do{SEND(cmd); SEND(u2str((uint32_t)x)); SEND(EQ); SEND(u2str(val));}while(0)

/**
 * @brief splitargs - get command parameter and setter from `args`
 * @param args (i) - rest of string after command (like `1 = PU OD OUT`)
 * @param parno (o) - parameter number or -1 if none
 * @return setter (part after `=` without leading spaces) or NULL if none
 */
static char *splitargs(char *args, int32_t *parno){
    if(!args) return nullptr;
    uint32_t U32;
    char *next = getnum(args, &U32);
    int p = -1;
    if(next != args && U32 <= MAXPARNO) p = U32;
    if(parno) *parno = p;
    next = strchr(next, '=');
    if(next){
        if(*(++next)) next = omit_spaces(next);
        if(*next == 0) next = nullptr;
    }
    return next;
}

/**
 * @brief argsvals - split `args` into `parno` and setter's value
 * @param args - rest of string after command
 * @param parno (o) - parameter number or -1 if none
 * @param parval - integer setter's value
 * @return false if no setter or it's not a number, true - got setter's num
 */
static bool argsvals(char *args, int32_t *parno, int32_t *parval){
    char *setter = splitargs(args, parno);
    if(!setter) return false;
    int32_t I32;
    char *next = getint(setter, &I32);
    if(next != setter){
        if(parval) *parval = I32;
        return true;
    }
    return false;
}

static errcodes_t cmd_help(const char*, char*){
    SEND(REPOURL);
    for(size_t i = 0; i < sizeof(cmdInfo)/sizeof(cmdInfo[0]); i++){
        if(cmdInfo[i].name){
            SEND(cmdInfo[i].name);
            SEND(" - ");
        }else SEND("    ");
        SEND(cmdInfo[i].desc); SEND("\n");
    }
    return ERR_AMOUNT;
}

static const char* parse_func_name(char *args){
    char *setter = splitargs(args, nullptr);
    if(!setter) return nullptr;
    // remove trailing spaces
    char *p = setter;
    while(*p && *p > ' ') ++p;
    *p = 0;
    return setter;
}

#if 0
static errcodes_t cmd_cordic(const char *cmd, char *args){
    char *setter = splitargs(args, nullptr);
    if(setter){
        uint32_t u;
        if(!getint(setter, &u)) return ERR_BADVAL;
        set_sincos(u);
    }
    int i = get_sincos();
    CMDEQ();

}
#endif
// calculate sin/cos
static errcodes_t cmd_sincos(const char *, char *args){
    char *setter = splitargs(args, nullptr);
    float f;
    if(!setter || setter == getfloat(setter, &f)) return ERR_BADVAL;
    f = f/180.f * M_PIf;
    SEND("mathsin="); SEND(float2str(sinf(f), 7));
    SEND("\nmathcos="); SEND(float2str(cosf(f), 7));
    float s, c;
    cordic_sincos(f, &s, &c);
    SEND("\ncordicsin="); SEND(float2str(s, 7));
    SEND("\ncordiccos="); SEND(float2str(c, 7));
    SEND("\n");
    return ERR_AMOUNT;
}

// test math function
static errcodes_t cmd_testm(const char*, char *args){
    const char *fname = parse_func_name(args);
    if(!fname) return ERR_BADPAR;
    uint32_t elapsed = 0;
    bool ok = true;
    if(strcmp(fname, "sin") == 0){
        elapsed = test_math_sin();
    }else if(strcmp(fname, "cos") == 0){
        elapsed = test_math_cos();
    }else if(strcmp(fname, "atan") == 0){
        elapsed = test_math_atan();
    }else if(strcmp(fname, "sqrt") == 0){
        elapsed = test_math_sqrt();
    }else if(strcmp(fname, "log") == 0){
        elapsed = test_math_log();
    }else{
        ok = false;
    }
    if(!ok) return ERR_BADVAL;
    SEND("TIMEus=");
    SEND(u2str(elapsed));
    SEND("\n");
    return ERR_AMOUNT;
}


// test CORDIC function
static errcodes_t cmd_testc(const char*, char *args){
    const char *fname = parse_func_name(args);
    if(!fname) return ERR_BADPAR;
    uint32_t elapsed = 0;
    bool ok = true;
    if(strcmp(fname, "sincos") == 0){
        elapsed = test_cordic_sincos();
    }
    else if(strcmp(fname, "sin") == 0){
        elapsed = test_cordic_sin();
    }else if(strcmp(fname, "cos") == 0){
        elapsed = test_cordic_cos();
    }else if(strcmp(fname, "atan") == 0){
        elapsed = test_cordic_atan();
    }else if(strcmp(fname, "sqrt") == 0){
        elapsed = test_cordic_sqrt();
    }else if(strcmp(fname, "log") == 0){
        elapsed = test_cordic_log();
    }else{
        ok = false;
    }
    if(!ok) return ERR_BADVAL;
    SEND("TIMEus=");
    SEND(u2str(elapsed));
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_time(const char *cmd, char *args){
    char *setter = splitargs(args, nullptr);
    if(setter){
        if(setter == getnum(setter, &unixt0)) return ERR_BADVAL;
        Tms0 = Tms;
    }
    CMDEQ();
    uint32_t tnow = curUNIXt();
    SEND(u2str(tnow));
    float mjd = MJD_from_unix(tnow);
    SEND("\nMJD="); SEND(float2str(mjd, 7));
    SEND("\nLST="); SEND(float2str(LST_from_unix(tnow), 7));
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_sets(const char *cmd, char *args){
    int32_t val;
    if(argsvals(args, nullptr, &val)) set_sincos(val);
    CMDEQ();
    if(get_sincos()) SEND("CORDIC\n");
    else SEND("MATH\n");
    return ERR_AMOUNT;
}

// get u32, return value or 0 in case of getter
static uint32_t getflag(char *args){
    uint32_t val = 0;
    char *setter = splitargs(args, nullptr);
    if(setter) getnum(setter, &val);
    return val;
}

static errcodes_t cmd_hara(const char *, char *args){
    float lst_deg = HOURS2DEG(LST_from_unix(curUNIXt()));
    if(getflag(args)){ // ra to ha
        ha = ra_to_ha(ra, lst_deg);
        SEND("ha = "); SEND(float2str(ha, 7));
    }else{ // ha to ra
        ra = ha_to_ra(ha, lst_deg);
        SEND("ra = "); SEND(float2str(ra, 7));
    }
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_azhd(const char *, char *args){
    float alt;
    if(getflag(args)){ // hd to az
        hadec_to_altaz(ha, dec, &alt, &az);
        zd = 90.f - alt;
        SEND("az = "); SEND(float2str(az, 7));
        SEND("zd = "); SEND(float2str(zd, 7));
    }else{ // az to hd
        alt = 90.f - zd;
        altaz_to_hadec(alt, az, &ha, &dec);
        // and recalculate ra
        float lst_deg = HOURS2DEG(LST_from_unix(curUNIXt()));
        ra = ha_to_ra(ha, lst_deg);
        SEND("ha = "); SEND(float2str(ha, 7));
        SEND("\nra = "); SEND(float2str(ra, 7));
        SEND("\ndec = "); SEND(float2str(dec, 7));
    }
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_refr(const char *cmd, char *){
    float A, B;
    refco_f32(phpa, tc, rh, 0.55, &A, &B);
    SEND("coeffs a/b: "); SEND(float2str(A, 3)); SEND(", "); SEND(float2str(B, 3)); SEND("\n");
    float refr = refraction(phpa, tc, rh, zd);
    CMDEQ();
    SEND(float2str(DEG2ARCSEC(refr), 2));
    SEND("\n");
    return ERR_AMOUNT;
}

// setter/getter of floats
static errcodes_t handle_var(const VarEntry* entry, const char* cmd, char* setter){
    if(setter){
        float val;
        if(setter == getfloat(setter, &val)) return ERR_BADPAR;
        if(val < entry->min_val || val > entry->max_val) return ERR_BADVAL;
        *entry->var = val;
    }
    CMDEQ();
    SEND(float2str(*entry->var, 7));
    SEND("\n");
    return ERR_AMOUNT;
}

// linear search by hash for variable
static const VarEntry* find_var_by_hash(uint32_t h){
    for(size_t i = 0; i < var_table_size; ++i){
        if(var_table[i].hash == h)
            return &var_table[i];
    }
    return nullptr;
}

const char *parse_cmd(int (*sendfun)(const char *), char *str){
    SEND = sendfun;
    char command[CMD_MAXLEN+1];
    int i = 0;
    while(*str > '@' && i < CMD_MAXLEN){ command[i++] = *str++; }
    command[i] = 0;
    while(*str && *str <= ' ') ++str;
    char *restof = (char*) str;
    uint32_t h = hash(command);
    errcodes_t ecode = ERR_AMOUNT;
    const VarEntry *entry = nullptr;
    switch(h){
#define COMMAND(name, desc) case hash(#name): ecode = cmd_ ## name(command, restof); break;
        COMMAND_TABLE
#undef COMMAND
        default:
            if((entry = find_var_by_hash(h))){
                ecode = handle_var(entry, command, splitargs(restof, nullptr));
            }else{
                SEND("Unknown command, try 'help'\n");
            }
    }
    if(ecode < ERR_AMOUNT) return errtxt[ecode];
    return nullptr;
}
