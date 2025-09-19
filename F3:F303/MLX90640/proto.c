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
#include "strfunc.h"
#include "usb_dev.h"
#include "version.inc"

#define LOCBUFFSZ       (32)
// local buffer for I2C data to send
static uint16_t locBuffer[LOCBUFFSZ];
static uint8_t I2Caddress = 0x33 << 1;
extern volatile uint32_t Tms;

static const char *OK = "OK\n", *ERR = "ERR\n";
const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/mlx90640 build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "    management of single IR bolometer MLX90640\n"
        "i0..3 - setup I2C with speed 10k, 100k, 400k, 1M or 2M (experimental!)\n"
        "Ia addr - set  device address\n"
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

TRUE_INLINE const char *chaddr(const char *buf){
    uint32_t addr;
    const char *nxt = getnum(buf, &addr);
    if(nxt && nxt != buf){
        if(addr > 0x7f) return ERR;
        I2Caddress = (uint8_t) addr << 1;
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
        locBuffer[N++] = (uint8_t) D&0xff;
    }
    return N;
}

static const char *wrI2C(const char *buf){
    uint16_t N = readNnumbers(buf);
    if(N == 0) return ERR;
    if(!i2c_write(I2Caddress, locBuffer, N)) return ERR;
    return OK;
}

const char *parse_cmd(char *buf){
    if(!buf || !*buf) return NULL;
    if(buf[1]){
        switch(*buf){ // "long" commands
            case 'i':
                return setupI2C(buf + 1);
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
        case 'i': return setupI2C(NULL); // current settings
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
