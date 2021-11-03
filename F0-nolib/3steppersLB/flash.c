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

#include <stm32f0.h>
#include <string.h> // memcpy
#include "flash.h"
#include "steppers.h"
#include "strfunct.h"

extern const uint32_t __varsstart, _BLOCKSIZE;

static const uint32_t blocksize = (uint32_t)&_BLOCKSIZE;

// max amount of Config records stored (will be recalculate in flashstorage_init()
static uint32_t maxCnum = 1024 / sizeof(user_conf); // can't use blocksize here

#define DEFMF   {.haveencoder = 1}

#define USERCONF_INITIALIZER  {             \
     .userconf_sz = sizeof(user_conf)       \
    ,.CANspeed = 100                        \
    ,.CANID = 0xaa                          \
    ,.microsteps = {32, 32, 32}             \
    ,.accel = {160, 160, 160}               \
    ,.maxspd = {1000, 1000, 1000}           \
    ,.maxsteps = {50000, 50000, 50000}      \
    ,.encrev = {800,800,800}                \
    ,.encperstepmin = {3,3,3}               \
    ,.encperstepmax = {5,5,5}               \
    ,.motflags = {DEFMF,DEFMF,DEFMF}        \
    ,.ESW_reaction = {ESW_ANYSTOP, ESW_ANYSTOP, ESW_ANYSTOP} \
    }
static int erase_flash(const void*, const void*);
static int write2flash(const void*, const void*, uint32_t);
// don't write `static` here, or get error:
//      'memcpy' forming offset 8 is out of the bounds [0, 4] of object '__varsstart' with type 'uint32_t'
const user_conf *Flash_Data = (const user_conf *)(&__varsstart);

user_conf the_conf = USERCONF_INITIALIZER;

static int currentconfidx = -1; // index of current configuration

/**
 * @brief binarySearch - binary search in flash for last non-empty cell
 *          any struct searched should have its sizeof() @ the first field!!!
 * @param l - left index
 * @param r - right index (should be @1 less than last index!)
 * @param start - starting address
 * @param stor_size - size of structure to search
 * @return index of non-empty cell or -1
 */
static int binarySearch(int r, const uint8_t *start, int stor_size){
    int l = 0;
    while(r >= l){
        int mid = l + (r - l) / 2;
        const uint8_t *s = start + mid * stor_size;
        if(*((const uint16_t*)s) == stor_size){
            if(*((const uint16_t*)(s + stor_size)) == 0xffff){ // next is free
                return mid;
            }else{ // element is to the right
                l = mid + 1;
            }
        }else{ // element is to the left
            r = mid - 1;
        }
    }
    return -1; // not found
}

/**
 * @brief flashstorage_init - initialization of user conf storage
 * run in once @ start
 */
void flashstorage_init(){
    if(FLASH_SIZE > 0 && FLASH_SIZE < 20000){
        uint32_t flsz = FLASH_SIZE * blocksize; // size in bytes
        flsz -= (uint32_t)(&__varsstart) - FLASH_BASE;
        maxCnum = flsz / sizeof(user_conf);
    }
    // -1 if there's no data at all & flash is clear; maxnum-1 if flash is full
    currentconfidx = binarySearch((int)maxCnum-2, (const uint8_t*)Flash_Data, sizeof(user_conf));
    if(currentconfidx > -1){
        memcpy(&the_conf, &Flash_Data[currentconfidx], sizeof(user_conf));
    }
}

// store new configuration
// @return 0 if all OK
int store_userconf(){
    // maxnum - 3 means that there always should be at least one empty record after last data
    // for binarySearch() checking that there's nothing more after it!
    if(currentconfidx > (int)maxCnum - 3){ // there's no more place
        currentconfidx = 0;
        if(erase_flash(Flash_Data, (&__varsstart))) return 1;
    }else ++currentconfidx; // take next data position (0 - within first run after firmware flashing)
    return write2flash((const void*)&Flash_Data[currentconfidx], &the_conf, sizeof(the_conf));
}

