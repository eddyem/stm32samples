/*
 * This file is part of the ir-allsky project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "BMP280.h"
#include "hardware.h"

static bme280_t environment; // current measurements

#ifndef EBUG
TRUE_INLINE void iwdg_setup(){
    uint32_t tmout = 16000000;
    /* Enable the peripheral clock RTC */
    /* (1) Enable the LSI (40kHz) */
    /* (2) Wait while it is not ready */
    RCC->CSR |= RCC_CSR_LSION; /* (1) */
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;} /* (2) */
    /* Configure IWDG */
    /* (1) Activate IWDG (not needed if done in option bytes) */
    /* (2) Enable write access to IWDG registers */
    /* (3) Set prescaler by 64 (1.6ms for each tick) */
    /* (4) Set reload value to have a rollover each 2s */
    /* (5) Check if flags are reset */
    /* (6) Refresh counter */
    IWDG->KR = IWDG_START; /* (1) */
    IWDG->KR = IWDG_WRITE_ACCESS; /* (2) */
    IWDG->PR = IWDG_PR_PR_1; /* (3) */
    IWDG->RLR = 1250; /* (4) */
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;} /* (5) */
    IWDG->KR = IWDG_REFRESH; /* (6) */
}
#endif

TRUE_INLINE void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN; // for USART and LEDs
    for(int i = 0; i < 10000; ++i) nop();
    // USB - alternate function 14 @ pins PA11/PA12; SWD - AF0 @PA13/14
    GPIOA->AFR[1] = AFRf(14, 11) | AFRf(14, 12);
    GPIOA->MODER = MODER_AF(11) | MODER_AF(12) | MODER_AF(13) | MODER_AF(14) | MODER_O(15);
    GPIOB->MODER = MODER_O(0) | MODER_O(1) | MODER_O(2);
    pin_set(GPIOB, 1<<1);
    SPI_CS_1();

}

void hw_setup(){
    gpio_setup();
#ifndef EBUG
    iwdg_setup();
#endif
}

int bme_init(){
    BMP280_setup();
    if(!BMP280_init()) return 0;
    if(!BMP280_start()) return 0;
    return 1;
}

// process sensor and make measurements
void bme_process(){
    static uint32_t Tmeas = 0;
    BMP280_status s = BMP280_get_status();
    if(s != BMP280_NOTINIT){
        if(s == BMP280_ERR) BMP280_init();
        else{
            BMP280_process();
            s = BMP280_get_status(); // refresh status after processing
            float Temperature, Pressure, Humidity;
            if(s == BMP280_RDY && BMP280_getdata(&Temperature, &Pressure, &Humidity)){
                environment.Tdew = Tdew(Temperature, Humidity);
                environment.T = Temperature;
                environment.P = Pressure;
                environment.H = Humidity;
                environment.Tmeas = Tms;
            }
            if(Tms - Tmeas > ENV_MEAS_PERIOD-1){
                if(BMP280_start()) Tmeas = Tms;
            }
        }
    }
}

int get_environment(bme280_t *env){
    if(!env) return 0;
    *env = environment;
    if(Tms - environment.Tmeas > 2*ENV_MEAS_PERIOD) return 0; // data may be wrong
    return 1; // good data
}
