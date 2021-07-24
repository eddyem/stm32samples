/*
 * This file is part of the canrelay project.
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

#include "adc.h"
#include "buttons.h"
#include "canproto.h"
#include "hardware.h"
#include "proto.h"

/*
 * Protocol (endian format depends on command!):
 * answer will be send to `OUTPID` CAN ID, length depends on received command
 * IN packet (sent to `CANID`) data bytes:
 *  0 - command (any from CAN_commands), 1..7 - data
 * To set value command should have CAN_CMD_SETFLAG == 1
 * OUT packet depending on command:
 * CAN_CMD_PING (or any incoming packet with zero lenght) - zero length answer
 * other commands have data[0] equal to in command (without CAN_CMD_SETFLAG)
 * CAN_CMD_ADC: data[1]..data[6] - 12bits*4channels RAW ADC data (ADC channels 0..3)
 * CAN_CMD_BTNS: data[1] = button number (0..3), data[2] - state (keyevent), data[4..7] - event time (LITTLE endian!)
 * CAN_CMD_LED: data[1] = LEDs state (bits 0..3 for LEDs number 0..3)
 * CAN_CMD_MCU: data[2,3] = int16_t MCUT*10, data[4,5] = uint16_t Vdd*100 (LITTLE endian!)
 * CAN_CMD_PWM: data[1..3] = pwm value for each channel (0..2)
 * CAN_CMD_RELAY: data[0] = (bits 0 & 1) - state of relay N]
 * CAN_CMD_TMS: data[4..7] = Tms (LITTLE endian!)
 */

// return total message length
TRUE_INLINE uint8_t ADCget(uint8_t values[8]){
    uint16_t v[NUMBER_OF_ADC_CHANNELS];
    for(int i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i) v[i] = getADCval(i);
    // if NUMBER_OF_ADC_CHANNELS != 4 - need for manual refactoring!
    values[1] = v[0] >> 4;
    values[2] = (v[0] << 4) | ((v[1] >> 8) & 0x0f);
    values[3] = v[1] & 0xff;
    values[4] = v[2] >> 4;
    values[5] = (v[2] << 4) | ((v[3] >> 8) & 0x0f);
    values[6] = v[3] & 0xff;
    return 7;
}

TRUE_INLINE void BTNSget(uint8_t values[8]){
    uint32_t T;
    for(uint8_t i = 0; i < BTNSNO; ++i){
        values[1] = i;
        values[2] = keystate(i, &T);
        values[3] = 0;
        *((uint32_t*)&values[4]) = T;
        int N = 1000;
        while(CAN_BUSY == can_send(values, 8, OUTPID)) if(--N == 0) break;
    }
}

TRUE_INLINE void LEDSset(uint8_t data[8]){
    for(int i = 0; i < LEDSNO; ++i){
        if(data[1] & 1<<i) LED_on(i);
        else LED_off(i);
    }
}

TRUE_INLINE uint8_t LEDSget(uint8_t data[8]){
    uint8_t x = 0;
    for(int i = 0; i < LEDSNO; ++i)
        if(LED_chk(i)) x |= 1<<i;
    data[1] = x;
    return 2;
}

TRUE_INLINE uint8_t MCUget(uint8_t data[8]){
    *((int16_t*)&data[2]) = (int16_t)getMCUtemp();
    *((uint16_t*)&data[4]) = (int16_t)getVdd();
    return 6;
}

TRUE_INLINE void PWMset(uint8_t data[8]){
    volatile uint32_t *reg = &TIM1->CCR1;
    for(int i = 0; i < 3; ++i)
        reg[i] = data[i+1];
}

TRUE_INLINE uint8_t PWMget(uint8_t data[8]){
    volatile uint32_t *reg = &TIM1->CCR1;
    for(int i = 0; i < 3; ++i)
        data[i+1] = reg[i];
    return 4;
}

TRUE_INLINE void Rset(uint8_t data[8]){
    for(int i = 0; i < RelaysNO; ++i){
        if(data[1] & 1<<i) Relay_ON(i);
        else Relay_OFF(i);
    }
}

TRUE_INLINE uint8_t Rget(uint8_t data[8]){
    uint8_t x = 0;
    for(int i = 0; i < RelaysNO; ++i){
        if(Relay_chk(i)) x |= 1<<i;
    }
    data[1] = x;
    return 2;
}

TRUE_INLINE uint8_t showTms(uint8_t data[8]){
    *((uint32_t*)&data[4]) = Tms;
    return 8;
}

void parseCANcommand(CAN_message *msg){
    uint8_t outpdata[8], len = 0;
#ifdef EBUG
    SEND("Get data: ");
    for(int i = 0; i < msg->length; ++i){
        printuhex(msg->data[i]); bufputchar(' ');
    }
    NL();
#endif
    int N = 1000;
    uint8_t cmd = msg->data[0] & CANCMDMASK, setter = msg->data[0] & CAN_CMD_SETFLAG;
    if(msg->length == 0 || cmd == CAN_CMD_PING){ // no commands or ping cmd -> return empty message (ping)
        goto sendans;
    }
    outpdata[0] = cmd;
    switch(cmd){
        case CAN_CMD_ADC:
            len = ADCget(outpdata);
        break;
        case CAN_CMD_BTNS:
            BTNSget(outpdata);
            return;
        break;
        case CAN_CMD_LED:
            if(setter) LEDSset(msg->data);
            len = LEDSget(outpdata);
        break;
        case CAN_CMD_MCU:
            len = MCUget(outpdata);
        break;
        case CAN_CMD_PWM:
            if(setter) PWMset(msg->data);
            len = PWMget(outpdata);
        break;
        case CAN_CMD_RELAY:
            if(setter) Rset(msg->data);
            len = Rget(outpdata);
        break;
        case CAN_CMD_TMS:
            len = showTms(outpdata);
        break;
        default: // wrong command ->
            outpdata[0] = CAN_CMD_ERRCMD;
            len = 1;
    }
sendans:
    while(CAN_BUSY == can_send(outpdata, len, OUTPID)) if(--N == 0) break;
}
