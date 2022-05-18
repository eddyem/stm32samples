/*
 * This file is part of the MLX90640 project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "hardware.h"
#include "i2c.h"
#include "mlx90640.h"
#include "mlx90640_regs.h"
#include "strfunct.h"

#ifdef EBUG
extern volatile uint32_t Tms;
#endif

mlx90640_state mlx_state = M_ERROR;

MLX90640_params params;

#if REG_CALIBRDATA_LEN > MLX_DMA_MAXLEN || MLX_PIXARRSZ > MLX_DMA_MAXLEN
#error "MLX_DMA_MAXLEN should be >= REG_CALIBRDATA_LEN"
#endif
static uint16_t dataarray[MLX_DMA_MAXLEN]; // array for raw data from sensor
static int portionlen = 0; // data length in `dataarray`
float mlx_image[MLX_PIXNO]; // ready image

#define CREG_VAL(reg) dataarray[CREG_IDX(reg)]
#define IMD_VAL(reg) dataarray[IMD_IDX(reg)]

static uint8_t simpleimage = 0; // ==1 not to calibrate T
static uint8_t subpageno = 0; // subpage number

// reg_control values for subpage #0 and #1
static const uint16_t reg_control_val[2] = {
    REG_CONTROL_CHESS | REG_CONTROL_RES18 | REG_CONTROL_REFR_2HZ | REG_CONTROL_SUBPSEL | REG_CONTROL_DATAHOLD | REG_CONTROL_SUBPEN,
    REG_CONTROL_CHESS | REG_CONTROL_RES18 | REG_CONTROL_REFR_2HZ | REG_CONTROL_SUBP1 | REG_CONTROL_SUBPSEL | REG_CONTROL_DATAHOLD | REG_CONTROL_SUBPEN
};

// read register value
int read_reg(uint16_t reg, uint16_t *val){
    reg = __REV16(reg);
    if(I2C_OK != i2c_7bit_send((uint8_t*)&reg, 2, 0)){
        DBG("Can't send address");
        return FALSE;
    }
    uint16_t d;
    i2c_status s = i2c_7bit_receive_twobytes((uint8_t*)&d);
    if(I2C_OK != s){
#ifdef EBUG
        DBG("Can't get info, s=");
        printu(s); NL();
#endif
        return FALSE;
    }
    *val = __REV16(d);
    return TRUE;
}

// blocking read N uint16_t values starting from `reg`
// @return N of registers read
int read_data(uint16_t reg, uint16_t *data, int N){
    if(N < 1 ) return 0;
    int i;
    for(i = 0; i < N; ++i){
        if(!read_reg(reg+i, data++)) break;
    }
    return i;
}

// write register value
int write_reg(uint16_t reg, uint16_t val){
    // little endian -> big endian
    uint8_t _4bytes[4];
    _4bytes[0] = reg >> 8;
    _4bytes[1] = reg & 0xff;
    _4bytes[2] = val >> 8;
    _4bytes[3] = val & 0xff;
    if(I2C_OK != i2c_7bit_send(_4bytes, 4, 1)) return FALSE;
    return TRUE;
}

/**
 * @brief read_data_dma - read big data buffer by DMA
 * @param reg - starting register number
 * @param N   - amount of data (in 16-bit words)
 * @return FALSE if can't run operation
 */
int read_data_dma(uint16_t reg, int N){
    if(N < 1 || N > MLX_DMA_MAXLEN) return FALSE;
    /*uint8_t _2bytes[2];
    _2bytes[0] = reg >> 8; // big endian!
    _2bytes[1] = reg & 0xff;*/
    reg = __REV16(reg);
    portionlen = N;
    if(I2C_OK != i2c_7bit_send((uint8_t*)&reg, 2, 0)){
        DBG("DMA: can't send address");
        return FALSE;
    }
    if(I2C_OK != i2c_7bit_receive_DMA((uint8_t*)dataarray, N*2)) return FALSE;
    return TRUE;
}

/*****************************************************************************
                Calculate parameters & values
 *****************************************************************************/
// calculate Vdd from vddRAM register
/*
static float getVdd(uint16_t vddRAM){
    int16_t ram = (int16_t) vddRAM;
    float vdd = (float)ram - params.vdd25;
    return vdd / params.kVdd + 3.3f;
}*/

