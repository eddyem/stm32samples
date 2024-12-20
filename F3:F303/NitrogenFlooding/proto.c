/*
 * This file is part of the nitrogen project.
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
#include "BMP280.h"
#include "hardware.h"
#include "i2c.h"
#include "proto.h"
#include "ili9341.h"
#include "screen.h"
#include "strfunc.h"
#include "version.inc"

#include <stm32f3.h>
#include <string.h>

static uint8_t I2Caddress = 0;

static const char *OK = "OK\n";

// parno - number of parameter (or -1); cargs - string with arguments (after '=') (==NULL for getter), iarg - integer argument
static int goodstub(const char *cmd, int parno, const char *carg, int32_t iarg){
    USB_sendstr("cmd="); USB_sendstr(cmd);
    USB_sendstr(", parno="); USB_sendstr(i2str(parno));
    USB_sendstr(", args="); USB_sendstr(carg);
    USB_sendstr(", intarg="); USB_sendstr(i2str(iarg)); newline();
    return RET_GOOD;
}

// send over USB cmd[parno][=i]
static void sendkey(const char *cmd, int parno, int32_t i){
    USB_sendstr(cmd);
    if(parno > -1) USB_sendstr(u2str((uint32_t)parno));
    USB_putbyte('='); USB_sendstr(i2str(i)); newline();
}
// `sendkey` for floating parameter
static void sendkeyf(const char *cmd, int parno, float f){
    USB_sendstr(cmd);
    if(parno > -1) USB_sendstr(u2str((uint32_t)parno));
    USB_putbyte('='); USB_sendstr(float2str(f, 2)); newline();
}
// `sendkey` for uint32_t
static void sendkeyu(const char *cmd, int parno, uint32_t u){
    USB_sendstr(cmd);
    if(parno > -1) USB_sendstr(u2str((uint32_t)parno));
    USB_putbyte('='); USB_sendstr(u2str(u)); newline();
}
// `sendkey` for uint32_t out in hex
static void sendkeyuhex(const char *cmd, int parno, uint32_t u){
    USB_sendstr(cmd);
    if(parno > -1) USB_sendstr(uhex2str((uint32_t)parno));
    USB_putbyte('='); USB_sendstr(uhex2str(u)); newline();
}

static int leds(const char *cmd, int parno, const char *c, int32_t i){
    if(parno < 0){ // enable/disable all
        if(c){
            if(i) LEDS_ON();
            else LEDS_OFF();
        }
        sendkey("LEDon", -1, LEDsON);
    }else{
        if(parno >= LEDS_AMOUNT) return RET_WRONGARG;
        if(c) switch(i){
            case 2: LED_blink(parno); break;
            case 1: LED_on(parno); break;
            default: LED_off(parno);
        }
        sendkey(cmd, parno, LED_get(parno));
    }
    return RET_GOOD;
}

static int bme(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    if(BMP280_get_status() == BMP280_NOTINIT){
        DBG("Need 2 init");
        if(!BMP280_init()){
            USND("Can't init");
            return RET_BAD;
        }
    }
    if(!BMP280_start()) return RET_BAD;
    return RET_GOOD;
}

static int bmefilter(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    BMP280_Filter f = BMP280_FILTER_OFF;
    if(c){
        if(i < 0 || i >= BMP280_FILTERMAX) return RET_WRONGARG;
        f = (BMP280_Filter) i;
        BMP280_setfilter(f);
        if(!BMP280_init()) return RET_BAD;
    }
    sendkey(cmd, -1, BMP280_getfilter());
    return RET_GOOD;
}

static int buzzer(const char *cmd, int _U_ parno, const char _U_ *c, int32_t i){
    if(c){
        if(i > 0) BUZZER_ON();
        else BUZZER_OFF();
    }
    sendkey(cmd, -1, BUZZER_STATE());
    return RET_GOOD;
}

static int i2scan(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    i2c_init_scan_mode();
    return RET_GOOD;
}
static int i2addr(const char *cmd, int _U_ parno, const char *c, int32_t i){
    if(c){
        if(i < 0 || i>= I2C_ADDREND) return RET_WRONGARG;
        I2Caddress = ((uint8_t) i)<<1;
    }
    sendkey(cmd, -1, I2Caddress>>1);
    return RET_GOOD;
}
static int i2reg(const char *cmd, int parno, const char _U_ *c, int32_t i){
    if(parno < 0 || parno > 127) return RET_WRONGPARNO;
    uint8_t d[2];
    if(c){
        if(i < 0 || i > 255) return RET_WRONGARG;
        d[0] = parno; d[1] = (uint8_t)i;
        if(!write_i2c(I2Caddress, d, 2)) return RET_BAD;
    }
    if(!read_i2c_reg(I2Caddress, (uint8_t)parno, d, 1)) return RET_BAD;
    sendkey(cmd, parno, d[0]);
    return RET_GOOD;
}
static int i2read(const char _U_ *cmd, int parno, const char _U_ *c, int32_t _U_ i){
    uint8_t buf[128];
    if(parno < 0 || parno > 128) return RET_WRONGPARNO;
    if(!read_i2c(I2Caddress, buf, (uint8_t)parno)) return RET_BAD;
    USB_sendstr("I2C got data:\n");
    hexdump(USB_sendstr, buf, parno);
    return RET_GOOD;
}

static int adcon(const char *cmd, int _U_ parno, const char *c, int32_t i){
    if(c) ADCON(i);
    sendkey(cmd, -1, ADCONSTATE());
    return RET_GOOD;
}
static int adcval(const char *cmd, int parno, const char _U_ *c, int32_t i){
    if(parno >= NUMBER_OF_ADC_CHANNELS) return RET_WRONGPARNO;
    if(parno < 0){ // all channels
        for(i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i) sendkey(cmd, i, ADCvals[i]);
    }else
        sendkey(cmd, parno, ADCvals[i]);
    return RET_GOOD;
}
static int adcvoltage(const char *cmd, int parno, const char _U_ *c, int32_t i){
    if(parno >= NUMBER_OF_ADC_CHANNELS) return RET_WRONGPARNO;
    if(parno < 0){ // all channels
        for(i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i) sendkeyf(cmd, i, getADCvoltage(i));
    }else
        sendkeyf(cmd, parno, getADCvoltage(parno));
    return RET_GOOD;
}
static int mcut(const char *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    sendkeyf(cmd, -1, getMCUtemp());
    return RET_GOOD;
}

static int reset(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    USB_sendstr("RESET!!!\n");
    USB_sendall();
    NVIC_SystemReset();
    return RET_GOOD; // never reached
}

static int tms(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    sendkeyu(cmd, -1, Tms);
    return RET_GOOD;
}

static int pwm(const char *cmd, int parno, const char *c, int32_t i){
    if(parno < 0 || parno > 3) return RET_WRONGPARNO;
    if(c){
        if(i < 0 || i > PWM_CCR_MAX) return RET_WRONGARG;
        setPWM(parno, (uint16_t)i);
    }
    sendkeyu(cmd, parno, getPWM(parno));
    return RET_GOOD;
}

static int scrnled(const char *cmd, int _U_ parno, const char *c, int32_t i){
    if(c) SCRN_LED_set(i);
    sendkeyu(cmd, -1, SCRN_LED_get());
    return RET_GOOD;
}
static int scrndcr(const char *cmd, int _U_ parno, const char *c, int32_t i){
    if(c){
        if(i) SCRN_Data();
        else SCRN_Command();
    }
    sendkeyu(cmd, -1, SCRN_DCX_get());
    return RET_GOOD;
}/*
static int scrnrst(const char *cmd, int _U_ parno, const char *c, int32_t i){
    if(c) SCRN_CS_set(i);
    sendkeyu(cmd, -1, SCRN_CS_get());
    return RET_GOOD;
}*/
static int scrnrdwr(const char *cmd, int parno, const char *c, int32_t i){
    if(parno < 0 || parno > 255) return RET_WRONGPARNO;
    if(c){
        if(i < 0 || i > 255) return RET_WRONGARG;
        if(!ili9341_writereg(parno, (uint8_t*)&i, 1)) return RET_BAD;
    }
    i = 0;
    if(!ili9341_readreg(parno, (uint8_t*)&i, 1)) return RET_BAD;
    sendkeyu(cmd, parno, i);
    return RET_GOOD;
}
static int scrnrdwr4(const char *cmd, int parno, const char *c, int32_t i){
    if(parno < 0 || parno > 255) return RET_WRONGPARNO;
    if(c){
        if(!ili9341_writereg(parno, (uint8_t*)&i, 4)) return RET_BAD;
    }
    if(!ili9341_readreg(parno, (uint8_t*)&i, 4)) return RET_BAD;
    sendkeyuhex(cmd, parno, i);
    return RET_GOOD;
}
static int scrnrdn(const char *cmd, int parno, const char *c, int32_t i){
    if(parno < 0 || parno > 255) return RET_WRONGPARNO;
    if(!c || i < 1 || i > COLORBUFSZ*2) return RET_WRONGARG;
    if(!(i = ili9341_readregdma(parno, (uint8_t*)colorbuf, i))) return RET_BAD;
    sendkey(cmd, parno, i);
    hexdump(USB_sendstr, (uint8_t*)colorbuf, i);
    return RET_GOOD;
}
static int scrncmd(const char _U_ *cmd, int parno, const char _U_ *c, int32_t _U_ i){
    if(parno < 0 || parno > 255) return RET_WRONGPARNO;
    if(!ili9341_writecmd((uint8_t)parno)) return RET_BAD;
    USB_sendstr(OK);
    return RET_GOOD;
}
static int scrndata(const char *cmd, int parno, const char _U_ *c, int32_t _U_ i){
    if(parno < 0 || parno > 255) return RET_WRONGPARNO;
    uint8_t s = (uint8_t)parno;
    if(!ili9341_writedata(&s, 1)) return RET_BAD;
    sendkeyuhex(cmd, parno, s);
    return RET_GOOD;
}
static int scrndata4(const char *cmd, int parno, const char *c, int32_t i){
    if(parno < 1 || parno > 4) return RET_WRONGPARNO;
    if(!c) return RET_WRONGARG;
    if(!ili9341_writedata((uint8_t*)&i, parno)) return RET_BAD;
    sendkeyuhex(cmd, parno, (uint32_t)i);
    return RET_GOOD;
}
static int scrninit(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    if(!ili9341_init()) return RET_BAD;
    USB_sendstr(OK);
    return RET_GOOD;
}
static int scrnonoff(const char _U_ *cmd, int _U_ parno, const char *c, int32_t i){
    if(!c) return RET_WRONGARG;
    int r = (i) ? ili9341_on() : ili9341_off();
    if(!r) return RET_BAD;
    sendkeyu(cmd, -1, i);
    return RET_GOOD;
}
static int scrnfill(const char *cmd, int parno, const char *c, int32_t i){
    if(parno < 0) parno = RGB(0xf, 0x1f, 0xf);
    if(parno > 0xffff) return RET_WRONGPARNO;
    if(!c){if(!(i=ili9341_fill((uint16_t)parno))) return RET_BAD;
    }else{
        if(i < 1) return RET_WRONGARG;
        if(!ili9341_fillp((uint16_t)parno, -i)) return RET_BAD;
    }
    sendkeyu(cmd, parno, i);
    return RET_GOOD;
}
static int scrnfilln(const char *cmd, int parno, const char *c, int32_t i){
    if(parno < 0) parno = RGB(0xf, 0x1f, 0xf);
    if(parno > 0xffff) return RET_WRONGPARNO;
    if(!c){if(!(i=ili9341_filln((uint16_t)parno))) return RET_BAD;
    }else{
        if(i < 1) return RET_WRONGARG;
        if(!ili9341_fillp((uint16_t)parno, i)) return RET_BAD;
    }
    sendkeyu(cmd, parno, i);
    return RET_GOOD;
}
static int smadctl(const char *cmd, int parno, const char *c, int32_t i){
    if(!c) i = 0;
    if(i < 0 || i > 255) return RET_WRONGARG;
    if(!ili9341_writereg(ILI9341_MADCTL, (uint8_t*)&i, 1)) return RET_BAD;
    sendkeyuhex(cmd, parno, i);
    return RET_GOOD;
}
static int scolrow(int col, const char *cmd, int parno, const char *c, int32_t i){
    if(!c || i < 0 || i > 319) return RET_WRONGARG;
    if(parno < 0 || parno > 319) return RET_WRONGPARNO;
    if(col){if(!ili9341_setcol((uint16_t)parno, (uint16_t)i)) return RET_BAD;}
    else{if(!ili9341_setrow((uint16_t)parno, (uint16_t)i)) return RET_BAD;}
    sendkeyu(cmd, parno, i);
    return RET_GOOD;
}
static int scol(const char *cmd, int parno, const char *c, int32_t i){
    return scolrow(1, cmd, parno, c, i);
}
static int srow(const char *cmd, int parno, const char *c, int32_t i){
    return scolrow(0, cmd, parno, c, i);
}
static int sread(const char *cmd, int parno, const char _U_ *c, int32_t _U_ i){
    if(parno < 0 || parno > COLORBUFSZ*2) return RET_WRONGARG;
    if(!(parno = ili9341_readdata((uint8_t*)colorbuf, parno))) return RET_BAD;
    USB_sendstr(cmd); USB_sendstr(u2str(parno)); newline();
    hexdump(USB_sendstr, (uint8_t*)colorbuf, parno);
    return RET_GOOD;
}
static int scls(const char *cmd, int parno, const char *c, int32_t i){
    // fg=bg, default: fg=0xffff, bg=0
    if(parno < 0 || parno > 0xffff) parno = 0xffff;
    if(!c) i = 0;
    setBGcolor(i);
    setFGcolor(parno);
    ClearScreen();
    sendkeyuhex(cmd, parno, i);
    return RET_GOOD;
}
static int srefr(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    UpdateScreen(0, SCRNH-1);
    USB_sendstr(OK);
    return RET_GOOD;
}

