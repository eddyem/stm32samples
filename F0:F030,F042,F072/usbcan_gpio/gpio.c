/*
 * This file is part of the usbcangpio project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stm32f0.h>
#include <string.h>

#include "adc.h"
#include "flash.h"
#include "gpio.h"
#include "usart.h"

static uint16_t monitor_mask[2] = {0}; // pins to monitor == 1 (ONLY GPIO and ADC)
static uint16_t oldstates[2][16] = {0}; // previous state (16 bits - as some pins could be analog)

// intermediate buffer to change pin's settings by user request; after checking in will be copied to the_conf
static pinconfig_t pinconfig[2][16] = {0};
static uint8_t pinconfig_notinited = 1; // ==0 after first memcpy from the_conf to pinconfig
#define CHKPINCONFIG()  do{if(pinconfig_notinited) chkpinconf();}while(0)

// TODO: remove AFmask, make function to get right AF number by pin's FuncValues
typedef struct{
    uint8_t funcs;  // bitmask according to enum FuncNames
    uint8_t AF[FUNC_AMOUNT]; // alternate function number for corresponding `FuncNames` number
} pinprops_t;

#define CANADC(x)   ((x) & (1<<FUNC_AIN))
#define CANUSART(x) ((x) & (1<<FUNC_USART))
#define CANSPI(x)   ((x) & (1<<FUNC_SPI))
#define CANI2C(x)   ((x) & (1<<FUNC_I2C))

// AF for USART, SPI, I2C:
#define _U(x)  [FUNC_USART] = x
// _S(0) or _U(0) have no sence, but lets understand that this pin have SPI or USART
#define _S(x)  [FUNC_SPI] = x
#define _I(x)  [FUNC_I2C] = x
static const pinprops_t pin_props[2][16] = {
    [0] = { // PORT A
        [0]  = { .funcs = 0b00000001, .AF = {0}}, // PA0: ADC0, AF2 (TIM2_CH1)
        [1]  = { .funcs = 0b00000001, .AF = {0}}, // PA1: ADC1, AF2 (TIM2_CH2)
        [2]  = { .funcs = 0b00000011, .AF = {_U(1)}}, // PA2: ADC2, AF2 (TIM2_CH3), AF1 (USART2_TX)
        [3]  = { .funcs = 0b00000011, .AF = {_U(1)}}, // PA3: ADC3, AF2 (TIM2_CH4), AF1 (USART2_RX)
        [5]  = { .funcs = 0b00000101, .AF = {_S(0)}}, // PA5: ADC5, AF9 (SPI1_SCK)
        [6]  = { .funcs = 0b00000101, .AF = {_S(0)}}, // PA6: ADC6, AF0 (SPI1_MISO)
        [7]  = { .funcs = 0b00000101, .AF = {_S(0)}}, // PA7: ADC7, AF0 (SPI1_MOSI)
        [9]  = { .funcs = 0b00000010, .AF = {_U(1)}}, // PA9: AF1 (USART1_TX)
        [10] = { .funcs = 0b00000010, .AF = {_U(1)}}, // PA10: AF1 (USART1_RX)
    },
    [1] = { // PORT B
        [0]  = { .funcs = 0b00000001, .AF = {0}}, // PB0: ADC8, AF1 (TIM3_CH3), AF2 (TIM1_CH2N)
        [1]  = { .funcs = 0b00000001, .AF = {0}}, // PB1: ADC9, AF0 (TIM14_CH1), AF1 (TIM3_CH4), AF2 (TIM1_CH3N)
        [2]  = { .funcs = 0b00000000, .AF = {0}}, // PB2: nothing except GPIO
        [3]  = { .funcs = 0b00000100, .AF = {_S(0)}}, // PB3: AF0, (SPI1_SCK), AF2 (TIM2_CH2)
        [4]  = { .funcs = 0b00000100, .AF = {_S(0)}}, // PB4: AF0 (SPI1_MISO), AF1 (TIM3_CH1)
        [5]  = { .funcs = 0b00000100, .AF = {_S(0)}}, // PB5: AF0 (SPI1_MOSI), AF1 (TIM3_CH2)
        [6]  = { .funcs = 0b00001010, .AF = {_U(0), _I(1)}}, // PB6: AF0 (USART1_TX), AF1 (I2C1_SCL), AF2 (TIM16_CH1N)
        [7]  = { .funcs = 0b00001010, .AF = {_U(0), _I(1)}}, // PB7: AF0 (USART1_RX), AF1 (I2C1_SDA), AF2 (TIM17_CH1N)
        [10] = { .funcs = 0b00001000, .AF = {_I(1)}}, // PB10: AF1 (I2C1_SCL), AF2 (TIM2_CH3)
        [11] = { .funcs = 0b00001000, .AF = {_I(1)}}, // PB11: AF1 (I2C1_SDA), AF2 (TIM2_CH4)
    }
};
#undef _U
#undef _S
#undef _I


typedef struct{
    uint8_t isrx : 1;
    uint8_t istx : 1;
} usart_props_t;
/**
 * @brief get_usart_index - get USART index (0 or 1 for USART1 or USART2) by given pin
 * @param port
 * @param pin
 * @return -1 if error
 */
