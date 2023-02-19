/*
 * This file is part of the flash project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32g0.h>
#include <string.h> // memcpy
#include "flash.h"
#include "strfunc.h"
#include "usart.h"

extern const uint32_t __varsstart, _BLOCKSIZE;

static const uint32_t blocksize = (uint32_t)&_BLOCKSIZE;

// max amount of Config records stored (will be recalculate in flashstorage_init()
static uint32_t maxCnum = 1024 / sizeof(user_conf); // can't use blocksize here

#define DEFMF   {.haveencoder = 1, .donthold = 1, .eswinv = 1, .keeppos = 1}

#define USERCONF_INITIALIZER  {             \
     .userconf_sz = sizeof(user_conf)       \
    ,.flagU16 = 0xabcd                      \
    ,.flagU32 = 0xdeadbeef                  \
    ,.str = "test string"                   \
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
        if(erase_flash(Flash_Data, NULL)) return 1;
    }else ++currentconfidx; // take next data position (0 - within first run after firmware flashing)
    return write2flash((const void*)&Flash_Data[currentconfidx], &the_conf, sizeof(the_conf));
}

static int write2flash(const void *start, const void *wrdata, uint32_t stor_size){
    int ret = 0;
    if (FLASH->CR & FLASH_CR_LOCK){ // unloch flash
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
    // FLASH_SR_BSY2 for some
    uint32_t count = (stor_size + 7) / 8;
    volatile uint32_t *address = (volatile uint32_t*) start;
    const uint32_t *data = (const uint32_t*) wrdata;
    for(uint32_t i = 0; i < count; ++i){
        while (FLASH->SR & (FLASH_SR_BSY1)); // 1: check BSY1
        if(FLASH->SR & FLASH_SR_WRPERR){ // 2: check errors
            return 1; // write protection
        }
        FLASH->SR = 0xffff; // clear all flags
        FLASH->CR |= FLASH_CR_PG; // 3: set PG bit
        IWDG->KR = IWDG_REFRESH;
        *address++ = *data++; // 4: write both 32 bit words
        *address++ = *data++;
        while(FLASH->SR & FLASH_SR_BSY1);
        if(FLASH->SR &  FLASH_SR_PGSERR){
            ret = 1; // program error - meet not 0xffff
            break;
        }/*else{
            for(int _ = 0; _ < 9999 && (!(FLASH->SR & FLASH_SR_EOP)); ++_);
        }*/
        FLASH->SR = 0xffff;
    }
    FLASH->CR &= ~(FLASH_CR_PG); // 7: clear PG bit at end of process
    FLASH->CR |= FLASH_CR_LOCK; // lock it back
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
    if(!start) return 1;
    uint32_t startb = (((uint32_t)start - FLASH_BASE) + blocksize - 1) / blocksize, endb;
    if(!end){ // erase all remaining
        endb = FLASH_SIZE / blocksize;
    }else{ // erase a part
        endb = (((uint32_t)end - FLASH_BASE) + blocksize - 1) / blocksize;
    }
    if ((FLASH->CR & FLASH_CR_LOCK) != 0){
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
    for(uint32_t i = startb; i < endb; ++i){
        IWDG->KR = IWDG_REFRESH;
        /* (1) Wait till no operation is on going */
        /* (2) Clear error & EOP bits */
        while ((FLASH->SR & FLASH_SR_BSY1) != 0){} /* (1) */
        FLASH->SR = 0xffff;  /* (2) */
        /* (1) Set the PER bit in the FLASH_CR register to enable page erasing */
        /* (2) Select the page to erase (PNB) */
        /* (3) Set the STRT bit in the FLASH_CR register to start the erasing */
        /* (4) Wait until BSY1 cleared */
        FLASH->CR |= FLASH_CR_PER | i << FLASH_CR_PNB_Pos; /* (1) (2) */
        FLASH->CR |= FLASH_CR_STRT; /* (3) */
        while ((FLASH->SR & FLASH_SR_BSY1) != 0){} /* (4) */
        FLASH->SR = FLASH_SR_EOP;
        if(FLASH->SR & FLASH_SR_WRPERR){ /* Check Write protection error */
            ret = 1;
            FLASH->SR = FLASH_SR_WRPERR; /* Clear the flag by software by writing it at 1*/
            break;
        }
        FLASH->CR &= ~FLASH_CR_PER; // clear PER
    }
    return ret;
}

void dump_userconf(){
    SEND("flashsize="); printu(FLASH_SIZE);
    SEND("\nuserconf_addr="); printuhex((uint32_t)Flash_Data);
    SEND("\nuserconf_idx="); usart3_sendstr(i2str(currentconfidx));
    SEND("\nuserconf_sz="); printu(the_conf.userconf_sz);
    SEND("\nflagU16="); printuhex(the_conf.flagU16);
    SEND("\nflagU32="); printuhex(the_conf.flagU32);
    SEND("\nstr="); SEND(the_conf.str);
    newline();
    usart3_sendbuf();
}

int erase_storage(){
    return erase_flash(Flash_Data, NULL);
}
