/*
 * This file is part of the si7005 project.
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

#include "htu21d.h"
#include "i2c.h"

#define DEVID  (0x40)

#ifdef EBUG
#include "usb.h"
#include "proto.h"
#define DBG(x)  do{USB_send(x);}while(0)
#else
#define DBG(x)
#endif

// read with no hold master!
#define HTU21_READ_TEMP     (0xF3)
#define HTU21_READ_HUMID    (0xF5)
#define HTU21_READ_REG      (0xE7)
#define HTU21_WRITE_REG     (0xE6)
#define HTU21_SOFT_RESET    (0xFE)
// status mask & val
#define HTU21_STATUS_MASK   (0x03)
#define HTU21_HUMID_FLAG    (0x02)
// user reg fields
#define HTU21_REG_VBAT      (0x40)
#define HTU21_REG_D1        (0x80)
#define HTU21_REG_D0        (0x01)
#define HTU21_REG_HTR       (0x04)
#define HTU21_REG_ODIS      (0x02)

typedef enum{
	RELAX = 0,
	WAITFORT,       // T
	WAITFORH        // humidity
} HTU21D_state;

static HTU21D_state state = RELAX;
static HTU21D_status htustatus = HTU21D_RELAX;
static uint16_t TH; // data for T or H

HTU21D_status HTU21D_get_status(){
    return htustatus;
}

//output in Cx10
int32_t HTU21D_getT(){
    DBG("HTU getT\n");
    htustatus = HTU21D_RELAX;
    if(TH & HTU21_HUMID_FLAG) return -5000; // humidity measured
    uint32_t a = TH & 0xFFFC;
    a *= 17572;
    a >>= 16;
    int32_t val = (a - 4685)/10;
    return val;
}

//output in %x10
uint32_t HTU21D_getH(){
    DBG("HTU getH\n");
    htustatus = HTU21D_RELAX;
    if(!(TH & HTU21_HUMID_FLAG)) return 5000; // temperature measured
    uint32_t a = TH & 0xFFFC;
    a *= 1250;
    a >>= 16;
    a -= 60;
    return a;
}

void HTU21D_setup(){
    DBG("HTU setup\n");
    htustatus = HTU21D_RELAX;
    state = RELAX;
	i2c_setup();
	i2c_set_addr7(DEVID);
}


#define SHIFTED_DIVISOR 0x988000    //This is the 0x0131 polynomial shifted to farthest left of three bytes
// check CRC, return 0 if all OK
static uint32_t htu_check_crc(uint8_t *crc){
    DBG("HTU check CRC\n");
    uint32_t remainder = (crc[0] << 16) | (crc[1] << 8) | crc[2];
    uint32_t divsor = (uint32_t)SHIFTED_DIVISOR;
    int i;
    for(i = 0; i < 16; i++) {
        if (remainder & (uint32_t)1 << (23 - i))
            remainder ^= divsor;
        divsor >>= 1;
    }
    return remainder;
}

static int htusendcmd(uint8_t cmd){
    DBG("htu cmd\n");
    if(state != RELAX) return 1;
    htustatus = HTU21D_BUSY;
    if(I2C_OK != i2c_7bit_send_onebyte(cmd, 1)){
        htustatus = HTU21D_ERR;
        DBG("htu read err\n");
        return 1;
    }
    return 0;
}

int HTU21D_cmdT(){
    DBG("htu read T\n");
    if(htusendcmd(HTU21_READ_TEMP)) return 1;
    state = WAITFORT;
    return 0;
}

int HTU21D_cmdH(){
    DBG("htu read H\n");
    if(htusendcmd(HTU21_READ_HUMID)) return 1;
    state = WAITFORH;
    return 0;
}

void HTU21D_process(){
    uint8_t d[3];
    if(state == RELAX) return;
	if(state == WAITFORH || state == WAITFORT){ // poll RDY
        if(I2C_OK != i2c_7bit_receive(d, 3)) return; // NACKed
        DBG("HTU got H or T:");
        DBG(u2str(d[0])); DBG(", ");
        DBG(u2str(d[1])); DBG(", ");
        DBG(u2str(d[2])); DBG("\n");
        if(htu_check_crc(d)){
            htustatus = HTU21D_ERR;
            DBG("CRC failed\n");
            state = RELAX;
            return;
        }
        TH = (d[0] << 8) | d[1];
        htustatus = (state == WAITFORH) ? HTU21D_HRDY : HTU21D_TRDY;
        state = RELAX;
    }
}