static int get_usart_index(uint8_t port, uint8_t pin, usart_props_t *p){
    int idx = -1;
    usart_props_t curprops = {0};
    if(port == 0){ // GPIOA
        switch(pin){
        case 2: // USART2_TX
            idx = 1;
            curprops.istx = 1;
            break;
        case 3: // USART2_RX
            idx = 1;
            curprops.isrx = 1;
            break;
        case 9: // USART1_TX
            idx = 0;
            curprops.istx = 1;
            break;
        case 10: // USART1_RX
            idx = 0;
            curprops.isrx = 1;
            break;
        default:
            break;
        }
    }else if(port == 1){ // GPIOB
        switch(pin){
        case 6: // USART1_TX
            idx = 0;
            curprops.istx = 1;
            break;
        case 7: // USART1_RX
            idx = 0;
            curprops.isrx = 1;
            break;
        default:
            break;
        }
    }
    if(p) *p = curprops;
    return idx;
}

// default config
static void defconfig(pinconfig_t *cfg){
    if(!cfg) return;
    cfg->af = FUNC_AIN;
    cfg->afno = 0;
    cfg->mode = MODE_INPUT;
    cfg->monitor = 0;
    cfg->speed = SPEED_LOW;
    cfg->pull = PULL_NONE;
}

// check current pin configuration; in case of errors set default values (floating input)
int chkpinconf(){
    int ret = TRUE;
    if(pinconfig_notinited){
        memcpy(pinconfig, the_conf.pinconfig, sizeof(pinconfig));
        pinconfig_notinited = 0;
    }
    usartconf_t UC;
    if(!get_curusartconf(&UC)){
        get_defusartconf(&UC);
    }else{ // got current config -> clear `RXen`, `TXen` and `monitor`: if none appeared, turn OFF USART!
        UC.RXen = 0; UC.TXen = 0; UC.monitor = 0;
    }
    int active_usart = -1; // number of USART if user selects it (we can't check it by UC->idx)
    for(int port = 0; port < 2; ++port){
        for(int pin = 0; pin < 16; ++pin){
            pinconfig_t *cfg = &pinconfig[port][pin];
            if(!cfg->enable) continue;
            const pinprops_t *props = &pin_props[port][pin];
            // wrong configuration -> don't mind AF, make FLIN
            if(cfg->mode == MODE_AF){
                if(cfg->af >= FUNC_AMOUNT || !((1<<cfg->af) & props->funcs)){
                    DBG("Wrong AF config -> FL IN\n");
                    defconfig(cfg);
                    ret = FALSE;
                }else{ // set afno to proper number
                    cfg->afno = props->AF[cfg->af];
                    switch(cfg->af){
                    case FUNC_USART:{
                        usart_props_t up;
                        int usart_idx = get_usart_index(port, pin, &up);
                        if(usart_idx < 0){ // error -> defaults
                            DBG("no USART on this pin\n");
                            defconfig(cfg);
                            ret = FALSE;
                            break;
                        }
                        if(active_usart == -1){
                            active_usart = usart_idx;
                        }else if(active_usart != usart_idx){
                            // User tries to configure Rx/Tx on different USARTs
                            DBG("USART conflicted!\n");
                            defconfig(cfg);
                            ret = FALSE;
                            break;
                        }
                        if(up.isrx) UC.RXen = 1;
                        else if(up.istx) UC.TXen = 1;
                        if(cfg->monitor) UC.monitor = 1;
                    }
                        break;
                    default: break; // later fill other functions
                    }
                }
            }
        }
    }
    // now check USART configuration
    if(active_usart != -1){
        UC.idx = active_usart;
        if(!chkusartconf(&UC)) ret = FALSE;
    }else{
        get_defusartconf(&UC); // clear global configuration
        the_conf.usartconfig = UC;
    }
    return ret;
}

int is_disabled(uint8_t port, uint8_t pin){
    if(port > 1 || pin > 15) return FALSE;
    if(the_conf.pinconfig[port][pin].enable) return FALSE;
    return TRUE;
}

