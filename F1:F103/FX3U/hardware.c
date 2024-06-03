/*
 * This file is part of the fx3u project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "can.h"
#include "canproto.h"
#include "flash.h"
#include "hardware.h"
#ifdef EBUG
#include "strfunc.h"
#endif

/* pinout:

| **Pin #** | **Pin name ** | **function** | **settings**           | **comment **                            |
| --------- | ------------- | ------------ | ---------------------- | --------------------------------------- |
|  15       | PC0/adcin10   | ADC4         | ADC in                 |                                         |
|  16       | PC1/adcin11   | ADC5         | ADC in                 |                                         |
|  23       | PA0           | Y3           | PPOUT                  |                                         |
|  24       | PA1/adcin1    | ADC0         | ADC in                 |                                         |
|  25       | PA2           | Y11          | PPOUT                  |                                         |
|  26       | PA3/adcin3    | ADC1         | ADC in                 |                                         |
|  31       | PA6           | Y10          | PPOUT                  |                                         |
|  32       | PA7           | Y7           | PPOUT                  |                                         |
|  33       | PC4/adcin14   | ADC2         | ADC in                 |                                         |
|  34       | PC5/adcin15   | ADC3         | ADC in                 |                                         |
|  37       | PB2/boot1     | PROG SW      | PUIN                   |                                         |
|  38       | PE7           | X14          | PUIN                   |                                         |
|  39       | PE8           | X15          | PUIN                   |                                         |
|  40       | PE9           | X12          | PUIN                   |                                         |
|  41       | PE10          | X13          | PUIN                   |                                         |
|  42       | PE11          | X10          | PUIN                   |                                         |
|  43       | PE12          | X11          | PUIN                   |                                         |
|  44       | PE13          | X6           | PUIN                   |                                         |
|  45       | PE14          | X7           | PUIN                   |                                         |
|  46       | PE15          | X4           | PUIN                   |                                         |
|  47       | PB10          | X5           | PUIN                   |                                         |
|  48       | PB11          | X2           | PUIN                   |                                         |
|  51       | PB12          | X3           | PUIN                   |                                         |
|  52       | PB13          | X0           | PUIN                   |                                         |
|  53       | PB14          | X1           | PUIN                   |                                         |
|  54       | PB15          | Y6           | PPOUT                  |                                         |
|  59       | PD12          | Y5           | PPOUT                  |                                         |
|  65       | PC8           | Y1           | PPOUT                  |                                         |
|  66       | PC9           | Y0           | PPOUT                  |                                         |
|  67       | PA8           | Y2           | PPOUT                  |                                         |
|  68       | PA9           | RS TX        | AFPP                   |                                         |
|  69       | PA10          | RS RX        | FLIN                   |                                         |
|  76       | PA14/SWCLK    | 485 DE *     | (default)              | (Not now)                               |
|  81       | PD0           | CAN RX       | FLIN                   |                                         |
|  82       | PD1           | CAN TX       | AFPP                   |                                         |
|  89       | PB3/JTDO      | Y4           | PPOUT                  |                                         |
|           |               |              |                        |                                         |
|           |               |              |                        |                                         |
|           |               |              |                        |                                         |
 */

