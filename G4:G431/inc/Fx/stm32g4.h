/*
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#pragma once

#include "vector.h"
#include "stm32g4xx.h"
#include "common_macros.h"

#define WAITWHILE(x)  do{register uint32_t StartUpCounter = 0; while((x) && (++StartUpCounter < 0xfffff)){nop();}}while(0)
//#define WAITWHILE(x)  do{while(x){nop();}}while(0)
TRUE_INLINE void StartHSE(){
    // ----- 1. Turn off PLL -----
    RCC->CR &= ~RCC_CR_PLLON;
    WAITWHILE(RCC->CR & RCC_CR_PLLRDY);

    // ----- 2. Turn on HSE -----
    // Параметр isHSE игнорируется  всегда используем HSE по условию задачи
    RCC->CR |= RCC_CR_HSEON;
    WAITWHILE(!(RCC->CR & RCC_CR_HSERDY));

    // ----- 3. Turn on power block PWR and setup for max freq (VOS = 0) -----
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    PWR->CR1 &= ~(PWR_CR1_VOS_0 | PWR_CR1_VOS_1);
    WAITWHILE(PWR->SR2 & PWR_SR2_VOSF);

    // ----- 4. PLL setup (SYSCLK = 170 MHz) -----
    // HSE = 8 MHz, PLLM = 2, si PLL in = 4 MHz
    // PLLN = 85, so VCO = 4 * 85 = 340 MHz
    // PLLR = 2, so SYSCLK = 340 / 2 = 170 MHz
    //
    RCC->PLLCFGR =
        ((2 - 1)   << RCC_PLLCFGR_PLLM_Pos) |    // PLLM = 2
        (85        << RCC_PLLCFGR_PLLN_Pos) |    // PLLN = 85
        (0         << RCC_PLLCFGR_PLLR_Pos) |    // PLLR = 2 (code 0)
        RCC_PLLCFGR_PLLREN |                     // turn on Rclk for SYSCLK
        RCC_PLLCFGR_PLLSRC_HSE;                  // source is HSE

    // ----- 5. Turn PLL on -----
    RCC->CR |= RCC_CR_PLLON;
    WAITWHILE(!(RCC->CR & RCC_CR_PLLRDY));

    // ----- 6. Flash setup (4 wait cycles for 170 МГц) -----
    FLASH->ACR |= FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_4WS;

    // ----- 7. Switch from SYSCLK to PLL -----
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;

    // ----- 8. Turn on HSI48 and wait until ready -----
    RCC->CRRCR |= RCC_CRRCR_HSI48ON;
    WAITWHILE(!(RCC->CRRCR & RCC_CRRCR_HSI48RDY));

    // ----- 9. Turn on CRS (Clock Recovery System) -----
    RCC->APB1ENR1 |= RCC_APB1ENR1_CRSEN;

    // ----- 10. Setup CRS for USB SOF sync -----
    CRS->CR &= ~(CRS_CR_CEN | CRS_CR_AUTOTRIMEN | CRS_CR_SWSYNC);
    CRS->CFGR &= ~CRS_CFGR_SYNCDIV_Msk;
    CRS->CFGR = (CRS->CFGR & ~CRS_CFGR_SYNCSRC_Msk) | (1 << CRS_CFGR_SYNCSRC_Pos);
    CRS->CFGR &= ~CRS_CFGR_SYNCDIV_Msk;
    CRS->CR |= CRS_CR_AUTOTRIMEN | CRS_CR_CEN;

    // ----- 11. Select HSI48 as USB clock -----
    RCC->CCIPR &= ~RCC_CCIPR_CLK48SEL_Msk; // 00: HSI48 clock selected as 48 MHz clock

    SysFreq = 170000000UL;
}

/************************* GPIO *************************/