static int scolor(const char *cmd, int parno, const char *c, int32_t i){
    // fg=bg, default: fg=0xffff, bg=0
    if(parno < 0 || parno > 0xffff) parno = 0xffff;
    if(!c) i = 0;
    setBGcolor(i);
    setFGcolor(parno);
    sendkeyuhex(cmd, parno, i);
    return RET_GOOD;
}
static int sputstr(const char _U_ *cmd, int parno, const char *c, int32_t _U_ i){
    if(!c) return RET_WRONGARG;
    if(parno < 0) parno = 0;
    if(parno > SCRNH-1) parno = SCRNH-1;
    PutStringAt(0, parno, c);
    int fs = SetFontScale(0); // get font scale
    UpdateScreen(parno-13*fs, parno+3*fs);
    USB_sendstr("put string: '"); USB_sendstr(c); USB_sendstr("'\n");
    return RET_GOOD;
}
static int sstate(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    const char *s = "unknown";
    switch(ScrnState){
        case SCREEN_INIT:
            s = "init";
        break;
        case SCREEN_W4INIT:
            s = "wait for init";
        break;
        case SCREEN_RELAX:
            s = "relax";
        break;
        case SCREEN_CONVBUF:
            s = "convert buffer";
        break;
        case SCREEN_UPDATENXT:
            s = "update next";
        break;
        case SCREEN_ACTIVE:
            s = "transmission in progress";
        break;
    }
    USB_sendstr("ScreenState="); USB_sendstr(s); newline();
    return RET_GOOD;
}
static int sfscale(const char *cmd, int _U_ parno, const char _U_ *c, int32_t i){
    sendkeyu(cmd, -1, SetFontScale((uint8_t)i));
    return RET_GOOD;
}

