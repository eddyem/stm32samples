/*
 * This file is part of the rtc project.
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

#include "rtc.h"

#ifndef WAITWHILE
#define WAITWHILE(x)  do{register uint32_t StartUpCounter = 0; while((x) && (++StartUpCounter < 0xffffff)){nop();}}while(0)
#endif

void rtc_setup(){
    PWR->CR1 |= PWR_CR1_DBP; // disable RTC write protection
    // turn on LSE and switch RTC to it
    RCC->APBENR1 |= RCC_APBENR1_RTCAPBEN;
    RCC->BDCR = RCC_BDCR_LSEON;
    WAITWHILE(!(RCC->BDCR & RCC_BDCR_LSERDY));
    RCC->BDCR |= RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_0;
    // unlock RCC
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
    RTC->ICSR |= RTC_ICSR_INIT;
    WAITWHILE(!(RTC->ICSR & RTC_ICSR_INITF));
    RTC->PRER = (0x7f << RTC_PRER_PREDIV_A_Pos) | 0xff; // prediv_a = 127, prediv_s = 255
    // here we can init starting time: monday of 1 jan 2001, 00:00:00
//    RTC->DR = 1 << RTC_DR_YU_Pos | 1 << RTC_DR_WDU_Pos | 1 << RTC_DR_MU_Pos | 1 << RTC_DR_DU_Pos;
//    RTC->TR = 0;
    // RTC->CR is default - 0
    // now turn off INIT bit, RTC will go on
    RTC->ICSR &= ~RTC_ICSR_INIT;
}

void get_curtime(rtc_t *t){
    WAITWHILE(!(RTC->ICSR & RTC_ICSR_RSF));
    register uint32_t r = RTC->TR;
    #define BCDu(shift)     ((r >> shift) & 0xf)
    t->second = BCDu(RTC_TR_SU_Pos) + 10 * BCDu(RTC_TR_ST_Pos);
    t->minute = BCDu(RTC_TR_MNU_Pos) + 10 * BCDu(RTC_TR_MNT_Pos);
    t->hour = BCDu(RTC_TR_HU_Pos) + 10 * BCDu(RTC_TR_HT_Pos);
    r = RTC->DR;
    t->day = BCDu(RTC_DR_DU_Pos) + 10 * BCDu(RTC_DR_DT_Pos);
    t->month = BCDu(RTC_DR_MU_Pos);
    if(r & RTC_DR_MT) t->month += 10;
    t->weekday = (r >> RTC_DR_WDU_Pos) & 0x7;
    t->year = BCDu(RTC_DR_YU_Pos) + 10 * BCDu(RTC_DR_YT_Pos);
}