// return current conf from local `pinconfig`
int get_curpinconf(uint8_t port, uint8_t pin, pinconfig_t *c){
    CHKPINCONFIG();
    if(!c || port > 1 || pin > 15) return FALSE;
    *c = pinconfig[port][pin];
    return TRUE;
}

/**
 * @brief set_pinfunc - check if alternate function `afno` allowed on given pin
 * @param port - 0 for GPIOA and 1 for GPIOB
 * @param pin - 0..15
 * @param pcfg (io) - pin configuration
 * @return TRUE if all OK
 */
int set_pinfunc(uint8_t port, uint8_t pin, pinconfig_t *pcfg){
    DBG("set_pinfunc()\n");
    CHKPINCONFIG();
    if(is_disabled(port, pin) || !pcfg){
        DBG("Disabled?\n");
        return FALSE;
    }
    const pinprops_t *props = &pin_props[port][pin];
    switch(pcfg->mode){
    case MODE_ANALOG:
        DBG("Analog\n");
        if(!CANADC(props->funcs)){
            DBG("Can't ADC\n");
            return FALSE;
        }
        pcfg->pull = PULL_NONE; // no PullUp for analog mode
        break;
    case MODE_AF:
        DBG("Altfun\n");
        // here af is one of enum FuncValues !!! we should change `af` later
        if(pcfg->af >= FUNC_AMOUNT || !((1<<pcfg->af) & props->funcs)){
            DBG("Wrong AF\n");
            return FALSE;
        }
        pcfg->afno = props->AF[pcfg->af];
        pcfg->speed = SPEED_HIGH; // many AF needs high speed
        pcfg->otype = OUTPUT_PP; // no OD for AF
        break;
    case MODE_INPUT: // no limits
        DBG("Input\n");
        break;
    case MODE_OUTPUT: // remove pullup/pulldown for PP
        DBG("Output\n");
        if(pcfg->otype == OUTPUT_PP) pcfg->pull = PULL_NONE;
        break;
    default:
        DBG("Wrong\n");
        return FALSE;
    }
    pcfg->enable = 1; // don't forget to set enable flag!
    pinconfig[port][pin] = *pcfg;
    DBG("All OK\n");
    return TRUE;
}

/**
 * @brief get_adc_channel - get ADC channel number for given pin
 * @param port - 0/1 (GPIOA/GPIOB)
 * @param pin - 0..16
 * @return ADC channel number or -1
 */
TRUE_INLINE int8_t get_adc_channel(uint8_t port, uint8_t pin){
    if(port == 0){ // GPIOA
        if (pin <= 7) return pin; // PA0..PA7 -> IN0..IN7
    }else{ // GPIOB
        if(pin == 0) return 8; // PB0 -> IN8
        if(pin == 1) return 9; // PB1 -> IN9
    }
    return -1; // No ADC channel on this pin
}

// reinit all GPIO registers due to config; also configure (if need) USART1/2, SPI1 and I2C1
// return FALSE if found some errors in current configuration (and it was fixed to default)
int gpio_reinit(){
    bzero(monitor_mask, sizeof(monitor_mask));
    bzero(oldstates, sizeof(oldstates));
    int ret = TRUE;
    int tocopy = chkpinconf(); // if config is wrong, don't copy it to flash
    for(int port = 0; port < 2; port++){
        GPIO_TypeDef *gpio = (port == 0) ? GPIOA : GPIOB;
        for(int pin = 0; pin < 16; pin++){
            pinconfig_t *cfg = &pinconfig[port][pin];
            if(!cfg->enable) continue;
            int shift2 = pin << 1;
            gpio->MODER = (gpio->MODER & ~(3 << shift2))| (cfg->mode << shift2);
            gpio->OTYPER = (gpio->OTYPER & ~(1 << pin)) | (cfg->otype << pin);
            gpio->OSPEEDR = (gpio->OSPEEDR & ~(3 << shift2)) | (cfg->speed << shift2);
            gpio->PUPDR = (gpio->PUPDR & ~(3 << shift2)) | (cfg->pull << shift2);
            if(pin < 8){
                int shift4 = pin << 4;
                gpio->AFR[0] = (gpio->AFR[0] & ~(0xf << shift4)) | (cfg->afno << shift4);
            }else{
                int shift4 = (pin - 8) << 2;
                gpio->AFR[1] = (gpio->AFR[1] & ~(0xf << shift4)) | (cfg->afno << shift4);
            }
            if(cfg->monitor && cfg->mode != MODE_AF){
                monitor_mask[port] |= (1 << pin);
                if(cfg->mode == MODE_ANALOG){
                    if(cfg->threshold > 4095) cfg->threshold = 4095; // no threshold
                    int8_t chan = get_adc_channel(port, pin);
                    if(chan >= 0){
                        oldstates[port][pin] = getADCval(chan);
                    }
                }else{
                    // save old state for regular GPIO
                    oldstates[port][pin] = (gpio->IDR >> pin) & 1;
                }
            }
        }
    }
    // if all OK, copy to the_conf
    if(tocopy) memcpy(the_conf.pinconfig, pinconfig, sizeof(pinconfig));
    else ret = FALSE;
    // TODO: configure SPI etc
    usartconf_t usc;
    if(get_curusartconf(&usc) && (usc.RXen | usc.TXen)){
        if(!usart_config(NULL)) ret = FALSE;
        else if(!usart_start()) ret = FALSE;
    }
    return ret;
}

