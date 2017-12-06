/*
 *                                                                                                  geany_encoding=koi8-r
 * flash.c
 *
 * Copyright 2017 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#include "stm32f0.h"
#include <string.h> // memcpy

#include "flash.h"
#include "usart.h"

// start of configuration data in flash (from 15kB, one kB size)
#define FLASH_CONF_START_ADDR   ((uint32_t)0x08003C00)
static const int maxnum = 1024 / sizeof(user_conf);

user_conf the_conf = {
     .userconf_sz = sizeof(user_conf)
    ,.devID = 0
    ,.v12numerator = 1
    ,.v12denominator = 1
    ,.i12numerator = 1
    ,.i12denominator = 1
    ,.v33denominator = 1
    ,.v33numerator = 1
    ,.ESW_thres = 150
};

static int erase_flash();

static int get_gooddata(){
    user_conf *c = (user_conf*) FLASH_CONF_START_ADDR;
    // have data - move it to `the_conf`
    int idx;
//write2trbuf("get_gooddata()\n");
    for(idx = 0; idx < maxnum; ++idx){ // find current settings index - first good
        uint16_t sz = c[idx].userconf_sz;
/*write2trbuf("idx=");
put_int((int32_t) idx);
write2trbuf(", sz=");
put_uint((uint32_t) sz);
write2trbuf(", devID=");
put_uint((uint32_t) c[idx].devID);
write2trbuf(", ESW_thres=");
put_uint((uint32_t) c[idx].ESW_thres);
SENDBUF();*/
        if(sz != sizeof(user_conf)){
            if(sz == 0xffff) break; // first clear
            else{
                return -2; // flash corrupt, need to erase
            }
        }
    }
    return idx-1; // -1 if there's no data at all & flash is clear; maxnum-1 if flash is full
}

void get_userconf(){
    user_conf *c = (user_conf*) FLASH_CONF_START_ADDR;
    int idx = get_gooddata();
    if(idx < 0) return; // no data stored
    memcpy(&the_conf, &c[idx], sizeof(user_conf));
}

// store new configuration
// @return 0 if all OK
int store_userconf(){
    int ret = 0;
    user_conf *c = (user_conf*) FLASH_CONF_START_ADDR;
    int idx = get_gooddata();
    if(idx == -2 || idx == maxnum - 1){ // data corruption or there's no more place
        idx = 0;
        if(erase_flash()) return 1;
    }else ++idx; // take next data position
/*write2trbuf("store_userconf()\nidx=");
put_int((int32_t) idx);
SENDBUF();*/
    if (FLASH->CR & FLASH_CR_LOCK){ // unloch flash
        FLASH->KEYR = FLASH_FKEY1;
        FLASH->KEYR = FLASH_FKEY2;
    }
    while (FLASH->SR & FLASH_SR_BSY);
    if(FLASH->SR & FLASH_SR_WRPERR) return 1; // write protection
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR; // clear all flags
    FLASH->CR |= FLASH_CR_PG;
    uint16_t *data = (uint16_t*) &the_conf;
    uint16_t *address = (uint16_t*) &c[idx];
    uint32_t i, count = sizeof(user_conf) / 2;
    for (i = 0; i < count; ++i){
        *(volatile uint16_t*)(address + i) = data[i];
        while (FLASH->SR & FLASH_SR_BSY);
        if(FLASH->SR &  FLASH_SR_PGERR) ret = 1; // program error - meet not 0xffff
        else while (!(FLASH->SR & FLASH_SR_EOP));
/*write2trbuf("write byte ");
put_int((int32_t) i);
write2trbuf(", write value=");
put_uint(data[i]);
write2trbuf(", read value=");
put_uint(address[i]);
SENDBUF();
if(ret){
write2trbuf("PGERR");
SENDBUF();*/
}
        FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;
    }
    FLASH->CR &= ~(FLASH_CR_PG);
    return ret;
}


static int erase_flash(){
    int ret = 0;
/*write2trbuf("erase_flash()");
SENDBUF();*/
    /* (1) Wait till no operation is on going */
    /* (2) Clear error & EOP bits */
    /* (3) Check that the Flash is unlocked */
    /* (4) Perform unlock sequence */
    while ((FLASH->SR & FLASH_SR_BSY) != 0){} /* (1) */
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;  /* (2) */
  /*  if (FLASH->SR & FLASH_SR_EOP){
        FLASH->SR |= FLASH_SR_EOP;
    }*/
    if ((FLASH->CR & FLASH_CR_LOCK) != 0){ /* (3) */
        FLASH->KEYR = FLASH_FKEY1; /* (4) */
        FLASH->KEYR = FLASH_FKEY2;
    }
    /* (1) Set the PER bit in the FLASH_CR register to enable page erasing */
    /* (2) Program the FLASH_AR register to select a page to erase */
    /* (3) Set the STRT bit in the FLASH_CR register to start the erasing */
    /* (4) Wait until the  EOP flag in the FLASH_SR register set */
    /* (5) Clear EOP flag by software by writing EOP at 1 */
    /* (6) Reset the PER Bit to disable the page erase */
    FLASH->CR |= FLASH_CR_PER; /* (1) */
    FLASH->AR = FLASH_CONF_START_ADDR; /* (2) */
    FLASH->CR |= FLASH_CR_STRT; /* (3) */
    while(!(FLASH->SR & FLASH_SR_EOP));
    FLASH->SR |= FLASH_SR_EOP; /* (5)*/
    if(FLASH->SR & FLASH_SR_WRPERR){ /* Check Write protection error */
        ret = 1;
        FLASH->SR |= FLASH_SR_WRPERR; /* Clear the flag by software by writing it at 1*/
    }
    FLASH->CR &= ~FLASH_CR_PER; /* (6) */
    return ret;
}

