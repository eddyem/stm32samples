/*
 * This file is part of the canbus4bta project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "can.h"
#include "encoder.h"
#include "flash.h"
#include "gpio.h"
#include "spi.h"
#include "usart.h"

#include <string.h>

void encoder_setup(){
    if(FLAG(ENC_IS_SSI)){
        usart_deinit();
        spi_setup(ENCODER_SPI);
    }else{
        spi_deinit(ENCODER_SPI);
        usart_setup();
    }
}

// read encoder value into buffer `outbuf`
// return TRUE if all OK
int read_encoder(uint8_t outbuf[4]){
    if(!FLAG(ENC_IS_SSI)) return FALSE;
    *((uint32_t*)outbuf) = 0;
    return spi_writeread(ENCODER_SPI, outbuf, 4);
}

// send encoder data
void CANsendEnc(){
    CAN_message msg = {.data = {0}, .ID = the_conf.encoderID, .length = 8};
    if(!read_encoder(msg.data)) return;
    uint32_t ctr = TIM2->CNT;
    //msg.data[4] = 0;
    msg.data[5] = (ctr >> 16) & 0xff;
    msg.data[6] = (ctr >> 8 ) & 0xff;
    msg.data[7] = (ctr >> 0 ) & 0xff;
    CAN_send(&msg);
}
// send limit-switches data
void CANsendLim(){
    CAN_message msg = {.data = {0}, .ID = the_conf.limitsID, .length = 8};
    msg.data[2] = getESW(0);
    msg.data[3] = getESW(1);
    CAN_send(&msg);
}
