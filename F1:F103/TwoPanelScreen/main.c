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

#include "adcrandom.h"
#include "fonts.h"
#include "hardware.h"
#include "proto.h"
#include "screen.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(){
    ++Tms;
}

int main(){
    uint32_t lastT = 0;

    sysreset();
    StartHSE();
    SysTick_Config(72000);
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    hw_setup();
    USBPU_OFF();
    adc_setup();
    USB_setup();
    //iwdg_setup();
    USBPU_ON();

    setBGcolor(COLOR_BLACK);
    setFGcolor(COLOR_WHITE);
    choose_font(FONTN8);
    ClearScreen();
    ScreenON();

    while(1){
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        IWDG->KR = IWDG_REFRESH;

        usb_proc();
        proto_process();

        IWDG->KR = IWDG_REFRESH;
    }
    return 0;
}