// get MODER for current pin
TRUE_INLINE uint32_t get_moder(volatile GPIO_TypeDef * GPIOx, uint8_t pin){
    return (GPIOx->MODER >> (pin << 1)) & 3;
}

/**
 * @brief pin_out - change pin  value
 * @param port - 0 for GPIOA, 1 for GPIOB
 * @param pin - 0..15
 * @param newval - 0 or 1 (reset/set)
 * @return FALSE if pin isn't OUT or other err
 * here I check real current settings by GPIOx->MODER
 */
int pin_out(uint8_t port, uint8_t pin, uint8_t newval){
    if(port > 1 || pin > 15) return FALSE;
    volatile GPIO_TypeDef * GPIOx = (port == 0) ? GPIOA : GPIOB;
    uint16_t mask = 1 << pin;
    uint32_t moder = get_moder(GPIOx, pin);
    if(moder != MODE_OUTPUT) return FALSE;
    if(newval) GPIOx->BSRR = mask;
    else GPIOx->BRR = mask;
    return TRUE;
}

/**
 * @brief pin_in - get current pin's value (0/1 for regular GPIO, 0..4095 for ADC)
 * @param port - 0..1
 * @param pin - 0..15
 * @return value or -1 if pin have AF or don't used
 */
int16_t pin_in(uint8_t port, uint8_t pin){
    if(port > 1 || pin > 15) return -1;
    volatile GPIO_TypeDef * GPIOx = (port == 0) ? GPIOA : GPIOB;
    uint32_t moder = get_moder(GPIOx, pin);
    int16_t val = -1;
    switch(moder){ // check REAL pin config
    case MODE_INPUT:
    case MODE_OUTPUT:
        if(GPIOx->IDR & (1<<pin)) val = 1;
        else val = 0;
    break;
    case MODE_ANALOG:{
        int8_t chan = get_adc_channel(port, pin);
        if(chan >= 0){
            return (int16_t)getADCval(chan); // getADCval возвращает uint16_t
        }
    }
        break;
    default:
        break;
    }
    return val;
}

/**
 * @brief gpio_alert - return bitmask for alerted pins (whos state changed over last check)
 *      AF don't checked! Use appropriate function for them
 * @param port - 0 for GPIOA, 1 for GPIOB
 * @return pin mask where 1 is for changed state
 */
uint16_t gpio_alert(uint8_t port){
    if(port > 1) return 0;
    if(0 == monitor_mask[port]) return 0; // nothing to monitor
    volatile GPIO_TypeDef * GPIOx = (port == 0) ? GPIOA : GPIOB;
    uint32_t moder = GPIOx->MODER;
    uint16_t curpinbit = 1; // shift each iteration
    uint16_t *oldstate = oldstates[port];
    uint16_t alert = 0;
    for(int pin = 0; pin < 16; ++pin, curpinbit <<= 1, moder >>= 2){
        uint8_t curm = moder & 3;
        if((curm == MODE_AF) || 0 == (monitor_mask[port] & curpinbit)) continue; // monitor also OUT (if OD)
        // TODO: add AIN
        if(curm == MODE_ANALOG){
            int8_t chan = get_adc_channel(port, pin);
            if(chan < 0) continue; // can't be in normal case
            uint16_t cur = getADCval(chan);
            uint16_t thresh = the_conf.pinconfig[port][pin].threshold;
            uint16_t diff = (cur > oldstate[pin]) ? (cur - oldstate[pin]) : (oldstate[pin] - cur);
            if(diff > thresh){
                oldstate[pin] = cur;
                alert |= curpinbit;
            }
        }else{
            uint16_t curval = (GPIOx->IDR & curpinbit) ? 1 : 0;
            if(oldstate[pin] != curval){
                oldstate[pin] = curval;
                alert |= curpinbit;
            }
        }
    }
    return alert;
}
