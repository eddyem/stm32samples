/*
 * This file is part of the Chiller project.
 * Copyright 2018 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "hardware.h"
#include "protocol.h"
#include "usart.h"



/**
 * @brief process_command - command parser
 * @param command - command text (all inside [] without spaces)
 * @return text to send over terminal or NULL
 */
char *process_command(const char *command){
    char *ret = NULL;
    usart1_sendbuf(); // send buffer (if it is already filled)
    switch(*command){
        case '?': // help
            SEND_BLK(
                "D - get rotation direction\n"
                "R - reset\n"
                "T - get timer value\n"
                );
        break;
        case 'D':
            if(TIM3->CR1 & TIM_CR1_DIR) SEND("negative\n");
            else SEND("positive\n");
        break;
        case 'R': // reset MCU
            NVIC_SystemReset();
        break;
        case 'T':
            put_string("TIM3->CNT=");
            put_uint(TIM3->CNT);
            put_char('\n');
        break;
    }
    usart1_sendbuf();
    return ret;
}