// fill OCC/ACC row/col arrays
static void occacc(int8_t *arr, int l, uint16_t *regstart){
    int n = l >> 2; // divide by 4
    int8_t *p = arr;
    for(int i = 0; i < n; ++i){
        register uint16_t val = *regstart++;
        *p++ = (val & 0x000F) >> 0;
        *p++ = (val & 0x00F0) >> 4;
        *p++ = (val & 0x0F00) >> 8;
        *p++ = (val         ) >> 12;
    }
    for(int i = 0; i < l; ++i, ++arr){
        if(*arr > 0x07) *arr -= 0x10;
    }
}

// get all parameters' values from `dataarray`, return FALSE if something failed
static int get_parameters(){
#ifdef EBUG
    SEND("0 Tms="); printu(Tms); newline();
#endif
    int8_t i8;
    int16_t i16, *pi16;
    uint16_t *pu16;
    uint16_t val = CREG_VAL(REG_VDD);
    i8 = (int8_t) (val >> 8);
    params.kVdd = i8 << 5;
    if(params.kVdd == 0) return FALSE;
    i16 = val & 0xFF;
    params.vdd25 = ((i16 - 0x100) << 5) - (1<<13);
    val = CREG_VAL(REG_KVTPTAT);
    i16 = (val & 0xFC00) >> 10;
    if(i16 > 0x1F) i16 -= 0x40;
    params.KvPTAT = (float)i16 / (1<<12);
    i16 = (val & 0x03FF);
    if(i16 > 0x1FF) i16 -= 0x400;
    params.KtPTAT = (float)i16 / 8.f;
    params.vPTAT25 = (int16_t) CREG_VAL(REG_PTAT);
    val = CREG_VAL(REG_APTATOCCS) >> 12;
    params.alphaPTAT = val / 4.f + 8.f;
    params.gainEE = (int16_t)CREG_VAL(REG_GAIN);
    if(params.gainEE == 0) return FALSE;
#ifdef EBUG
    SEND("1 Tms="); printu(Tms); newline();
#endif
    int8_t occRow[24];
    int8_t occColumn[32];
    occacc(occRow, 24, &CREG_VAL(REG_OCCROW14));
    occacc(occColumn, 32, &CREG_VAL(REG_OCCCOL14));
    int8_t accRow[24];
    int8_t accColumn[32];
    occacc(accRow, 24, &CREG_VAL(REG_ACCROW14));
    occacc(accColumn, 32, &CREG_VAL(REG_ACCCOL14));
    val = CREG_VAL(REG_APTATOCCS);
    // need to do multiplication instead of bitshift, so:
    float occRemScale = 1<<(val&0x0F),
          occColumnScale = 1<<((val>>4)&0x0F),
          occRowScale = 1<<((val>>8)&0x0F);
    int16_t offavg = (int16_t) CREG_VAL(REG_OSAVG);
    // even/odd column/row numbers are for starting from 1, so for starting from 0 we chould swap them:
    // even - for 1,3,5,...; odd - for 0,2,4,... etc
    int8_t ktaavg[4];
    // 0 - odd row, odd col; 1 - odd row even col; 2 - even row, odd col; 3 - even row, even col
    val = CREG_VAL(REG_KTAAVGODDCOL);
    ktaavg[2] = (int8_t)(val & 0xFF); // odd col, even row -> col 0,2,..; row 1,3,..
    ktaavg[0] = (int8_t)(val >> 8);; // odd col, odd row -> col 0,2,..; row 0,2,..
    val = CREG_VAL(REG_KTAAVGEVENCOL);
    ktaavg[3] = (int8_t)(val & 0xFF); // even col, even row -> col 1,3,..; row 1,3,..
    ktaavg[1] = (int8_t)(val >> 8); // even col, odd row -> col 1,3,..; row 0,2,..
    // so index of ktaavg is 2*(row&1)+(col&1)
    val = CREG_VAL(REG_KTAVSCALE);
    uint8_t scale1 = ((val & 0xFF)>>4) + 8, scale2 = (val&0xF);
    if(scale1 == 0 || scale2 == 0) return FALSE;
    float mul = (float)(1<<scale2), div = (float)(1<<scale1); // kta_scales
    uint16_t a_r = CREG_VAL(REG_SENSIVITY); // alpha_ref
    val = CREG_VAL(REG_SCALEACC);
    float *a = params.alpha, diva = (float)(val >> 12);
    diva *= (float)(1<<30); // alpha_scale
    float accRowScale = 1<<((val & 0x0f00)>>8),
          accColumnScale = 1<<((val & 0x00f0)>>4),
          accRemScale = 1<<(val & 0x0f);
    pi16 = params.offset;
    pu16 = &CREG_VAL(REG_OFFAK1);
    float *fp = params.kta;
#ifdef EBUG
    SEND("2 Tms="); printu(Tms); newline();
#endif
    for(int row = 0; row < 24; ++row){
        int idx = (row&1)<<1;
        for(int col = 0; col < 32; ++col){
            // offset
            register uint16_t rv = *pu16++;
            i16 = (rv & 0xFC00) >> 10;
            if(i16 > 0x1F) i16 -= 0x40;
            register float oft = (float)offavg + occRow[row]*occRowScale + occColumn[col]*occColumnScale + i16*occRemScale;
            *pi16++ = (int16_t)oft;
            // kta
            i16 = (rv & 0xF) >> 1;
            if(i16  > 0x03) i16 -= 0x08;
            *fp++ = (ktaavg[idx|(col&1)] + i16*mul) / div;
            // alpha
            i16 = (rv & 0x3F0) >> 4;
            if(i16 > 0x1F) i16 -= 0x40;
            oft = (float)a_r + accRow[row]*accRowScale + accColumn[col]*accColumnScale +i16*accRemScale;
            *a++ = oft / diva;
            //*a++ /= diva;
        }
    }
#ifdef EBUG
    SEND("3 Tms="); printu(Tms); newline();
#endif
    scale1 = (CREG_VAL(REG_KTAVSCALE) >> 8) & 0xF; // kvscale
    div = (float)(1<<scale1);
    val = CREG_VAL(REG_KVAVG);
    i16 = val >> 12; if(i16 > 0x07) i16 -= 0x10;
    ktaavg[0] = i16; // odd col, odd row
    i16 = (val & 0xF0) >> 4; if(i16 > 0x07) i16 -= 0x10;
    ktaavg[1] = i16; // even col, odd row
    i16 = (val & 0x0F00) >> 8; if(i16 > 0x07) i16 -= 0x10;
    ktaavg[2] = i16; // odd col, even row
    i16 = val & 0x0F; if(i16 > 0x07) i16 -= 0x10;
    ktaavg[3] = i16; // even col, even row
    for(int i = 0; i < 4; ++i) params.kv[i] = ktaavg[i] / div;
    val = CREG_VAL(REG_CPOFF);
    params.cpOffset[0] = (val & 0x03ff);
    if(params.cpOffset[0] > 0x1ff) params.cpOffset[0] -= 0x400;
    params.cpOffset[1] = val >> 10;
    if(params.cpOffset[1] > 0x1f) params.cpOffset[1] -= 0x40;
    params.cpOffset[1] += params.cpOffset[0];
    val = ((CREG_VAL(REG_KTAVSCALE) & 0xF0) >> 4) + 8;
    i8 = (int8_t)(CREG_VAL(REG_KVTACP) & 0xFF);
    params.cpKta = (float)i8 / (1<<val);
    val = (CREG_VAL(REG_KTAVSCALE) & 0x0F00) >> 8;
    i16 = CREG_VAL(REG_KVTACP) >> 8;
    if(i16 > 0x7F) i16 -= 0x100;
    params.cpKv = (float)i16 / (1<<val);
    i16 = CREG_VAL(REG_KSTATGC) & 0xFF;
    if(i16 > 0x7F) i16 -= 0x100;
    params.tgc = (float)i16;
    params.tgc /= 32.;
#ifdef EBUG
    SEND("4 Tms="); printu(Tms); newline();
#endif
    val = (CREG_VAL(REG_SCALEACC)>>12); // alpha_scale_CP
    i16 = CREG_VAL(REG_ALPHA)>>10; // cp_P1_P0_ratio
    if(i16 > 0x1F) i16 -= 0x40;
    div = (float)(1<<val);
    div *= (float)(1<<27);
    params.cpAlpha[0] = (float)(CREG_VAL(REG_ALPHA) & 0x03FF) / div;
    div = (float)(1<<7);
    params.cpAlpha[1] = params.cpAlpha[0] * (1.f + (float)i16/div);
    i8 = (int8_t)(CREG_VAL(REG_KSTATGC) >> 8);
    params.KsTa = (float)i8/(1<<13);
    div = 1<<((CREG_VAL(REG_CT34) & 0x0F) + 8); // kstoscale
    val = CREG_VAL(REG_KSTO12);
    i8 = (int8_t)(val & 0xFF);
    params.ksTo[0] = (float)i8 / div;
    i8 = (int8_t)(val >> 8);
    params.ksTo[1] = (float)i8 / div;
    val = CREG_VAL(REG_KSTO34);
    i8 = (int8_t)(val & 0xFF);
    params.ksTo[2] = (float)i8 / div;
    i8 = (int8_t)(val >> 8);
    params.ksTo[3] = (float)i8 / div;
    params.CT[0] = 0.f; // 0degr - between ranges 1 and 2
    val = CREG_VAL(REG_CT34);
    mul = ((val & 0x3000)>>12)*10.f; // step
    params.CT[1] = ((val & 0xF0)>>4)*mul; // CT3 - between ranges 2 and 3
    params.CT[2] = ((val & 0x0F00) >> 8)*mul + params.CT[1]; // CT4 - between ranges 3 and 4
    params.alphacorr[0] = 1.f/(1.f + params.ksTo[0] * 40.f);
    params.alphacorr[1] = 1.f;
    params.alphacorr[2] = (1.f + params.ksTo[2] * params.CT[1]);
    params.alphacorr[3] = (1.f + params.ksTo[3] * (params.CT[2] - params.CT[1])) * params.alphacorr[2];
    // Don't forget to check 'outlier' flags for wide purpose
#ifdef EBUG
    SEND("end Tms="); printu(Tms);
    NL();
#endif
    return TRUE;
}


