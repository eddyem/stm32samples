/*
 *                                                                                                  geany_encoding=koi8-r
 * can.c
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#include "can.h"
#include "hardware.h"
#include "usart.h"

#define CMD_TOGGLE (0xDA)
#define CAN_ID_MASK (0xFF70U)
#define CAN_ID1 (0x651U)
#define CAN_ID2 (0x652U)
#define FILTER_LIST (0) /* 0: filter mode = identifier mask, 1: filter mode = identifier list */

#define CAN_FLAG_GOTDUMMY (1)
static uint8_t CAN_addr = 255;
static uint8_t CAN_flags = 0;

// get CAN address data from GPIO pins
void readCANaddr(){
    CAN_addr = READ_CAN_INV_ADDR();
    CAN_addr = ~CAN_addr & 0x7;
}

uint8_t getCANaddr(){
    return CAN_addr;
}

void CAN_setup(){
    if(CAN_addr == 255) readCANaddr();
    // Configure GPIO: PB8 - CAN_Rx, PB9 - CAN_Tx
    /* (1) Select AF mode (10) on PB8 and PB9 */
    /* (2) AF4 for CAN signals */
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9))
                 | (GPIO_MODER_MODER8_AF | GPIO_MODER_MODER9_AF); /* (1) */
    GPIOB->AFR[1] = (GPIOB->AFR[1] &~ (GPIO_AFRH_AFRH0 | GPIO_AFRH_AFRH1))\
                  | (4 << (0 * 4)) | (4 << (1 * 4)); /* (2) */
    /* Enable the peripheral clock CAN */
    RCC->APB1ENR |= RCC_APB1ENR_CANEN;
      /* Configure CAN */
    /* (1) Enter CAN init mode to write the configuration */
    /* (2) Wait the init mode entering */
    /* (3) Exit sleep mode */
    /* (4) Loopback mode, set timing to 1Mb/s: BS1 = 4, BS2 = 3, prescaler = 6 */
    /* (5) Leave init mode */
    /* (6) Wait the init mode leaving */
    /* (7) Enter filter init mode, (16-bit + mask, filter 0 for FIFO 0) */
    /* (8) Acivate filter 0 */
    /* (9) Identifier list mode */
    /* (11) Set the Id list */
    /* (12) Set the Id + mask (all bits of standard id will care) */
    /* (13) Leave filter init */
    /* (14) Set FIFO0 message pending IT enable */
    CAN->MCR |= CAN_MCR_INRQ; /* (1) */
    while((CAN->MSR & CAN_MSR_INAK)!=CAN_MSR_INAK) /* (2) */
    {
    /* add time out here for a robust application */
    }
    CAN->MCR &=~ CAN_MCR_SLEEP; /* (3) */
    CAN->BTR |= CAN_BTR_LBKM | 2 << 20 | 3 << 16 | 5 << 0; /* (4) */
    CAN->MCR &=~ CAN_MCR_INRQ; /* (5) */
    while((CAN->MSR & CAN_MSR_INAK)==CAN_MSR_INAK) /* (6) */
    {
    /* add time out here for a robust application */
    }
    CAN->FMR = CAN_FMR_FINIT; /* (7) */
    CAN->FA1R = CAN_FA1R_FACT0; /* (8) */
    #if (FILTER_LIST)
    CAN->FM1R = CAN_FM1R_FBM0; /* (9) */
    CAN->sFilterRegister[0].FR1 = CAN_ID2 << 5 | CAN_ID1 << (16+5); /* (10) */
    #else
    CAN->sFilterRegister[0].FR1 = CAN_ID1 << 5 | CAN_ID_MASK << 16; /* (11) */
    #endif /* FILTER_LIST */

    CAN->FMR &=~ CAN_FMR_FINIT; /* (12) */
    CAN->IER |= CAN_IER_FMPIE0; /* (13) */

    /* Configure IT */
    /* (14) Set priority for CAN_IRQn */
    /* (15) Enable CAN_IRQn */
    NVIC_SetPriority(CEC_CAN_IRQn, 0); /* (16) */
    NVIC_EnableIRQ(CEC_CAN_IRQn); /* (17) */
}

void can_proc(){
    if(CAN_flags){
        if(CAN_flags & CAN_FLAG_GOTDUMMY){
            SEND("Got dummy message\n");
        }
        CAN_flags = 0;
    }
}

void can_send_dummy(){
    if((CAN->TSR & CAN_TSR_TME0) == CAN_TSR_TME0){ /* check mailbox 0 is empty */
        CAN->sTxMailBox[0].TDTR = 1; /* fill data length = 1 */
        CAN->sTxMailBox[0].TDLR = CMD_TOGGLE; /* fill 8-bit data */
        CAN->sTxMailBox[0].TIR = (uint32_t)(CAN_ID1 << 21 | CAN_TI0R_TXRQ); /* fill Id field and request a transmission */
    }
}

void cec_can_isr(){
    uint32_t CAN_ReceiveMessage = 0;

    if(CAN->RF0R & CAN_RF0R_FMP0){ /* check if a message is filtered and received by FIFO 0 */
        LED_blink(LED1); /* Toggle LED1 */
        CAN_ReceiveMessage = CAN->sFIFOMailBox[0].RDLR; /* read data */
        CAN->RF0R |= CAN_RF0R_RFOM0; /* release FIFO */
        if((CAN_ReceiveMessage & 0xFF) == CMD_TOGGLE){
            CAN_flags |= CAN_FLAG_GOTDUMMY;
        }
    }
}
