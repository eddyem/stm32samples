/*
 * This file is part of the DHT22 project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "dht.h"

// max amount of bits (including starting)
#define NmeasurementMax     (45)
// min length of start pulse
#define STARTLEN            (150)
// start pulse max position
#define STARTMAXPOS         (4)
// border between zero and one: <border is zero, >border is 1
#define ZEROBORDER          (100)

//#define EBUG

#ifdef EBUG
#include "usb.h"
#include "proto.h"
#define DBG(x)  do{USB_send(x);}while(0)
#else
#define DBG(x)
#endif

static DHT_state dhtstate = DHT_SLEEP;
static uint32_t Tstart = 0; // time of start pulse
uint8_t DHT_tim_overflow = 0;
static uint8_t CC1array[NmeasurementMax];
static uint16_t Humidity;
static int16_t Temperature;

DHT_state DHT_getstate(){return dhtstate;}

void DHT_getdata(uint16_t *Hum, int16_t *T){
    *Hum = Humidity;
    *T = Temperature;
    dhtstate = DHT_SLEEP;
}

// TIM1_CH1 & TIM1_CH2 are used for total len & zero len pulse measurement
// T1ch1 - DMA1Ch2
void DHT_pinsetup(){
    TIM1->CR1 = 0;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_TIM1EN | RCC_APB2ENR_AFIOEN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    pin_set(GPIOA, 1<<8);
    // PA8 as opendrain output
    GPIOA->CRH = (GPIOA->CRH & ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8)) |
            CRH(8, CNF_ODOUTPUT | MODE_FAST);
    TIM1->PSC = 71; // 1MHz
    TIM1->ARR = 200; // 200mks - max time for measurement
    // dma->mem, 16bit->8bit,
    DMA1_Channel2->CCR = DMA_CCR_MINC | DMA_CCR_PSIZE_0 | DMA_CCR_TCIE;
    DMA1_Channel2->CPAR = (uint32_t)&TIM1->CCR1;
    DMA1_Channel2->CMAR = (uint32_t)CC1array;
    NVIC_EnableIRQ(TIM1_UP_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    dhtstate = DHT_SLEEP;
}

// start measurement
static void DHT_startmeas(uint32_t Tms){
    DBG("DHT_startmeas()\n");
    pin_set(GPIOA, 1<<8);
    dhtstate = DHT_MEASURING;
    Tstart = Tms;
    // CC1 is input, IC1@TI1, IC2@TI1
    TIM1->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_1;
    // CC1 active on falling edge, CC2 - on rising, enable capture on both channels
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC1P;
    // TS=0b101 (trigger input TI1FP1, falling edge), SMS=0b100 (slave mode in reset mode)
    TIM1->SMCR = TIM_SMCR_TS_2 | TIM_SMCR_TS_0 | TIM_SMCR_SMS_2;
    // enable update interrupt & DMA events generation
    TIM1->DIER = TIM_DIER_UIE | TIM_DIER_CC1DE;
    // prepare DMA
    DMA1->IFCR = DMA_IFCR_CGIF2; // clear all flags
    DMA1_Channel2->CNDTR = NmeasurementMax;
    // enable DMA channel
    DMA1_Channel2->CCR |= DMA_CCR_EN;
    // turn it on in, update generation only by overflow
    TIM1->CR1 = TIM_CR1_CEN | TIM_CR1_URS;
}

static uint8_t getnext8(int startidx){
    uint8_t *p = CC1array + startidx;
    uint8_t x = 0;
    for(int i = 0; i < 8; ++i){
        x <<= 1;
        if(*p++ > ZEROBORDER) x |= 1;
    }
    return x;
}

static void DHT_stopmeas(){
    DBG("DHT_stopmeas()\n");
    if(DHT_tim_overflow){
        DBG("Overflow="); DBG(u2str(DHT_tim_overflow)); DBG("\n");
    }
    TIM1->CR1 = 0;
    TIM1->SMCR = 0;
    // disnable DMA channel
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DHT_tim_overflow = 0;
    DBG("ctr="); DBG(u2str(DMA1_Channel2->CNDTR)); DBG("\nn\tCC1\n");
    int nmax = NmeasurementMax - DMA1_Channel2->CNDTR;
    for(int i = 0; i < nmax; ++i){
        DBG(u2str(i)); DBG("\t"); DBG(u2str(CC1array[i])); DBG("\n");
    }
    dhtstate = DHT_ERROR;
    // find start pulse
    int start = -1;
    for(int i = 0; i < STARTMAXPOS; ++i){
        if(CC1array[i] > STARTLEN){
            start = i + 1;
            break;
        }
    }
    if(start == -1){
        DBG("No start pulse\n");
        return;
    }
    if(nmax - start < 40){ // no data & start pulse
        DBG("Insufficient length\n");
        return;
    }
    uint8_t x[5];
    for(int i = 0; i < 5; ++i, start += 8){
        x[i] = getnext8(start);
        DBG(u2str(getnext8(start))); DBG(" ");
    }
    uint8_t sum = x[0];
    for(int i = 1; i < 4; ++i) sum += x[i];
    if(sum != x[4]){
        DBG("Checksum failed\n");
        DBG("sum="); DBG(u2str(sum));
        DBG(", ch="); DBG(u2str(x[4])); DBG("\n");
        return;
    }
#ifndef DHT11
    Humidity = (x[0] << 8) | x[1];
    Temperature = ((x[2]&0x7f) << 8) | x[3];
#else
    Humidity = x[0];
    Temperature = x[2]&0x7f;
#endif
    if(x[2] & 0x80) Temperature = -Temperature;
    dhtstate = DHT_GOTRESULT;
}

// processing, Tms - current time in milliseconds
void DHT_process(uint32_t Tms){
    switch(dhtstate){
        case DHT_RESETTING:
            if(Tms - Tstart > DHT_RESETPULSE_LEN) DHT_startmeas(Tms);
        break;
        case DHT_MEASURING:
            if(Tms - Tstart > DHT_MEASUR_LEN || DHT_tim_overflow)
                DHT_stopmeas();
        break;
        default:
            return;
    }
}

/**
 * @brief DHT_start - start measurement process
 */
int DHT_start(uint32_t Tms){
    if(dhtstate != DHT_SLEEP && dhtstate != DHT_ERROR) return 0;
    DBG("DHT_start()\n");
    DHT_tim_overflow = 0;
    // setup pin into opendrain output mode
//    GPIOA->CRH = (GPIOA->CRH & ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8)) |
//            CRH(8, CNF_ODOUTPUT | MODE_NORMAL);
    pin_clear(GPIOA, 1<<8);
    Tstart = Tms;
    dhtstate = DHT_RESETTING;
    return 1;
}

void dma1_channel2_isr(){
    DMA1->IFCR = DMA_IFCR_CGIF2;
    TIM1->CR1 = 0;
    DHT_tim_overflow = 1;
}

// update IRQ: no data on input
void tim1_up_isr(){
    TIM1->SR = 0;
    TIM1->CR1 = 0;
    TIM1->DIER = 0;
    TIM1->SMCR = 0;
    DHT_tim_overflow = 2;
}

