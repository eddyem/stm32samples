/*
 * This file is part of the ir-allsky project.
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

#include <math.h>
#include <stm32f3.h>
#include <string.h>

#include "hardware.h"
#include "i2c.h"
#include "mlxproc.h"
#include "proto.h"
#include "strfunc.h"
#include "usart.h"
#include "usb_dev.h"
#include "version.inc"

#define LOCBUFFSZ       (32)
// local buffer for I2C data to send
static uint16_t locBuffer[LOCBUFFSZ];
static uint8_t I2Caddress = 0x33 << 1;
extern volatile uint32_t Tms;
uint8_t cartoon = 0; // "cartoon" mode: refresh image each time we get new

// functions to send data over USB or USART: to change them use flag in `parse_cmd`
typedef struct{
    int (*S)(const char*);          // send string
    int (*P)(uint8_t);              // put byte
    int (*B)(const uint8_t*, int);  // send raw bytes
} sendfun_t;

static sendfun_t usbsend = {
    .S = USB_sendstr, .P = USB_putbyte, .B = USB_send
};
static sendfun_t usartsend = {
    .S = usart_sendstr, .P = usart_putbyte, .B = usart_send
};

static sendfun_t *sendfun = &usbsend;

void chsendfun(int sendto){
    if(sendto == SEND_USB) sendfun = &usbsend;
    else sendfun = &usartsend;
}

// newline
#define N() sendfun->P('\n')
#define printu(x)       do{sendfun->S(u2str(x));}while(0)
#define printi(x)       do{sendfun->S(i2str(x));}while(0)
#define printuhex(x)    do{sendfun->S(uhex2str(x));}while(0)
#define printfl(x,n)    do{sendfun->S(float2str(x, n));}while(0)

// common names for frequent keys
const char* const Timage = "TIMAGE";
const char* const Image = "IMAGE";
static const char *const Sensno = "SENSNO=";

static const char *const OK = "OK\n", *const ERR = "ERR\n";
const char *const helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/MLX90640multi build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "    management of single IR bolometer MLX90640\n"
        "dn - draw nth image in ASCII\n"
        "gn - get nth image 'as is' - float array of 768x4 bytes\n"
        "l - list active sensors IDs\n"
        "mn - show temperature map of nth image\n"
        "tn - show nth image aquisition time\n"
        "B - reinit BME280\n"
        "E - get environment parameters (temperature etc)\n"
        "G - get MLX state\n"
        "R - reset device\n"
        "T - print current Tms\n"
        "    Debugging options:\n"
        "aa - change I2C address to a (a should be non-shifted value!!!)\n"
        "c - continue MLX\n"
        "i0..4 - setup I2C with speed 10k, 100k, 400k, 1M or 2M (experimental!)\n"
        "p - pause MLX\n"
        "s - stop MLX (and start from zero @ 'c')\n"
        "C - \"cartoon\" mode on/off (show each new image) - USB only!!!\n"
        "Dn - dump MLX parameters for sensor number n\n"
        "Ia addr [n] - set  device address for interactive work or (with n) change address of n'th sensor\n"
        "Ir reg n - read n words from 16-bit register\n"
        "Iw words - send words (hex/dec/oct/bin) to I2C\n"
        "Is - scan I2C bus\n"
        "Us - send string 's' to other interface\n"
;

TRUE_INLINE const char *setupI2C(char *buf){
    static const char * const speeds[I2C_SPEED_AMOUNT] = {
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
    sendfun->S("I2CSPEED="); sendfun->S(speeds[i2c_curspeed]); N();
    return NULL;
}

TRUE_INLINE const char *chhwaddr(const char *buf){
    uint32_t a;
    if(buf && *buf){
        const char *nxt = getnum(buf, &a);
        if(nxt && nxt != buf){
            if(!mlx_sethwaddr(I2Caddress, a)) return ERR;
        }else{
            sendfun->S("Wrong number"); N();
            return ERR;
        }
    }else{
        sendfun->S("Need address"); N();
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
    sendfun->S("I2CADDR="); sendfun->S(uhex2str(addr)); N();
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
        sendfun->S(b); N();
    }else hexdump16(sendfun->S, b16, N);
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
        sendfun->S("byte "); sendfun->S(u2str(i));
        sendfun->S(" :"); sendfun->S(uhex2str(locBuffer[i])); N();
    }
    if(!i2c_write(I2Caddress, locBuffer, N)) return ERR;
    return OK;
}

static void dumpfarr(float *arr){
    for(int row = 0; row < 24; ++row){
        for(int col = 0; col < 32; ++col){
            printfl(*arr++, 2); sendfun->P(' ');
        }
        N();
    }
}
// dump MLX parameters
TRUE_INLINE void dumpparams(const char *buf){
    int N = getsensnum(buf);
    if(N < 0){ sendfun->S(ERR); return; }
    MLX90640_params *params = mlx_getparams(N);
    if(!params){ sendfun->S(ERR); return; }
    N(); sendfun->S(Sensno); sendfun->S(i2str(N));
    sendfun->S("\nkVdd="); printi(params->kVdd);
    sendfun->S("\nvdd25="); printi(params->vdd25);
    sendfun->S("\nKvPTAT="); printfl(params->KvPTAT, 4);
    sendfun->S("\nKtPTAT="); printfl(params->KtPTAT, 4);
    sendfun->S("\nvPTAT25="); printi(params->vPTAT25);
    sendfun->S("\nalphaPTAT="); printfl(params->alphaPTAT, 2);
    sendfun->S("\ngainEE="); printi(params->gainEE);
    sendfun->S("\nPixel offset parameters:\n");
    float *offset = params->offset;
    for(int row = 0; row < 24; ++row){
        for(int col = 0; col < 32; ++col){
            printfl(*offset++, 2); sendfun->P(' ');
        }
        N();
    }
    sendfun->S("K_talpha:\n");
    dumpfarr(params->kta);
    sendfun->S("Kv: ");
    for(int i = 0; i < 4; ++i){
        printfl(params->kv[i], 2); sendfun->P(' ');
    }
    sendfun->S("\ncpOffset=");
    printi(params->cpOffset[0]); sendfun->S(", "); printi(params->cpOffset[1]);
    sendfun->S("\ncpKta="); printfl(params->cpKta, 2);
    sendfun->S("\ncpKv="); printfl(params->cpKv, 2);
    sendfun->S("\ntgc="); printfl(params->tgc, 2);
    sendfun->S("\ncpALpha="); printfl(params->cpAlpha[0], 2);
    sendfun->S(", "); printfl(params->cpAlpha[1], 2);
    sendfun->S("\nKsTa="); printfl(params->KsTa, 2);
    sendfun->S("\nAlpha:\n");
    dumpfarr(params->alpha);
    sendfun->S("\nCT3="); printfl(params->CT[1], 2);
    sendfun->S("\nCT4="); printfl(params->CT[2], 2);
    for(int i = 0; i < 4; ++i){
        sendfun->S("\nKsTo"); sendfun->P('0'+i); sendfun->P('=');
        printfl(params->KsTo[i], 2);
        sendfun->S("\nalphacorr"); sendfun->P('0'+i); sendfun->P('=');
        printfl(params->alphacorr[i], 2);
    }
    N();
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
    sendfun->S("MLXSTATE=");
    sendfun->S(states[s]); N();
}

// `draw`==1 - draw, ==0 - show T map, 2 - send raw float array with prefix 'TIMAGEX=y\nIMAGEX=' and postfix "ENDIMAGE\n"
static const char *drawimg(const char *buf, int draw){
    int sensno = getsensnum(buf);
    if(sensno > -1){
        uint32_t T = mlx_lastimT(sensno);
        fp_t *img = mlx_getimage(sensno);
        if(img){
            //sendfun->S(Sensno); sendfun->S(u2str(sensno)); N();
            sendfun->S(Timage); sendfun->P('0'+sensno); sendfun->P('='); sendfun->S(u2str(T)); N();
            switch(draw){
                case 0:
                    dumpIma(img);
                break;
                case 1:
                    drawIma(img);
                break;
                case 2:
                    sendfun->S(Image); sendfun->P('0'+sensno); sendfun->P('=');
                    uint8_t *d = (uint8_t*)img;
                    uint32_t _2send = MLX_PIXNO * sizeof(float);
                    // send by portions of 256 bytes (as image is larger than ringbuffer)
                    while(_2send){
                        uint32_t portion = (_2send > 256) ? 256 : _2send;
                        sendfun->B(d, portion);
                        _2send -= portion;
                        d += portion;
                    }
                    sendfun->S("ENDIMAGE"); N();
                break;
            }
            return NULL;
        }
    }
    return ERR;
}

TRUE_INLINE void listactive(){
    int N =  mlx_nactive();
    if(!N){ sendfun->S("No active sensors found!\n"); return; }
    uint8_t *ids = mlx_activeids();
    sendfun->S("Found "); sendfun->P('0'+N);
    sendfun->S(" active sensors:"); N();
    for(int i = 0; i < N_SESORS; ++i)
        if(ids[i]){
            sendfun->S("SENSID");
            sendfun->S(u2str(i)); sendfun->P('=');
            sendfun->S(uhex2str(ids[i] >> 1));
            N();
        }
}

static void getimt(const char *buf){
    int sensno = getsensnum(buf);
    if(sensno > -1){
        sendfun->S(Timage); sendfun->P('0'+sensno); sendfun->P('='); sendfun->S(u2str(mlx_lastimT(sensno))); N();
    }else sendfun->S(ERR);
}

TRUE_INLINE void getenv(){
    bme280_t env;
    if(!get_environment(&env)) sendfun->S("BADENVIRONMENT\n");
    sendfun->S("TEMPERATURE="); sendfun->S(float2str(env.T, 2));
    sendfun->S("\nPRESSURE_HPA="); sendfun->S(float2str(env.P/100.f, 2));
    sendfun->S("\nPRESSURE_MM="); sendfun->S(float2str(env.P * 0.00750062f, 2));
    sendfun->S("\nHUMIDITY="); sendfun->S(float2str(env.H, 2));
    sendfun->S("\nTEMP_DEW="); sendfun->S(float2str(env.Tdew, 1));
    sendfun->S("\nT_MEASUREMENT="); sendfun->S(u2str(env.Tmeas));
    N();
}

/**
 * @brief parse_cmd - user string parser
 * @param buf - user data
 * @param isusb - ==1 to send answer over usb, else send over USART1
 * @return answer OK/ERR or NULL
 */
