/*
 *                                                                                                  geany_encoding=koi8-r
 * sensors_manage.c
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */
#include "sensors_manage.h"
#include "can_process.h"
#include "i2c.h"
#include "proto.h" // addtobuf, bufputchar, memcpy

extern volatile uint32_t Tms;
uint8_t sensors_scan_mode = 0; // infinite scan mode
static uint32_t lastSensT = 0;
SensorsState Sstate = SENS_OFF; // turn on sensors only by request
static uint8_t curr_mul_addr = 0;  // current sensors pair address @ multiplexer
static uint8_t overcurnt_ctr = 0;  // if this counter > 32 go to OFF state
uint8_t sens_present[2] = {0,0};   // bit flag: Nth bit == 1 if sensor[s] on given channel found
uint8_t Nsens_present = 0;  // total amount of sensors found
uint8_t Ntemp_measured = 0; // total amount of themperatures measured

// 8 - amount of pairs, 2 - amount in pair, 5 - amount of Coef.
static uint16_t coefficients[MUL_MAX_ADDRESS+1][2][5]; // Coefficients for given sensors
// measured temperatures * 100
int16_t Temperatures[MUL_MAX_ADDRESS+1][2];

// pair addresses
static const uint8_t Taddr[2] = {TSYS01_ADDR0, TSYS01_ADDR1};

static const char *statenames[] = {
    [SENS_INITING]       = "init"
   ,[SENS_RESETING]      = "reset"
   ,[SENS_GET_COEFFS]    = "getcoeff"
   ,[SENS_SLEEPING]      = "sleep"
   ,[SENS_START_MSRMNT]  = "startmeasure"
   ,[SENS_WAITING]       = "waitresults"
   ,[SENS_GATHERING]     = "collectdata"
   ,[SENS_OFF]           = "off"
   ,[SENS_OVERCURNT]     = "overcurrent"
   ,[SENS_OVERCURNT_OFF] = "offbyovercurrent"
};

static uint8_t getcoeff(uint8_t i);

const char *sensors_get_statename(SensorsState x){
    if(x >= SENS_STATE_CNT) return "wrongstate";
    return statenames[x];
}

// TODO: check if we can convert double to float!

#ifndef EBUG
#define TYPE double
#else
#define TYPE float
#endif

const TYPE mul[5] = {-1.5e-2, 1., -2., 4., -2.};
/**
 * Get temperature & calculate it by polinome
 * T =    (-2) * k4 * 10^{-21} * ADC16^4
 *      +   4  * k3 * 10^{-16} * ADC16^3
 *      + (-2) * k2 * 10^{-11} * ADC16^2
 *      +   1  * k1 * 10^{-6}  * ADC16
 *      +(-1.5)* k0 * 10^{-2}
 * k0*(-1.5e-2) + 1e-6*val*(k1 + 1e-5*val*(-2*k2 + 1e-5*val*(4*k3 + -2e-5*k4*val)))
 *
 * @param t - value from sensor
 * @param i - number of sensor in pair
 * @return -30000 if something wrong or T*100 if all OK
 */
static uint16_t calc_t(uint32_t t, int i){
    uint16_t *coeff = coefficients[curr_mul_addr][i];
    if(coeff[0] == 0){
        if(!getcoeff(i)) return BAD_TEMPERATURE; // what is with coeffs?
    }
    if(t < 600000 || t > 30000000) return BAD_TEMPERATURE; // wrong value - too small or too large
    int j;
    TYPE d = (TYPE)t/256., tmp = 0.;
    // k0*(-1.5e-2) + 0.1*1e-5*val*(1*k1 + 1e-5*val*(-2.*k2 + 1e-5*val*(4*k3 + 1e-5*val*(-2*k4))))
    for(j = 4; j > 0; --j){
        tmp += mul[j] * (TYPE)coeff[j];
        tmp *= 1e-5*d;
    }
    tmp = tmp * 10. + 100. * mul[0] * coeff[0];
    return (uint16_t)tmp;
}

