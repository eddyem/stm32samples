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

static const uint8_t maxdays[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// i - in, r - rest of divide
#if 0
static uint16_t div10s(uint16_t i, uint16_t *r){ // divide by 10
    uint32_t u = i;
    u *= 52429;
    u >>= 19;
    if(r) *r = i - 10*u;
    return (uint16_t)u;
}
#endif
static uint8_t div10b(uint8_t i, uint8_t *r){ // divide by 10
    uint32_t u = i;
    u *= 52429;
    u >>= 19;
    if(r) *r = i - 10*u;
    return (uint8_t)u;
}

static uint8_t DEC2BCD(uint8_t x){
    uint8_t d, r;
    d = div10b(x, &r);
    return (d << 4 | r);
}

int rtc_setdate(rtc_t *d){
    if(d->year > 99) return FALSE;
    if(d->month > 12 || d->month == 0) return FALSE;
    if(d->day > maxdays[d->month - 1] || d->day == 0) return FALSE;
    if(d->month == 2 && d->day == 29 && (d->year & 4) != 4) return FALSE; // not leap year
    if(d->weekday > 7 || d->weekday == 0) return FALSE;
    RTC->ICSR |= RTC_ICSR_INIT;
    WAITWHILE(!(RTC->ICSR & RTC_ICSR_INITF));
    RTC->DR = DEC2BCD(d->year) << RTC_DR_YU_Pos | d->weekday << RTC_DR_WDU_Pos | DEC2BCD(d->month) << RTC_DR_MU_Pos | DEC2BCD(d->day) << RTC_DR_DU_Pos;
    RTC->ICSR &= ~RTC_ICSR_INIT;
    return TRUE;
}

int rtc_settime(rtc_t *t){
    if(t->hour > 23) return FALSE;
    if(t->minute > 59) return FALSE;
    if(t->second > 59) return FALSE;
    RTC->ICSR |= RTC_ICSR_INIT;
    WAITWHILE(!(RTC->ICSR & RTC_ICSR_INITF));
    RTC->TR = DEC2BCD(t->hour) << RTC_TR_HU_Pos | DEC2BCD(t->minute) << RTC_TR_MNU_Pos | DEC2BCD(t->second) << RTC_TR_SU_Pos;
    RTC->ICSR &= ~RTC_ICSR_INIT;
    return TRUE;
}

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

// set calibration value
int rtc_setcalib(int calval){
    if(calval < -511 || calval > 512) return FALSE;
    uint32_t calp = 0, calm = 0;
    if(calval < 0)  calm = -calval;
    else if(calval > 0){
        calp = RTC_CALR_CALP;
        calm = 512 - calval;
    }
    // unlock RCC
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
    RTC->CALR = calp | calm;
    return 1;
}

int rtc_getcalib(){
    int calval = (RTC->CALR & RTC_CALR_CALP) ? 512 : 0;
    calval -= RTC->CALR & 0x1ff;
    return calval;
}
