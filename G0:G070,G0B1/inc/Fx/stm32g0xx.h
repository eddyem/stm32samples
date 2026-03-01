/**
  ******************************************************************************
  * @file    stm32g0xx.h
  * @author  MCD Application Team
  * @brief   CMSIS STM32G0xx Device Peripheral Access Layer Header File.
  *
  *          The file is the unique include file that the application programmer
  *          is using in the C source code, usually in main.c. This file contains:
  *           - Configuration section that allows to select:
  *              - The STM32G0xx device used in the target application
  *              - To use or not the peripherals drivers in application code(i.e.
  *                code will be based on direct access to peripherals registers
  *                rather than drivers API), this option is controlled by
  *                "#define USE_HAL_DRIVER"
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32g0xx
  * @{
  */

#ifndef STM32G0xx_H
#define STM32G0xx_H

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/** @addtogroup Library_configuration_section
  * @{
  */

/**
  * @brief STM32 Family
  */
#if !defined (STM32G0)
#error "DEFINE STM32G0 first!"
#endif /* STM32G0 */

/**
  * @brief CMSIS Device version number $VERSION$
  */
#define __STM32G0_CMSIS_VERSION_MAIN   (0x01U) /*!< [31:24] main version */
#define __STM32G0_CMSIS_VERSION_SUB1   (0x04U) /*!< [23:16] sub1 version */
#define __STM32G0_CMSIS_VERSION_SUB2   (0x03U) /*!< [15:8]  sub2 version */
#define __STM32G0_CMSIS_VERSION_RC     (0x00U) /*!< [7:0]  release candidate */
#define __STM32G0_CMSIS_VERSION        ((__STM32G0_CMSIS_VERSION_MAIN << 24)\
                                       |(__STM32G0_CMSIS_VERSION_SUB1 << 16)\
                                       |(__STM32G0_CMSIS_VERSION_SUB2 << 8 )\
                                       |(__STM32G0_CMSIS_VERSION_RC))

/**
  * @}
  */

/** @addtogroup Device_Included
  * @{
  */

#if defined(STM32G0B1xx)
  #include "stm32g0b1xx.h"
#elif defined(STM32G0C1xx)
  #include "stm32g0c1xx.h"
#elif defined(STM32G0B0xx)
  #include "stm32g0b0xx.h"
#elif defined(STM32G071xx)
  #include "stm32g071xx.h"
#elif defined(STM32G081xx)
  #include "stm32g081xx.h"
#elif defined(STM32G070xx)
  #include "stm32g070xx.h"
#elif defined(STM32G031xx)
  #include "stm32g031xx.h"
#elif defined(STM32G041xx)
  #include "stm32g041xx.h"
#elif defined(STM32G030xx)
  #include "stm32g030xx.h"
#elif defined(STM32G051xx)
  #include "stm32g051xx.h"
#elif defined(STM32G061xx)
  #include "stm32g061xx.h"
#elif defined(STM32G050xx)
  #include "stm32g050xx.h"
#else
 #error "Please select first the target STM32G0xx device used in your application (in stm32g0xx.h file)"
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
  SUCCESS = 0,
  ERROR = !SUCCESS
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

/* Use of interrupt control for register exclusive access */
/* Atomic 32-bit register access macro to set one or several bits */
#define ATOMIC_SET_BIT(REG, BIT)                             \
  do {                                                       \
    uint32_t primask;                                        \
    primask = __get_PRIMASK();                               \
    __set_PRIMASK(1);                                        \
    SET_BIT((REG), (BIT));                                   \
    __set_PRIMASK(primask);                                  \
  } while(0)

/* Atomic 32-bit register access macro to clear one or several bits */
#define ATOMIC_CLEAR_BIT(REG, BIT)                           \
  do {                                                       \
    uint32_t primask;                                        \
    primask = __get_PRIMASK();                               \
    __set_PRIMASK(1);                                        \
    CLEAR_BIT((REG), (BIT));                                 \
    __set_PRIMASK(primask);                                  \
  } while(0)

/* Atomic 32-bit register access macro to clear and set one or several bits */
#define ATOMIC_MODIFY_REG(REG, CLEARMSK, SETMASK)            \
  do {                                                       \
    uint32_t primask;                                        \
    primask = __get_PRIMASK();                               \
    __set_PRIMASK(1);                                        \
    MODIFY_REG((REG), (CLEARMSK), (SETMASK));                \
    __set_PRIMASK(primask);                                  \
  } while(0)

/* Atomic 16-bit register access macro to set one or several bits */
#define ATOMIC_SETH_BIT(REG, BIT) ATOMIC_SET_BIT(REG, BIT)                                   \

/* Atomic 16-bit register access macro to clear one or several bits */
#define ATOMIC_CLEARH_BIT(REG, BIT) ATOMIC_CLEAR_BIT(REG, BIT)                               \

/* Atomic 16-bit register access macro to clear and set one or several bits */
#define ATOMIC_MODIFYH_REG(REG, CLEARMSK, SETMASK) ATOMIC_MODIFY_REG(REG, CLEARMSK, SETMASK) \

/*#define POSITION_VAL(VAL)     (__CLZ(__RBIT(VAL)))*/
/**
  * @}
  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STM32G0xx_H */
/**
  * @}
  */

/**
  * @}
  */