// turn off sensors' power
void sensors_off(){
    mesg("Turn off sensors");
    MUL_OFF(); // turn off multiplexers
    SENSORS_OFF(); // turn off sensors' power
    Sstate = SENS_OFF;
}

/**
 * if all OK with current, turn ON sensors' power
 */
static int sensors_on(){
    mesg("Turn on sensors");
    curr_mul_addr = 0;
    sensors_scan_mode = 0;
    MUL_OFF();
    if(SENSORS_OVERCURNT()){
        mesg("OVERCURRENT!");
        SENSORS_OFF();
        Sstate = (++overcurnt_ctr > 32) ? SENS_OVERCURNT_OFF : SENS_OVERCURNT;
        if(Sstate == SENS_OVERCURNT_OFF) mesg("sensors_on() ---> OFF by overcurrent");
        return FALSE;
    }else{
        mesg("Powered on");
        SENSORS_ON();
        return TRUE;
    }
}

// init sensors
void sensors_init(){
    sens_present[0] = sens_present[1] = 0;
    overcurnt_ctr = 0;
    Nsens_present = 0;
    if(sensors_on()) Sstate = SENS_INITING;
}

/**
 * start measurement if sensors are sleeping,
 * turn ON if they were OFF
 * do nothing if measurement processing
 */
void sensors_start(){
    //if(sensors_scan_mode) return;
    switch(Sstate){
        case SENS_SLEEPING:
            Sstate = SENS_START_MSRMNT;
        break;
        case SENS_OFF:
            if(Nsens_present){ // already gon N sensors - use this information
                overcurnt_ctr = 0;
                if(sensors_on()) Sstate = SENS_START_MSRMNT;
             }else{
                sensors_init();
            }
        break;
        case SENS_OVERCURNT_OFF:
            sensors_init();
        break;
        default:
        break;
    }
}

// count 1 bits in sens_present & set `Nsens_present` to this value
static void count_sensors(){
    Nsens_present = 0;
    uint16_t B = sens_present[0]<<8 | sens_present[1];
    while(B){
        ++Nsens_present;
        B &= (B - 1);
    }
/*
SEND("count_sensors(): ");
printu(Nsens_present);
newline();
*/
}

/**
 * All procedures return TRUE if all OK or FALSE if failed and need to start scan again
 */
// procedure call each time @ resetting
static uint8_t resetproc(){
    uint8_t i;
    for(i = 0; i < 2; ++i){
        if(write_i2c(Taddr[i], TSYS01_RESET)){
            sens_present[i] |= 1<<curr_mul_addr; // set bit - found
        }else{ // not found
            sens_present[i] &= ~(1<<curr_mul_addr); // reset bit - not found
        }
    }
    return TRUE;
}

static uint8_t getcoeff(uint8_t i){
    const uint8_t regs[5] = {0xAA, 0xA8, 0xA6, 0xA4, 0xA2}; // commands for coefficients
    uint8_t err = 5;
    uint16_t *coef = coefficients[curr_mul_addr][i];
    for(uint8_t j = 0; j < 5; ++j){
        uint32_t K;
        if(write_i2c(Taddr[i], regs[j])){
            if(read_i2c(Taddr[i], &K, 2)){
                coef[j] = K;
                --err;
            }else break;
        }else break;
    }
    if(err){ // restart all procedures if we can't get coeffs of present sensor
        return FALSE;
    }
    return TRUE;
}

// procedure call each time @ getting coefficients
static uint8_t getcoefsproc(){
    uint8_t r = TRUE;
    for(uint8_t i = 0; i < 2; ++i){
        if(!(sens_present[i] & (1<<curr_mul_addr))) continue; // no sensors @ given line
        if(!getcoeff(i)) r = FALSE;
    }
    return r;
}