const char *parse_cmd(char *buf, int sendto){
    if(!buf || !*buf) return NULL;
    chsendfun(sendto);
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
            case 'U':
                if(sendto == SEND_USB) chsendfun(SEND_USART);
                else chsendfun(SEND_USB);
                if(sendfun->S(buf + 1) && N()) return OK;
                return ERR;
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
        case 'B':
            if(bme_init()) return OK;
            return ERR;
        case 'C':
            if(sendto != SEND_USB) return ERR;
            cartoon = !cartoon; return OK;
        case 'E':
            getenv();
        break;
        case 'G':
            getst();
        break;
        case 'R':
            NVIC_SystemReset();
        break;
        case 'T':
            sendfun->S("T="); sendfun->S(u2str(Tms)); N();
        break;
        case '?': // help
        case 'h':
        case 'H':
            sendfun->S(helpstring);
        break;
        default:
            return ERR;
        break;
    }
    return NULL;
}

// dump image as temperature matrix
void dumpIma(const fp_t im[MLX_PIXNO]){
    for(int row = 0; row < MLX_H; ++row){
        for(int col = 0; col < MLX_W; ++col){
            printfl(*im++, 1);
            sendfun->P(' ');
        }
        N();
    }
}

#define GRAY_LEVELS     (16)
// 16-level character set ordered by fill percentage (provided by user)
static const char *const CHARS_16 = " .':;+*oxX#&%B$@";
// draw image in ASCII-art
void drawIma(const fp_t im[MLX_PIXNO]){
    // Find min and max values
    fp_t min_val = im[0], max_val = im[0];
    const fp_t *iptr = im;
    for(int row = 0; row < MLX_H; ++row){
        for(int col = 0; col < MLX_W; ++col){
            fp_t cur = *iptr++;
            if(cur < min_val) min_val = cur;
            else if(cur > max_val) max_val = cur;
        }
    }
    fp_t range = max_val - min_val;
    sendfun->S("RANGE="); sendfun->S(float2str(range, 3));
    sendfun->S("\nMIN="); sendfun->S(float2str(min_val, 3));
    sendfun->S("\nMAX="); sendfun->S(float2str(max_val, 3)); N();
    if(fabsf(range) < 0.001) range = 1.; // solid fill -> blank
    // Generate and print ASCII art
    iptr = im;
    for(int row = 0; row < MLX_H; ++row){
        for(int col = 0; col < MLX_W; ++col){
            fp_t normalized = ((*iptr++) - min_val) / range;
            // Map to character index (0 to 15)
            int index = (int)(normalized * GRAY_LEVELS);
            // Ensure we stay within bounds
            if(index < 0) index = 0;
            else if(index > (GRAY_LEVELS-1)) index = (GRAY_LEVELS-1);
            sendfun->P(CHARS_16[index]);
        }
        N();
    }
    N();
}

