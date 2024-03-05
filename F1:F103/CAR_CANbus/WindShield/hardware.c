/*
 * This file is part of the windshield project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "buttons.h"
#include "hardware.h"

/* pinout:

| **Pin #** | **Pin name ** | **function** | **settings**           | **comment **                            |
| --------- | ------------- | ------------ | ---------------------- | --------------------------------------- |
| 10        | PA0           | Vsen         | Ain                    | motors current                          |
| 11        | PA1           | Power_EN     | slow PP (1)            | enable 5V power DC-DC                   |
| 12        | PA2           | L_UP         | slow PP (0)            | turn on left up semibridge              |
| 13        | PA3           | R_UP         | slow PP (0)            | turn on right up semibridge             |
| 16        | PA6           | R_DOWN       | TIM3_ch1 PWM out       | PWM of right down semibridge            |
| 17        | PA7           | L_DOWN       | TIM3_ch2 PWM out       | PWM of left up semibridge               |
| 30        | PA9           | USART Tx     | AFPP                   | USART commands                          | +
| 31        | PA10          | USART Rx     | AF floating in         | (test and so on)                        | +
| 32        | PA11          | DOWN_SW      | in pullup              | down Hall switch                        |
| 33        | PA12          | UP_SW        | in pullup              | upper Hall switch                       |
| 45        | PB8           | CAN Rx       | in floating or AF flin | CAN bus                                 | +
| 46        | PB9           | CAN Tx       | AFPP                   | -//-                                    | +
| 25        | PB12          | UP_DIR       | in pulldown            | high when got command to move window up |
| 26        | PB13          | DOWN_DIR     | in pulldown            | -//- down (when connected to old wires) |
| 27        | PB14          | UP_BTN       | in pullup              | button control - move up                |
| 28        | PB15          | DOWN_BTN     | in pullup              | -//- down                               |

 */

// last incr/decr time and minimal time between incr/decr CCRx
static uint32_t PWM_lasttime = 0, PWM_deltat = 1; // PWM_deltat should be more than timer period
static int accel = 0; // rotation direction and acceleration/deceleration
static motdir_t direction = MOTDIR_NONE; // current moving direction

void gpio_setup(void){
    // PB8 & PB9 (CAN) setup in can.c; PA9 & PA10 (USART) in usart.c; PA6 & PA7 - TIM3 PWM ch1/2
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    // pullups & initial values
    GPIOA->ODR = (1<<11) | (1<<12);
    GPIOB->ODR = (1<<14) | (1<<15);
    GPIOA->CRL = CRL(0, CNF_ANALOG) | CRL(1, CNF_PPOUTPUT|MODE_SLOW) | CRL(2, CNF_PPOUTPUT|MODE_SLOW) |
                 CRL(3, CNF_PPOUTPUT|MODE_SLOW) | CRL(6, CNF_AFPP|MODE_FAST) | CRL(7, CNF_AFPP|MODE_FAST);
    GPIOA->CRH = CRH(11, CNF_PUDINPUT) | CRH(12, CNF_PUDINPUT);
    GPIOB->CRH = CRH(12, CNF_PUDINPUT) | CRH(13, CNF_PUDINPUT) | CRH(14, CNF_PUDINPUT) | CRH(15, CNF_PUDINPUT);
    // setup timer: 100 ticks per full PWM range, 2kHz -> 200kHz timer frequency
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; // enable TIM3 clocking
    TIM3->CR1 = 0;
    TIM3->PSC = (TIM3FREQ/(PWMFREQ*PWMMAX)) - 1;  // 359 ticks for 200kHz
    TIM3->ARR = PWMMAX - 1;
    TIM3->CCR1 = 0;   // inactive
    TIM3->CCR2 = 0;
    // PWM mode 1 (active->inactive)
    TIM3->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE |
                  TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;
    // main PWM output
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
}

// set minimal pause between successive CCRx increments or decrements; return TRUE if OK
int set_dT(uint32_t d){
    if(d > 1000) return FALSE;
    PWM_deltat = d + 1;
    return TRUE;
}