// procedure call each time @ start measurement
static uint8_t msrtempproc(){
    uint8_t i, j;
    for(i = 0; i < 2; ++i){
        if(!(sens_present[i] & (1<<curr_mul_addr))){ // no sensors @ given line - try to find it
            if(write_i2c(Taddr[i], TSYS01_RESET)){
                sens_present[i] |= 1<<curr_mul_addr;
                mesg("One sensor added");
                count_sensors();
                // if(getcoeff(i)) count_sensors();
            }
            continue;
        }
        for(j = 0; j < 5; ++j){
            if(write_i2c(Taddr[i], TSYS01_START_CONV)) break;
            //if(!write_i2c(Taddr[i], TSYS01_RESET)) i2c_setup(CURRENT_SPEED); // maybe I2C restart will solve the problem?
        }
        if(j == 5){
            sens_present[i] &= ~(1<<curr_mul_addr); // clear presence flag
            mesg("One sensor removed");
            count_sensors();
        }
    }
    return TRUE;
}

// procedure call each time @ read temp
static uint8_t gettempproc(){
    uint8_t i;
    for(i = 0; i < 2; ++i){
        if(!(sens_present[i] & (1<<curr_mul_addr))) continue; // no sensors @ given line
        Temperatures[curr_mul_addr][i] = NO_SENSOR;
        uint8_t err = 1;
        if(write_i2c(Taddr[i], TSYS01_ADC_READ)){
            uint32_t t;
            if(read_i2c(Taddr[i], &t, 3) && t){
                if(BAD_TEMPERATURE != (Temperatures[curr_mul_addr][i] = calc_t(t, i))){
                    err = 0;
                    ++Ntemp_measured;
                    mesg("  got one T");
                }else mesg("  bad T");
            }
        }
        if(err){
            write_i2c(Taddr[i], TSYS01_RESET);
            mesg("  i2c err");
        }
    }
    return TRUE;
}

/**
 * 2 phase for each call: 1) set address & turn on mul.; 2) call function & turn off mul.
 * Change address @ call function `procfn`
 * @return TRUE if scan is over
 */
static uint8_t sensors_scan(uint8_t (* procfn)()){
    static uint8_t callctr = 0;
    if(callctr == 0){ // set address & turn on multiplexer
        MUL_ADDRESS(curr_mul_addr);
        MUL_ON();
        callctr = 1;
    }else{ // call procfn & turn off multiplexer
        callctr = 0;
        uint8_t s = procfn();
        MUL_OFF();
        if(!s){ // start scan again if error
            curr_mul_addr = 0;
            return FALSE;
        }
        if(++curr_mul_addr > MUL_MAX_ADDRESS){ // scan is over
            curr_mul_addr = 0;
            return TRUE;
        }
    }
    return FALSE;
}

// print coefficients @debug console
void showcoeffs(){
    int a, p, k;
    if(Nsens_present == 0){
        SEND("showcoeffs(): no sensors found\n");
        return;
    }
    for(a = 0; a <= MUL_MAX_ADDRESS; ++a){
        for(p = 0; p < 2; ++p){
            if(!(sens_present[p] & (1<<a))) continue; // no sensor
            for(k = 0; k < 5; ++k){
                char b[] = {'K', a+'0', p+'0', '_', k+'0', '=', 0};
                addtobuf(b);
                printu(coefficients[a][p][k]);
                newline();
            }
        }
    }
}

// print temperatures @debug console
void showtemperature(){
    int a, p;
    if(Nsens_present == 0){
        SEND("showtemperature(): no sensors found\n");
        return;
    }
    if(Ntemp_measured == 0){
        SEND("showtemperature(): no temperatures measured\n");
        return;
    }
    for(a = 0; a <= MUL_MAX_ADDRESS; ++a){
        for(p = 0; p < 2; ++p){
            if(!(sens_present[p] & (1<<a))){
                continue; // no sensor
            }
            bufputchar('T');
            bufputchar('0' + Controller_address);
            bufputchar('_');
            printu(a*10+p);
            bufputchar('=');
            int16_t t = Temperatures[a][p];
            if(t < 0){
                t = -t;
                bufputchar('-');
            }
            printu(t);
            newline();
            IWDG->KR = IWDG_REFRESH;
        }
    }
}