/*******************  Bit definition for GPIO_MODER register  *****************/
// _AI - analog inpt, _O - general output, _AF - alternate function
#define GPIO_MODER_MODER0_AI        ((uint32_t)0x00000003)
#define GPIO_MODER_MODER0_O         ((uint32_t)0x00000001)
#define GPIO_MODER_MODER0_AF        ((uint32_t)0x00000002)
#define GPIO_MODER_MODER1_AI        ((uint32_t)0x0000000C)
#define GPIO_MODER_MODER1_O         ((uint32_t)0x00000004)
#define GPIO_MODER_MODER1_AF        ((uint32_t)0x00000008)
#define GPIO_MODER_MODER2_AI        ((uint32_t)0x00000030)
#define GPIO_MODER_MODER2_O         ((uint32_t)0x00000010)
#define GPIO_MODER_MODER2_AF        ((uint32_t)0x00000020)
#define GPIO_MODER_MODER3_AI        ((uint32_t)0x000000C0)
#define GPIO_MODER_MODER3_O         ((uint32_t)0x00000040)
#define GPIO_MODER_MODER3_AF        ((uint32_t)0x00000080)
#define GPIO_MODER_MODER4_AI        ((uint32_t)0x00000300)
#define GPIO_MODER_MODER4_O         ((uint32_t)0x00000100)
#define GPIO_MODER_MODER4_AF        ((uint32_t)0x00000200)
#define GPIO_MODER_MODER5_AI        ((uint32_t)0x00000C00)
#define GPIO_MODER_MODER5_O         ((uint32_t)0x00000400)
#define GPIO_MODER_MODER5_AF        ((uint32_t)0x00000800)
#define GPIO_MODER_MODER6_AI        ((uint32_t)0x00003000)
#define GPIO_MODER_MODER6_O         ((uint32_t)0x00001000)
#define GPIO_MODER_MODER6_AF        ((uint32_t)0x00002000)
#define GPIO_MODER_MODER7_AI        ((uint32_t)0x0000C000)
#define GPIO_MODER_MODER7_O         ((uint32_t)0x00004000)
#define GPIO_MODER_MODER7_AF        ((uint32_t)0x00008000)
#define GPIO_MODER_MODER8_AI        ((uint32_t)0x00030000)
#define GPIO_MODER_MODER8_O         ((uint32_t)0x00010000)
#define GPIO_MODER_MODER8_AF        ((uint32_t)0x00020000)
#define GPIO_MODER_MODER9_AI        ((uint32_t)0x000C0000)
#define GPIO_MODER_MODER9_O         ((uint32_t)0x00040000)
#define GPIO_MODER_MODER9_AF        ((uint32_t)0x00080000)
#define GPIO_MODER_MODER10_AI       ((uint32_t)0x00300000)
#define GPIO_MODER_MODER10_O        ((uint32_t)0x00100000)
#define GPIO_MODER_MODER10_AF       ((uint32_t)0x00200000)
#define GPIO_MODER_MODER11_AI       ((uint32_t)0x00C00000)
#define GPIO_MODER_MODER11_O        ((uint32_t)0x00400000)
#define GPIO_MODER_MODER11_AF       ((uint32_t)0x00800000)
#define GPIO_MODER_MODER12_AI       ((uint32_t)0x03000000)
#define GPIO_MODER_MODER12_O        ((uint32_t)0x01000000)
#define GPIO_MODER_MODER12_AF       ((uint32_t)0x02000000)
#define GPIO_MODER_MODER13_AI       ((uint32_t)0x0C000000)
#define GPIO_MODER_MODER13_O        ((uint32_t)0x04000000)
#define GPIO_MODER_MODER13_AF       ((uint32_t)0x08000000)
#define GPIO_MODER_MODER14_AI       ((uint32_t)0x30000000)
#define GPIO_MODER_MODER14_O        ((uint32_t)0x10000000)
#define GPIO_MODER_MODER14_AF       ((uint32_t)0x20000000)
#define GPIO_MODER_MODER15_AI       ((uint32_t)0xC0000000)
#define GPIO_MODER_MODER15_O        ((uint32_t)0x40000000)
#define GPIO_MODER_MODER15_AF       ((uint32_t)0x80000000)

