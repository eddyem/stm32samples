/*
 * stm32f1.h
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
#ifndef __STM32F1_H__
#define __STM32F1_H__

#include "vector.h"
#include "stm32f10x.h"
#include "common_macros.h"


/************************* RCC *************************/
// reset clocking registers
TRUE_INLINE void sysreset(void){
  /* Reset the RCC clock configuration to the default reset state(for debug purpose) */
  /* Set HSION bit */
  RCC->CR |= (uint32_t)0x00000001;
  /* Reset SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits */
#ifndef STM32F10X_CL
  RCC->CFGR &= (uint32_t)0xF8FF0000;
#else
  RCC->CFGR &= (uint32_t)0xF0FF0000;
#endif /* STM32F10X_CL */   
  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFF;
  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;
  /* Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE/OTGFSPRE bits */
  RCC->CFGR &= (uint32_t)0xFF80FFFF;
#ifdef STM32F10X_CL
  /* Reset PLL2ON and PLL3ON bits */
  RCC->CR &= (uint32_t)0xEBFFFFFF;
  /* Disable all interrupts and clear pending bits  */
  RCC->CIR = 0x00FF0000;
  /* Reset CFGR2 register */
  RCC->CFGR2 = 0x00000000;
#elif defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || (defined STM32F10X_HD_VL)
  /* Disable all interrupts and clear pending bits  */
  RCC->CIR = 0x009F0000;
  /* Reset CFGR2 register */
  RCC->CFGR2 = 0x00000000;      
#else
  /* Disable all interrupts and clear pending bits  */
  RCC->CIR = 0x009F0000;
#endif /* STM32F10X_CL */ 

#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE; /* Vector Table Relocation in Internal SRAM. */
#else
  SCB->VTOR = FLASH_BASE; /* Vector Table Relocation in Internal FLASH. */
#endif 
}

TRUE_INLINE void StartHSE()
{
  __IO uint32_t StartUpCounter = 0;
  
  /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
  /* Enable HSE */    
  RCC->CR |= ((uint32_t)RCC_CR_HSEON);
 
  /* Wait till HSE is ready and if Time out is reached exit */
  do
  {
    ++StartUpCounter;
  } while(!(RCC->CR & RCC_CR_HSERDY) && (StartUpCounter < 10000));


  if (RCC->CR & RCC_CR_HSERDY) // HSE started
  {
    /* Enable Prefetch Buffer */
    FLASH->ACR |= FLASH_ACR_PRFTBE;

    /* Flash 2 wait state */
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;    
 
    /* HCLK = SYSCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
    /* PCLK2 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
    /* PCLK1 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;

#ifdef STM32F10X_CL
    /* Configure PLLs ------------------------------------------------------*/
    /* PLL2 configuration: PLL2CLK = (HSE / 5) * 8 = 40 MHz */
    /* PREDIV1 configuration: PREDIV1CLK = PLL2 / 5 = 8 MHz */
        
    RCC->CFGR2 &= (uint32_t)~(RCC_CFGR2_PREDIV2 | RCC_CFGR2_PLL2MUL |
                              RCC_CFGR2_PREDIV1 | RCC_CFGR2_PREDIV1SRC);
    RCC->CFGR2 |= (uint32_t)(RCC_CFGR2_PREDIV2_DIV5 | RCC_CFGR2_PLL2MUL8 |
                             RCC_CFGR2_PREDIV1SRC_PLL2 | RCC_CFGR2_PREDIV1_DIV5);
  
    /* Enable PLL2 */
    RCC->CR |= RCC_CR_PLL2ON;
    /* Wait till PLL2 is ready */
    StartUpCounter = 0;
    while((RCC->CR & RCC_CR_PLL2RDY) == 0 && ++StartUpCounter < 1000){}
       
    /* PLL configuration: PLLCLK = PREDIV1 * 9 = 72 MHz */ 
    RCC->CFGR &= (uint32_t)~(RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLXTPRE_PREDIV1 | RCC_CFGR_PLLSRC_PREDIV1 | 
                            RCC_CFGR_PLLMULL9); 
#else    
    /*  PLL configuration: PLLCLK = HSE * 9 = 72 MHz */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE |
                                        RCC_CFGR_PLLMULL));
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);
#endif /* STM32F10X_CL */

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait till PLL is ready */
    StartUpCounter = 0;
    while((RCC->CR & RCC_CR_PLLRDY) == 0 && ++StartUpCounter < 1000){}
    
    /* Select PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

    /* Wait till PLL is used as system clock source */
    StartUpCounter = 0;
    while(((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) && ++StartUpCounter < 1000){}
  }
  else // HSE fails to start-up
  { 
        ; // add some code here (use HSI)
  }  
}



/************************* GPIO *************************/
/**
CNF1: 0 - general output or input; 1 - alternate output or pullup/down input
CNF0: 0 - push/pull, analog or pullup/down input
MODE: 00 - input, 01 - 10MHz, 10 - 2MHz, 11 - 50MHz
Pullup/down: ODR = 0 - pulldown, 1 - pullup
GPIO_BSRR and BRR also works
IDR - input, ODR - output (or pullups management), 
*/
// MODE:
#define MODE_INPUT      0
#define MODE_NORMAL     1  // 10MHz
#define MODE_SLOW       2  // 2MHz
#define MODE_FAST       3  // 50MHz
// CNF:
#define CNF_ANALOG      (0 << 2)
#define CNF_PPOUTPUT    (0 << 2)
#define CNF_FLINPUT     (1 << 2)
#define CNF_ODOUTPUT    (1 << 2)
#define CNF_PUDINPUT    (2 << 2)
#define CNF_AFPP        (2 << 2)
#define CNF_AFOD        (3 << 2)

#define CRL(pin, cnfmode)  ((cnfmode) << (pin*4))
#define CRH(pin, cnfmode)  ((cnfmode) << ((pin-8)*4))




#if 0
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


#endif

#endif // __STM32F1_H__
