/**
  ******************************************************************************
  * @file    stm32f0xx.h
  * @author  MCD Application Team
  * @version V2.2.0
  * @date    05-December-2014
  * @brief   CMSIS STM32F0xx Device Peripheral Access Layer Header File.
  *
  *          The file is the unique include file that the application programmer
  *          is using in the C source code, usually in main.c. This file contains:
  *           - Configuration section that allows to select:
  *              - The STM32F0xx device used in the target application
  *              - To use or not the peripheral's drivers in application code(i.e.
  *                code will be based on direct access to peripheral's registers
  *                rather than drivers API), this option is controlled by
  *                "#define USE_HAL_DRIVER"
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32f0xx
  * @{
  */

#ifndef __STM32F0xx_H
#define __STM32F0xx_H

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/** @addtogroup Library_configuration_section
  * @{
  */

#if !defined (STM32F030x4) && !defined (STM32F030x6) && !defined (STM32F030x8) && \
    !defined (STM32F031x6) && !defined (STM32F038xx) &&                           \
    !defined (STM32F042x6) && !defined (STM32F048xx) && !defined (STM32F070x6) && \
    !defined (STM32F051x8) && !defined (STM32F058xx) &&                           \
    !defined (STM32F071xB) && !defined (STM32F072xB) && !defined (STM32F078xx) && !defined (STM32F070xB) && \
    !defined (STM32F091xC) && !defined (STM32F098xx) && !defined (STM32F030xC)
    #error "Define STM32 family, for example -DSTM32F042x6"
#endif

#ifndef WEAK
#define WEAK __attribute__((weak))
#endif

void WEAK reset_handler(void);
void WEAK nmi_handler(void);
void WEAK hard_fault_handler(void);
void WEAK sv_call_handler(void);
void WEAK pend_sv_handler(void);
void WEAK sys_tick_handler(void);

void WEAK wwdg_isr(void);
void WEAK pvd_isr(void);
void WEAK rtc_isr(void);
void WEAK flash_isr(void);
void WEAK rcc_isr(void);
void WEAK exti0_1_isr(void);
void WEAK exti2_3_isr(void);
void WEAK exti4_15_isr(void);
void WEAK tsc_isr(void);
void WEAK dma1_channel1_isr(void);
void WEAK dma1_channel2_3_isr(void);
void WEAK dma1_channel4_5_isr(void);
void WEAK adc_comp_isr(void);
void WEAK tim1_brk_up_trg_com_isr(void);
void WEAK tim1_cc_isr(void);
void WEAK tim2_isr(void);
void WEAK tim3_isr(void);
void WEAK tim6_dac_isr(void);
void WEAK tim7_isr(void);
void WEAK tim14_isr(void);
void WEAK tim15_isr(void);
void WEAK tim16_isr(void);
void WEAK tim17_isr(void);
void WEAK i2c1_isr(void);
void WEAK i2c2_isr(void);
void WEAK spi1_isr(void);
void WEAK spi2_isr(void);
void WEAK usart1_isr(void);
void WEAK usart2_isr(void);
void WEAK usart3_4_isr(void);
void WEAK cec_can_isr(void);
void WEAK usb_isr(void);


/**
  * @brief CMSIS Device version number V2.2.0
  */
#define __STM32F0xx_CMSIS_DEVICE_VERSION_MAIN   (0x02) /*!< [31:24] main version */
#define __STM32F0xx_CMSIS_DEVICE_VERSION_SUB1   (0x00) /*!< [23:16] sub1 version */
#define __STM32F0xx_CMSIS_DEVICE_VERSION_SUB2   (0x00) /*!< [15:8]  sub2 version */
#define __STM32F0xx_CMSIS_DEVICE_VERSION_RC     (0x00) /*!< [7:0]  release candidate */
#define __STM32F0xx_CMSIS_DEVICE_VERSION        ((__CMSIS_DEVICE_VERSION_MAIN     << 24)\
                                                |(__CMSIS_DEVICE_HAL_VERSION_SUB1 << 16)\
                                                |(__CMSIS_DEVICE_HAL_VERSION_SUB2 << 8 )\
                                                |(__CMSIS_DEVICE_HAL_VERSION_RC))

/**
  * @}
  */

/** @addtogroup Device_Included
  * @{
  */

// arch-dependent defines
#if defined(STM32F030x4)
  #include "stm32f030x6.h"
#elif defined(STM32F030x6)
  #include "stm32f030x6.h"
#elif defined(STM32F030x8)
  #include "stm32f030x8.h"
#elif defined(STM32F031x6)
  #include "stm32f031x6.h"
#elif defined(STM32F038xx)
  #include "stm32f038xx.h"
#elif defined(STM32F042x6)
  #include "stm32f042x6.h"
#elif defined(STM32F048xx)
  #include "stm32f048xx.h"
#elif defined(STM32F051x8)
  #include "stm32f051x8.h"
#elif defined(STM32F058xx)
  #include "stm32f058xx.h"
#elif defined(STM32F070x6)
  #include "stm32f070x6.h"
#elif defined(STM32F070xB)
  #include "stm32f070xb.h"
#elif defined(STM32F071xB)
  #include "stm32f071xb.h"
#elif defined(STM32F072xB)
  #include "stm32f072xb.h"
#elif defined(STM32F078xx)
  #include "stm32f078xx.h"
#elif defined(STM32F091xC)
  #include "stm32f091xc.h"
#elif defined(STM32F098xx)
  #include "stm32f098xx.h"
#elif defined(STM32F030xC)
  #include "stm32f030xc.h"
#endif

/**
  * @}
  */

/** @addtogroup Exported_types
  * @{
  */
typedef enum
{
  RESET = 0,
  SET = !RESET
} FlagStatus, ITStatus;

typedef enum
{
  DISABLE = 0,
  ENABLE = !DISABLE
} FunctionalState;
#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))

typedef enum
{
  ERROR = 0,
  SUCCESS = !ERROR
} ErrorStatus;

/**
  * @}
  */


/** @addtogroup Exported_macros
  * @{
  */
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))

#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

#define READ_BIT(REG, BIT)    ((REG) & (BIT))

#define CLEAR_REG(REG)        ((REG) = (0x0))

#define WRITE_REG(REG, VAL)   ((REG) = (VAL))

#define READ_REG(REG)         ((REG))

#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))


/**
  * @}
  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STM32F0xx_H */
/**
  * @}
  */

/**
  * @}
  */




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