void gpio_setup(void){
    // PD0 & PD1 (CAN) setup in can.c; PA9 & PA10 (USART) in usart.c
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
                    RCC_APB2ENR_IOPEEN | RCC_APB2ENR_AFIOEN;
    // Turn off JTAG
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
    GPIOA->CRL = CRL(0, CNF_PPOUTPUT|MODE_NORMAL) | CRL(1, CNF_ANALOG) | CRL(2, CNF_PPOUTPUT|MODE_NORMAL) |
                 CRL(3, CNF_ANALOG) | CRL(6, CNF_PPOUTPUT|MODE_NORMAL) | CRL(7, CNF_PPOUTPUT|MODE_NORMAL);
    GPIOA->CRH = 0;
    GPIOB->CRL = CRL(2, CNF_PUDINPUT);
    GPIOB->CRH = CRH(10, CNF_PUDINPUT) | CRH(11, CNF_PUDINPUT) | CRH(12, CNF_PUDINPUT) | CRH(13, CNF_PUDINPUT) |
                 CRH(14, CNF_PUDINPUT) | CRH(15, CNF_PPOUTPUT|MODE_NORMAL);
    GPIOC-> CRL = CRL(0, CNF_ANALOG) | CRL(1, CNF_ANALOG) | CRL(4, CNF_ANALOG) | CRL(5, CNF_ANALOG);
    GPIOC->CRH = CRH(8, CNF_PPOUTPUT|MODE_NORMAL) | CRH(9, CNF_PPOUTPUT|MODE_NORMAL);
    GPIOD->CRL = 0;
    GPIOD->CRH = CRH(12, CNF_PPOUTPUT|MODE_NORMAL);
    GPIOE->CRL = CRL(7, CNF_PUDINPUT);
    GPIOE->CRH = CRH(8, CNF_PUDINPUT) | CRH(9, CNF_PUDINPUT) | CRH(10, CNF_PUDINPUT) | CRH(11, CNF_PUDINPUT) |
                 CRH(12, CNF_PUDINPUT) | CRH(13, CNF_PUDINPUT) | CRH(14, CNF_PUDINPUT) | CRH(15, CNF_PUDINPUT);
    // pullups & initial values
    GPIOA->ODR = 0;
    GPIOB->ODR = (1<<2) | (1<<10) | (1<<11) | (1<<12) | (1<<13) | (1<<14);
    GPIOC->ODR = 0;
    GPIOD->ODR = 0;
    GPIOE->ODR = (1<<7) | (1<<8) | (1<<9) | (1<<10) | (1<<11) | (1<<12) | (1<<13) | (1<<14) | (1<<15);
}


void iwdg_setup(){
    uint32_t tmout = 16000000;
    RCC->CSR |= RCC_CSR_LSION;
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;}
    IWDG->KR = IWDG_START;
    IWDG->KR = IWDG_WRITE_ACCESS;
    IWDG->PR = IWDG_PR_PR_1;
    IWDG->RLR = 1250;
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;}
    IWDG->KR = IWDG_REFRESH;
}

typedef struct{
    GPIO_TypeDef* port; // GPIOx or NULL if no such pin
    uint16_t pin;       // (1 << pin)
} pin_t;

// input pins (X0..X15)
static const pin_t IN[INMAX+1] = { // youbannye uskoglazye pidarasy! Ready to fuck their own mother to save kopeyka
    {GPIOB, 1<<13}, // X0 - PB13
    {GPIOB, 1<<14}, // X1 - PB14
    {GPIOB, 1<<11}, // X2 - PB11
    {GPIOB, 1<<12}, // X3 - PB12
    {GPIOE, 1<<15}, // X4 - PE15
    {GPIOB, 1<<10}, // X5 - PB10
    {GPIOE, 1<<13}, // X6 - PE13
    {GPIOE, 1<<14}, // X7 - PE14
    {NULL, 0},      // X8 - absent
    {NULL, 0},      // X9 - absent
    {GPIOE, 1<11},  // X10 - PE11
    {GPIOE, 1<<12}, // X11 - PE12
    {GPIOE, 1<<9},  // X12 - PE9
    {GPIOE, 1<<10}, // X13 - PE10
    {GPIOE, 1<<7},  // X14 - PE7
    {GPIOE, 1<<8},  // X15 - PE8
};

// output (relay) pins (Y0..Y15)
static const pin_t OUT[OUTMAX+1] = {
    {GPIOC, 1<<9},  // Y0 - PC9
    {GPIOC, 1<<8},  // Y1 - PC8
    {GPIOA, 1<<8},  // Y2 - PA8
    {GPIOA, 1<<0},  // Y3 - PA0
    {GPIOB, 1<<3},  // Y4 - PB3
    {GPIOD, 1<<12}, // Y5 - PD12
    {GPIOB, 1<<15}, // Y6 - PB15
    {GPIOA, 1<<7},  // Y7 - PA7
    {NULL, 0},      // Y8 - absent
    {NULL, 0},      // Y9 - absent
    {GPIOA, 1<<6},  // Y10 - PA6
    {GPIOA, 1<<2},  // Y11 - PA2
};

