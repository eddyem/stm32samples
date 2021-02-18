/**
 * Ciastkolog.pl (https://github.com/ciastkolog)
 *
*/
/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 sheinz (https://github.com/sheinz)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*
 * This file is part of the BMP280 project.
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
#include "BMP280.h"

//#define EBUG

#ifdef EBUG
#include "usb.h"
#include "proto.h"
#define DBG(x)  do{USB_send(x);}while(0)
#else
#define DBG(x)
#endif

#define BMP280_I2C_ADDRESS_MASK 0x76
#define BMP280_I2C_ADDRESS_0  0x76
#define BMP280_I2C_ADDRESS_1  0x77
/**
 * BMP280 registers
 */
#define BMP280_REG_HUM_LSB      0xFE
#define BMP280_REG_HUM_MSB      0xFD
#define BMP280_REG_HUM          (BMP280_REG_HUM_MSB)
#define BMP280_REG_TEMP_XLSB    0xFC /* bits: 7-4 */
#define BMP280_REG_TEMP_LSB     0xFB
#define BMP280_REG_TEMP_MSB     0xFA
#define BMP280_REG_TEMP         (BMP280_REG_TEMP_MSB)
#define BMP280_REG_PRESS_XLSB   0xF9 /* bits: 7-4 */
#define BMP280_REG_PRESS_LSB    0xF8
#define BMP280_REG_PRESS_MSB    0xF7
#define BMP280_REG_PRESSURE     (BMP280_REG_PRESS_MSB)
#define BMP280_REG_ALLDATA      (BMP280_REG_PRESS_MSB) // all data: P, T & H
#define BMP280_REG_CONFIG       0xF5 /* bits: 7-5 t_sb; 4-2 filter; 0 spi3w_en */
#define BMP280_REG_CTRL         0xF4 /* bits: 7-5 osrs_t; 4-2 osrs_p; 1-0 mode */
#define BMP280_REG_STATUS       0xF3 /* bits: 3 measuring; 0 im_update */
#define BMP280_REG_CTRL_HUM     0xF2 /* bits: 2-0 osrs_h; */
#define BMP280_REG_RESET        0xE0
    #define BMP280_RESET_VALUE     0xB6
#define BMP280_REG_ID           0xD0

#define BMP280_REG_CALIBA       0x88
#define BMP280_CALIBA_SIZE      (26)  // 26 bytes of calibration registers sequence from 0x88 to 0xa1
#define BMP280_CALIBB_SIZE      (7)   // 7 bytes of calibration registers sequence from 0xe1 to 0xe7
#define BMP280_REG_CALIBB       0xE1

#define BMP280_MODE_FORSED      (1)  // force single measurement
#define BMP280_MODE_NORMAL      (3)  // run continuosly
#define BMP280_STATUS_MSRNG     (1<<3) // measuring in process

static uint8_t curaddress = BMP280_I2C_ADDRESS_0;

static struct {
    // temperature
    uint16_t dig_T1;    // 0x88 (LSB), 0x98 (MSB)
    int16_t  dig_T2;    // ...
    int16_t  dig_T3;
    // pressure
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;    // 0x9e, 0x9f
    // humidity (partially calculated from EEE struct)
    uint8_t unused;     // 0xA0
    uint8_t dig_H1;     // 0xA1
    int16_t dig_H2;     // --------------------
    uint8_t dig_H3;     // only from EEE
    uint16_t dig_H4;
    uint16_t dig_H5;
    int8_t dig_H6;
    // data is ready
    uint8_t  rdy;
} __attribute__ ((packed)) CaliData = {0};

//T: 28222 26310 50
//P: 37780 -10748 3024 7965 -43 -7 9900 -10230 4285
//H: 75 25601 0 334 50 30

// data for humidity calibration of BME280
static uint8_t EEE[BMP280_CALIBB_SIZE] = {0};

static struct{
    BMP280_Filter filter;       // filtering
    BMP280_Oversampling p_os;   // oversampling for pressure
    BMP280_Oversampling t_os;   // -//- temperature
    BMP280_Oversampling h_os;   // -//- humidity
    uint8_t ID;                 // identificator
    uint8_t regctl;             // control register base value [(params.t_os << 5) | (params.p_os << 2)]
} params = {
    .filter = BMP280_FILTER_OFF,
    .p_os   = BMP280_OVERS16,
    .t_os   = BMP280_OVERS16,
    .h_os   = BMP280_OVERS16,
    .ID     = 0
};

static BMP280_status bmpstatus = BMP280_NOTINIT;

BMP280_status BMP280_get_status(){
    return bmpstatus;
}

// address: 0 or 1
void BMP280_setup(uint8_t address){
    curaddress = BMP280_I2C_ADDRESS_MASK | (address & 1);
    bmpstatus = BMP280_NOTINIT;
}

// setters for `params`
void BMP280_setfilter(BMP280_Filter f){
    params.filter = f;
}
void BMP280_setOSt(BMP280_Oversampling os){
    params.t_os = os;
}
void BMP280_setOSp(BMP280_Oversampling os){
    params.p_os = os;
}
void BMP280_setOSh(BMP280_Oversampling os){
    params.h_os = os;
}


/*
// read register, @return 1 if all OK
static int read_reg16(uint8_t reg, uint16_t *val){
    if(I2C_OK != i2c_7bit_send_onebyte(reg, 0)) return 0;
    if(I2C_OK != i2c_7bit_receive_twobytes((uint8_t*)val)) return 0;
    return 1;
}*/

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
    if(I2C_OK != i2c_7bit_send_onebyte(BMP280_REG_CALIBA, 0)) return 0;
    if(I2C_OK != i2c_7bit_receive((uint8_t*)&CaliData, BMP280_CALIBA_SIZE)) return 0;
    CaliData.rdy = 1;
    if(params.ID == BME280_CHIP_ID){
        if(I2C_OK == i2c_7bit_send_onebyte(BMP280_REG_CALIBB, 0) &&
            I2C_OK == i2c_7bit_receive(EEE, BMP280_CALIBB_SIZE)){
            CaliData.dig_H2 = (EEE[1] << 8) | EEE[0];
            CaliData.dig_H3 = EEE[2];
            CaliData.dig_H4 = (EEE[3] << 4) | (EEE[4] & 0x0f);
            CaliData.dig_H5 = (EEE[5] << 4) | (EEE[4] >> 4);
            CaliData.dig_H6 = EEE[6];
        }
    }
    return 1;
}

