/*
 * This file is part of the fx3u project.
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

#include "adc.h"
#include "flash.h"
#include "hardware.h"
#include "modbusproto.h"
#include "modbusrtu.h"
#include "strfunc.h"
#include "usart.h"

// send error
static void senderr(modbus_request *r, uint8_t exception){
    if(r->ID == 0) return; // don't answer to broadcasting packets
    modbus_response resp = {.ID = the_conf.modbusID, .Fcode = r->Fcode | MODBUS_RESPONSE_ERRMARK, .datalen = exception};
    modbus_send_response(&resp);
}

// relays status
TRUE_INLINE void readcoil(modbus_request *r){
    // starting number shoul be 0
    if(r->startreg){
        senderr(r, ME_ILLEGAL_ADDRESS);
        return;
    }
    // amount of bytes: should be 8-multiple
    int amount = r->regno >> 3;
    if(amount == 0 || (r->regno & 7) || amount > OUTMAXBYTES){
        senderr(r, ME_ILLEGAL_VALUE);
        return;
    }
    uint8_t bytes[OUTMAXBYTES] = {0};
    int curidx = OUTMAXBYTES;
    int vals = get_relay(OUTMAX+1);
    for(int i = 0; i < amount; ++i){
        bytes[--curidx] = vals & 0xff;
        vals >>= 8;
    }
    modbus_response resp = {.Fcode = r->Fcode, .ID = the_conf.modbusID, .data = bytes+curidx, .datalen = amount};
    modbus_send_response(&resp);
}

// input status
TRUE_INLINE void readdiscr(modbus_request *r){
    if(r->startreg){
        senderr(r, ME_ILLEGAL_ADDRESS);
        return;
    }
    // amount of bytes: should be 8-multiple
    int amount = r->regno >> 3;
    if(amount == 0 || (r->regno & 7) || amount > INMAXBYTES){
        senderr(r, ME_ILLEGAL_VALUE);
        return;
    }
    uint8_t bytes[INMAXBYTES] = {0};
    int vals = get_esw(INMAX+1);
    for(int i = amount - 1; i > -1; --i){
        bytes[i] = vals & 0xff;
        vals >>= 8;
    }
    modbus_response resp = {.Fcode = r->Fcode, .ID = the_conf.modbusID, .data = bytes, .datalen = amount};
    modbus_send_response(&resp);
}

// registers: read only by one!
// !!! To simplify code I suppose that all registers read are 32-bit!
TRUE_INLINE void readreg(modbus_request *r){
    if(r->regno != 1){
        senderr(r, ME_ILLEGAL_VALUE);
        return;
    }
    uint32_t regval;
    switch(r->startreg){
        case MR_TIME:
            regval = Tms;
        break;
        case MR_LED:
            regval = LED(-1);
        break;
        case MR_INCHANNELS:
            regval = inchannels();
        break;
        case MR_OUTCHANNELS:
            regval = outchannels();
        break;
        default:
            senderr(r, ME_ILLEGAL_ADDRESS);
            return;
    }
    regval = __builtin_bswap32(regval);
    modbus_response resp = {.Fcode = r->Fcode, .ID = the_conf.modbusID, .data = (uint8_t*)&regval, .datalen = 4};
    modbus_send_response(&resp);
}

// read ADC values; startreg = N of starting sensor, regno = N values
TRUE_INLINE void readadc(modbus_request *r){
    if(r->startreg >= ADC_CHANNELS){
        senderr(r, ME_ILLEGAL_ADDRESS);
        return;
    }
    int nlast = r->regno + r->startreg;
    if(r->regno == 0 || nlast > ADC_CHANNELS){
        senderr(r, ME_ILLEGAL_VALUE);
        return;
    }
    uint16_t vals[ADC_CHANNELS];
    for(int i = r->startreg; i < nlast; ++i){
        uint16_t v = getADCval(i);
        vals[i - r->startreg] = __builtin_bswap16(v);
    }
    modbus_response resp = {.Fcode = r->Fcode, .ID = the_conf.modbusID, .data = (uint8_t*) vals, .datalen = r->regno * 2};
    modbus_send_response(&resp);
}

TRUE_INLINE void writecoil(modbus_request *r){
    if(r->startreg > OUTMAX || set_relay(r->startreg, r->regno) < 0){
        senderr(r, ME_ILLEGAL_ADDRESS);
        return;
    }
    if(r->ID == 0) return;
    modbus_send_request(r); // answer with same data
}

TRUE_INLINE void writereg(modbus_request *r){
    switch(r->startreg){
        case MR_LED:
            LED(r->regno);
        break;
        case MR_RESET:
            NVIC_SystemReset();
        break;
        default:
            senderr(r, ME_ILLEGAL_ADDRESS);
            return;
    }
    if(r->ID == 0) return;
    modbus_send_request(r);
}

// support ONLY write to ALL!
// data - by bits, like in readcoil
// N registers is N bits
TRUE_INLINE void writecoils(modbus_request *r){
    if(r->startreg){
        senderr(r, ME_ILLEGAL_ADDRESS);
        return;
    }
    int amount = (r->regno + 7) >> 3;
    if(amount == 0 || amount > OUTMAXBYTES || r->datalen < amount){
        senderr(r, ME_ILLEGAL_VALUE);
        return;
    }
    uint32_t v = 0;
    for(int i = 0; i < amount; ++i){
        v <<= 8;
        v |= r->data[i];
    }
    if(set_relay(OUTMAX+1, v) < 0){
        senderr(r, ME_NACK);
        return;
    }
    if(r->ID == 0) return;
    r->datalen = 0;
    modbus_send_request(r);
}

// amount of registers should be equal 1, data should have 4 bytes
TRUE_INLINE void writeregs(modbus_request *r){
    if(r->datalen != 4 || r->regno != 1){
        senderr(r, ME_ILLEGAL_VALUE);
        return;
    }
    uint32_t val = __builtin_bswap32(*(uint32_t*)r->data);
    switch(r->startreg){
        case MR_TIME:
            Tms = val;
        break;
        default:
            senderr(r, ME_ILLEGAL_ADDRESS);
            return;
    }
    if(r->ID == 0) return;
    r->datalen = 0;
    modbus_send_request(r);
}

// get request as slave and send answer about some registers
// DO NOT ANSWER for broadcasting requests!
// Understand only requests with codes <= 6
void parse_modbus_request(modbus_request *r){
    if(!r) return;
    int bcast = (r->ID == 0) ? 1 : 0;
    switch(r->Fcode){
        case MC_READ_COIL:
            if(bcast) break; // block broadcast reading requests
            readcoil(r);
        break;
        case MC_READ_DISCRETE:
            if(bcast) break;
            readdiscr(r);
        break;
        case MC_READ_HOLDING_REG:
            if(bcast) break;
            readreg(r);
        break;
        case MC_READ_INPUT_REG:
            if(bcast) break;
            readadc(r);
        break;
        case MC_WRITE_COIL:
            writecoil(r);
        break;
        case MC_WRITE_REG:
            writereg(r);
        break;
        case MC_WRITE_MUL_COILS:
            writecoils(r);
        break;
        case MC_WRITE_MUL_REGS: // write uint32_t as 2 registers
            writeregs(r);
        break;
        default:
            senderr(r, ME_ILLEGAL_FUNCION);
    }
}

// get responce as master and show it on terminal
void parse_modbus_response(modbus_response *r){
    if(!r) return;
    if(r->Fcode & MODBUS_RESPONSE_ERRMARK){ // error - three bytes
        usart_send("MODBUS ERR (ID=");
        printuhex(r->ID);
        usart_send(", Fcode=");
        printuhex(r->Fcode & ~MODBUS_RESPONSE_ERRMARK);
        usart_send(") > ");
        printuhex(r->datalen);
        newline();
        return;
    }
    // regular answer
    usart_send("MODBUS RESP (ID=");
    printuhex(r->ID);
    usart_send(", Fcode=");
    printuhex(r->Fcode & ~MODBUS_RESPONSE_ERRMARK);
    usart_send(", Datalen=");
    printu(r->datalen);
    usart_send(") > ");
    if(r->datalen){
        for(int i = 0; i < r->datalen; ++i){
            //uint16_t nxt = r->data[i] >> 8 | r->data[i] << 8;
            printuhex(r->data[i]);
            usart_putchar(' ');
        }
    }
    newline();
}
