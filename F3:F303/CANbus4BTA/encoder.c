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
#include <strings.h>

static int rdy = 0;

void encoder_process(){
    if(!FLAG(ENC_IS_SSI)) return;
    static uint32_t lastT = 0;
    if(!rdy){
        encoder_setup();
        return;
    }
    if(Tms - lastT > ENCODER_RD_INTERVAL){
        if(spi_start_enc()) lastT = Tms; // start rd process
    }
}

void encoder_setup(){
    if(FLAG(ENC_IS_SSI)){
        usart_deinit();
        spi_setup(ENCODER_SPI);
    }else{
        spi_deinit(ENCODER_SPI);
        usart_setup();
    }
    rdy = 1;
}

// read encoder value into buffer `outbuf`
// return TRUE if all OK
int read_encoder(uint8_t outbuf[8]){
    bzero(outbuf, 8);
    if(FLAG(ENC_IS_SSI)){
        spi_read_enc(outbuf);
        return TRUE;
    }
    usart_rstbuf();
    // just send some trash over USART1 if encoder is RS-422
    usart_send("teststring\n");
    char *x;
    uint32_t Tl = Tms;
    while(Tms - Tl < 10){
        int l = usart_getline(&x);
        if(l){
            for(int i = 0; i < l && i < 4; ++i) outbuf[i] = x[i];
            return TRUE;
        }
    }
    return FALSE;
}

// send encoder data
void CANsendEnc(){
    CAN_message msg = {.data = {0}, .ID = the_conf.encoderID, .length = 8};
    if(!read_encoder(msg.data)) return;
    CAN_send(&msg);
}
// send limit-switches data
void CANsendLim(){
    CAN_message msg = {.data = {0}, .ID = the_conf.limitsID, .length = 8};
    msg.data[2] = getESW(1);
    CAN_send(&msg);
}