// read compensation data & write registers
int BMP280_init(){
    i2c_setup();
	i2c_set_addr7(curaddress);
    if(!read_reg8(BMP280_REG_ID, &params.ID)) return 0;
    DBG("Got device ID: "); DBG(u2str(params.ID)); DBG("\n");
    if(params.ID != BMP280_CHIP_ID && params.ID != BME280_CHIP_ID){
        DBG("Not BMP/BME\n");
        return 0;
    }
    if(!write_reg8(BMP280_REG_RESET, BMP280_RESET_VALUE)){
        DBG("Can't reset\n");
        return 0;
    }
    uint8_t reg = 1;
    while(reg & 1){
        if(!read_reg8(BMP280_REG_STATUS, &reg)) return 0;
    }
    if(!readcompdata()){
        DBG("Can't read calibration data\n");
    }else{
        DBG("T: "); DBG(u2str(CaliData.dig_T1)); DBG(" "); DBG(i2str(CaliData.dig_T2));
        DBG(" "); DBG(i2str(CaliData.dig_T3));
        DBG("\nP: "); DBG(u2str(CaliData.dig_P1)); DBG(" ");
        DBG(i2str(CaliData.dig_P2)); DBG(" "); DBG(i2str(CaliData.dig_P3)); DBG(" ");
        DBG(i2str(CaliData.dig_P4)); DBG(" "); DBG(i2str(CaliData.dig_P5)); DBG(" ");
        DBG(i2str(CaliData.dig_P6)); DBG(" "); DBG(i2str(CaliData.dig_P7)); DBG(" ");
        DBG(i2str(CaliData.dig_P8)); DBG(" "); DBG(i2str(CaliData.dig_P9));
        DBG("\nH: "); DBG(u2str(CaliData.dig_H1)); DBG(" ");
        if(params.ID == BME280_CHIP_ID){ // read H compensation
            DBG(i2str(CaliData.dig_H2)); DBG(" "); DBG(u2str(CaliData.dig_H3)); DBG(" ");
            DBG(i2str(CaliData.dig_H4)); DBG(" "); DBG(i2str(CaliData.dig_H5)); DBG(" ");
            DBG(i2str(CaliData.dig_H6));
        }else{DBG("not BME!");}
        DBG("\n");
    }
    // write filter configuration
    reg = params.filter << 2;
    if(!write_reg8(BMP280_REG_CONFIG, reg)){DBG("Can't save filter settings\n");}
    reg = (params.t_os << 5) | (params.p_os << 2); // oversampling for P/T, sleep mode
    if(!write_reg8(BMP280_REG_CTRL, reg)){
        DBG("Can't write settings for P/T\n");
        return 0;
    }
    params.regctl = reg;
    if(params.ID == BME280_CHIP_ID){ // write CTRL_HUM only AFTER CTRL!
        reg = params.h_os;
        if(!write_reg8(BMP280_REG_CTRL_HUM, reg)){
            DBG("Can't write settings for H\n");
            return 0;
        }
    }
    return 1;
}

