/*
 * stm32f3.h
 *
 * Copyright 2021 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#ifndef __STM32F3_H__
#define __STM32F3_H__

#include "vector.h"
#if defined STM32F303x8
#include "stm32f303x8.h"
#elif defined STM32F303xb || defined STM32F303xc
#include "stm32f303xc.h"
#elif defined STM32F302xb || defined STM32F302xc
#include "stm32f302xc.h"
#elif defined STM32F303xd || defined STM32F303xe
#include "stm32f303xe.h"
#else
#error "Define STM32F303xX"
#endif
#include "common_macros.h"

#define VECT_TAB_OFFSET  0x0 /*!< Vector Table base offset field.
                                  This value must be a multiple of 0x200. */
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
  RCC->CFGR &= 0xF87FC00CU;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= 0xFEF6FFFFU;

  /* Reset HSEBYP bit */
  RCC->CR &= 0xFFFBFFFFU;

  /* Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE bits */
  RCC->CFGR &= 0xFF80FFFFU;

  /* Reset PREDIV1[3:0] bits */
  RCC->CFGR2 &= 0xFFFFFFF0U;

  /* Reset USARTSW[1:0], I2CSW and TIMs bits */
  RCC->CFGR3 &= 0xFF00FCCCU;

  /* Disable all interrupts */
  RCC->CIR = 0x00000000U;

#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}
#endif

#define WAITWHILE(x)  do{StartUpCounter = 0; while((x) && (++StartUpCounter < 3600000)){nop();}}while(0)
TRUE_INLINE void StartHSI(){ // system bus 48MHz from PLL
    __IO uint32_t StartUpCounter = 0;
    RCC->CR |= RCC_CR_HSION;
    // To adjust HSI set value of HSITRIM here
    WAITWHILE(!(RCC->CR & RCC_CR_HSIRDY));
    RCC->CFGR &= ~RCC_CFGR_SW; // set sysclock to HSI
    WAITWHILE(RCC->CFGR & RCC_CFGR_SWS);
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_HSEON);
    WAITWHILE(RCC->CR & RCC_CR_PLLRDY); // wait while PLL will be off
    FLASH->ACR = (FLASH->ACR & ~(FLASH_ACR_LATENCY)) |
            FLASH_ACR_LATENCY_1 | FLASH_ACR_PRFTBE;
    RCC->CFGR = RCC_CFGR_PLLSRC_HSI_DIV2 | RCC_CFGR_PLLMUL12 | RCC_CFGR_USBPRE_DIV1 | RCC_CFGR_PPRE1_DIV2;
    RCC->CR |= RCC_CR_PLLON; // Enable PLL
    // Wait till PLL is ready
    WAITWHILE(!(RCC->CR & RCC_CR_PLLRDY));
    // Select PLL as system clock source
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    // Wait till PLL is used as system clock source
    WAITWHILE((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
    SysFreq = 48000000;
}

// @return 1 if OK, 0 if failed
TRUE_INLINE int StartHSE(){ // system bus 72MHz from PLL
    __IO uint32_t StartUpCounter = 0;
    RCC->CFGR &= ~RCC_CFGR_SW; // set sysclock to HSI
    WAITWHILE(RCC->CFGR & RCC_CFGR_SWS);
    RCC->CR &= ~RCC_CR_PLLON;
    WAITWHILE(RCC->CR & RCC_CR_PLLRDY); // wait while PLL will be off
    RCC->CR |= RCC_CR_HSEON; // disable PLL to reconfigure, enable HSE
    WAITWHILE(!(RCC->CR & RCC_CR_HSERDY));
    // Enable Prefetch Buffer. Flash 4 wait states for 48..72MHz
    FLASH->ACR = (FLASH->ACR & ~(FLASH_ACR_LATENCY)) |
            FLASH_ACR_LATENCY_2 | FLASH_ACR_PRFTBE;
    // HCLK = SYSCLK (AHB prescaler = 1), PCLK1 = HCLK/2 (APB1 prescaler = 2, max freq = 36MHz),
    // PCLK2 = HCLK (APB2 prescaler = 1), PLLCLK = HSE * 9 = 72MHz
    RCC->CFGR = RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PLLMUL9 | RCC_CFGR_PPRE1_DIV2;
    RCC->CR |= RCC_CR_PLLON; // Enable PLL
    // Wait till PLL is ready
    WAITWHILE(!(RCC->CR & RCC_CR_PLLRDY));
    // Select PLL as system clock source
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    // select system clock as I2C source
    RCC->CFGR3 |= RCC_CFGR3_I2C1SW_SYSCLK | RCC_CFGR3_I2C1SW_SYSCLK;
    // Wait till PLL is used as system clock source
    WAITWHILE((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
    SysFreq = 72000000;
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
#define OSPEED_LO(n)    (0)
#define OSPEED_MED(n)   (1<<(n*2))
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


#endif // __STM32F3_H__