// calculate Vsup, Tamb, gain, off, Vdd, Ta
static void stage1(){
    int16_t i16a = (int16_t)IMD_VAL(REG_IVDDPIX);
    float dvdd = i16a - params.vdd25;
    dvdd = dvdd / params.kVdd;
    float vdd = dvdd + 3.3f;
    SEND("Vd="); float2str(vdd, 2); newline();
    i16a = (int16_t)IMD_VAL(REG_ITAPTAT);
    int16_t i16b = (int16_t)IMD_VAL(REG_ITAVBE);
    float Ta = (float)i16a / (i16a * params.alphaPTAT + i16b); // vptatart
    Ta *= (float)(1<<18);
    Ta = (Ta / (1 + params.KvPTAT*dvdd) - params.vPTAT25);
    Ta = Ta / params.KtPTAT + 25.;
    SEND("Ta="); float2str(Ta, 2); newline();
    i16a = (int16_t)IMD_VAL(REG_IGAIN);
    float Kgain = params.gainEE / (float)i16a;
    SEND("Kgain="); float2str(Kgain, 2); newline();
    ;
    //int idx = (row&1)<<1;
    //for(int col = 0; col < 32; ++col){
    //    *fp++ = (ktaavg[idx|(col&1)]
    // pix_gain = pix*Kgain
    // pix_os = pix_gain - offset*(1+kta*(Ta-Ta0))*(1+kv*(vdd-vdd0))
}

