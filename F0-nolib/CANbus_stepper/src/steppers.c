/*
 * This file is part of the Stepper project.
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
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "steppers.h"

static drv_type driver = DRV_NONE;

/**
 * @brief checkDrv - test if driver connected
 */
static void checkDrv(){
    uint8_t oldstate;
    // turn power on and wait a little
    SLEEP_ON(); // sleep -> 0
    DRV_DISABLE();
    VIO_ON(); sleep(15);
    // check Vdd
    if(FAULT_STATE()){
        MSG("No power @ Vin, mailfunction\n");
        driver = DRV_MAILF;
        VIO_OFF();
        goto ret;
    }
    SLP_CFG_IN();
    sleep(2);
    // Check is ~SLEEP is in air
    oldstate = SLP_STATE();
    if(oldstate) MSG("SLP=1\n"); else MSG("SLP=0\n");
    SLEEP_OFF(); SLP_CFG_OUT();  // sleep -> 1
    sleep(2);
    SLP_CFG_IN();
    sleep(2);
    if(SLP_STATE()) MSG("SLP=1\n"); else MSG("SLP=0\n");
    if(SLP_STATE() != oldstate){
        MSG("~SLP is in air\n");
        if(driver != DRV_2130){
            driver = DRV_NONE;
            VIO_OFF();
        }
        goto ret;
    }
    SLEEP_ON(); SLP_CFG_OUT();
    // check if ~EN is in air
    DRV_ENABLE(); // EN->0
    sleep(2);
    EN_CFG_IN();
    sleep(2);
    oldstate = EN_STATE();
    if(oldstate) MSG("EN=1\n"); else MSG("EN=0\n");
    DRV_DISABLE(); // EN->1
    EN_CFG_OUT();
    sleep(2);
    EN_CFG_IN();
    sleep(2);
    if(EN_STATE()) MSG("EN=1\n"); else MSG("EN=0\n");
    if(oldstate != EN_STATE()){
        MSG("~EN is in air\n");
        driver = DRV_NONE;
        VIO_OFF();
        goto ret;
    }
ret:
    DRV_DISABLE();
    EN_CFG_OUT(); // return OUT conf
    SLEEP_OFF();
    SLP_CFG_OUT();
#ifdef EBUG
    sendbuf();
#endif
}

/**
 * @brief initDriver - try to init driver
 * @return driver type
 */
drv_type initDriver(){
    if(driver != DRV_NOTINIT){ // reset all settings
        MSG("clear GPIO & other setup\n");
        // TODO: turn off SPI & timer
        gpio_setup(); // reset pins control
    }
    driver = the_conf.driver_type;
    checkDrv();
    if(driver == DRV_NONE) return driver;
    // TODO: some stuff here
    return driver;
}
