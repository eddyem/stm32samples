/*
 * This file is part of the mlx90640 project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f3.h>
#include <string.h>

#include "i2c.h"
#include "mlxproc.h"
#include "strfunc.h"
#include "usb_dev.h"
#include "version.inc"

#define LOCBUFFSZ       (32)
// local buffer for I2C data to send
static uint16_t locBuffer[LOCBUFFSZ];
static uint8_t I2Caddress = 0x33 << 1;
extern volatile uint32_t Tms;
uint8_t cartoon = 0; // "cartoon" mode: refresh image each time we get new

// common names for frequent keys
const char *Timage = "TIMAGE=";
const char *Sensno = "SENSNO=";

static const char *OK = "OK\n", *ERR = "ERR\n";
const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/MLX90640multi build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "    management of single IR bolometer MLX90640\n"
        "aa - change I2C address to a (a should be non-shifted value!!!)\n"
        "c - continue MLX\n"
        "dn - draw nth image in ASCII\n"
        "gn - get nth image 'as is' - float array of 768x4 bytes\n"
        "i0..4 - setup I2C with speed 10k, 100k, 400k, 1M or 2M (experimental!)\n"
        "l - list active sensors IDs\n"
        "tn - show temperature map of nth image\n"
        "p - pause MLX\n"
        "s - stop MLX (and start from zero @ 'c'\n"
        "tn - show nth image aquisition time\n"
        "C - \"cartoon\" mode on/off (show each new image)\n"
        "Dn - dump MLX parameters for sensor number n\n"
        "G - get MLX state\n"
        "Ia addr [n] - set  device address for interactive work or (with n) change address of n'th sensor\n"
        "Ir reg n - read n words from 16-bit register\n"
        "Iw words - send words (hex/dec/oct/bin) to I2C\n"
        "Is - scan I2C bus\n"
        "T - print current Tms\n"
;

TRUE_INLINE const char *setupI2C(char *buf){
    static const char *speeds[I2C_SPEED_AMOUNT] = {
        [I2C_SPEED_10K] = "10K",
        [I2C_SPEED_100K] = "100K",
        [I2C_SPEED_400K] = "400K",
        [I2C_SPEED_1M] = "1M",
        [I2C_SPEED_2M] = "2M"
    };
    if(buf && *buf){
        buf = omit_spaces(buf);
        int speed = *buf - '0';
        if(speed < 0 || speed >= I2C_SPEED_AMOUNT){
            return ERR;
        }
        i2c_setup((i2c_speed_t)speed);
    }
    U("I2CSPEED="); USND(speeds[i2c_curspeed]);
    return NULL;
}

TRUE_INLINE const char *chhwaddr(const char *buf){
    uint32_t a;
    if(buf && *buf){
        const char *nxt = getnum(buf, &a);
        if(nxt && nxt != buf){
            if(!mlx_sethwaddr(I2Caddress, a)) return ERR;
        }else{
            USND("Wrong number");
            return ERR;
        }
    }else{
        USND("Need address");
        return ERR;
    }
    return OK;
}

// read sensor's number from `buf`; return -1 if error
static int getsensnum(const char *buf){
    if(!buf || !*buf) return -1;
    uint32_t num;
    const char *nxt = getnum(buf, &num);
    if(!nxt || nxt == buf || num >= N_SESORS) return -1;
    return (int) num;
}

TRUE_INLINE const char *chaddr(const char *buf){
    uint32_t addr;
    const char *nxt = getnum(buf, &addr);
    if(nxt && nxt != buf){
        if(addr > 0x7f) return ERR;
        I2Caddress = (uint8_t) addr << 1;
        int n = getsensnum(nxt);
        if(n > -1) mlx_setaddr(n, addr);
    }else addr = I2Caddress >> 1;
    U("I2CADDR="); USND(uhex2str(addr));
    return NULL;
}

// read I2C register[s] - only blocking read! (DMA allowable just for config/image reading in main process)
static const char *rdI2C(const char *buf){
    uint32_t N = 0;
    const char *nxt = getnum(buf, &N);
    if(!nxt || buf == nxt || N > 0xffff) return ERR;
    buf = nxt;
    uint16_t reg = N, *b16 = NULL;
    nxt = getnum(buf, &N);
    if(!nxt || buf == nxt || N == 0 || N > I2C_BUFSIZE) return ERR;
    if(!(b16 = i2c_read_reg16(I2Caddress, reg, N, 0))) return ERR;
    if(N == 1){
        char b[5];
        u16s(*b16, b);
        b[4] = 0;
        USND(b);
    }else hexdump16(USB_sendstr, b16, N);
    return NULL;
}

// read N numbers from buf, @return 0 if wrong or none
TRUE_INLINE uint16_t readNnumbers(const char *buf){
    uint32_t D;
    const char *nxt;
    uint16_t N = 0;
    while((nxt = getnum(buf, &D)) && nxt != buf && N < LOCBUFFSZ){
        buf = nxt;
        locBuffer[N++] = (uint16_t) D;
    }
    return N;
}

static const char *wrI2C(const char *buf){
    uint16_t N = readNnumbers(buf);
    if(N == 0) return ERR;
    for(int i = 0; i < N; ++i){
        U("byte "); U(u2str(i)); U(" :"); USND(uhex2str(locBuffer[i]));
    }
    if(!i2c_write(I2Caddress, locBuffer, N)) return ERR;
    return OK;
}

static void dumpfarr(float *arr){
    for(int row = 0; row < 24; ++row){
        for(int col = 0; col < 32; ++col){
            printfl(*arr++, 2); USB_putbyte(' ');
        }
        newline();
    }
}
// dump MLX parameters
TRUE_INLINE void dumpparams(const char *buf){
    int N = getsensnum(buf);
    if(N < 0){ U(ERR); return; }
    MLX90640_params *params = mlx_getparams(N);
    if(!params){ U(ERR); return; }
    U(Sensno); USND(i2str(N));
    U("\nkVdd="); printi(params->kVdd);
    U("\nvdd25="); printi(params->vdd25);
    U("\nKvPTAT="); printfl(params->KvPTAT, 4);
    U("\nKtPTAT="); printfl(params->KtPTAT, 4);
    U("\nvPTAT25="); printi(params->vPTAT25);
    U("\nalphaPTAT="); printfl(params->alphaPTAT, 2);
    U("\ngainEE="); printi(params->gainEE);
    U("\nPixel offset parameters:\n");
    float *offset = params->offset;
    for(int row = 0; row < 24; ++row){
        for(int col = 0; col < 32; ++col){
            printfl(*offset++, 2); USB_putbyte(' ');
        }
        newline();
    }
    U("K_talpha:\n");
    dumpfarr(params->kta);
    U("Kv: ");
    for(int i = 0; i < 4; ++i){
        printfl(params->kv[i], 2); USB_putbyte(' ');
    }
    U("\ncpOffset=");
    printi(params->cpOffset[0]); U(", "); printi(params->cpOffset[1]);
    U("\ncpKta="); printfl(params->cpKta, 2);
    U("\ncpKv="); printfl(params->cpKv, 2);
    U("\ntgc="); printfl(params->tgc, 2);
    U("\ncpALpha="); printfl(params->cpAlpha[0], 2);
    U(", "); printfl(params->cpAlpha[1], 2);
    U("\nKsTa="); printfl(params->KsTa, 2);
    U("\nAlpha:\n");
    dumpfarr(params->alpha);
    U("\nCT3="); printfl(params->CT[1], 2);
    U("\nCT4="); printfl(params->CT[2], 2);
    for(int i = 0; i < 4; ++i){
        U("\nKsTo"); USB_putbyte('0'+i); USB_putbyte('=');
        printfl(params->KsTo[i], 2);
        U("\nalphacorr"); USB_putbyte('0'+i); USB_putbyte('=');
        printfl(params->alphacorr[i], 2);
    }
    newline();
}
// get MLX state
TRUE_INLINE void getst(){
    static const char *states[] = {
        [MLX_NOTINIT] = "not init",
        [MLX_WAITPARAMS] = "wait parameters DMA read",
        [MLX_WAITSUBPAGE] = "wait subpage",
        [MLX_READSUBPAGE] = "wait subpage DMA read",
        [MLX_RELAX] = "do nothing"
    };
    mlx_state_t s = mlx_state();
    U("MLXSTATE=");
    USND(states[s]);
}

// `draw`==1 - draw, ==0 - show T map, 2 - send raw float array with prefix 'SENSNO=x\nTimage=y\n' and postfix "ENDIMAGE\n"
static const char *drawimg(const char *buf, int draw){
    int sensno = getsensnum(buf);
    if(sensno > -1){
        uint32_t T = mlx_lastimT(sensno);
        fp_t *img = mlx_getimage(sensno);
        if(img){
            U(Sensno); USND(u2str(sensno));
            U(Timage); USND(u2str(T));
            switch(draw){
                case 0:
                    dumpIma(img);
                break;
                case 1:
                    drawIma(img);
                break;
                case 2:
                {
                    uint8_t *d = (uint8_t*)img;
                    uint32_t _2send = MLX_PIXNO * sizeof(float);
                    // send by portions of 256 bytes (as image is larger than ringbuffer)
                    while(_2send){
                        uint32_t portion = (_2send > 256) ? 256 : _2send;
                        USB_send(d, portion);
                        _2send -= portion;
                        d += portion;
                    }
                }
                    USND("ENDIMAGE");
            }
            return NULL;
        }
    }
    return ERR;
}

TRUE_INLINE void listactive(){
    int N =  mlx_nactive();
    if(!N){ USND("No active sensors found!"); return; }
    uint8_t *ids = mlx_activeids();
    U("Found "); USB_putbyte('0'+N); USND(" active sensors:");
    for(int i = 0; i < N_SESORS; ++i)
        if(ids[i]){
            U("SENSID"); U(u2str(i)); USB_putbyte('=');
            U(uhex2str(ids[i] >> 1));
            newline();
        }
}

static void getimt(const char *buf){
    int sensno = getsensnum(buf);
    if(sensno > -1){
        U(Timage); USND(u2str(mlx_lastimT(sensno)));
    }else U(ERR);
}

const char *parse_cmd(char *buf){
    if(!buf || !*buf) return NULL;
    if(buf[1]){
        switch(*buf){ // "long" commands
            case 'a':
                return chhwaddr(buf + 1);
            case 'd':
                return drawimg(buf+1, 1);
            case 'g':
                return drawimg(buf+1, 2);
            case 'i':
                return setupI2C(buf + 1);
            case 'm':
                return drawimg(buf+1, 0);
            case 't':
                getimt(buf + 1); return NULL;
            case 'D':
                dumpparams(buf + 1);
                return NULL;
            break;
            case 'I':
                buf = omit_spaces(buf + 1);
                switch(*buf){
                    case 'a':
                        return chaddr(buf + 1);
                    case 'r':
                        return rdI2C(buf + 1);
                    case 'w':
                        return wrI2C(buf + 1);
                    case 's':
                        i2c_init_scan_mode();
                        return OK;
                    default:
                        return ERR;
                }
                break;
            default:
                return ERR;
        }
    }
    switch(*buf){ // "short" (one letter) commands
        case 'c':
            mlx_continue(); return OK;
        break;
        case 'i': return setupI2C(NULL); // current settings
        case 'l':
            listactive();
        break;
        case 'p':
            mlx_pause(); return OK;
        break;
        case 's':
            mlx_stop(); return OK;
        case 'C':
            cartoon = !cartoon; return OK;
        case 'G':
            getst();
        break;
        case 'T':
            U("T=");
            USND(u2str(Tms));
        break;
        case '?': // help
        case 'h':
        case 'H':
            U(helpstring);
        break;
        default:
            return ERR;
        break;
    }
    return NULL;
}