static int write2flash(const void *start, const void *wrdata, uint32_t stor_size){
    int ret = 0;
    if (FLASH->CR & FLASH_CR_LOCK){ // unloch flash
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
    while (FLASH->SR & FLASH_SR_BSY);
    if(FLASH->SR & FLASH_SR_WRPRTERR){
        return 1; // write protection
    }
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR; // clear all flags
    FLASH->CR |= FLASH_CR_PG;
    const uint16_t *data = (const uint16_t*) wrdata;
    volatile uint16_t *address = (volatile uint16_t*) start;
    uint32_t i, count = (stor_size + 1) / 2;
    for (i = 0; i < count; ++i){
        IWDG->KR = IWDG_REFRESH;
        *(volatile uint16_t*)(address + i) = data[i];
        while (FLASH->SR & FLASH_SR_BSY);
        if(FLASH->SR &  FLASH_SR_PGERR){
            ret = 1; // program error - meet not 0xffff
            break;
        }else while (!(FLASH->SR & FLASH_SR_EOP));
        FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
    }
    FLASH->CR |= FLASH_CR_LOCK; // lock it back
    FLASH->CR &= ~(FLASH_CR_PG);
    return ret;
}

/**
 * @brief erase_flash - erase N pages of flash memory
 * @param start  - first address
 * @param end    - last address (or NULL if need to erase all flash remaining)
 * @return 0 if succeed
 */
static int erase_flash(const void *start, const void *end){
    int ret = 0;
    uint32_t nblocks = 1, flsz = 0;
    if(!end){ // erase all remaining
        if(FLASH_SIZE > 0 && FLASH_SIZE < 20000){
            flsz = FLASH_SIZE * blocksize; // size in bytes
            flsz -= (uint32_t)start - FLASH_BASE;
        }
    }else{ // erase a part
        flsz = (uint32_t)end - (uint32_t)start;
    }
    nblocks = flsz / blocksize;
    if(nblocks == 0 || nblocks >= FLASH_SIZE) return 1;
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
        FLASH->AR = (uint32_t)Flash_Data + i*blocksize; /* (2) */
        FLASH->CR |= FLASH_CR_STRT; /* (3) */
        while(!(FLASH->SR & FLASH_SR_EOP));
        FLASH->SR |= FLASH_SR_EOP; /* (5)*/
        if(FLASH->SR & FLASH_SR_WRPRTERR){ /* Check Write protection error */
            ret = 1;
            FLASH->SR |= FLASH_SR_WRPRTERR; /* Clear the flag by software by writing it at 1*/
            break;
        }
        FLASH->CR &= ~FLASH_CR_PER; /* (6) */
    }
    return ret;
}

void dump_userconf(_U_ char *txt){
#ifdef EBUG
    SEND("flashsize="); printu(FLASH_SIZE); bufputchar('*');
    printu(blocksize); bufputchar('='); printu(FLASH_SIZE*blocksize);
    newline();
#endif
    SEND("userconf_addr="); printuhex((uint32_t)Flash_Data);
    SEND("\nuserconf_idx="); printi(currentconfidx);
    SEND("\nuserconf_sz="); printu(the_conf.userconf_sz);
    SEND("\ncanspeed="); printu(the_conf.CANspeed);
    SEND("\ncanid="); printu(the_conf.CANID);
    // motors' data
    for(int i = 0; i < MOTORSNO; ++i){
        char cur = '0' + i;
#define PROPNAME(nm)    do{newline(); SEND(nm); bufputchar(cur); bufputchar('=');}while(0)
        PROPNAME("reverse");
        bufputchar('0' + the_conf.motflags[i].reverse);
        PROPNAME("microsteps");
        printu(the_conf.microsteps[i]);
        PROPNAME("accdecsteps");
        printu(the_conf.accel[i]);
        PROPNAME("maxspeed");
        printu(the_conf.maxspd[i]);
        PROPNAME("maxsteps");
        printu(the_conf.maxsteps[i]);
#undef PROPNAME
    }
    NL();
}
