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

/**
  ATTENTION!!
  This things works only if you will add next section:

    .myvars :
    {
     . = ALIGN(1024);
     KEEP(*(.myvars))
    } > rom

  after section .data
*/

#include "stm32f1.h"
#include <string.h> // memcpy

#include "flash.h"
#include "lidar.h"
#ifdef EBUG
#include "usart.h"
#endif

extern uint32_t _edata, _etext, _sdata;
static int maxnum = FLASH_BLOCK_SIZE / sizeof(user_conf);


typedef struct{
	const user_conf all_stored;
} flash_storage;

#define USERCONF_INITIALIZER  { \
	.userconf_sz = sizeof(user_conf)	\
	,.dist_min = LIDAR_MIN_DIST			\
	,.dist_max = LIDAR_MAX_DIST			\
    ,.trig_pullups = 0xff               \
    ,.trigstate = 0                     \
    ,.trigpause = {400, 400, 400}       \
	}

__attribute__((section(".myvars"))) static const flash_storage Flash_Storage = {
	.all_stored = USERCONF_INITIALIZER
};

static const user_conf *Flash_Data = &Flash_Storage.all_stored;

user_conf the_conf = USERCONF_INITIALIZER;

static int erase_flash();

static int currentconfidx = -1; // index of current configuration

/**
 * @brief binarySearch - binary search in flash for last non-empty cell
 * @param l - left index
 * @param r - right index (should be @1 less than last index!)
 * @return index of non-empty cell or -1
 */
static int binarySearch(int l, int r){
    while(r >= l){
        int mid = l + (r - l) / 2;
        // If the element is present at the middle
        // itself
        uint16_t sz = Flash_Data[mid].userconf_sz;
        if(sz == sizeof(user_conf)){
            if(Flash_Data[mid+1].userconf_sz == 0xffff){
#if 0
        SEND("Found at "); printu(1, mid); newline();
#endif
                return mid;
            }else{ // element is to the right
                l = mid + 1;
#if 0
        SEND("To the right, L="); printu(1, l); newline();
#endif
            }
        }else{ // element is to the left
            r = mid - 1;
#if 0
        SEND("To the left, R="); printu(1, r); newline();
#endif
        }
    }
    DBG("Not found!");
    return -1; // not found
}

static int get_gooddata(){
    static uint8_t firstrun = 1;
    if(firstrun){
        firstrun = 0;
        if(FLASH_SIZE > 0 && FLASH_SIZE < 20000){
            uint32_t flsz = FLASH_SIZE * 1024; // size in bytes
            flsz -= (uint32_t)Flash_Data - FLASH_BASE;
#if 0
            SEND("All size: "); printu(1, flsz); newline();
#endif
            uint32_t usz = (sizeof(user_conf) + 1) / 2;
            maxnum = flsz / 2 / usz;
#if 0
            SEND("Maxnum: "); printu(1, maxnum); newline();
#endif
        }
    }
    return binarySearch(0, maxnum-2); // -1 if there's no data at all & flash is clear; maxnum-1 if flash is full
}

void get_userconf(){
    const user_conf *c = Flash_Data;
    int idx = get_gooddata();
    if(idx < 0) return; // no data stored
    currentconfidx = idx;
    memcpy(&the_conf, &c[idx], sizeof(user_conf));
}