// bit 1 - input channel is working, 0 - no
uint32_t inchannels(){
    return 0b1111110011111111;
}
// bit 1 - input channel is working, 0 - no
uint32_t outchannels(){
    return 0b110011111111;
}

/**
 * @brief set_relay - turn on/off relay `Nch` (if Nch > OUTMAX - all)
 * @param Nch - single relay channel No or >Ymax to set/reset all relays
 * @param val - value to set/reset/change
 * @return TRUE if OK, -1 if `Nch` is wrong
 */
int set_relay(uint8_t Nch, uint32_t val){
    int chpin(uint8_t N, uint32_t v){
        const pin_t *cur = &OUT[N];
        if(NULL == cur->port) return -1;
        if(v) pin_set(cur->port, cur->pin);
        else pin_clear(cur->port, cur->pin);
        return TRUE;
    }
    if(Nch > OUTMAX){ // all
        uint32_t mask = 1;
        for(uint8_t i = 0; i <= OUTMAX; ++i, mask <<= 1){
            chpin(i, val & mask);
        }
        return TRUE;
    }
    return chpin(Nch, val);
}

static int readpins(uint8_t Nch, const pin_t *pins, uint8_t max){
    int gpin(uint8_t N){
        const pin_t *cur = &pins[N];
        if(NULL == cur->port) return -1;
        return pin_read(cur->port, cur->pin);
    }
    if(Nch > max){ // all
        int val = 0;
        for(int i = max; i > -1; --i){
            val <<= 1;
            int p = gpin(i);
            if(p > -1) val |= gpin(i);
        }
usart_send("readpins, val="); usart_send(i2str(val)); newline();
        return val;
    }
    return gpin(Nch);
}

/**
 * @brief get_relay - get `Nch` relay state (if Nch > OUTMAX - all)
 * @param Nch - single relay channel No or <0 for all
 * @return current state or -1 if `Nch` is wrong
 */
int get_relay(uint8_t Nch){
    return readpins(Nch, OUT, OUTMAX);
}

/**
 * @brief get_esw - get input `Nch` state (or all if Nch > INMAX)
 * @param Nch - channel number or -1 for all
 * @return ESW state or -1 if `Nch` is wrong
 */
int get_esw(uint8_t Nch){
    return readpins(Nch, IN, INMAX);
}

static uint32_t ESW_ab_values = 0; // current anti-bounce values of ESW
static uint32_t lastET[INMAX+1] = {0}; // last changing time

// anti-bouce process esw
void proc_esw(){
    uint32_t mask = 1, oldesw = ESW_ab_values;
    for(uint8_t i = 0; i <= OUTMAX; ++i, mask <<= 1){
        if(Tms - lastET[i] < the_conf.bouncetime) continue;
        if(NULL == IN[i].port) continue;
        uint32_t now = pin_read(IN[i].port, IN[i].pin);
        if(now) ESW_ab_values |= mask;
        else ESW_ab_values &= ~mask;
        lastET[i] = Tms;
    }
    if(oldesw != ESW_ab_values){
        usart_send("esw="); usart_send(u2str(ESW_ab_values)); newline();
        CAN_message msg = {.ID = the_conf.CANIDout, .length = 8};
        msg.data[0] = CMD_GETESW;
        *((uint32_t*)(&msg.data[4])) = ESW_ab_values;
        uint32_t Tstart = Tms;
        while(Tms - Tstart < SEND_TIMEOUT_MS){
            if(CAN_OK == CAN_send(&msg)) return;
        }
    }
}

// get all anti-bounce ESW values
uint32_t get_ab_esw(){
    return ESW_ab_values;
}
