/*
 * This file is part of the mlxtest project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#pragma once

#define REG_STATUS              0x8000
#define REG_STATUS_OVWEN        (1<<4)
#define REG_STATUS_NEWDATA      (1<<3)
#define REG_STATUS_SPNO         (1<<0)
#define REG_STATUS_SPMASK       (3<<0)
#define REG_CONTROL             0x800D
#define REG_CONTROL_CHESS       (1<<12)
#define REG_CONTROL_RES16       (0<<10)
#define REG_CONTROL_RES17       (1<<10)
#define REG_CONTROL_RES18       (2<<10)
#define REG_CONTROL_RES19       (3<<10)
#define REG_CONTROL_RESMASK     (3<<10)
#define REG_CONTROL_REFR_05HZ   (0<<7)
#define REG_CONTROL_REFR_1HZ    (1<<7)
#define REG_CONTROL_REFR_2HZ    (2<<7)
#define REG_CONTROL_REFR_4HZ    (3<<7)
#define REG_CONTROL_REFR_8HZ    (4<<7)
#define REG_CONTROL_REFR_16HZ   (5<<7)
#define REG_CONTROL_REFR_32HZ   (6<<7)
#define REG_CONTROL_REFR_64HZ   (7<<7)
#define REG_CONTROL_SUBP1       (1<<4)
#define REG_CONTROL_SUBPMASK    (3<<4)
#define REG_CONTROL_SUBPSEL     (1<<3)
#define REG_CONTROL_DATAHOLD    (1<<2)
#define REG_CONTROL_SUBPEN      (1<<0)

// default value
#define REG_CONTROL_DEFAULT     (REG_CONTROL_CHESS|REG_CONTROL_RES18|REG_CONTROL_REFR_2HZ|REG_CONTROL_SUBPEN)

// calibration data start & len
#define REG_CALIDATA            0x2400
#define REG_CALIDATA_LEN        832

#define REG_APTATOCCS           0x2410
#define REG_OSAVG               0x2411
#define REG_OCCROW14            0x2412
#define REG_OCCCOL14            0x2418
#define REG_SCALEACC            0x2420
#define REG_SENSIVITY           0x2421
#define REG_ACCROW14            0x2422
#define REG_ACCCOL14            0x2428
#define REG_GAIN                0x2430
#define REG_PTAT                0x2431
#define REG_KVTPTAT             0x2432
#define REG_VDD                 0x2433
#define REG_KVAVG               0x2434
#define REG_ILCHESS             0x2435
#define REG_KTAAVGODDCOL        0x2436
#define REG_KTAAVGEVENCOL       0x2437
#define REG_KTAVSCALE           0x2438
#define REG_ALPHA               0x2439
#define REG_CPOFF               0x243A
#define REG_KVTACP              0x243B
#define REG_KSTATGC             0x243C
#define REG_KSTO12              0x243D
#define REG_KSTO34              0x243E
#define REG_CT34                0x243F
#define REG_OFFAK1              0x2440
// index of register in array (from REG_CALIDATA)
#define CREG_IDX(addr)          ((addr)-REG_CALIDATA)

#define REG_IMAGEDATA           0x0400
#define REG_ITAVBE              0x0700
#define REG_ICPSP0              0x0708
#define REG_IGAIN               0x070A
#define REG_ITAPTAT             0x0720
#define REG_ICPSP1              0x0728
#define REG_IVDDPIX             0x072A
// index of register in array (from REG_IMAGEDATA)
#define IMD_IDX(addr)           ((addr)-REG_IMAGEDATA)
