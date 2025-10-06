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
#include "mlxproc.h"
#include "mlx90640.h"

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
    // USB - alternate function 14 @ pins PA11/PA12; SWD - AF0 @PA13/14; USB pullup - PA15
    // PA6 - PWM for external heater (TIM3_CH1 or TIM16_CH1); PA7 - PWM propto (humidity - 50%)
    GPIOA->AFR[0] = AFRf(2, 6) | AFRf(2, 7);
    GPIOA->AFR[1] = AFRf(14, 11) | AFRf(14, 12);
    GPIOA->MODER = MODER_AI(0) | MODER_AI(1)  | MODER_AI(4)  | MODER_AI(5)  | MODER_AF(6)  |
                   MODER_AF(7) | MODER_AF(11) | MODER_AF(12) | MODER_AF(13) | MODER_AF(14) | MODER_O(15);
    // PB0 - PWM propto Text (<=20 - 0%, >=30 - 100%), PB1 - PWM propto (Text-Tsky) (<=-5 - 0%, >=+35 - 100%) PB2 - SPI_CS
    GPIOB->AFR[0] = AFRf(2, 0) | AFRf(2, 1);
    GPIOB->MODER = MODER_AF(0) | MODER_AF(1) | MODER_O(2);
    pin_set(GPIOB, 1<<1);
    SPI_CS_1();
}

// setup PWM (TIM3_CH1) for external heater management over optocoupler (active level - 0)
TRUE_INLINE void pwm_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->CR1 = TIM_CR1_ARPE;
    TIM3->PSC = 71; // 72M/72 = 1MHz; PWMfreq=1M/100=10kHz
    // PWM mode 1 (active -> inactive), enable buffering
    TIM3->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE |
                  TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;
    TIM3->CCMR2 = TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3PE |
                  TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4PE;
    TIM3->CCR1 = 0; TIM3->CCR2 = 0; TIM3->CCR3 = 0; TIM3->CCR4 = 0;
    TIM3->ARR = PWM_CCR_MAX-1; // 8bit PWM from 0 to 99
    TIM3->BDTR |= TIM_BDTR_MOE; // enable main output
    // enable PWM output, all outputs active - low
    TIM3->CCER = TIM_CCER_CC1P | TIM_CCER_CC1E | TIM_CCER_CC2P | TIM_CCER_CC2E |
                 TIM_CCER_CC3P | TIM_CCER_CC3E | TIM_CCER_CC4P | TIM_CCER_CC4E;
    TIM3->CR1 |= TIM_CR1_CEN; // run timer
}

// change PWM value in percents; return 0 if `val` is bad or `ch` not 0..3
int setPWM(uint8_t ch, uint8_t val){
    if(ch > 3 || val > PWM_CCR_MAX) return 0;
    volatile uint32_t *CCRs = &(TIM3->CCR1);
    CCRs[ch] = val;
    return 1;
}

void hw_setup(){
    gpio_setup();
    pwm_setup();
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

// calculate mean sky temperature
static float Tsky(){
    int nmeas = 0;
    float Tacc = 0.f;
    for(int n = 0; n < N_SENSORS; ++n){ // sum all sensors
        float *im = mlx_getimage(n);
        if(!im) continue;
        for(int i = 0; i < MLX_PIXNO; ++i){
            Tacc += im[i]; ++nmeas;
        }
    }
    if(nmeas < MLX_PIXNO) return +1000.; // all very bad: no sensors found?
    return (Tacc / (float)nmeas);
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
                // set PWM duty propto humidity
                float h = (Humidity - 50.f) * 2.f;
                if(h < 0.f) h = 0.f; else if(h > 100.f) h = 100.f;
                setPWM(PWM_CH_HUMIDITY, (uint8_t)h);
                environment.Tmeas = Tms;
                // set PWM duty propto external T
                float t = (Temperature + 20.f) * 2.f;
                if(t < 0.f) t = 0.f; else if(t > 100.f) t = 100.f;
                setPWM(PWM_CH_TEXT, (uint8_t)t);
                // set PWM propto skyqual
                environment.Tsky = Tsky();
                float q = (35.f - Temperature + environment.Tsky) * 2.5f;
                if(q < 0.f) q = 0.f; else if(q > 100.f) q = 100.f;
                setPWM(PWM_CH_TSKY, (uint8_t)q);
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