// stop or start rotation in given direction (only if motor stops); return TRUE if OK
int motor_ctl(int32_t dir){
    if(dir < MOTDIR_STOP || dir > MOTDIR_DOWN) return FALSE;
    if(dir == MOTDIR_NONE) return TRUE;
    if(direction == dir) return TRUE; // already do this
    if(TIM3->CR1 & TIM_CR1_CEN){
        if(direction > MOTDIR_NONE && dir > MOTDIR_NONE) return FALSE; // motor is moving while trying to move it into another dir
        if(direction == MOTDIR_BREAK && dir < MOTDIR_NONE) return TRUE; // already stopped
    }
    if(dir == MOTDIR_STOP){ // stop motor -> deceleration
        if(direction == MOTDIR_UP || direction == MOTDIR_DOWN) accel = -1;
        return TRUE;
    }else if(dir == MOTDIR_BREAK){ // emergency stop
        if(TIM3->CR1 & TIM_CR1_CEN) motor_break(); // break only if is moving
        return TRUE;
    }
    accel = 1;
    direction = dir;
    keyevent uh = keyevt(HALL_U), dh = keyevt(HALL_D);
    if(dir == MOTDIR_UP){ // start in positive direction (move UP)
        if(uh == EVT_PRESS || uh == EVT_HOLD) return FALSE; // can't move in given direction
        set_up(UP_LEFT);
        set_pwm(PWM_RIGHT, 1);
    }else{ // negative (move DOWN)
        if(dh == EVT_PRESS || dh == EVT_HOLD) return FALSE;
        set_up(UP_RIGHT);
        set_pwm(PWM_LEFT, 1);
    }
    PWM_lasttime = Tms;
    start_pwm();
    return TRUE;
}

// extremal stop
void motor_break(){
    up_off();
    direction = MOTDIR_BREAK;
    accel = 0;
    stop_pwm();
    PWM_lasttime = Tms;
}

// simplest state machine of motor control
void motor_process(){
    if(process_keys()){ // check buttons
        keyevent evt;
        motdir_t newdir = MOTDIR_NONE; // new moving direction (-2 - do nothing, 2 - emerg. stop)
        // first, check HALL sensors if motor is moving
        if(direction){
            if(keystate(HALL_D, &evt)){ // HALL DOWN
                if(direction < 0 && (evt == EVT_PRESS || evt == EVT_HOLD)){
                    newdir = MOTDIR_BREAK;
                }
            }else if(keystate(HALL_U, &evt)){
                if(direction > 0 && (evt == EVT_PRESS || evt == EVT_HOLD)){
                    newdir = MOTDIR_BREAK;
                }
            }
        }
        // short key pressed - full open/close (or stop while moving); long - move with stop after release
        if(MOTDIR_BREAK != newdir){ // process keys if don't need to break
            evt = EVT_NONE;
            if(keystate(KEY_U, &evt)){
                if(evt == EVT_PRESS){
                    if(direction == MOTDIR_UP || direction == MOTDIR_DOWN) newdir = MOTDIR_STOP;
                    else newdir = MOTDIR_UP;
                }
            }else if(keystate(KEY_D, &evt)){
                if(evt == EVT_PRESS){
                    if(direction == MOTDIR_UP || direction == MOTDIR_DOWN) newdir = MOTDIR_STOP;
                    else newdir = MOTDIR_DOWN;
                }
            }
            if(evt == EVT_RELEASE) newdir = MOTDIR_STOP; // stop after long press
            evt = EVT_NONE;
            int extsig = FALSE;
            // now chech external signals, they have an advantage over local keys
            if(keystate(DIR_U, &evt)){ // react on PRESS and both NONE/RELEASE
                extsig = TRUE;
                if(evt == EVT_PRESS) newdir = MOTDIR_UP;
            }else if(keystate(DIR_D, &evt)){
                extsig = TRUE;
                if(evt == EVT_PRESS) newdir = MOTDIR_DOWN;
            }
            if(evt == EVT_RELEASE || (extsig && evt == EVT_NONE)){ // EVT_NONE - released after short press - do nothing; EVT_RELEASE - after hold, stop
                newdir = MOTDIR_STOP;
            }
        }
        motor_ctl(newdir);
        clear_events();
    }
    if(direction == MOTDIR_NONE) return; // motor isn't moving
    else if(direction == MOTDIR_BREAK && 0 == (TIM3->CR1 & TIM_CR1_CEN)){ // motor stopped
        direction = MOTDIR_NONE;
        accel = 0;
        return;
    }
    if(Tms < PWM_lasttime + PWM_deltat) return;
    volatile uint16_t *CCRx = (direction > 0) ? &TIM3->CCR1 : &TIM3->CCR2; // current CCRx
    PWM_lasttime = Tms;
    if(accel < 0){ // deceleration
        if(*CCRx == 0){ // stopped
            motor_break(); // wait for another cycle
            return;
        }
        // TODO: here we should check currents and if failure turn off immediatelly
        --*CCRx;
    }else{ // constant speed or acceleration
        // TODO: here we should check currents and increment only if all OK; decrement and change to accel if fail
        if(accel){ // acceleration
            if(*CCRx == PWMMAX) accel = 0;
            else ++*CCRx;
        } // else do nothing - moving with constant speed
    }
}

void iwdg_setup(){
    uint32_t tmout = 16000000;
    RCC->CSR |= RCC_CSR_LSION;
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;}
    IWDG->KR = IWDG_START;
    IWDG->KR = IWDG_WRITE_ACCESS;
    IWDG->PR = IWDG_PR_PR_1;
    IWDG->RLR = 1250;
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;}
    IWDG->KR = IWDG_REFRESH;
}
