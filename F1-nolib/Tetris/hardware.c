/*
 * This file is part of the TETRIS project.
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

#include "hardware.h"
#include "usb.h"

uint8_t transfer_done = 0; // ==1 when DMA transfer ready
// buffer for DMA
static uint32_t dmabuf[SCREEN_WIDTH];

void iwdg_setup(){
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

static inline void gpio_setup(){
    // Enable clocks to the GPIO subsystems, turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    // turn off JTAG (PB3/4 is in use)
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
    // turn off USB pullup
    USBPU_OFF();
    // Set led as opendrain output
    GPIOC->CRH = CRH(13, CNF_ODOUTPUT|MODE_SLOW);
    // SCREEN color PINs: PA0..PA5
    GPIOA->CRL = CRL(0, CNF_PPOUTPUT|MODE_FAST) | CRL(1, CNF_PPOUTPUT|MODE_FAST) | CRL(2, CNF_PPOUTPUT|MODE_FAST) |
                 CRL(3, CNF_PPOUTPUT|MODE_FAST) | CRL(4, CNF_PPOUTPUT|MODE_FAST) | CRL(5, CNF_PPOUTPUT|MODE_FAST);
    // USB pullup - opendrain output
    GPIOA->CRH = CRH(15, CNF_PPOUTPUT|MODE_SLOW);
    // A..D, LAT
    GPIOB->CRL = CRL(4, CNF_PPOUTPUT|MODE_FAST) | CRL(5, CNF_PPOUTPUT|MODE_FAST) | CRL(6, CNF_PPOUTPUT|MODE_FAST) | CRL(7, CNF_PPOUTPUT|MODE_FAST);
    // nOE: PB4..PB9; Joystick: PB10..PB15 (pullup input)
    GPIOB->ODR = 0b111111 << 10; // pullup
    GPIOB->CRH = CRH(8, CNF_PPOUTPUT|MODE_FAST) | CRH(9, CNF_PPOUTPUT|MODE_FAST) |
                CRH(10, CNF_PUDINPUT|MODE_INPUT) | CRH(11, CNF_PUDINPUT|MODE_INPUT) | CRH(12, CNF_PUDINPUT|MODE_INPUT) |
                CRH(13, CNF_PUDINPUT|MODE_INPUT) | CRH(14, CNF_PUDINPUT|MODE_INPUT) | CRH(15, CNF_PUDINPUT|MODE_INPUT);
}

// setup timer3 for DMA transfers & SCK @ TIM3_CH1 (PA6)
static inline void tim_setup(){
    SCRN_DISBL(); // turn off output
    GPIOA->CRL |= CRL(6, CNF_AFPP|MODE_FAST);
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    // PCS=1 don't work;
    TIM3->PSC = 9; // 7.2MHz
    TIM3->ARR = 7;
    TIM3->CCMR1 = TIM_CCMR1_OC1M; // PWM mode 2 (inactive->active)
    //TIM3->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1; // PWM mode 1 (active->inactive)
    TIM3->CCER = TIM_CCER_CC1E; // output 1 enable
    TIM3->DIER = TIM_DIER_UDE; // enable DMA requests
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    // memsize 32bit, periphsize 32bit, memincr, mem2periph, full transfer interrupt
    DMA1_Channel3->CCR = DMA_CCR_PSIZE_1 | DMA_CCR_MSIZE_1 | DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE;
    DMA1_Channel3->CMAR = (uint32_t)dmabuf;
    DMA1_Channel3->CPAR = (uint32_t)&GPIOA->BSRR;
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
}

void hw_setup(){
    gpio_setup();
    tim_setup();
}

void stopTIMDMA(){
    DMA1_Channel3->CCR &= ~DMA_CCR_EN; // stop DMA
    TIM3->CR1 = 0; // turn off timer
}

static inline uint8_t getcolrvals(uint8_t colr, uint8_t Ntick){
    uint8_t result = 0; // bits: 0-R, 1-G, 2-B
#ifdef SCREEN_IS_NEGATIVE
    if(colr >> 5 <= Ntick) result = 1;
    colr &= 0x1f;
    if(colr >> 2 <= Ntick) result |= 2;
    colr &= 3;
    if(colr) colr = (colr << 1) + 1;
    if(colr <= Ntick) result |= 4;
#else
    if(colr >> 5 > Ntick) result = 1;
    colr &= 0x1f;
    if(colr >> 2 > Ntick) result |= 2;
    colr &= 3;
    if(colr) colr = (colr << 1) + 1;
    if(colr > Ntick) result |= 4;
#endif
    return result;
}
/**
 * @brief ConvertScreenBuf - convert creen buffer into DMA buffer
 * @param sbuf - screen buffer
 * @param Nrow - current row number
 * @param Ntick - tick number (0..7) for quasi-PWM colors
 */
void ConvertScreenBuf(uint8_t *sbuf, uint8_t Nrow,  uint8_t Ntick){ // __attribute__((unused))
    //USB_send("ConvertScreenBuf\n");
    uint8_t *_1st = &sbuf[SCREEN_WIDTH*Nrow], *_2nd = _1st + SCREEN_WIDTH*NBLOCKS; // first and second lines
    for(int i = 0; i < SCREEN_WIDTH; ++i){ // ~50us for 64 pixels
        uint8_t pinsset = getcolrvals(_1st[i], Ntick);
        pinsset |= getcolrvals(_2nd[i], Ntick) << 3;
        //dmabuf[i] = (i%2) ? ((COLR_pin<<16) | i) : (0b100010);
        dmabuf[i] = (COLR_pin << 16) | pinsset; // All BR are 1, BS depending on color
    }
}


static uint8_t blknum_curr = 0;
/**
 * @brief TIM_DMA_transfer - start DMA transfer
 * @param buf - buffer with data for GPIOA->BSRR
 * @param blknum - number of active block
 */
void TIM_DMA_transfer(uint8_t blknum){
    //USB_send("TIM_DMA_transfer\n");
    TIM3->CR1 = 0; // turn off timer
    transfer_done = 0;
    // set first pixel color
    //COLR_port->BSRR = dmabuf[0];
    DMA1_Channel3->CNDTR = SCREEN_WIDTH;
    DMA1_Channel3->CCR |= DMA_CCR_EN; // start DMA
    TIM3->CCR1 = 4; // 50% PWM
    TIM3->CR1 = TIM_CR1_CEN; // turn on timer
    blknum_curr = blknum;
}

// DMA transfer complete - stop transfer
void dma1_channel3_isr(){
    //TIM3->CR1 |= TIM_CR1_OPM; // set one pulse mode to turn off timer after last CLK pulse
    //TIM3->SR = 0;
    TIM3->CR1 = 0;
    DMA1_Channel3->CCR &= ~DMA_CCR_EN; // stop DMA
    //while(!(TIM3->SR & TIM_SR_UIF)); // wait for last pulse
    SCRN_DISBL(); // clear main output
    ADDR_port->ODR = (ADDR_port->ODR & ~(ADDR_pin)) | (blknum_curr << ADDR_roll); // set address
    SET(LAT); // activate latch
    DMA1->IFCR = DMA_IFCR_CGIF3;
    transfer_done = 1;
    CLEAR(LAT);
    SCRN_ENBL(); // activate main output
    process_screen();
    //USB_send("transfer done\n");
}

