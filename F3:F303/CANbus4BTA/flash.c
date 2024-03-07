/*
 * This file is part of the multistepper project.
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

#include <stm32f3.h>
#include <string.h> // memcpy
#include "flash.h"
#include "strfunc.h"
#include "usb.h"

// from ld script
extern const uint32_t __varsstart, _BLOCKSIZE;

const uint32_t FLASH_blocksize = (uint32_t)&_BLOCKSIZE;

// max amount of Config records stored (will be recalculate in flashstorage_init()
static uint32_t maxCnum = 1024 / sizeof(user_conf); // can't use blocksize here


static int write2flash(const void*, const void*, uint32_t);
// don't write `static` here, or get error:
//      'memcpy' forming offset 8 is out of the bounds [0, 4] of object '__varsstart' with type 'uint32_t'
const user_conf *Flash_Data = (const user_conf *)(&__varsstart);

user_conf the_conf = {
    .userconf_sz = sizeof(user_conf),
    .CANspeed = 100000,
    .CANID = 0xaa,
    .bounce_ms = 50,
    .adcmul[0] = 10.930f,
    .adcmul[1] = 2.028f,
    .adcmul[2] = 1.f,
    .adcmul[3] = 1.f,
    .usartspeed = 115200,
    .flags = FLAGP(ENC_IS_SSI),
};

int currentconfidx = -1; // index of current configuration

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
        uint32_t flsz = FLASH_SIZE * 1024; // size in bytes
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
        if(erase_storage(-1)) return 1;
    }else ++currentconfidx; // take next data position (0 - within first run after firmware flashing)
    return write2flash((const void*)&Flash_Data[currentconfidx], &the_conf, sizeof(the_conf));
}

static int write2flash(const void *start, const void *wrdata, uint32_t stor_size){
    int ret = 0;
    if (FLASH->CR & FLASH_CR_LOCK){ // unloch flash
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
    while(FLASH->SR & FLASH_SR_BSY) IWDG->KR = IWDG_REFRESH;
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR; // clear all flags
    FLASH->CR |= FLASH_CR_PG;
    const uint16_t *data = (const uint16_t*) wrdata;
    volatile uint16_t *address = (volatile uint16_t*) start;
    USB_sendstr("Start address="); printuhex((uint32_t)start); newline();
    uint32_t i, count = (stor_size + 1) / 2;
    for(i = 0; i < count; ++i){
        IWDG->KR = IWDG_REFRESH;
        *(volatile uint16_t*)(address + i) = data[i];
        while (FLASH->SR & FLASH_SR_BSY) IWDG->KR = IWDG_REFRESH;
        if(*(volatile uint16_t*)(address + i) != data[i]){
            USB_sendstr("DON'T MATCH!\n");
            ret = 1;
            break;
        }
#ifdef EBUG
        else{ USB_sendstr("Written "); printuhex(data[i]); newline();}
#endif
        if(FLASH->SR & FLASH_SR_PGERR){
            USB_sendstr("Prog err\n");
            ret = 1; // program error - meet not 0xffff
            break;
        }
        FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;
    }
    FLASH->CR &= ~(FLASH_CR_PG);
    return ret;
}

// erase Nth page of flash storage (flash should be prepared!)
static int erase_pageN(int N){
    int ret = 0;
    FLASH->AR = (uint32_t)Flash_Data + N*FLASH_blocksize;
    FLASH->CR |= FLASH_CR_STRT;
    uint32_t xx = 100000;
    while((FLASH->SR & FLASH_SR_BSY) && --xx) IWDG->KR = IWDG_REFRESH;
    if(xx == 0) return 1;
    FLASH->SR = FLASH_SR_EOP;
    if(FLASH->SR & FLASH_SR_WRPERR){ /* Check Write protection error */
        ret = 1;
        FLASH->SR = FLASH_SR_WRPERR; /* Clear the flag by software by writing it at 1*/
    }
    return ret;
}

// erase full storage (npage < 0) or its nth page; @return 0 if all OK
int erase_storage(int npage){
    int ret = 0;
    uint32_t end = 1, start = 0, flsz = 0;
    if(FLASH_SIZE > 0 && FLASH_SIZE < 20000){
        flsz = FLASH_SIZE * 1024; // size in bytes
        flsz -= (uint32_t)Flash_Data - FLASH_BASE;
    }
    DBG("total free blocks: "); printu(flsz / FLASH_blocksize);
    end = flsz / FLASH_blocksize;
    if(end == 0) return 1;
    if(npage > -1){ // erase only one page
        if((uint32_t)npage >= end) return 1;
        start = npage;
        end = start + 1;
    }
    if((FLASH->CR & FLASH_CR_LOCK) != 0){
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
    while(FLASH->SR & FLASH_SR_BSY) IWDG->KR = IWDG_REFRESH;
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;
    FLASH->CR |= FLASH_CR_PER;
    __disable_irq();
    for(uint32_t i = start; i < end; ++i){
        IWDG->KR = IWDG_REFRESH;
        if(erase_pageN(i)){
            ret = 1;
            break;
        }
    }
    __enable_irq();
    FLASH->CR &= ~FLASH_CR_PER;
    return ret;
}