// @return 1 if OK, *devid -> BMP/BME
int BMP280_read_ID(uint8_t *devid){
    if(params.ID != BMP280_CHIP_ID && params.ID != BME280_CHIP_ID) return 0;
    *devid = params.ID;
    return 1;
}

// start measurement, @return 1 if all OK
int BMP280_start(){
    if(!CaliData.rdy || bmpstatus == BMP280_BUSY) return 0;
    uint8_t reg = params.regctl | BMP280_MODE_FORSED;
    if(!write_reg8(BMP280_REG_CTRL, reg)){
        DBG("Can't write CTRL reg\n");
        return 0;
    }
    bmpstatus = BMP280_BUSY;
    return 1;
}

void BMP280_process(){
    if(bmpstatus != BMP280_BUSY) return;
    // BUSY state: poll data ready
    uint8_t reg;
    if(!read_reg8(BMP280_REG_STATUS, &reg)) return;
    if(reg & BMP280_STATUS_MSRNG) return; // still busy
    bmpstatus = BMP280_RDY; // data ready
}

// return T*100 degC
static inline int32_t compTemp(int32_t adc_temp, int32_t *t_fine){
	int32_t var1, var2;
	var1 = ((((adc_temp >> 3) - ((int32_t) CaliData.dig_T1 << 1)))
			* (int32_t) CaliData.dig_T2) >> 11;
	var2 = (((((adc_temp >> 4) - (int32_t) CaliData.dig_T1)
			* ((adc_temp >> 4) - (int32_t) CaliData.dig_T1)) >> 12)
			* (int32_t) CaliData.dig_T3) >> 14;
	*t_fine = var1 + var2;
	return (*t_fine * 5 + 128) >> 8;
}

// return p*256 hPa
static inline uint32_t compPres(int32_t adc_press, int32_t fine_temp) {
	int64_t var1, var2, p;
	var1 = (int64_t) fine_temp - 128000;
	var2 = var1 * var1 * (int64_t) CaliData.dig_P6;
	var2 = var2 + ((var1 * (int64_t) CaliData.dig_P5) << 17);
	var2 = var2 + (((int64_t) CaliData.dig_P4) << 35);
	var1 = ((var1 * var1 * (int64_t) CaliData.dig_P3) >> 8)
			+ ((var1 * (int64_t) CaliData.dig_P2) << 12);
	var1 = (((int64_t) 1 << 47) + var1) * ((int64_t) CaliData.dig_P1) >> 33;
	if (var1 == 0){
		return 0;  // avoid exception caused by division by zero
	}
	p = 1048576 - adc_press;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = ((int64_t) CaliData.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
	var2 = ((int64_t) CaliData.dig_P8 * p) >> 19;
	p = ((p + var1 + var2) >> 8) + ((int64_t) CaliData.dig_P7 << 4);
	return p;
}

// return H*1024 %
static inline uint32_t compHum(int32_t adc_hum, int32_t fine_temp){
	int32_t v_x1_u32r;
	v_x1_u32r = fine_temp - (int32_t) 76800;
	v_x1_u32r = ((((adc_hum << 14) - (((int32_t)CaliData.dig_H4) << 20)
			- (((int32_t)CaliData.dig_H5) * v_x1_u32r)) + (int32_t)16384) >> 15)
			* (((((((v_x1_u32r * ((int32_t)CaliData.dig_H6)) >> 10)
					* (((v_x1_u32r * ((int32_t)CaliData.dig_H3)) >> 11)
							+ (int32_t)32768)) >> 10) + (int32_t)2097152)
					* ((int32_t)CaliData.dig_H2) + 8192) >> 14);
    DBG("Step1: "); DBG(i2str(v_x1_u32r)); DBG(".. ");
	v_x1_u32r = v_x1_u32r
			- (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7)
					* ((int32_t)CaliData.dig_H1)) >> 4);
    DBG("Step2: "); DBG(i2str(v_x1_u32r)); DBG(".. ");
	v_x1_u32r = v_x1_u32r < 0 ? 0 : v_x1_u32r;
	v_x1_u32r = v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r;
    DBG("Step3: "); DBG(i2str(v_x1_u32r)); DBG("\n");
	return v_x1_u32r >> 12;
}