/*******************  Bit definition for GPIO_PUPDR register  *****************/
// no/pullup/pulldown/reserved
// for n in $(seq 0 15); do echo "#define GPIO_PUPDR${n}_PU              ((uint32_t)(1<<$((n*2))))";
// echo "#define GPIO_PUPDR${n}_PD              ((uint32_t)(1<<$((n*2+1))))"; done
// alt+select column -> delete
#define GPIO_PUPDR0_PU              ((uint32_t)(1<<0))
#define GPIO_PUPDR0_PD              ((uint32_t)(1<<1))
#define GPIO_PUPDR1_PU              ((uint32_t)(1<<2))
#define GPIO_PUPDR1_PD              ((uint32_t)(1<<3))
#define GPIO_PUPDR2_PU              ((uint32_t)(1<<4))
#define GPIO_PUPDR2_PD              ((uint32_t)(1<<5))
#define GPIO_PUPDR3_PU              ((uint32_t)(1<<6))
#define GPIO_PUPDR3_PD              ((uint32_t)(1<<7))
#define GPIO_PUPDR4_PU              ((uint32_t)(1<<8))
#define GPIO_PUPDR4_PD              ((uint32_t)(1<<9))
#define GPIO_PUPDR5_PU              ((uint32_t)(1<<10))
#define GPIO_PUPDR5_PD              ((uint32_t)(1<<11))
#define GPIO_PUPDR6_PU              ((uint32_t)(1<<12))
#define GPIO_PUPDR6_PD              ((uint32_t)(1<<13))
#define GPIO_PUPDR7_PU              ((uint32_t)(1<<14))
#define GPIO_PUPDR7_PD              ((uint32_t)(1<<15))
#define GPIO_PUPDR8_PU              ((uint32_t)(1<<16))
#define GPIO_PUPDR8_PD              ((uint32_t)(1<<17))
#define GPIO_PUPDR9_PU              ((uint32_t)(1<<18))
#define GPIO_PUPDR9_PD              ((uint32_t)(1<<19))
#define GPIO_PUPDR10_PU             ((uint32_t)(1<<20))
#define GPIO_PUPDR10_PD             ((uint32_t)(1<<21))
#define GPIO_PUPDR11_PU             ((uint32_t)(1<<22))
#define GPIO_PUPDR11_PD             ((uint32_t)(1<<23))
#define GPIO_PUPDR12_PU             ((uint32_t)(1<<24))
#define GPIO_PUPDR12_PD             ((uint32_t)(1<<25))
#define GPIO_PUPDR13_PU             ((uint32_t)(1<<26))
#define GPIO_PUPDR13_PD             ((uint32_t)(1<<27))
#define GPIO_PUPDR14_PU             ((uint32_t)(1<<28))
#define GPIO_PUPDR14_PD             ((uint32_t)(1<<29))
#define GPIO_PUPDR15_PU             ((uint32_t)(1<<30))
#define GPIO_PUPDR15_PD             ((uint32_t)(1<<31))
// OSPEEDR
// for n in $(seq 0 15); do echo "#define GPIO_OSPEEDR${n}_MED           ((uint32_t)(1<<$((n*2))))";
// echo "#define GPIO_OSPEEDR${n}_HIGH          ((uint32_t)(3<<$((2*n))))"; done
#define GPIO_OSPEEDR0_MED           ((uint32_t)(1<<0))
#define GPIO_OSPEEDR0_HIGH          ((uint32_t)(3<<0))
#define GPIO_OSPEEDR1_MED           ((uint32_t)(1<<2))
#define GPIO_OSPEEDR1_HIGH          ((uint32_t)(3<<2))
#define GPIO_OSPEEDR2_MED           ((uint32_t)(1<<4))
#define GPIO_OSPEEDR2_HIGH          ((uint32_t)(3<<4))
#define GPIO_OSPEEDR3_MED           ((uint32_t)(1<<6))
#define GPIO_OSPEEDR3_HIGH          ((uint32_t)(3<<6))
#define GPIO_OSPEEDR4_MED           ((uint32_t)(1<<8))
#define GPIO_OSPEEDR4_HIGH          ((uint32_t)(3<<8))
#define GPIO_OSPEEDR5_MED           ((uint32_t)(1<<10))
#define GPIO_OSPEEDR5_HIGH          ((uint32_t)(3<<10))
#define GPIO_OSPEEDR6_MED           ((uint32_t)(1<<12))
#define GPIO_OSPEEDR6_HIGH          ((uint32_t)(3<<12))
#define GPIO_OSPEEDR7_MED           ((uint32_t)(1<<14))
#define GPIO_OSPEEDR7_HIGH          ((uint32_t)(3<<14))
#define GPIO_OSPEEDR8_MED           ((uint32_t)(1<<16))
#define GPIO_OSPEEDR8_HIGH          ((uint32_t)(3<<16))
#define GPIO_OSPEEDR9_MED           ((uint32_t)(1<<18))
#define GPIO_OSPEEDR9_HIGH          ((uint32_t)(3<<18))
#define GPIO_OSPEEDR10_MED          ((uint32_t)(1<<20))
#define GPIO_OSPEEDR10_HIGH         ((uint32_t)(3<<20))
#define GPIO_OSPEEDR11_MED          ((uint32_t)(1<<22))
#define GPIO_OSPEEDR11_HIGH         ((uint32_t)(3<<22))
#define GPIO_OSPEEDR12_MED          ((uint32_t)(1<<24))
#define GPIO_OSPEEDR12_HIGH         ((uint32_t)(3<<24))
#define GPIO_OSPEEDR13_MED          ((uint32_t)(1<<26))
#define GPIO_OSPEEDR13_HIGH         ((uint32_t)(3<<26))
#define GPIO_OSPEEDR14_MED          ((uint32_t)(1<<28))
#define GPIO_OSPEEDR14_HIGH         ((uint32_t)(3<<28))
#define GPIO_OSPEEDR15_MED          ((uint32_t)(1<<30))
#define GPIO_OSPEEDR15_HIGH         ((uint32_t)(3<<30))