// store new configuration
// @return 0 if all OK
int store_userconf(){
    IWDG->KR = IWDG_REFRESH;
    int ret = 0;
    const user_conf *c = Flash_Data;
    int idx = currentconfidx;
    // maxnum - 3 means that there always should be at least one empty record after last data
    if(idx < 0 || idx > maxnum - 3){ // data corruption or there's no more place
        idx = 0;
        DBG("Need to erase flash!");
        if(erase_flash()) return 1;
    }else ++idx; // take next data position
    currentconfidx = idx;
    if (FLASH->CR & FLASH_CR_LOCK){ // unloch flash
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
    while (FLASH->SR & FLASH_SR_BSY);
    if(FLASH->SR & FLASH_SR_WRPRTERR) return 1; // write protection
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR; // clear all flags
    FLASH->CR |= FLASH_CR_PG;
    uint16_t *data = (uint16_t*) &the_conf;
    uint16_t *address = (uint16_t*) &c[idx];
    uint32_t i, count = (sizeof(user_conf) + 1) / 2;
    for (i = 0; i < count; ++i){
        *(volatile uint16_t*)(address + i) = data[i];
        while (FLASH->SR & FLASH_SR_BSY);
        if(FLASH->SR &  FLASH_SR_PGERR) ret = 1; // program error - meet not 0xffff
        else while (!(FLASH->SR & FLASH_SR_EOP));
        FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
    }
    FLASH->CR &= ~(FLASH_CR_PG);
    return ret;
}

static int erase_flash(){
    int ret = 0;
    uint32_t nblocks = 1;
    if(FLASH_SIZE > 0 && FLASH_SIZE < 20000){
        uint32_t flsz = FLASH_SIZE * 1024; // size in bytes
        flsz -= (uint32_t)Flash_Data - FLASH_BASE;
        nblocks = flsz / FLASH_BLOCK_SIZE;
#if 0
        SEND("N blocks:"); printu(1, nblocks); newline();
#endif
    }
    for(uint32_t i = 0; i < nblocks; ++i){
        IWDG->KR = IWDG_REFRESH;
        /* (1) Wait till no operation is on going */
        /* (2) Clear error & EOP bits */
        /* (3) Check that the Flash is unlocked */
        /* (4) Perform unlock sequence */
        while ((FLASH->SR & FLASH_SR_BSY) != 0){} /* (1) */
        FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;  /* (2) */
      /*  if (FLASH->SR & FLASH_SR_EOP){
            FLASH->SR |= FLASH_SR_EOP;
        }*/
        if ((FLASH->CR & FLASH_CR_LOCK) != 0){ /* (3) */
            FLASH->KEYR = FLASH_KEY1; /* (4) */
            FLASH->KEYR = FLASH_KEY2;
        }
        /* (1) Set the PER bit in the FLASH_CR register to enable page erasing */
        /* (2) Program the FLASH_AR register to select a page to erase */
        /* (3) Set the STRT bit in the FLASH_CR register to start the erasing */
        /* (4) Wait until the  EOP flag in the FLASH_SR register set */
        /* (5) Clear EOP flag by software by writing EOP at 1 */
        /* (6) Reset the PER Bit to disable the page erase */
        FLASH->CR |= FLASH_CR_PER; /* (1) */
#if 0
        SEND("Delete block number "); printu(1, i); newline();
#endif
        FLASH->AR = (uint32_t)Flash_Data + i*FLASH_BLOCK_SIZE; /* (2) */
        FLASH->CR |= FLASH_CR_STRT; /* (3) */
        while(!(FLASH->SR & FLASH_SR_EOP));
        FLASH->SR |= FLASH_SR_EOP; /* (5)*/
        if(FLASH->SR & FLASH_SR_WRPRTERR){ /* Check Write protection error */
            ret = 1;
            DBG("Write protection error!");
            FLASH->SR |= FLASH_SR_WRPRTERR; /* Clear the flag by software by writing it at 1*/
            break;
        }
        FLASH->CR &= ~FLASH_CR_PER; /* (6) */
    }
    return ret;
}

#ifdef EBUG
void dump_userconf(){
    SEND("userconf_sz="); printu(1, the_conf.userconf_sz);
    SEND("\ndist_min="); printu(1, the_conf.dist_min);
    SEND("\ndist_max="); printu(1, the_conf.dist_max);
    SEND("\ntrig_pullups="); printuhex(1, the_conf.trig_pullups);
    SEND("\ntrigstate="); printuhex(1, the_conf.trigstate);
    SEND("\ntrigpause={");
    for(int i = 0; i < TRIGGERS_AMOUNT; ++i){
        if(i) SEND(", ");
        printu(1, the_conf.trigpause[i]);
    }
    SEND("}\n");
    transmit_tbuf(1);
}

void addNrecs(int N){
    SEND("Try to store userconf for "); printu(1, N); SEND(" times\n");
    for(int i = 0; i < N; ++i){
        if(store_userconf()){
            SEND("Error @ "); printu(1, i); newline();
            return;
        }
    }
    SEND("Curr idx: "); printu(1, currentconfidx); newline();
}

#endif
