/*
 * This file is part of the I2Cscan project.
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

#include "i2cscan.h"
#include "i2c.h"
#include "hardware.h"
#include "proto.h"
#include "usb.h"
#include "usb_lib.h"

#define USBBUFSZ    127

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
	char buf[256];
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    hw_setup();
    SysTick_Config(72000);

    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    USBPU_OFF();
    USB_setup();
    i2c_setup();
    iwdg_setup();
    USBPU_ON();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        if(scanmode){ // get next address & print it
            uint8_t addr;
            int ok = i2c_scan_next_addr(&addr);
            if(addr == I2C_ADDREND) USB_sendstr("Scan ends\n");
            else if(ok){
                USB_sendstr(u2hexstr(addr));
                USB_sendstr(" ("); USB_sendstr(u2str(addr));
                USB_sendstr(") - found device\n");
            }
        }
        if(USB_receivestr(buf, 255)){
            IWDG->KR = IWDG_REFRESH;
            char *ans = (char*)parse_cmd(buf);
            if(ans){
                uint16_t l = 0; char *p = ans;
                while(*p++) l++;
                USB_sendstr(ans);
            }
        }
    }
    return 0;
}