// clear MODER: ~GPIO_MODER_MODERXX_Msk, you should AND these
#define MODER_CLR(n)    (~(3<<(n*2)))
// _AI - analog inpt, _O - general output, _AF - alternate function
// these should be OR'ed
#define MODER_I(n)      (0)
#define MODER_O(n)      (1<<(n*2))
#define MODER_AF(n)     (2<<(n*2))
#define MODER_AI(n)     (3<<(n*2))
// OSPEED: low, medium, high
#define OSPEED_CLR(n)   (~(3<<(n*2)))
#define OSPEED_VLO(n)   (0)
#define OSPEED_LO(n)    (1<<(n*2))
#define OSPEED_MED(n)   (2<<(n*2))
#define OSPEED_HI(n)    (3<<(n*2))
// PUPD: pull up/down
#define PUPD_CLR(n)     (~(3<<(n*2)))
#define PUPD_PU(n)      (1<<(n*2))
#define PUPD_PD(n)      (2<<(n*2))
// OTYPER: bit==1 for OD
#define OTYPER_PP(n)    0
#define OTYPER_OD(n)    (1<<n)

// AFR field: afr - AFR number, pin - pin (0..15)
TRUE_INLINE uint32_t AFRf(uint8_t afr, uint8_t pin){
    if(pin > 7) pin -= 8;
    return (afr << (pin * 4));
}


/******************  FLASH Keys  **********************************************/
#define RDP_Key                 ((uint16_t)0x00A5)
#define FLASH_KEY1              ((uint32_t)0x45670123)
#define FLASH_KEY2              ((uint32_t)0xCDEF89AB)
#define FLASH_SIZE_REG          ((uint32_t)0x1FFFF7CC)

/************************* ADC *************************/
/* inner termometer calibration values
 *         Temp = (V30 - Vsense)/Avg_Slope + 30
 *         Avg_Slope = (V30 - V110) / (110 - 30)
 */
#define TEMP110_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7C2))
#define TEMP30_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7B8))
// VDDA_Actual = 3.3V * VREFINT_CAL / average vref value
#define VREFINT_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7BA))
#define VDD_CALIB ((uint16_t) (330))
#define VDD_APPLI ((uint16_t) (300))

/************************* USART *************************/

#define USART_CR2_ADD_SHIFT     24
// set address/character match value
#define USART_CR2_ADD_VAL(x)        ((x) << USART_CR2_ADD_SHIFT)

/************************* IWDG *************************/
#define IWDG_REFRESH      (uint32_t)(0x0000AAAA)
#define IWDG_WRITE_ACCESS (uint32_t)(0x00005555)
#define IWDG_START        (uint32_t)(0x0000CCCC)


//#define  do{}while(0)

