#include "stm32f411xe.h"
#include <stddef.h>

void SystemInit();
void __libc_init_array();
int main();

// These magic symbols are provided by the linker.
extern void *_estack;
extern void *_sidata, *_sdata, *_edata;
extern void *_sbss, *_ebss;
extern void (*__preinit_array_start[]) (void) __attribute__((weak));
extern void (*__preinit_array_end[]) (void) __attribute__((weak));
extern void (*__init_array_start[]) (void) __attribute__((weak));
extern void (*__init_array_end[]) (void) __attribute__((weak));
extern void (*__fini_array_start[]) (void) __attribute__((weak));
extern void (*__fini_array_end[]) (void) __attribute__((weak));

void __attribute__((naked, noreturn)) Reset_Handler()
{
  #ifdef __DEBUG_SRAM__
    __set_MSP((uint32_t)&_estack);
  #endif

  SystemInit();

  for (void **pSrc = &_sidata, **pDst = &_sdata; pDst < &_edata; *pDst++ = *pSrc++);
  for (void **pDst = &_sbss; pDst < &_ebss; *pDst++ = 0);  // Zero -> BSS

  // Use with the "-nostartfiles" linker option instead __libc_init_array();
  // Iterate over all the preinit/init routines (mainly static constructors).
  for(void(**fConstr)() = __preinit_array_start; fConstr < __preinit_array_end; (*fConstr++)());
  for(void(**fConstr)() = __init_array_start;    fConstr < __init_array_end;    (*fConstr++)());

  //__libc_init_array(); // Use with libc start files

  (void)main();
}

void Default_Handler() { for(;;); }

void NMI_Handler()                    __attribute__ ((weak, alias ("Default_Handler")));
void HardFault_Handler()              __attribute__ ((weak, alias ("Default_Handler")));
void MemManage_Handler()              __attribute__ ((weak, alias ("Default_Handler")));
void BusFault_Handler()               __attribute__ ((weak, alias ("Default_Handler")));
void UsageFault_Handler()             __attribute__ ((weak, alias ("Default_Handler")));
void SVC_Handler()                    __attribute__ ((weak, alias ("Default_Handler")));
void DebugMon_Handler()               __attribute__ ((weak, alias ("Default_Handler")));
void PendSV_Handler()                 __attribute__ ((weak, alias ("Default_Handler")));
void SysTick_Handler()                __attribute__ ((weak, alias ("Default_Handler")));
void WWDG_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void PVD_IRQHandler()                 __attribute__ ((weak, alias ("Default_Handler")));
void TAMP_STAMP_IRQHandler()          __attribute__ ((weak, alias ("Default_Handler")));
void RTC_WKUP_IRQHandler()            __attribute__ ((weak, alias ("Default_Handler")));
void FLASH_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void RCC_IRQHandler()                 __attribute__ ((weak, alias ("Default_Handler")));
void EXTI0_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void EXTI1_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void EXTI2_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void EXTI3_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void EXTI4_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Stream0_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Stream1_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Stream2_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Stream3_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Stream4_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Stream5_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Stream6_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void ADC_IRQHandler()                 __attribute__ ((weak, alias ("Default_Handler")));
void EXTI9_5_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void TIM1_BRK_TIM9_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void TIM1_UP_TIM10_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void TIM1_TRG_COM_TIM11_IRQHandler()  __attribute__ ((weak, alias ("Default_Handler")));
void TIM1_CC_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void TIM2_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void TIM3_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void TIM4_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void I2C1_EV_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void I2C1_ER_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void I2C2_EV_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void I2C2_ER_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void SPI1_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void SPI2_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void USART1_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void USART2_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void EXTI15_10_IRQHandler()           __attribute__ ((weak, alias ("Default_Handler")));
void RTC_Alarm_IRQHandler()           __attribute__ ((weak, alias ("Default_Handler")));
void OTG_FS_WKUP_IRQHandler()         __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Stream7_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void SDIO_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void TIM5_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void SPI3_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Stream0_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Stream1_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Stream2_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Stream3_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Stream4_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void OTG_FS_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Stream5_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Stream6_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Stream7_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void USART6_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void I2C3_EV_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void I2C3_ER_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void FPU_IRQHandler()                 __attribute__ ((weak, alias ("Default_Handler")));
void SPI4_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void SPI5_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));


typedef void(*intvec_elem)();

__EXTERN_C const intvec_elem __vector_table[] __VECTOR_TABLE_ATTRIBUTE =
{ (intvec_elem)&_estack, &Reset_Handler,
  &NMI_Handler,
  &HardFault_Handler,
  &MemManage_Handler,
  &BusFault_Handler,
  &UsageFault_Handler,
  NULL, NULL, NULL, NULL,
  &SVC_Handler,
  &DebugMon_Handler,
  NULL,
  &PendSV_Handler,
  &SysTick_Handler,
  &WWDG_IRQHandler,
  &PVD_IRQHandler,
  &TAMP_STAMP_IRQHandler,
  &RTC_WKUP_IRQHandler,
  &FLASH_IRQHandler,
  &RCC_IRQHandler,
  &EXTI0_IRQHandler,
  &EXTI1_IRQHandler,
  &EXTI2_IRQHandler,
  &EXTI3_IRQHandler,
  &EXTI4_IRQHandler,
  &DMA1_Stream0_IRQHandler,
  &DMA1_Stream1_IRQHandler,
  &DMA1_Stream2_IRQHandler,
  &DMA1_Stream3_IRQHandler,
  &DMA1_Stream4_IRQHandler,
  &DMA1_Stream5_IRQHandler,
  &DMA1_Stream6_IRQHandler,
  &ADC_IRQHandler,
  NULL, NULL, NULL, NULL,
  &EXTI9_5_IRQHandler,
  &TIM1_BRK_TIM9_IRQHandler,
  &TIM1_UP_TIM10_IRQHandler,
  &TIM1_TRG_COM_TIM11_IRQHandler,
  &TIM1_CC_IRQHandler,
  &TIM2_IRQHandler,
  &TIM3_IRQHandler,
  &TIM4_IRQHandler,
  &I2C1_EV_IRQHandler,
  &I2C1_ER_IRQHandler,
  &I2C2_EV_IRQHandler,
  &I2C2_ER_IRQHandler,
  &SPI1_IRQHandler,
  &SPI2_IRQHandler,
  &USART1_IRQHandler,
  &USART2_IRQHandler,
  NULL,
  &EXTI15_10_IRQHandler,
  &RTC_Alarm_IRQHandler,
  &OTG_FS_WKUP_IRQHandler,
  NULL, NULL, NULL, NULL,
  &DMA1_Stream7_IRQHandler,
  NULL,
  &SDIO_IRQHandler,
  &TIM5_IRQHandler,
  &SPI3_IRQHandler,
  NULL, NULL, NULL, NULL,
  &DMA2_Stream0_IRQHandler,
  &DMA2_Stream1_IRQHandler,
  &DMA2_Stream2_IRQHandler,
  &DMA2_Stream3_IRQHandler,
  &DMA2_Stream4_IRQHandler,
  NULL, NULL, NULL, NULL, NULL, NULL,
  &OTG_FS_IRQHandler,
  &DMA2_Stream5_IRQHandler,
  &DMA2_Stream6_IRQHandler,
  &DMA2_Stream7_IRQHandler,
  &USART6_IRQHandler,
  &I2C3_EV_IRQHandler,
  &I2C3_ER_IRQHandler,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  &FPU_IRQHandler,
  NULL, NULL,
  &SPI4_IRQHandler,
  &SPI5_IRQHandler,
};

#ifdef __cplusplus
}
#endif
