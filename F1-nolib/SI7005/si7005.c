/*
 * This file is part of the si7005 project.
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

#include "i2c.h"
#include "si7005.h"

#ifdef EBUG
#include "usb.h"
#include "proto.h"
#define DBG(x)  do{USB_send(x);}while(0)
#else
#define DBG(x)
#endif


#define DEVID  (0x40)

typedef enum{
	RELAX = 0,
	WAITFORT,       // T
	WAITFORH        // humidity
} si7005_state;

static si7005_state state = RELAX;
static si7005_status sistatus = SI7005_RELAX;
static uint16_t TH; // data for T or H

si7005_status si7005_get_status(){
    return sistatus;
}

void si7005_setup(){
    sistatus = SI7005_RELAX;
    state = RELAX;
	i2c_setup();
	i2c_set_addr7(DEVID);
}

/**
 * @brief si7005_read_ID - read device ID
 * @param devid (o) - ID
 * @return 1 if all OK
 */
int si7005_read_ID(uint8_t *devid){
	uint8_t ID;
	i2c_status st = I2C_OK;
	if(state != RELAX){ // measurements are in process
		return 0;
	}
    sistatus = SI7005_RELAX;
	if((st = i2c_7bit_send_onebyte(0x11, 0)) == I2C_OK){
		if((st = i2c_7bit_receive_onebyte(&ID, 1)) == I2C_OK){
            *devid = ID;
            return 1;
		}
	}
    return 0;
}

/*
 * start themperature reading @return 0 if all OK
 */
int si7005_cmdT(){
	const uint8_t cmd[2] = {0x03, 0x11};
	if(state != RELAX){
		return 1;
	}
    sistatus = SI7005_BUSY;
	i2c_status st = i2c_7bit_send(cmd, 2);
	if(st != I2C_OK){
		return 1;
	}
    DBG("Wait for T\n");
	state = WAITFORT;
    return 0;
}

/*
 * start humidity reading @return 0 if all OK
 */
int si7005_cmdH(){
	const uint8_t cmd[2] = {0x03, 0x01};
	if(state != RELAX){
		return 1;
	}
    sistatus = SI7005_BUSY;
	i2c_status st = i2c_7bit_send(cmd, 2);
	if(st != I2C_OK){
		return 1;
	}
	state = WAITFORH;
    DBG("Wait for H\n");
    return 0;
}

int32_t si7005_getT(){ // T*10
    if(sistatus != SI7005_TRDY) return -5000;
    DBG("TH="); DBG(u2str(TH)); DBG("\n");
    TH >>= 2;
    uint32_t d = (TH*10)/32 - 500;
    sistatus = SI7005_RELAX;
    return d;
}
uint32_t si7005_getH(){ // hum * 10
    if(sistatus != SI7005_HRDY) return 0;
    TH >>= 4;
    uint32_t d = (TH*10)/16 - 240;
    sistatus = SI7005_RELAX;
    return d;
}

/*
 * process state machine
 */
void si7005_process(){
	uint8_t b, d[2];
	i2c_status st;
	if(state == RELAX) return;
	if(state == WAITFORH || state == WAITFORT){ // poll RDY
        DBG("Poll\n");
		if((st = i2c_7bit_send_onebyte(0, 0)) == I2C_OK){
            DBG("0 sent\n");
			if(i2c_7bit_receive_onebyte(&b, 1) == I2C_OK){
                DBG("received: "); DBG(u2str(b)); DBG("\n");
				if(b) return; // !RDY
                if((st = i2c_7bit_send_onebyte(1, 0)) == I2C_OK){
                    DBG("sent 1\n");
                    if((st = i2c_7bit_receive_twobytes(d)) == I2C_OK){
                        DBG("got data: ");
                        DBG(u2str(d[0])); DBG(" "); DBG(u2str(d[1])); DBG("\n");
                        sistatus = (state == WAITFORH) ? SI7005_HRDY : SI7005_TRDY;
                        TH = (d[0]<<8) | d[1];
                    }
                }
				state = RELAX;
				if(st != I2C_OK){
					sistatus = SI7005_ERR;
				}
			}
		}else{
			state = RELAX;
            sistatus = SI7005_ERR;
		}
	}
}