static int buttons(const char *cmd, int parno, const char _U_ *c, int32_t _U_ i){
    if(parno < 0 || parno >= BTNSNO) return RET_WRONGPARNO;
    uint32_t T;
    keyevent evt = keystate((uint8_t)parno, &T);
    sendkeyu(cmd, parno, evt);
    if(evt != EVT_NONE){
        USB_sendstr("Tevent="); USB_sendstr(u2str(T)); newline();
    }
    return RET_GOOD;
}

typedef struct{
    int (*fn)(const char*, int, const char*, int32_t);
    const char *cmd;
    const char *help;
} commands;

commands cmdlist[] = {
    {goodstub, "stub", "simple stub"},
    {NULL, "Different commands", NULL},
    {bme, "BME", "get pressure, temperature and humidity"},
    {bmefilter, "BMEf", "set filter (0..4)"},
    {buttons, "button", "get buttonx state"},
    {buzzer, "buzzer", "get/set (0 - off, 1 - on) buzzer"},
    {leds, "LED", "LEDx=y; where x=0..3 to work with single LED (then y=1-set, 0-reset, 2-toggle), absent to set LEDsON (y=0 - disable, 1-enable)"},
    {pwm, "pwm", "set/get x channel (0..3) pwm value (0..100)"},
    {reset, "reset", "reset MCU"},
    {tms, "tms", "print Tms"},
    {NULL, "I2C commands", NULL},
    {i2addr, "iicaddr", "set/get I2C address"},
    {i2read, "iicread", "read X (0..128) bytes"},
    {i2reg, "iicreg", "read/write I2C register"},
    {i2scan, "iicscan", "scan I2C bus"},
    {NULL, "Screen commands", NULL},
    {scrnled, "Sled", "turn on/off screen lights"},
    {scrndcr, "Sdcr", "set data(1)/command(0)"},
   // {scrnrst, "Srst", "reset (1/0)"},
    {scrnrdwr, "Sreg", "read/write 8-bit register"},
    {scrnrdwr4, "Sregx", "read/write 32-bit register"},
    {scrnrdn, "Sregn", "read from register x =n bytes"},
    {scrncmd, "Scmd", "write 8bit command"},
    {scrndata, "Sdat", "write 8bit data"},
    {scrndata4, "Sdatn", "write x bytes of =data"},
    {sread, "Sread", "read "},
    {scrninit, "Sini", "init screen"},
    {scrnonoff, "Spower", "=0 - off, =1 - on"},
    {scrnfill, "Sfill", "fill screen with color (=npix)"},
    {scrnfilln, "Sfilln", "fill screen (next) with color (=npix)"},
    {smadctl, "Smad", "change MADCTL"},
    {scol, "Scol", "set column limits (low=high)"},
    {srow, "Srow", "set row limits (low=high)"},
    {scls, "Scls", "clear screen fg=bg"},
    {srefr, "Srefresh", "refresh full screen"},
    {scolor, "Scolor", "seg color fg=bg"},
    {sputstr, "Sstr", "put string y=string"},
    {sstate, "Sstate", "current screen state"},
    {sfscale, "Sfscale", "set/get =font scale"},
    {NULL, "ADC commands", NULL},
    {adcval, "ADC", "get ADCx value (without x - for all)"},
    {adcvoltage, "ADCv", "get ADCx voltage (without x - for all)"},
    {mcut, "mcut", "get MCU temperature"},
    {adcon, "sensv", "turn on (1) or off (0) Tsens voltage"},
    {NULL, NULL, NULL}
};

