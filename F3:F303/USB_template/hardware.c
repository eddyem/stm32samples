#include "hardware.h"

static inline void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN; // for USART and LEDs
    for(int i = 0; i < 10000; ++i) nop();
    GPIOB->MODER = GPIO_MODER_MODER0_O | GPIO_MODER_MODER1_O;
    GPIOB->ODR = 1;
}

void hw_setup(){
    gpio_setup();
}

