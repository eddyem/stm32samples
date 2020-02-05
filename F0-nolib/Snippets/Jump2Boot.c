// different for differ MCU
#define SYSMEM03x 0x1FFFEC00
#define SYSMEM04x 0x1FFFC400
#define SYSMEM05x 0x1FFFEC00
#define SYSMEM07x 0x1FFFC800
#define SYSMEM09x 0x1FFFD800

#define SystemMem SYSMEM07x
typedef void (*pF)(void);
uint32_t JumpAddress = *(volatile uint32_t*) (SystemMem + 4);
pF Jump_To_Boot = (pFunction) JumpAddress;

// first - deinit all peripherial!!!!
/*
// default timing settings
SysTick->CTRL = 0;
SysTick->LOAD = 0;
SysTick->VAL = 0;
*/
__disable_irq(); // - may not work

// remap memory to 0
SYSCFG->CFGR1 = 0x01;

__set_MSP(*(volatile uint32_t*) SystemMem);
Jump_To_Boot();