/**
 * @brief process_subpage - calculate all parameters from `dataarray` into `mlx_image`
 */
static void process_subpage(){
    DBG("process_subpage()");
    SEND("subpage="); printu(subpageno); newline();
    (void)subpageno; (void)simpleimage;
for(int i = 0; i < 32; ++i){
    printi((int8_t)dataarray[i]); bufputchar(' ');
} newline();
    stage1();
    NL();
}

// start image acquiring for next subpage
static int startima(){
    DBG("startima()");
    if(!write_reg(REG_CONTROL, reg_control_val[subpageno]) ||
            !write_reg(REG_STATUS, REG_STATUS_OVWEN)) return FALSE;
    return TRUE;
}

/**
 * @brief parse_buffer - swap bytes in `dataarray` (after receiving or before transmitting data)
 */
static void parse_buffer(){
    uint16_t *ptr = dataarray;
    DBG("parse_buffer()");
    for(uint16_t i = 0; i < portionlen; ++i, ++ptr){
        *ptr = __REV16(*ptr);
#if 0
        printu(i);
        addtobuf(" ");
        printuhex(*ptr);
        newline();
#endif
    }
#if 0
    sendbuf();
#endif
}

/**
 * @brief mlx90640_process - main finite-state machine
 */
void mlx90640_process(){
#define chstate(s) do{errctr = 0; Tlast = Tms; mlx_state = s;}while(0)
#define chkerr()   do{if(++errctr > MLX_MAXERR_COUNT){chstate(M_ERROR); DBG("-> M_ERROR");}}while(0)
#define chktmout() do{if(Tms - Tlast > MLX_TIMEOUT){chstate(M_ERROR); DBG("Timeout! -> M_ERROR"); }}while(0)
    static int errctr = 0;
    static uint32_t Tlast = 0;
    uint8_t gotdata = 0;
    uint16_t reg;
    if(i2cDMAr == I2C_DMA_READY){ // convert received data into little-endian
        i2cDMAr = I2C_DMA_RELAX;
        parse_buffer();
        gotdata = 1;
    }
    switch(mlx_state){
        case M_FIRSTSTART: // init working mode by request
            if(write_reg(REG_CONTROL, reg_control_val[0])
                    && read_reg(REG_CONTROL, &reg)){
                SEND("REG_CTRL="); printuhex(reg); NL();
                if(read_reg(REG_STATUS, &reg)){
                    SEND("REG_STATUS="); printuhex(reg); NL();}
                if(read_data_dma(REG_CALIDATA, REG_CALIDATA_LEN)){
                    chstate(M_READCONF);
                    DBG("-> M_READCONF");
                }else chkerr();
            }else chkerr();
        break;
        case M_READCONF:
            if(gotdata){ // calculate calibration parameters
                if(get_parameters()){
                    chstate(M_RELAX);
                    DBG("-> M_RELAX");
                }else{ // error -> go to M_FIRSTSTART again
                    chstate(M_FIRSTSTART);
                    DBG("-> M_FIRSTSTART");
                }
            }else chktmout();
        break;
        case M_STARTIMA:
            subpageno = 0;
            if(startima()){
                chstate(M_PROCESS);
                DBG("-> M_PROCESS");
            }else{
                chstate(M_ERROR);
                DBG("can't start sp0 -> M_ERROR");
            }
        break;
        case M_PROCESS:
            if(read_reg(REG_STATUS, &reg)){
                if(reg & REG_STATUS_NEWDATA){
                    if(subpageno != (reg & REG_STATUS_SPNO)){
                        chstate(M_ERROR);
                        DBG("wrong subpage number -> M_ERROR");
                    }else{ // all OK, run image reading
                        if(read_data_dma(REG_IMAGEDATA, MLX_PIXARRSZ)){
                            chstate(M_READOUT);
                            DBG("-> M_READOUT");
                        }else chkerr();
                    }
                }else chktmout();
            }else chkerr();
        break;
        case M_READOUT:
            if(gotdata){
                process_subpage();
                if(++subpageno > 1){ // image ready
                    chstate(M_RELAX);
                    DBG("Image READY!");
                }else{
                    if(startima()){
                        chstate(M_PROCESS);
                        DBG("-> M_PROCESS");
                    }else{
                        chstate(M_ERROR);
                        DBG("can't start sp1 -> M_ERROR");
                    }
                }
            }else chktmout();
        break;
        case M_POWERON:
            if(Tms - Tlast > MLX_POWON_WAIT){
                if(params.kVdd == 0){ // get all parameters
                    chstate(M_FIRSTSTART);
                    DBG("M_FIRSTSTART");
                }else{ // rewrite settings register
                    if(write_reg(REG_CONTROL, reg_control_val[0])){
                        chstate(M_RELAX);
                        DBG("-> M_RELAX");
                    }else chkerr();
                }
            }
        break;
        case M_POWEROFF1:
            MLXPOW_OFF();
            chstate(M_POWEROFF);
            DBG("-> M_POWEROFF");
        break;
        case M_POWEROFF:
            if(Tms - Tlast > MLX_POWOFF_WAIT){
                MLXPOW_ON();
                chstate(M_POWERON);
                DBG("-> M_POWERON");
            }
        break;
        default:
        break;
    }
}

void mlx90640_restart(){
    DBG("restart");
    mlx_state = M_POWEROFF1;
}

// if state of MLX allows, make an image else return error
// @param simple ==1 for simplest image processing (without T calibration)
int mlx90640_take_image(uint8_t simple){
    simpleimage = simple;
    if(mlx_state != M_RELAX) return FALSE;
    if(params.kVdd == 0){ // no parameters -> make first run
        mlx_state = M_FIRSTSTART;
        DBG("no params -> M_FIRSTSTART");
        return TRUE;
    }
    mlx_state = M_STARTIMA;
    return TRUE;
}