static void printhelp(){
    commands *c = cmdlist;
    USB_sendstr("https://github.com/eddyem/stm32samples/tree/master/F3:F303/NitrogenFlooding build#" BUILD_NUMBER " @ " BUILD_DATE "\n");
    while(c->cmd){
        if(!c->fn){ // header
            USB_sendstr("\n    ");
            USB_sendstr(c->cmd);
            USB_putbyte(':');
        }else{
            USB_sendstr(c->cmd);
            USB_sendstr(" - ");
            USB_sendstr(c->help);
        }
        newline();
        ++c;
    }
}

static int parsecmd(const char *str){
    char cmd[CMD_MAXLEN + 1];
    //USB_sendstr("cmd="); USB_sendstr(str); USB_sendstr("__\n");
    if(!str || !*str) return RET_CMDNOTFOUND;
    int i = 0;
    while(*str > '@' && i < CMD_MAXLEN){ cmd[i++] = *str++; }
    cmd[i] = 0;
    int parno = -1;
    int32_t iarg = __INT32_MAX__;
    if(*str){
        uint32_t N;
        const char *nxt = getnum(str, &N);
        if(nxt != str) parno = (int) N;
        str = strchr(str, '=');
        if(str){
            str = omit_spaces(++str);
            getint(str, &iarg);
        }
    }else str = NULL;
    commands *c = cmdlist;
    while(c->cmd){
        if(strcmp(c->cmd, cmd) == 0){
            if(!c->fn) return RET_CMDNOTFOUND;
            return c->fn(cmd, parno, str, iarg);
        }
        ++c;
    }
    return RET_CMDNOTFOUND;
}


/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 */
const char *cmd_parser(const char *txt){
    int ret = parsecmd(txt);
    switch(ret){
        case RET_WRONGPARNO: return "Wrong parameter number\n"; break;
        case RET_CMDNOTFOUND: printhelp(); return NULL; break;
        case RET_WRONGARG: return "Wrong command parameters\n"; break;
        case RET_GOOD: return NULL; break;
        default: return "FAIL\n"; break;
    }
}
