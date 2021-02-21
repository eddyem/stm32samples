/*
 * This file is part of the BMP180 project.
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

#include "i2c.h"
#include "BMP180.h"

//#define EBUG

#ifdef EBUG
#include "usb.h"
#include "proto.h"
#define DBG(x)  do{USB_send(x);}while(0)
#else
#define DBG(x)
#endif

#define BMP180_I2C_ADDRESS     (0x77)
/**
 * BMP180 registers
 */
#define BMP180_REG_OXLSB        (0xF8)
#define BMP180_REG_OLSB         (0xF7)
#define BMP180_REG_OMSB         (0xF6)
#define BMP180_REG_OUT          (BMP180_REG_OMSB)
#define BMP180_REG_CTRLMEAS     (0xF4)
#define BMP180_REG_SOFTRESET    (0xE0)
#define BMP180_REG_ID           (0xD0)
#define BMP180_REG_CALIB        (0xAA)

// shift for oversampling
#define BMP180_CTRLM_OSS_SHIFT  (6)
// start measurement
#define BMP180_CTRLM_SCO        (1<<5)
// write it to BMP180_REG_SOFTRESET for soft reset
#define BMP180_SOFTRESET_VAL    (0xB6)
// start measurement of T/P
#define BMP180_READ_T           (0x0E)
#define BMP180_READ_P           (0x14)

// delays in milliseconds
//#define BMP180_T_DELAY          (2)

static BMP180_oversampling bmp180_os = BMP180_OVERS_2;

static struct {
    int16_t     AC1;
    int16_t     AC2;
    int16_t     AC3;
    uint16_t    AC4;
    uint16_t    AC5;
    uint16_t    AC6;
    int16_t     B1;
    int16_t     B2;
    int16_t     MB;
    int16_t     MC;
    int16_t     MD;
} __attribute__ ((packed)) CaliData = {0};

static BMP180_status bmpstatus = BMP180_NOTINIT;
static uint8_t calidata_rdy = 0;
static uint32_t milliseconds_start = 0; // time of measurement start
//static uint32_t p_delay = 8; // delay for P measurement
static uint8_t uncomp_data[3]; // raw uncompensated data
static int32_t Tval; // uncompensated T value
// compensated values:
static uint32_t Pmeasured; // in Pa
static int32_t  Tmeasured; // /10degC
static uint8_t devID = 0;

BMP180_status BMP180_get_status(){
    return bmpstatus;
}

void BMP180_setOS(BMP180_oversampling os){
    bmp180_os = os & 0x03;
    /*
    switch(os){
        case BMP180_OVERS_1:
            p_delay = 5;
        break;
        case BMP180_OVERS_2:
            p_delay = 8;
        break;
        case BMP180_OVERS_4:
            p_delay = 14;
        break;
        default:
            p_delay = 26;
    }*/
}

static int read_reg8(uint8_t reg, uint8_t *val){
    if(I2C_OK != i2c_7bit_send_onebyte(reg, 0)) return 0;
    if(I2C_OK != i2c_7bit_receive_onebyte(val, 1)) return 0;
    return 1;
}

static int write_reg8(uint8_t reg, uint8_t val){
    uint8_t d[2] = {reg, val};
    if(I2C_OK != i2c_7bit_send(d, 2)) return 0;
    return 1;
}

// get compensation data, return 1 if OK
static int readcompdata(){
    if(I2C_OK != i2c_7bit_send_onebyte(BMP180_REG_CALIB, 0)) return 0;
    if(I2C_OK != i2c_7bit_receive((uint8_t*)&CaliData, sizeof(CaliData))) return 0;
    // convert big-endian into little-endian
    uint8_t *d, val;
    uint16_t *arr = (uint16_t*)&CaliData;
    for(int i = 0; i < (int)sizeof(CaliData)/2; ++i){
        d = (uint8_t*)(&arr[i]);
        val = d[0];
        d[0] = d[1];
        d[1] = val;
    }
    calidata_rdy = 1;
    return 1;
}

// do a soft-reset procedure
int BMP180_reset(){
    if(!write_reg8(BMP180_REG_SOFTRESET, BMP180_SOFTRESET_VAL)){
        DBG("Can't reset\n");
        return 0;
    }
    return 1;
}

// read compensation data & write registers
int BMP180_init(){
    bmpstatus = BMP180_NOTINIT;
    i2c_setup();
	i2c_set_addr7(BMP180_I2C_ADDRESS);
    if(!read_reg8(BMP180_REG_ID, &devID)) return 0;
    DBG("Got device ID: "); DBG(u2str(devID)); DBG("\n");
    if(devID != BMP180_CHIP_ID){
        DBG("Not BMP180\n");
        return 0;
    }
    if(!readcompdata()){
        DBG("Can't read calibration data\n");
    }else{
        DBG("AC1=");
        DBG(i2str(CaliData.AC1)); DBG(", AC2="); DBG(i2str(CaliData.AC2)); DBG(", AC3=");
        DBG(i2str(CaliData.AC3)); DBG(", AC4="); DBG(u2str(CaliData.AC4)); DBG(", AC5=");
        DBG(u2str(CaliData.AC5)); DBG(", AC6="); DBG(u2str(CaliData.AC6)); DBG(", B1=");
        DBG(i2str(CaliData.B1)); DBG(", B2="); DBG(i2str(CaliData.B2)); DBG(", MB=");
        DBG(i2str(CaliData.MB)); DBG(", MC="); DBG(i2str(CaliData.MC)); DBG(", MD=");
        DBG(i2str(CaliData.MD)); DBG("\n");
    }
    return 1;
}

