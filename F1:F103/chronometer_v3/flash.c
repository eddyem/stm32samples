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

#include "adc.h"
#include "flash.h"
#include "lidar.h"
#include "str.h"
#include "usart.h"  // DBG
#include "usb.h"    // printout
#include <string.h> // memcpy

// max amount of records stored: Config & Logs
uint32_t maxCnum = FLASH_BLOCK_SIZE / sizeof(user_conf);
uint32_t maxLnum = FLASH_BLOCK_SIZE / sizeof(user_conf);

// common structure for all datatypes stored
/*typedef struct {
    uint16_t userconf_sz;
} flash_storage;*/

#define USERCONF_INITIALIZER  {             \
     .userconf_sz = sizeof(user_conf)       \
    ,.dist_min = LIDAR_MIN_DIST             \
    ,.dist_max = LIDAR_MAX_DIST             \
    ,.trigstate = 0                         \
    ,.trigpause = {400, 400, 400, 300}      \
    ,.USART_speed = USART1_DEFAULT_SPEED    \
    ,.LIDAR_speed = LIDAR_DEFAULT_SPEED     \
    ,.defflags = 0                          \
    ,.NLfreeWarn = 100                      \
    ,.ledshow_time = 5000                   \
    }

// change to placement
/*
__attribute__ ((section(".logs"))) const uint32_t *logsstart;
__attribute__ ((section(".myvars"))) const user_conf *Flash_Data;
*/

static int erase_flash(const void*, const void*);
static int write2flash(const void*, const void*, uint32_t);

const user_conf *Flash_Data = (const user_conf *)&__varsstart;
const event_log *logsstart = (event_log*) &__logsstart;
TODO("Add to event_log a comment - up to 8 chars")

user_conf the_conf = USERCONF_INITIALIZER;

static int currentconfidx = -1; // index of current configuration
static int currentlogidx = -1; // index of current logs record

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
#ifdef EBUG
    SEND("stor_size=");
    SEND(u2str(stor_size));
    newline(1);
