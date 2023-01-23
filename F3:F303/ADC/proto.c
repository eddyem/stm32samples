/*
 * This file is part of the adc project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f3.h>
#include <string.h>

#include "adc.h"
#include "strfunc.h"
#include "usb.h"
#include "version.inc"

//#define LOCBUFFSZ       (32)
// local buffer for I2C data to send
//static uint8_t locBuffer[LOCBUFFSZ];
extern volatile uint32_t Tms;
volatile uint8_t ADCmon = 0;

const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/I2C_scan build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "A - get ADC values\n"
        "dx - change DAC current value to x\n"
        "m - monitor ADC on/off\n"
        "t - MCU temperature\n"
        "T - print current Tms\n"
        "v - Vdd\n"
;
/*
// read N numbers from buf, @return 0 if wrong or none
TRUE_INLINE uint16_t readNnumbers(const char *buf){
    uint32_t D;
    const char *nxt;
    uint16_t N = 0;
    while((nxt = getnum(buf, &D)) && nxt != buf && N < LOCBUFFSZ){
        buf = nxt;
        locBuffer[N++] = (uint8_t) D&0xff;
        USND("add byte: "); USND(uhex2str(D&0xff)); USND("\n");
    }
    USND("Send "); USND(u2str(N)); USND(" bytes\n");
    return N;
}
*/

TRUE_INLINE const char *DAC_chval(const char *buf){
    uint32_t D;
    const char *nxt = getnum(buf, &D);
    if(!nxt || nxt == buf || D > 4095) return "Wrong DAC amplitude\n";
    DAC1->DHR12R1 = D;
    return "OK";
}

void printADCvals(){
    USB_sendstr("AIN0: "); USB_sendstr(u2str(getADCval(ADC_AIN0)));
    USB_sendstr(" ("); USB_sendstr(float2str(getADCvoltage(ADC_AIN0), 2));
    USB_sendstr(" V)\nAIN1: "); USB_sendstr(u2str(getADCval(ADC_AIN1)));
    USB_sendstr(" ("); USB_sendstr(float2str(getADCvoltage(ADC_AIN1), 2));
    USB_sendstr(" V)\nAIN5: "); USB_sendstr(u2str(getADCval(ADC_AIN5)));
    USB_sendstr(" ("); USB_sendstr(float2str(getADCvoltage(ADC_AIN5), 2));
    USB_sendstr(" V)\nTS: ");
    USB_sendstr(u2str(getADCval(ADC_TS)));
    USB_sendstr("\nVREF: ");
    USB_sendstr(u2str(getADCval(ADC_VREF)));
    newline();
}

const char *parse_cmd(const char *buf){
    // "long" commands
    switch(*buf){
        case 'd':
            return DAC_chval(buf + 1);
        break;
    }
    // "short" commands
    if(buf[1]) return buf; // echo wrong data
    switch(*buf){
        case 'A':
            printADCvals();
        break;
        case 'm':
            ADCmon = !ADCmon;
            USB_sendstr("Monitoring is ");
            if(ADCmon) USB_sendstr("on\n");
            else USB_sendstr("off\n");
        break;
        case 't':
            return float2str(getMCUtemp(), 2);
        break;
        case 'T':
            USB_sendstr("T=");
            USB_sendstr(u2str(Tms));
            newline();
        break;
        case 'v':
            return float2str(getVdd(), 2);
        break;
        default: // help
            USB_sendstr(helpstring);
        break;
    }
    return NULL;
}