// @return 1 if OK, *devid -> BMP/BME
void BMP180_read_ID(uint8_t *devid){
    *devid = devID;
}

// start measurement, @return 1 if all OK
int BMP180_start(uint32_t curr_milliseconds){
    if(!calidata_rdy || bmpstatus == BMP180_BUSYT || bmpstatus == BMP180_BUSYP) return 0;
    uint8_t reg = BMP180_READ_T | BMP180_CTRLM_SCO;
    if(!write_reg8(BMP180_REG_CTRLMEAS, reg)){
        DBG("Can't write CTRL reg\n");
        return 0;
    }
    bmpstatus = BMP180_BUSYT;
    milliseconds_start = curr_milliseconds;
    return 1;
}


// calculate T/10 degC and P in Pa
static inline void compens(uint32_t Pval){
    // T:
    int32_t X1 = ((Tval - CaliData.AC6)*CaliData.AC5) >> 15;
    int32_t X2 = (CaliData.MC << 11) / (X1 + CaliData.MD);
    int32_t B5 = X1 + X2;
    Tmeasured = (B5 + 8) >> 4;
    // P:
    int32_t B6 = B5 - 4000;
    X1 = (CaliData.B2 * ((B6*B6) >> 12)) >> 11;
    X2 = (CaliData.AC2 * B6) >> 11;
    int32_t X3 = X1 + X2;
    int32_t B3 = ((((CaliData.AC1 << 2) + X3) << bmp180_os) + 2) >> 2;
    X1 = (CaliData.AC3 * B6) >> 13;
    X2 = (CaliData.B1 * ((B6 * B6) >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;
    uint32_t B4 = (CaliData.AC4 * (uint32_t) (X3 + 32768)) >> 15;
    uint32_t B7 = (uint32_t)(Pval - B3) * (50000 >> bmp180_os);
    if(B7 < 0x80000000){
        Pmeasured = (B7 << 1) / B4;
    }else{
        Pmeasured = (B7 / B4) << 1;
    }
    X1 = Pmeasured >> 8;
    X1 *= X1;
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * Pmeasured) >> 16;
    Pmeasured += (X1 + X2 + 3791) >> 4;
}

static int still_measuring(){
    uint8_t reg;
    if(!read_reg8(BMP180_REG_CTRLMEAS, &reg)) return 1;
    if(reg & BMP180_CTRLM_SCO){
        DBG("Still measure\n");
        return 1;
    }
    return 0;
}

void BMP180_process(uint32_t curr_milliseconds){
    uint8_t reg;
    if(bmpstatus != BMP180_BUSYT && bmpstatus != BMP180_BUSYP) return;
    if(bmpstatus == BMP180_BUSYT){ // wait for temperature
        //if(curr_milliseconds - milliseconds_start < BMP180_T_DELAY) return;
        if(still_measuring()) return;
        // get uncompensated data
        DBG("Read uncompensated T\n");
        if(I2C_OK != i2c_7bit_send_onebyte(BMP180_REG_OUT, 0)){
            bmpstatus = BMP180_ERR;
            return;
        }
        if(I2C_OK != i2c_7bit_receive(uncomp_data, 2)){
            bmpstatus = BMP180_ERR;
            return;
        }
        Tval = uncomp_data[0] << 8 | uncomp_data[1];
        DBG("Start P measuring\n");
        reg = BMP180_READ_P | BMP180_CTRLM_SCO | (bmp180_os << BMP180_CTRLM_OSS_SHIFT);
        if(!write_reg8(BMP180_REG_CTRLMEAS, reg)){
            bmpstatus = BMP180_ERR;
            return;
        }
        milliseconds_start = curr_milliseconds;
        bmpstatus = BMP180_BUSYP;
    }else{ // wait for pressure
        //if(curr_milliseconds - milliseconds_start < p_delay) return;
        if(still_measuring()) return;
        DBG("Read uncompensated P\n");
        if(I2C_OK != i2c_7bit_send_onebyte(BMP180_REG_OUT, 0)){
            bmpstatus = BMP180_ERR;
            return;
        }
        if(I2C_OK != i2c_7bit_receive(uncomp_data, 3)){
            bmpstatus = BMP180_ERR;
            return;
        }
        uint32_t Pval = uncomp_data[0] << 16 | uncomp_data[1] << 8 | uncomp_data[2];
        Pval >>= (8 - bmp180_os);
        // calculate compensated values
        compens(Pval);
        DBG("All data ready\n");
        bmpstatus = BMP180_RDY; // data ready
    }
}

// read data & convert it
void BMP180_getdata(int32_t *T, uint32_t *P){
    *T = Tmeasured;
    *P = Pmeasured;
    bmpstatus = BMP180_RELAX;
}
