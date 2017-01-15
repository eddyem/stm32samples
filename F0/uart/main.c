/*
 * main.c
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "stm32f0.h"
#include "usart.h"

static uint32_t stk_ctr = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    pin_toggle(GPIOB, GPIO3);
    ++stk_ctr;
}

/*
 * Set up timer to fire every x milliseconds
 * This is a unusual usage of systick, be very careful with the 24bit range
 * of the systick counter!  You can range from 1 to 2796ms with this.
 */
static void systick_setup(int xms){
    static int curms = 0;
    if(curms == xms) return;
    // 6MHz
    systick_set_clocksource(STK_CSR_CLKSOURCE_EXT);
    /* clear counter so it starts right away */
    STK_CVR = 0;

    systick_set_reload(6000 * xms);
    systick_counter_enable();
    systick_interrupt_enable();
    curms = xms;
}

/* set STM32 to clock by 48MHz from HSI oscillator */
static void clock_setup(void){
    rcc_clock_setup_in_hsi_out_48mhz();
    //rcc_clock_setup_in_hse_8mhz_out_48mhz();

    /* Enable clocks to the GPIO subsystems (A&B) */
    RCC_AHBENR = RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
}

static void gpio_setup(void){
    // Set green led (PB3) as output
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
}

/**
 * reverce line
 */
char *rline(char *in, int L){
    static char out[UARTBUFSZ];
    if(L > UARTBUFSZ - 1) return in;
    char *optr = out, *iptr = &in[L-1];
    for(; L > 0; --L) *optr++ = *iptr--;
    *optr = '\n';
    return out;
}


int main(void){
    clock_setup();
    gpio_setup();
    usart2_setup();

    systick_setup(500); // 1Hz in normal mode

    const char dummy[] = "dummy text\n", err[] = "Error!\n";
    char *txt;
    int L = 0, dummyctr = 0;
    uint32_t ostctr = 0;

    while (1){
        if(L){ // there's something to send
            if(ALL_OK == usart2_send(txt, L)){
                L = 0;
            }else
                usart2_send_blocking(err, sizeof(err));
        }else if(ostctr != stk_ctr){
            ostctr = stk_ctr;
            if(++dummyctr > 10){ // once per 5 seconds
                dummyctr = 0;
                usart2_send(dummy, sizeof(dummy));
            }
        }
        if(usart2rx()){ // data received
            L = usart2_getline(&txt);
            if(L){
                txt = rline(txt, L);
                ++L; // for trailing '\n'
            }
        }
    }
    return 0;
}

