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
#include <stdint.h>
#include <string.h>

#include "strfunc.h"

#include "mlx90640.h"
#include "mlx90640_regs.h"
#include "mlxproc.h"

// tolerance of floating point comparison
#define FP_TOLERANCE    (1e-3)

// 3072 bytes
static fp_t mlx_image[MLX_PIXNO] = {0}; // ready image
// 10100 bytes:
static MLX90640_params params; // calculated parameters (in heap, not stack!) for other functions

/*****************************************************************************
                Calculate parameters & values
 *****************************************************************************/

// fill OCC/ACC row/col arrays
static void occacc(int8_t *arr, int l, const uint16_t *regstart){
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
MLX90640_params *get_parameters(const uint16_t dataarray[MLX_DMA_MAXLEN]){
    #define CREG_VAL(reg) dataarray[CREG_IDX(reg)]
    int8_t i8;
    int16_t i16;
    uint16_t *pu16;
    uint16_t val = CREG_VAL(REG_VDD);
    i8 = (int8_t) (val >> 8);
    params.kVdd = i8 * 32; // keep sign
    if(params.kVdd == 0){UN("kvdd=0"); return NULL;}
    i16 = val & 0xFF;
    params.vdd25 = ((i16 - 0x100) * 32) - (1<<13);
    val = CREG_VAL(REG_KVTPTAT);
    i16 = (val & 0xFC00) >> 10;
    if(i16 > 0x1F) i16 -= 0x40;
    params.KvPTAT = (fp_t)i16 / (1<<12);
    i16 = (val & 0x03FF);
    if(i16 > 0x1FF) i16 -= 0x400;
    params.KtPTAT = (fp_t)i16 / 8.;
    params.vPTAT25 = (int16_t) CREG_VAL(REG_PTAT);
    val = CREG_VAL(REG_APTATOCCS) >> 12;
    params.alphaPTAT = val / 4. + 8.;
    params.gainEE = (int16_t)CREG_VAL(REG_GAIN);
    if(params.gainEE == 0){UN("gainee=0"); return NULL;}
    int8_t occRow[MLX_H];
    int8_t occColumn[MLX_W];
    occacc(occRow, MLX_H, &CREG_VAL(REG_OCCROW14));
    occacc(occColumn, MLX_W, &CREG_VAL(REG_OCCCOL14));
    int8_t accRow[MLX_H];
    int8_t accColumn[MLX_W];
    occacc(accRow, MLX_H, &CREG_VAL(REG_ACCROW14));
    occacc(accColumn, MLX_W, &CREG_VAL(REG_ACCCOL14));
    val = CREG_VAL(REG_APTATOCCS);
    // need to do multiplication instead of bitshift, so:
    fp_t occRemScale = 1<<(val&0x0F),
          occColumnScale = 1<<((val>>4)&0x0F),
          occRowScale = 1<<((val>>8)&0x0F);
    int16_t offavg = (int16_t) CREG_VAL(REG_OSAVG);
    // even/odd column/row numbers are for starting from 1, so for starting from 0 we should swap them:
    // even - for 1,3,5,...; odd - for 0,2,4,... etc
    int8_t ktaavg[4];
    // 0 - odd row, odd col; 1 - odd row even col; 2 - even row, odd col; 3 - even row, even col
    val = CREG_VAL(REG_KTAAVGODDCOL);
    ktaavg[2] = (int8_t)(val & 0xFF); // odd col (1,3,..), even row (2,4,..) -> col 0,2,..; row 1,3,..
    ktaavg[0] = (int8_t)(val >> 8); // odd col, odd row -> col 0,2,..; row 0,2,..
    val = CREG_VAL(REG_KTAAVGEVENCOL);
    ktaavg[3] = (int8_t)(val & 0xFF); // even col, even row -> col 1,3,..; row 1,3,..
    ktaavg[1] = (int8_t)(val >> 8); // even col, odd row -> col 1,3,..; row 0,2,..
    // so index of ktaavg is 2*(row&1)+(col&1)
    val = CREG_VAL(REG_KTAVSCALE);
    uint8_t scale1 = ((val & 0xFF)>>4) + 8, scale2 = (val&0xF);
    if(scale1 == 0 || scale2 == 0){UN("scale1/2=0"); return NULL;}
    fp_t mul = (fp_t)(1<<scale2), div = (fp_t)(1<<scale1); // kta_scales
    uint16_t a_r = CREG_VAL(REG_SENSIVITY); // alpha_ref
    val = CREG_VAL(REG_SCALEACC);
    fp_t *a = params.alpha;
    uint32_t diva32 = 1 << (val >> 12);
    fp_t diva = (fp_t)(diva32);
    diva *= (fp_t)(1<<30); // alpha_scale
    fp_t accRowScale = 1<<((val & 0x0f00)>>8),
          accColumnScale = 1<<((val & 0x00f0)>>4),
          accRemScale = 1<<(val & 0x0f);
    pu16 = (uint16_t*)&CREG_VAL(REG_OFFAK1);
    fp_t *kta = params.kta, *offset = params.offset;
    //uint8_t *ol = params.outliers;
    for(int row = 0; row < MLX_H; ++row){
        int idx = (row&1)<<1;
        for(int col = 0; col < MLX_W; ++col){
            // offset
            register uint16_t rv = *pu16++;
            i16 = (rv & 0xFC00) >> 10;
            if(i16 > 0x1F) i16 -= 0x40;
            *offset++ = (fp_t)offavg + (fp_t)occRow[row]*occRowScale + (fp_t)occColumn[col]*occColumnScale + (fp_t)i16*occRemScale;
            // kta
            i16 = (rv & 0xF) >> 1;
            if(i16  > 0x03) i16 -= 0x08;
            *kta++ = (ktaavg[idx|(col&1)] + i16*mul) / div;
            // alpha
            i16 = (rv & 0x3F0) >> 4;
            if(i16 > 0x1F) i16 -= 0x40;
            fp_t oft = (fp_t)a_r + accRow[row]*accRowScale + accColumn[col]*accColumnScale +i16*accRemScale;
            *a++ = oft / diva;
            //*ol++ = (rv&1) ? 1 : 0;
        }
    }
    scale1 = (CREG_VAL(REG_KTAVSCALE) >> 8) & 0xF; // kvscale
    div = (fp_t)(1<<scale1);
    val = CREG_VAL(REG_KVAVG);
    // kv indexes: +2 for odd (нечетных) rows, +1 for odd columns, so:
    // [ 3, 2; 1, 0] for left upper corner (because datashit counts from 1, not from 0!)
    i16 = val >> 12; if(i16 > 0x07) i16 -= 0x10;
    ktaavg[0] = (int8_t)i16; // odd col, odd row
    i16 = (val & 0xF0) >> 4; if(i16 > 0x07) i16 -= 0x10;
    ktaavg[1] = (int8_t)i16; // even col, odd row
    i16 = (val & 0x0F00) >> 8; if(i16 > 0x07) i16 -= 0x10;
    ktaavg[2] = (int8_t)i16; // odd col, even row
    i16 = val & 0x0F; if(i16 > 0x07) i16 -= 0x10;
    ktaavg[3] = (int8_t)i16; // even col, even row
    for(int i = 0; i < 4; ++i) params.kv[i] = ktaavg[i] / div;
    val = CREG_VAL(REG_CPOFF);
    params.cpOffset[0] = (val & 0x03ff);
    if(params.cpOffset[0] > 0x1ff) params.cpOffset[0] -= 0x400;
    params.cpOffset[1] = val >> 10;
    if(params.cpOffset[1] > 0x1f) params.cpOffset[1] -= 0x40;
    params.cpOffset[1] += params.cpOffset[0];
    val = ((CREG_VAL(REG_KTAVSCALE) & 0xF0) >> 4) + 8;
    i8 = (int8_t)(CREG_VAL(REG_KVTACP) & 0xFF);
    params.cpKta = (fp_t)i8 / (1<<val);
    val = (CREG_VAL(REG_KTAVSCALE) & 0x0F00) >> 8;
    i16 = CREG_VAL(REG_KVTACP) >> 8;
    if(i16 > 0x7F) i16 -= 0x100;
    params.cpKv = (fp_t)i16 / (1<<val);
    i16 = CREG_VAL(REG_KSTATGC) & 0xFF;
    if(i16 > 0x7F) i16 -= 0x100;
    params.tgc = (fp_t)i16;
    params.tgc /= 32.;
    val = (CREG_VAL(REG_SCALEACC)>>12); // alpha_scale_CP
    i16 = CREG_VAL(REG_ALPHA)>>10; // cp_P1_P0_ratio
    if(i16 > 0x1F) i16 -= 0x40;
    div = (fp_t)(1<<val);
    div *= (fp_t)(1<<27);
    params.cpAlpha[0] = (fp_t)(CREG_VAL(REG_ALPHA) & 0x03FF) / div;
    div = (fp_t)(1<<7);
    params.cpAlpha[1] = params.cpAlpha[0] * (1. + (fp_t)i16/div);
    i8 = (int8_t)(CREG_VAL(REG_KSTATGC) >> 8);
    params.KsTa = (fp_t)i8/(1<<13);
    div = 1<<((CREG_VAL(REG_CT34) & 0x0F) + 8); // kstoscale
    val = CREG_VAL(REG_KSTO12);
    i8 = (int8_t)(val & 0xFF);
    params.KsTo[0] = i8 / div;
    i8 = (int8_t)(val >> 8);
    params.KsTo[1] = i8 / div;
    val = CREG_VAL(REG_KSTO34);
    i8 = (int8_t)(val & 0xFF);
    params.KsTo[2] = i8 / div;
    i8 = (int8_t)(val >> 8);
    params.KsTo[3] = i8 / div;
    // CT1 = -40, CT2 = 0 -> start from zero index, so CT[0] is CT2, CT[1] is CT3, CT[2] is CT4
    params.CT[0] = 0.; // 0degr - between ranges 1 and 2
    val = CREG_VAL(REG_CT34);
    mul = ((val & 0x3000)>>12)*10.; // step
    params.CT[1] = ((val & 0xF0)>>4)*mul; // CT3 - between ranges 2 and 3
    params.CT[2] = ((val & 0x0F00) >> 8)*mul + params.CT[1]; // CT4 - between ranges 3 and 4
    // alphacorr for each range: 11.1.11
    params.alphacorr[0] = 1./(1. + params.KsTo[0] * 40.);
    params.alphacorr[1] = 1.;
    params.alphacorr[2] = (1. + params.KsTo[1] * params.CT[1]);
    params.alphacorr[3] = (1. + params.KsTo[2] * (params.CT[2] - params.CT[1])) * params.alphacorr[2];
    params.resolEE = (uint8_t)((CREG_VAL(REG_KTAVSCALE) & 0x3000) >> 12);
    // Don't forget to check 'outlier' flags for wide purpose
    return &params;
#undef CREG_VAL
}

/**
 * @brief process_image - process both subpages (image data of sp0 lays in sp1, service data is in spare array)
 * @param params
 * @param subpages
 * @param Service0
 * @param subpageno
 * @return
 */
fp_t *process_image(const int16_t subpage1[REG_IMAGEDATA_LEN]){
#define IMD_VAL(reg) subpage1[IMD_IDX(reg)]
    // 11.2.2.1. Resolution restore
    //fp_t resol_corr = (fp_t)(1<<params.resolEE) / (1<<mlx_getresolution()); // calibrated resol/current resol
    fp_t resol_corr = (fp_t)(1<<params.resolEE) / (1<<2); // ONLY DEFAULT!
    int16_t i16a;
    fp_t dvdd, dTa, Kgain, pixOS[2]; // values for both subpages
    // 11.2.2.2. Supply voltage value calculation
    i16a = (int16_t)IMD_VAL(REG_IVDDPIX);
    //U("rval="); UN(i2str(i16a));
    dvdd = resol_corr*i16a - params.vdd25;
    dvdd /= params.kVdd;
    //U("dvdd="); UN(float2str(dvdd, 2));
    fp_t dV = i16a - params.vdd25; // for next step
    dV /= params.kVdd;
    // 11.2.2.3. Ambient temperature calculation
    i16a = (int16_t)IMD_VAL(REG_ITAPTAT);
    int16_t i16b = (int16_t)IMD_VAL(REG_ITAVBE);
    dTa = (fp_t)i16a / (i16a * params.alphaPTAT + i16b); // vptatart
    dTa *= (fp_t)(1<<18);
    dTa = (dTa / (1. + params.KvPTAT*dV)) - params.vPTAT25;
    dTa = dTa / params.KtPTAT; // without 25degr - Ta0
    // 11.2.2.4. Gain parameter calculation
    i16a = (int16_t)IMD_VAL(REG_IGAIN);
    Kgain = params.gainEE / (fp_t)i16a;
    // 11.2.2.6.1
    pixOS[0] = ((int16_t)IMD_VAL(REG_ICPSP0))*Kgain; // pix_OS_CP_SPx
    pixOS[1] = ((int16_t)IMD_VAL(REG_ICPSP1))*Kgain;
    // 11.2.2.6.2
    for(int sp = 0; sp < 2; ++sp)
        pixOS[sp] -= params.cpOffset[sp]*(1. + params.cpKta*dTa)*(1. + params.cpKv*dvdd);
    // now make first approximation to image
    uint16_t pixno = 0;  // current pixel number - for indexing in parameters etc
    for(int row = 0, rowidx = 0; row < MLX_H; ++row, rowidx ^= 2){
        for(int col = 0, idx = rowidx; col < MLX_W; ++col, ++pixno, idx ^= 1){
            uint8_t sp = (row&1)^(col&1); // subpage of current pixel - for `pixOS` and `cpAlpha`
            // 11.2.2.5.1
            fp_t curval = (fp_t)(subpage1[pixno]) * Kgain; // gain compensation
            // 11.2.2.5.3
            curval -= params.offset[pixno] * (1. + params.kta[pixno]*dTa) *
                    (1. + params.kv[idx]*dvdd); // add offset
            // now `curval` is pix_OS == V_IR_emiss_comp (we can divide it by `emissivity` to compensate for it)
            // 11.2.2.7: 'Pattern' is just subpage number!
            fp_t IRcompens = curval - params.tgc * pixOS[sp]; // 11.2.2.8. Normalizing to sensitivity
            // 11.2.2.8
            fp_t alphaComp = params.alpha[pixno] - params.tgc * params.cpAlpha[sp];
            alphaComp *= 1. + params.KsTa * dTa;
            // 11.2.2.9: calculate To for basic range
            fp_t Tar = dTa + 273.15 + 25.; // Ta+273.15
            Tar = Tar*Tar*Tar*Tar; // T_aK4 (when \epsilon==1 this is T_{a-r} too)
            fp_t ac3 = alphaComp*alphaComp*alphaComp;
            fp_t Sx = ac3*IRcompens + alphaComp*ac3*Tar;
            Sx = params.KsTo[1] * SQRT(SQRT(Sx));
            fp_t To4 = IRcompens / (alphaComp * (1. - 273.15*params.KsTo[1]) + Sx) + Tar;
            curval = SQRT(SQRT(To4)) - 273.15;
            // 11.2.2.9.1.3. Extended To range calculation
            int r = 0; // range 1 by default
            fp_t ctx = -40.;
            if(curval > params.CT[2]){ // range 4
                r = 3; ctx = params.CT[2];
            }else if(curval > params.CT[1]){ // range 3
                r = 2; ctx = params.CT[1];
            }else if(curval > params.CT[0]){ // range 2, default
                r = 1; ctx = params.CT[0];
            }
            if(r != 1){ // recalculate for extended range if we are out of standard range
                To4 = IRcompens / (alphaComp * params.alphacorr[r] * (1. + params.KsTo[r]*(curval - ctx))) + Tar;
                curval = SQRT(SQRT(To4)) - 273.15;
            }
            mlx_image[pixno] = curval;
        }
    }
    return mlx_image;
#undef IMD_VAL
}

