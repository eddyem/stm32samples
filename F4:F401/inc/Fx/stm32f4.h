/*
 * This file is part of the stm32f4 project.
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

#pragma once

#include "vector.h"
#ifdef STM32F407xx
#include "stm32f407xx.h"
#else
#error "Define STM32F407xx"
#endif

#include "common_macros.h"

// HSE=12MHz, fVCO=288MHz (PLL_M=12, PLL_N=288), HCLK=144MHz (PLL_P=2), fUSB=48MHz (PLL_Q=6)
#ifndef PLL_M
#define PLL_M   12
#endif
#ifndef PLL_N
#define PLL_N   288
#endif
#ifndef PLL_P
#define PLL_P   2
#endif
#ifndef PLL_Q
#define PLL_Q   6
#endif

#ifndef VECT_TAB_OFFSET
#define VECT_TAB_OFFSET  0x0 /*!< Vector Table base offset field.
                                  This value must be a multiple of 0x200. */
#endif

#if 0
/**
  * @brief  Setup the microcontroller system
  *         Initialize the FPU setting, vector table location and the PLL configuration is reset.
  * @param  None
  * @retval None
  */
TRUE_INLINE void sysreset(void) // not usable
{
  /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set HSION bit */
  RCC->CR |= RCC_CR_HSION;

  /* Reset CFGR register */
  RCC->CFGR = 0;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &=(uint32_t)0xFEF6FFFF;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = (uint32_t)0x24003010;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  /* Disable all interrupts */
  RCC->CIR = 0x00000000;

#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}
#endif

#define WAITWHILE(x)  do{StartUpCounter = 0; while((x) && (++StartUpCounter < 0xffffff)){} if(StartUpCounter == 0xffffff) return 0;}while(0)
TRUE_INLINE int StartHSI(){ // HSI is 16MHz, so PLL_M=16, PLL_N=288, PLL_P=2, PLL_Q=6
    uint32_t StartUpCounter = 0;
    RCC->CR = (RCC->CR & ~RCC_CR_PLLON) | RCC_CR_HSION;
    WAITWHILE(!(RCC->CR & RCC_CR_HSIRDY));
    // Enable high performance mode (default after reset), System frequency up to 168 MHz, Vreg += 10%
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS;
    WAITWHILE(!(PWR->CSR & PWR_CSR_VOSRDY));
    // HCLK = SYSCLK, PCLK1 = HCLK/4, PCLK2 = HCLK/2
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)
                 ) | RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;
    /* Configure the main PLL */
    RCC->PLLCFGR = 16 | (288 << 6) | (6 << 24);
    RCC->CR |= RCC_CR_PLLON; // Enable PLL
    // Wait till PLL is ready
    WAITWHILE(!(RCC->CR & RCC_CR_PLLRDY));
    /* Configure Flash prefetch, Instruction cache, Data cache and wait state */
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;
    // Select PLL as system clock source
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    return 1;
}

// @return 1 if OK, 0 if failed
TRUE_INLINE int StartHSE(){ // fVCO can be from 192 to 432MHz
    uint32_t StartUpCounter = 0;
    RCC->CR = (RCC->CR & ~RCC_CR_PLLON) | RCC_CR_HSEON; // disable PLL to reconfigure, enable HSE
    WAITWHILE(!(RCC->CR & RCC_CR_HSERDY));
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    // Enable high performance mode (default after reset), System frequency up to 168 MHz, Vreg += 10%
    PWR->CR |= PWR_CR_VOS;
    WAITWHILE(!(PWR->CSR & PWR_CSR_VOSRDY));
    // HCLK = SYSCLK, PCLK1 = HCLK/4, PCLK2 = HCLK/2
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)
                 ) | RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;
    /* Configure the main PLL */
    RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) -1) << 16) |
                   (RCC_PLLCFGR_PLLSRC_HSE) | (PLL_Q << 24);
    RCC->CR |= RCC_CR_PLLON; // Enable PLL
    // Wait till PLL is ready
    WAITWHILE(!(RCC->CR & RCC_CR_PLLRDY));
    /* Configure Flash prefetch, Instruction cache, Data cache and wait state */
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;
    // Select PLL as system clock source
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    return 1;
}
#undef WAITWHILE


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