// finite state machine for sensors switching & checking
void sensors_process(){
    static int8_t NsentOverCAN = -1; // number of T (N*10+p) sent over CAN bus; -1 - nothing to send
    if(SENSORS_OVERCURNT()){
        mesg("sensors_process(): overcurrent!");
        MUL_OFF();
        SENSORS_OFF();
        Sstate = (++overcurnt_ctr > 32) ? SENS_OVERCURNT_OFF : SENS_OVERCURNT;
        if(Sstate == SENS_OVERCURNT_OFF) mesg("sensors_process(): ---> OFF by overcurrent");
        return;
    }
    switch(Sstate){
        case SENS_INITING: // initialisation (restart I2C)
            mesg("SENS_INITING");
            i2c_setup(CURRENT_SPEED);
            Sstate = SENS_RESETING;
            lastSensT = Tms;
            //NsentOverCAN = -1;
        break;
        case SENS_RESETING: // reset & discovery procedure
            /*if(NsentOverCAN == -1){
                mesg("SENS_RESETING");
                NsentOverCAN = 0;
            }*/
            if(Tms - lastSensT > POWERUP_TIME){
                if(sensors_scan(resetproc)){
                    count_sensors(); // get total amount of sensors
                    if(Nsens_present){
                        Sstate = SENS_GET_COEFFS;
                        mesg("SENS_RESETING -> SENS_GET_COEFFS");
                    }else{ // no sensors found
                        mesg("No sensors found -> off");
                        sensors_off();
                    }
                }
            }
        break;
        case SENS_GET_COEFFS: // get coefficients
            if(sensors_scan(getcoefsproc)){
                Sstate = SENS_SLEEPING; // sleep after got coefficients
                mesg("SENS_GET_COEFFS -> SENS_SLEEPING");
            }
        break;
        case SENS_START_MSRMNT: // send all sensors command to start measurements
            if(sensors_scan(msrtempproc)){
                lastSensT = Tms;
                Sstate = SENS_WAITING;
                Ntemp_measured = 0; // reset value of good measurements
                mesg("SENS_START_MSRMNT -> SENS_WAITING");
            }
        break;
        case SENS_WAITING: // wait for end of conversion
            if(Tms - lastSensT > CONV_TIME){
                //NsentOverCAN = -1;
                mesg("SENS_WAITING -> SENS_GATHERING");
                Sstate = SENS_GATHERING;
            }
        break;
        case SENS_GATHERING: // scan all sensors, get thermal data & calculate temperature
            /*if(NsentOverCAN < 0){
                mesg("SENS_GATHERING");
                NsentOverCAN = 0;
            }*/
            if(sensors_scan(gettempproc)){
                lastSensT = Tms;
                NsentOverCAN = 0;
                Sstate = SENS_SENDING_DATA;
                mesg("SENS_GATHERING -> SENS_SENDING_DATA");
            }
        break;
        case SENS_SENDING_DATA:
            if(Nsens_present == 0){
                mesg("No sensors found -> off");
                sensors_off();
                NsentOverCAN = 0;
                break;
            }
            NsentOverCAN = send_temperatures(NsentOverCAN); // call sending T process
            if(NsentOverCAN < 0){ // all data sent -> sleep
                Sstate = SENS_SLEEPING;
                mesg("SENS_SENDING_DATA -> SENS_SLEEPING");
                /*
                if(Nsens_present != Ntemp_measured){ // restart sensors only after measurements sent
                        mesg("restart");
                        i2c_setup(CURRENT_SPEED);
                        sensors_on();
                }*/
            }
        break;
        case SENS_SLEEPING: // wait for `SLEEP_TIME` till next measurements in scan mode
            /*if(NsentOverCAN < 0){
                mesg("SENS_SLEEPING");
                NsentOverCAN = 0;
            }*/
            if(sensors_scan_mode){ // sleep until next measurement start
                if(Tms - lastSensT > SLEEP_TIME){
                    Sstate = SENS_START_MSRMNT;
                    mesg("SENS_SLEEPING -> SENS_START_MSRMNT");
                }
            }
        break;
        case SENS_OVERCURNT: // try to reinit all after overcurrent
            mesg("SENS_OVERCURNT");
            if(sensors_on()) Sstate = SENS_SLEEPING;
        break;
        default: // do nothing
        break;
    }
}