// read data & convert it
int BMP280_getdata(int32_t *T, uint32_t *P, uint32_t *H){
    if(bmpstatus != BMP280_RDY) return 0;
    bmpstatus = BMP280_RELAX;
    uint8_t datasz = 8; // amount of bytes to read
    if(params.ID != BME280_CHIP_ID){
        DBG("Not BME!\n");
        if(H) *H = 0;
        H = NULL;
        datasz = 6;
    }
    uint8_t data[8];
    if(I2C_OK != i2c_7bit_send_onebyte(BMP280_REG_ALLDATA, 0)) return 0;
    if(I2C_OK != i2c_7bit_receive(data, datasz)) return 0;
    int32_t p = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    DBG("puncomp = "); DBG(i2str(p)); DBG("\n");
    int32_t t = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
    DBG("tuncomp = "); DBG(i2str(t)); DBG("\n");
    int32_t t_fine;
    int32_t Temp = compTemp(t, &t_fine);
    DBG("tfine = "); DBG(i2str(t_fine)); DBG("\n");
    if(T) *T = Temp;
    if(P){
        float fp = compPres(p, t_fine) / 256.;
        *P = fp;// * 100.;
    }
    if(H){
        int32_t h = (data[6] << 8) | data[7];
        DBG("huncomp = "); DBG(i2str(h)); DBG("\n");
        float fh = compHum(h, t_fine)/1024.;
        *H = fh * 100.;
    }
    return 1;
}

#if 0
/*
 * start themperature reading @return 0 if all OK
 */
int BMP280_cmdT(){
	const uint8_t cmd[2] = {0x03, 0x11};
	if(state != RELAX){
		return 1;
	}
    bmpstatus = BMP280_BUSY;
	i2c_status st = i2c_7bit_send(cmd, 2);
	if(st != I2C_OK){
        bmpstatus = BMP280_ERR;
		return 1;
	}
    DBG("Wait for T\n");
	state = WAITFORT;
    return 0;
}

/*
 * start humidity reading @return 0 if all OK
 */
int BMP280_cmdH(){
	const uint8_t cmd[2] = {0x03, 0x01};
	if(state != RELAX){
		return 1;
	}
    bmpstatus = BMP280_BUSY;
	i2c_status st = i2c_7bit_send(cmd, 2);
	if(st != I2C_OK){
        bmpstatus = BMP280_ERR;
		return 1;
	}
	state = WAITFORH;
    DBG("Wait for H\n");
    return 0;
}

int32_t BMP280_getT(){ // T*10
    if(bmpstatus != BMP280_TRDY) return -5000;
    DBG("TH="); DBG(u2str(TH)); DBG("\n");
    TH >>= 2;
    uint32_t d = (TH*10)/32 - 500;
    bmpstatus = BMP280_RELAX;
    return d;
}
uint32_t BMP280_getH(){ // hum * 10
    if(bmpstatus != BMP280_HRDY) return 5000;
    TH >>= 4;
    uint32_t d = (TH*10)/16 - 240;
    bmpstatus = BMP280_RELAX;
    return d;
}

/*
 * process state machine
 */
void BMP280_process(){
	uint8_t b, d[2];
	i2c_status st;
	if(state == RELAX) return;
	if(state == WAITFORH || state == WAITFORT){ // poll RDY
        DBG("Poll\n");
		if((st = i2c_7bit_send_onebyte(0, 0)) == I2C_OK){
            DBG("0 sent\n");
			if(i2c_7bit_receive_onebyte(&b, 1) == I2C_OK){
                DBG("received: "); DBG(u2str(b)); DBG("\n");
				if(b) return; // !RDY
                if((st = i2c_7bit_send_onebyte(1, 0)) == I2C_OK){
                    DBG("sent 1\n");
                    if((st = i2c_7bit_receive_twobytes(d)) == I2C_OK){
                        DBG("got data: ");
                        DBG(u2str(d[0])); DBG(" "); DBG(u2str(d[1])); DBG("\n");
                        bmpstatus = (state == WAITFORH) ? BMP280_HRDY : BMP280_TRDY;
                        TH = (d[0]<<8) | d[1];
                    }
                }
				state = RELAX;
				if(st != I2C_OK){
					bmpstatus = BMP280_ERR;
				}
			}
		}else{
			state = RELAX;
            bmpstatus = BMP280_ERR;
		}
	}
}
#endif
