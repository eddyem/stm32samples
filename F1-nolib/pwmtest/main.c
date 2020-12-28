/*
 * This file is part of the pwmtest project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "proto.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

// usb getline
char *get_USB(){
    static char tmpbuf[512], *curptr = tmpbuf;
    static int rest = 511;
    uint8_t x = USB_receive((uint8_t*)curptr);
    curptr[x] = 0;
    if(!x) return NULL;
    if(curptr[x-1] == '\n'){
        curptr = tmpbuf;
        rest = 511;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = 511;
    }
    return NULL;
}

#ifndef HIGHSPEED
void tim1_cc_isr(){
    TIM1->SR = 0;
    TIM1->CCR1 = 0;
    TIM1->DIER = 0;
}
#endif

static int chkctr(){
    static int LED_ctr = 0;
    if(++LED_ctr > LEDS_NUM){
        TIM1->CR1 |= TIM_CR1_OPM;
        // THIS IS A DIRTY HACK! IT WORKS ONLY @ HIGH SPEEDS, WHEN THERE'S NO TIME TO GO INTO IRQ
        // On small frequencies comment this line and allow CC1 IRQ
#ifdef HIGHSPEED
        TIM1->CCR1 = 0;
#else
        TIM1->DIER = TIM_DIER_CC1IE;
#endif
        LED_ctr = 0;
        return 0;
    }
    return 1;
}

#define _1  (6)
#define _0  (3)

// R -> B (GRB)
static uint8_t dmabuff[] = {
    _0,_0,_0,_0,_0,_0,_0,_0,
    _0,_0,_0,_0,_0,_0,_0,_0,
    _0,_0,_0,_0,_0,_0,_0,_0,
    _0,_0,_0,_0,_1,_1,_1,_1,
    _0,_0,_0,_0,_0,_0,_0,_0,
    _0,_0,_0,_0,_0,_0,_0,_0,
};

void dma1_channel5_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF5){ // transfer complete
        if(chkctr()){
            for(int i = 28; i < 32; ++i) // invert G in second half
                if(dmabuff[i] == _0) dmabuff[i] = _1;
                else dmabuff[i] = _0;
        }
    }else if(DMA1->ISR & DMA_ISR_HTIF5){ // half transfer complete
        if(chkctr()){
            for(int i = 20; i < 24; ++i) // invert B in first half
                if(dmabuff[i] == _0) dmabuff[i] = _1;
                else dmabuff[i] = _0;
        }
    }
    DMA1->IFCR = DMA_IFCR_CGIF5;
}

static void sendone(){
    static uint8_t ctr = 7;
    TIM1->CR1 = 0; // stop
    DMA1_Channel5->CCR &= ~DMA_CCR_EN; // disable DMA to reconfigure
    // change values of dmabuff: G in first half & R in second
    if(--ctr == 1) ctr = 6; // 6..2
    for(int i = 0; i < 8; ++i){
        uint8_t val = (i == ctr) ? _1 : _0;
        dmabuff[i] = val;
        dmabuff[32+i] = val;
    }
    TIM1->DIER = TIM_DIER_UDE; // enable DMA requests
    DMA1->IFCR = DMA_IFCR_CGIF5;
    DMA1_Channel5->CNDTR = 48;
    DMA1_Channel5->CMAR = (uint32_t)dmabuff;
    DMA1_Channel5->CCR |= DMA_CCR_EN; // start DMA
    TIM1->CR1 = TIM_CR1_CEN | TIM_CR1_URS;// | TIM_CR1_DIR;
}

int main(void){
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    hw_setup();
    SysTick_Config(72000);

    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    USBPU_OFF();
    USB_setup();
    iwdg_setup();
    USBPU_ON();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
            sendone();
        }
        usb_proc();
        char *txt, *ans;
        if((txt = get_USB())){
            IWDG->KR = IWDG_REFRESH;
            ans = (char*)parse_cmd(txt);
            if(ans){
                uint16_t l = 0; char *p = ans;
                while(*p++) l++;
                USB_send(ans, l);
            }
        }
    }
    return 0;
}
