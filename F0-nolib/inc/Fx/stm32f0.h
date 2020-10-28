/*
 * stm32f0.h
 *
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
#ifndef __STM32F0_H__
#define __STM32F0_H__

#include "vector.h"
#include "stm32f0xx.h"
#include "common_macros.h"


/************************* RCC *************************/
// reset clocking registers
TRUE_INLINE void sysreset(void){
    /* Reset the RCC clock configuration to the default reset state ------------*/
    /* Set HSION bit */
    RCC->CR |= (uint32_t)0x00000001;
#if defined (STM32F051x8) || defined (STM32F058x8)
    /* Reset SW[1:0], HPRE[3:0], PPRE[2:0], ADCPRE and MCOSEL[2:0] bits */
    RCC->CFGR &= (uint32_t)0xF8FFB80C;
#else
    /* Reset SW[1:0], HPRE[3:0], PPRE[2:0], ADCPRE, MCOSEL[2:0], MCOPRE[2:0] and PLLNODIV bits */
    RCC->CFGR &= (uint32_t)0x08FFB80C;
#endif /* STM32F051x8 or STM32F058x8 */
    /* Reset HSEON, CSSON and PLLON bits */
    RCC->CR &= (uint32_t)0xFEF6FFFF;
    /* Reset HSEBYP bit */
    RCC->CR &= (uint32_t)0xFFFBFFFF;
    /* Reset PLLSRC, PLLXTPRE and PLLMUL[3:0] bits */
    RCC->CFGR &= (uint32_t)0xFFC0FFFF;
    /* Reset PREDIV[3:0] bits */
    RCC->CFGR2 &= (uint32_t)0xFFFFFFF0;
    #if defined (STM32F072xB) || defined (STM32F078xB)
    /* Reset USART2SW[1:0], USART1SW[1:0], I2C1SW, CECSW, USBSW and ADCSW bits */
    RCC->CFGR3 &= (uint32_t)0xFFFCFE2C;
#elif defined (STM32F071xB)
    /* Reset USART2SW[1:0], USART1SW[1:0], I2C1SW, CECSW and ADCSW bits */
    RCC->CFGR3 &= (uint32_t)0xFFFFCEAC;
#elif defined (STM32F091xC) || defined (STM32F098xx)
    /* Reset USART3SW[1:0], USART2SW[1:0], USART1SW[1:0], I2C1SW, CECSW and ADCSW bits */
    RCC->CFGR3 &= (uint32_t)0xFFF0FEAC;
#elif defined (STM32F030x4) || defined (STM32F030x6) || defined (STM32F030x8) || defined (STM32F031x6) || defined (STM32F038xx) || defined (STM32F030xC)
    /* Reset USART1SW[1:0], I2C1SW and ADCSW bits */
    RCC->CFGR3 &= (uint32_t)0xFFFFFEEC;
#elif defined (STM32F051x8) || defined (STM32F058xx)
    /* Reset USART1SW[1:0], I2C1SW, CECSW and ADCSW bits */
    RCC->CFGR3 &= (uint32_t)0xFFFFFEAC;
#elif defined (STM32F042x6) || defined (STM32F048xx)
    /* Reset USART1SW[1:0], I2C1SW, CECSW, USBSW and ADCSW bits */
    RCC->CFGR3 &= (uint32_t)0xFFFFFE2C;
#elif defined (STM32F070x6) || defined (STM32F070xB)
    /* Reset USART1SW[1:0], I2C1SW, USBSW and ADCSW bits */
    RCC->CFGR3 &= (uint32_t)0xFFFFFE6C;
    /* Set default USB clock to PLLCLK, since there is no HSI48 */
    RCC->CFGR3 |= (uint32_t)0x00000080;
#else
#error "No target selected"
#endif
    /* Disable all interrupts */
    RCC->CIR = 0x00000000;
    /* Reset HSI14 bit */
    RCC->CR2 &= (uint32_t)0xFFFFFFFE;
    // Enable Prefetch Buffer and set Flash Latency
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
    /* HCLK = SYSCLK */
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
    /* PCLK = HCLK */
    RCC->CFGR |= RCC_CFGR_PPRE_DIV1;
    /* PLL configuration = (HSI/2) * 12 = ~48 MHz */
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL);
    RCC->CFGR |= RCC_CFGR_PLLMUL12;
    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    /* Wait till PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0){}
    /* Select PLL as system clock source */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    /* Wait till PLL is used as system clock source */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL){}
}

TRUE_INLINE void StartHSE(){
    // disable PLL
    RCC->CR &= ~RCC_CR_PLLON;
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CIR & RCC_CIR_HSERDYF) != 0);
    RCC->CIR |= RCC_CIR_HSERDYC; // clear rdy flag
    /* PLL configuration = (HSE) * 12 = ~48 MHz */
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL);
    RCC->CFGR |= RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PLLMUL12;
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL){}
}

#if defined (STM32F042x6) || defined (STM32F072xb)
TRUE_INLINE void StartHSI48(){
    // disable PLL
    RCC->CR &= ~RCC_CR_PLLON;
    RCC->CR2 &= RCC_CR2_HSI48ON; // turn on HSI48
    while((RCC->CR2 & RCC_CR2_HSI48RDY) == 0);
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL));
    // HSI48/2 * 2 = HSI48
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSI48_PREDIV | RCC_CFGR_PLLMUL2);
    RCC->CR |= RCC_CR_PLLON;
    // select HSI48 as system clock source
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_HSI48;
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_HSI48){}
}
#endif

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



/******************  FLASH Keys  **********************************************/
#define RDP_Key                  ((uint16_t)0x00A5)
#define FLASH_KEY1               ((uint32_t)0x45670123)
#define FLASH_KEY2               ((uint32_t)0xCDEF89AB)

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




#endif // __STM32F0_H__
