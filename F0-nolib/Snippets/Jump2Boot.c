// different for differ MCU
#define SYSMEM03x 0x1FFFEC00
#define SYSMEM04x 0x1FFFC400
#define SYSMEM05x 0x1FFFEC00
#define SYSMEM07x 0x1FFFC800
#define SYSMEM09x 0x1FFFD800

#define SystemMem SYSMEM07x



void Jump2Boot(){ // for STM32F072
    void (*SysMemBootJump)(void);
    volatile uint32_t addr = 0x1FFFC800;
    // reset systick
    SysTick->CTRL = 0;
    // reset clocks
    RCC->APB1RSTR = RCC_APB1RSTR_CECRST    | RCC_APB1RSTR_DACRST    | RCC_APB1RSTR_PWRRST    | RCC_APB1RSTR_CRSRST  |
                    RCC_APB1RSTR_CANRST    | RCC_APB1RSTR_USBRST    | RCC_APB1RSTR_I2C2RST   | RCC_APB1RSTR_I2C1RST |
                    RCC_APB1RSTR_USART4RST | RCC_APB1RSTR_USART3RST | RCC_APB1RSTR_USART2RST | RCC_APB1RSTR_SPI2RST |
                    RCC_APB1RSTR_WWDGRST   | RCC_APB1RSTR_TIM14RST  | RCC_APB1RSTR_TIM7RST   | RCC_APB1RSTR_TIM6RST |
                    RCC_APB1RSTR_TIM3RST   | RCC_APB1RSTR_TIM2RST;
    RCC->APB2RSTR = RCC_APB2RSTR_DBGMCURST | RCC_APB2RSTR_TIM17RST | RCC_APB2RSTR_TIM16RST | RCC_APB2RSTR_TIM15RST |
                    RCC_APB2RSTR_USART1RST | RCC_APB2RSTR_SPI1RST  | RCC_APB2RSTR_TIM1RST  | RCC_APB2RSTR_ADCRST   | RCC_APB2RSTR_SYSCFGRST;
    RCC->AHBRSTR = 0;
    RCC->APB1RSTR = 0;
    RCC->APB2RSTR = 0;
    // Enable the SYSCFG peripheral.
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    // remap memory to 0 (only for STM32F0)
    SYSCFG->CFGR1 = 0x01; __DSB(); __ISB();
    SysMemBootJump = (void (*)(void)) (*((uint32_t *)(addr + 4)));
    // set main stack pointer
    __set_MSP(*((uint32_t *)addr));
    // jump to bootloader
    SysMemBootJump();
}