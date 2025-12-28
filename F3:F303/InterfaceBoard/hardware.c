#include "hardware.h"

static inline void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN; // for USART and LEDs
    for(int i = 0; i < 10000; ++i) nop();
    // USB - alternate function 14 @ pins PA11/PA12; SWD - AF0 @PA13/14
    GPIOA->AFR[1] = AFRf(14, 11) | AFRf(14, 12);
    GPIOA->MODER = MODER_O(10) | MODER_AF(11) | MODER_AF(12) | MODER_AF(13) | MODER_AF(14);
}

void hw_setup(){
    gpio_setup();
}