#endif
    while(r >= l){
        int mid = l + (r - l) / 2;
#ifdef EBUG
    SEND("r=");
    SEND(u2str(r));
    SEND(", l=");
    SEND(u2str(l));
    SEND(", mid=");
    SEND(u2str(mid));
    newline(1);
#endif
        const uint8_t *s = start + mid * stor_size;
#ifdef EBUG
    SEND("data=");
    SEND(u2hex(*((const uint16_t*)s)));
    newline(1);
#endif
        if(*((const uint16_t*)s) == stor_size){
            if(*((const uint16_t*)(s + stor_size)) == 0xffff){
#ifdef EBUG
    SEND("\nindex=");
    SEND(u2str(mid));
    newline(1);
#endif
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
 * @brief flashstorage_init - initialization of user conf & logs storage
 * run in once @ start
 */
void flashstorage_init(){
    maxCnum = ((uint32_t)&_varslen) / sizeof(user_conf);
//SEND("maxCnum="); printu(1, maxCnum);
    if(FLASH_SIZE > 0 && FLASH_SIZE < 20000){
        uint32_t flsz = FLASH_SIZE * 1024; // size in bytes
        flsz -= (uint32_t)logsstart - FLASH_BASE;
        maxLnum = flsz / sizeof(event_log);
//SEND("\nmaxLnum="); printu(1, maxLnum);
    }
    // -1 if there's no data at all & flash is clear; maxnum-1 if flash is full
    currentconfidx = binarySearch((int)maxCnum-2, (const uint8_t*)Flash_Data, sizeof(user_conf));
    if(currentconfidx > -1){
        memcpy(&the_conf, &Flash_Data[currentconfidx], sizeof(user_conf));
    }
    currentlogidx = binarySearch((int)maxLnum-2, (const uint8_t*)logsstart, sizeof(event_log));
}

// store new configuration
// @return 0 if all OK
int store_userconf(){
    // maxnum - 3 means that there always should be at least one empty record after last data
    // for binarySearch() checking that there's nothing more after it!
    if(currentconfidx > (int)maxCnum - 3){ // there's no more place
        currentconfidx = 0;
        DBG("Need to erase flash!");
        if(erase_flash(Flash_Data, logsstart)) return 1;
    }else ++currentconfidx; // take next data position (0 - within first run after firmware flashing)
    return write2flash((const void*)&Flash_Data[currentconfidx], &the_conf, sizeof(the_conf));
}

/**
 * @brief store_log - save log record L into flash memory
 * @param L - event log (or NULL to delete flash)
 * @return 0 if all OK
 */
int store_log(event_log *L){
    if(!L){
        currentlogidx = -1;
        return erase_flash(logsstart, NULL);
    }
#ifdef EBUG
    SEND("currentlogidx=");
    SEND(u2str(currentlogidx));
    newline(1);
#endif
    if(currentlogidx > (int)maxLnum - 3){ // there's no more place
        /*currentlogidx = 0;
        DBG("Need to erase flash!");
        if(erase_flash(logsstart, NULL)) return 1;*/
        // prevent automatic logs erasing!
        sendstring("\n\nERROR!\nCan't save logs: delete old manually!!!\n");
        return 1;
    }else ++currentlogidx; // take next data position (0 - within first run after firmware flashing)
    // put warning if there's little space
    if(currentlogidx + the_conf.NLfreeWarn > (int)maxLnum - 3){
        uint32_t nfree = maxLnum - 2 - (uint32_t)currentlogidx;
        sendstring("\n\nWARNING!\nCan store only ");
        sendstring(u2str(nfree));
        sendstring(" logs!\n\n");
    }
    L->evtlog_sz = sizeof(event_log);
    return write2flash(&logsstart[currentlogidx], L, sizeof(event_log));
}

/**
 * @brief dump_log - dump N log records
 * @param start - first record to show (if start<0, then first=last+1-start)
 * @param Nlogs - amount of logs to show (if Nlogs<=0, then show all logs)
 * @return 0 if all OK, 1 if there's no logs in flash
 */
int dump_log(int start, int Nlogs){
    if(currentlogidx < 0) return 1;
#ifdef EBUG
    SEND("currentlogidx=");
    SEND(u2str(currentlogidx));
    SEND("\nstart=");
    SEND(u2str(start));
    SEND("\nNlogs=");
    SEND(u2str(Nlogs));
    newline(1);
#endif
    if(start < 0){
        start += currentlogidx + 1;
        if(start < 0){
            if(Nlogs == 1) return 1; // out of range
            else start = 0; // show all
        }
    }
    if(start > currentlogidx) return 1;
    int nlast;
    if(Nlogs > 0){
        nlast = start + Nlogs - 1;
        if(nlast > currentlogidx) nlast = currentlogidx;
    }else nlast = currentlogidx;
    ++nlast;
    const event_log *l = logsstart + start;
    for(int i = start; i < nlast; ++i, ++l){
        IWDG->KR = IWDG_REFRESH;
        sendstring(get_trigger_shot(i, l));
        if(Nlogs == 1){ // show on LED
            lastTtrig = Tms;
            lastLog.shottime = l->shottime;
        }
    }
    return 0;
}

static int write2flash(const void *start, const void *wrdata, uint32_t stor_size){
    int ret = 0;
    if (FLASH->CR & FLASH_CR_LOCK){ // unloch flash
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
    while (FLASH->SR & FLASH_SR_BSY);
    if(FLASH->SR & FLASH_SR_WRPRTERR){
        DBG("Can't remove write protection");
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
            DBG("FLASH_SR_PGERR");
            break;
        }else while (!(FLASH->SR & FLASH_SR_EOP));
        FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
    }
    FLASH->CR &= ~(FLASH_CR_PG);
    DBG("Flash stored");
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
            flsz = FLASH_SIZE * 1024; // size in bytes
            flsz -= (uint32_t)start - FLASH_BASE;
        }
    }else{ // erase a part
        flsz = (uint32_t)end - (uint32_t)start;
    }
    nblocks = flsz / FLASH_BLOCK_SIZE;
    if(nblocks == 0 || nblocks >= FLASH_SIZE) return 1;
    for(uint32_t i = 0; i < nblocks; ++i){
#ifdef EBUG
        SEND("Try to erase page #"); printu(1,i); newline(1);
#endif
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
            SEND("Error @ "); printu(1, i); newline(1);
            return;
        }
    }
    SEND("Curr idx: "); printu(1, currentconfidx); newline(1);
}

#endif
