/*
 * common_macros.h - common usable things
 *
 * Copyright 2018 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 */
#pragma once
#ifndef __COMMON_MACROS_H__
#define __COMMON_MACROS_H__

#ifndef TRUE_INLINE
#define TRUE_INLINE  __attribute__((always_inline)) static inline
#endif

#ifndef NULL
#define NULL (0)
#endif

// some good things from CMSIS
#define nop()   __NOP()

#define pin_toggle(gpioport, gpios)  do{  \
    register uint32_t __port = gpioport->ODR;  \
    gpioport->BSRR = ((__port & gpios) << 16) | (~__port & gpios);}while(0)

#define pin_set(gpioport, gpios)  do{gpioport->BSRR = gpios;}while(0)
#define pin_clear(gpioport, gpios) do{gpioport->BSRR = (gpios << 16);}while(0)
#define pin_read(gpioport, gpios) (gpioport->IDR & gpios ? 1 : 0)
#define pin_write(gpioport, gpios)  do{gpioport->ODR = gpios;}while(0)



#endif // __COMMON_MACROS_H__