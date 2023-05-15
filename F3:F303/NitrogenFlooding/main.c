/*
 * This file is part of the nitrogen project.
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

#include "adc.h"
#include "BMP280.h"
#include "buttons.h"
//#include "can.h"
//#include "flash.h"
#include "hardware.h"
#include "i2c.h"
#include "ili9341.h"
#include "indication.h"
#include "proto.h"
#include "screen.h"
#include "strfunc.h"
#include "usb.h"

#define MAXSTRLEN    RBINSZ

volatile uint32_t Tms = 0;
void sys_tick_handler(void){
    ++Tms;
}

// global BME data and last measured time
float Temperature, Pressure, Humidity, Dewpoint;
uint32_t Sens_measured_time = 100000;
uint16_t ADCvals[NUMBER_OF_ADC_CHANNELS]; // raw ADC values

// get BME and sensors data for different calculations
static void getsensors(){
    // turn ON ADC voltage
    ADCON(1);
    BMP280_status s = BMP280_get_status();
    if(s == BMP280_NOTINIT || s == BMP280_ERR) BMP280_init();
    else if(s == BMP280_RDY && BMP280_getdata(&Temperature, &Pressure, &Humidity)){
        Dewpoint = Tdew(Temperature, Humidity);
#if 0
        USB_sendstr("T="); USB_sendstr(float2str(Temperature, 2)); USB_sendstr("\nP=");
        USB_sendstr(float2str(Pressure, 1));
        USB_sendstr("\nPmm="); USB_sendstr(float2str(Pressure * 0.00750062f, 1));
        USB_sendstr("\nH="); USB_sendstr(float2str(Humidity, 1));
        USB_sendstr("\nTdew="); USB_sendstr(float2str(Dewpoint, 1));
        newline();
#endif
    }
    // here we should collect and process ADC data - get level etc
    for(uint32_t i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i){
        ADCvals[i] = getADCval(i);
#if 0
        USB_sendstr("ADC"); USB_sendstr(u2str(i)); USB_putbyte('=');
        USB_sendstr(u2str(ADCvals[i])); newline();
#endif
    }
    ADCON(0); // and turn off ADC
    if(!BMP280_start()){ BMP280_init(); BMP280_start(); }
}

// SPI setup done in `screen.h`
int main(void){
    char inbuff[MAXSTRLEN+1];
    if(StartHSE()){
        SysTick_Config((uint32_t)72000); // 1ms
    }else{
        StartHSI();
        SysTick_Config((uint32_t)48000); // 1ms
    }
    USBPU_OFF(); // make a reconnection
    //flashstorage_init();
    hw_setup();
    USB_setup();
    //CAN_setup(the_conf.CANspeed);
    adc_setup();
    BMP280_setup(0);
    BMP280_start();
    USBPU_ON();
#ifndef EBUG
    iwdg_setup();
#endif
    // CAN_message *can_mesg;
    while(1){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - Sens_measured_time > SENSORS_DATA_TIMEOUT){ // get BME
            getsensors();
            Sens_measured_time = Tms;
        }
        /*CAN_proc();
        if(CAN_get_status() == CAN_FIFO_OVERRUN){
            USB_sendstr("CAN bus fifo overrun occured!\n");
        }
        while((can_mesg = CAN_messagebuf_pop())){
            if(can_mesg && isgood(can_mesg->ID)){
                if(ShowMsgs){ // display message content
                    IWDG->KR = IWDG_REFRESH;
                    uint8_t len = can_mesg->length;
                    printu(Tms);
                    USB_sendstr(" #");
                    printuhex(can_mesg->ID);
                    for(uint8_t i = 0; i < len; ++i){
                        USB_putbyte(' ');
                        printuhex(can_mesg->data[i]);
                    }
                    USB_putbyte('\n');
                }
            }
        }*/
        if(I2C_scan_mode){
            uint8_t addr;
            int ok = i2c_scan_next_addr(&addr);
            if(addr == I2C_ADDREND) USND("Scan ends");
            else if(ok){
                USB_sendstr(uhex2str(addr));
                USB_sendstr(" ("); USB_sendstr(u2str(addr));
                USB_sendstr(") - found device\n");
            }
        }
        //i2c_have_DMA_Rx(); // check if there's DMA Rx complete
        BMP280_process();
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l){
            const char *ans = cmd_parser(inbuff);
            if(ans) USB_sendstr(ans);
        }
        process_screen();
        process_keys();
        // turn off screen and LEDs if no keys was pressed last X ms
        if(LEDsON){
            if(Tms - lastUnsleep > BTN_ACTIVITY_TIMEOUT){ // timeout - turn off LEDs and screen
                LEDS_OFF();
                ili9341_off();
            }else{ // check operation buttons for menu etc
                IWDG->KR = IWDG_REFRESH;
                indication_process();
                IWDG->KR = IWDG_REFRESH;
            }
        }else{
            if(Tms - lastUnsleep < BTN_ACTIVITY_TIMEOUT/2){ // recent activity - turn on indication
                LEDS_ON();
                ili9341_on();
            }
        }
    }
}
