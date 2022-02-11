Loopback control of three stepper motors
========================================


# Pinout

- **PA0** Enc2a (motor2 encoder)
- **PA1** Enc2b
- **PA2** CLK1 (motor1 clock)
- **PA3** ADC1 (ADC1 in, 0-3.3V)
- **PA4** CLK2 (motor2 clock)
- **PA5** ADC2 (ADC2 in, 0-3.3V)
- **PA6** CLK3 (motor3 clock)
- **PA7** PWM (opendrain PWM, up to 12V)
- **PA8** Enc1a (motor1 encoder)
- **PA9** Enc1b
- **PA10** BTN1 (user button 1)
- **PA11** USBDM
- **PA12** USBDP
- **PA13** BTN2 (user button 2)
- **PA14** BTN3 (user button 3)
- **PA15** BTN4 (user button 4)
- **PB0** ~EN1 (motor1 not enable)
- **PB1** DIR1 (motor1 direction)
- **PB2** ~EN2 (motor2 not enable)
- **PB3** Buzzer (external buzzer or other non-inductive opendrain load up to 12V)
- **PB4** Enc3a (motor3 encoder)
- **PB5** Enc3b
- **PB6** I2C SCL (external I2C bus, have internal pullups of 4.7kOhm to +3.3V)
- **PB7** I2C SDA
- **PB8** CAN Rx (external CAN bus, with local galvanic isolation
- **PB9** CAN Tx
- **PB10** DIR2 (motor2 direction)
- **PB11** ~EN3 (motor3 not enable)
- **PB12** DIR3 (motor3 direction)
- **PB13** Ext0 (3 external outputs: 5V, up to 20mA)
- **PB14** Ext1
- **PB15** Ext2
- **PC13** ESW1 (motor1 zero limit switch)
- **PC14** ESW2 (motor2 zero limit switch)
- **PC15** ESW3 (motor3 zero limit switch)
- **PF0** Relay (10A 250VAC, 10A 30VDC)

# Connectors

## ADC inputs connector, J1

1. ADC1 (up to 3.3V)
2. ADC2 (up to 3.3V)
3. GND

## Encoders connectors, J2-J4

1. GND
2. Encoder B phase
3. Encoder A phase
4. +5V (through resistor 22Ohm)

## I2C connector, J5

1. +3.3V
2. SCL
3. SDA
4. GND

## PWM connector, J6
1. +3.3V
2. +5V
3. PWM GND (opendrain)

## CAN bus connector, J7

1. CANL (low signal)
2. CANGND (common - not need for short lines)
3. CANH (high signal)

## External buttons connector (WARNING! NO ESD PROTECTION!), J8

1. Button 1
2. Button 2
3. Button 3
4. Button 4
5. GND

## External Hall sensors connector (zero limit switches), J9

1. +3.3V (through resistor 47Ohm)
2. Motor1 limit switch
3. Motor2 limit switch
4. Motor3 limit switch
5. GND

## Relay connector, J10

1. Normally opened
2. Common
3. Normally closed

## 24V input power connector, J12

1. GND
2. +24V DC

## 24V motors power connector, J13

1. GND
2. +24V DC to motors' coils (reverse protected)

## Stepper motors control signals connectors, J14-J16

1. CLK (step signal)
2. DIR (rotation direction)
3. ~EN (not enable)
4. GND

## External 5V logic outputs connector (up to 20mA per each channel), J17

1. Ext0
2. Ext1
3. Ext2
4. GND

## External buzzer (or other load) connector (opendrain, up to 12V), J18

1. power (depending on JP1 jumper): 3.3V or 5.0V
2. GND (opendrain)

# Control points

- **TP1**  5V
- **TP2**  3.3V
- **TP3**  NRST
- **TP4**  GND

# Firmware download

Activate "Jump to DFU" menu entry through USB protocol. Flash MCU by `dfu-util`.

# USB protocol
https://github.com/eddyem/stm32samples/tree/master/F0-nolib/3steppersLB build#138 @ 2021-12-02
Common commands format is cmd[ N[ = val]]
        where N is command argument (0..127), val is its value
Different commands:
        adc - get ADC values
        button - get buttons state
        buzzer - change buzzer state (1/0)
        esw - get end switches state
        ext - external outputs
        mcut - get MCU T
        mcuvdd - get MCU Vdd
        ping - echo given command back
        pwm - pwm value
        relay - change relay state (1/0)
        reset - reset MCU
        time - get time from start
Confuguration:
        accel - set/get accel/decel (steps/s^2)
        encrev - set/get max encoder's pulses per revolution
        encstepmax - maximal encoder ticks per step
        encstepmin - minimal encoder ticks per step
        eswreact - end-switches reaction
        maxspeed - set/get max speed (steps per sec)
        maxsteps - set/get max steps (from zero)
        microsteps - set/get microsteps settings
        minspeed - set/get min speed (steps per sec)
        motflags - set/get motorN flags
        saveconf - save current configuration
        speedlimit - get limiting speed for current microsteps
Motors' commands:
        abspos - set/get position (in steps)
        emerg - emergency stop all motors
        emstop - emergency stop motor (right now)
        encpos - set/get encoder's position
        gotoz - find zero position & refresh counters
        motreinit - re-init motors after configuration changed
        relpos - set relative steps, get remaining
        relslow - set relative steps @ lowest speed
        state - get motor state
        stop - smooth motor stopping
USB-only commands:
        canid - get/set CAN ID
        canspeed - CAN bus speed
        delignlist - delete ignore list
        dfu - activate DFU mode
        dumperr - dump error codes
        dumpcmd - dump command codes
        dumpconf - dump current configuration
        filter - add/modify filter, format: bank# FIFO# mode(M/I) num0 [num1 [num2 [num3]]]
        getctr - get TIM1/2/3 counters
        ignbuf - print ignore buffer
        ignore - add ID to ignore list (max 10 IDs)
        listfilters - list all active filters
        pause - pause IN packets displaying
        resume - resume IN packets displaying
        send - send data over CAN: send ID byte0 .. byteN
        wd - check watchdog

# CAN bus protocol

bytes   descr
0       Lcmd - command code
1       Hcmd
2       par  - command paremeter (steper, ADC channel number etc)
3       err  - error code (only in answer)
4       Ldata - optional data in int32_t
5
6
7       Hdata

dumperr
Find known command: dumperr
Error codes:
0 - all OK
1 - wrong parameter's value
2 - wrong setter of parameter
3 - bad message length
4 - unknown command
5 - temporary can't run given command


dumpcmd
Find known command: dumpcmd
Commands list:
0 - Different commands:
1 - change relay state (1/0)
2 - change buzzer state (1/0)
3 - get ADC values
4 - get buttons state
5 - get end switches state
6 - get MCU T
7 - get MCU Vdd
8 - reset MCU
9 - get time from start
10 - pwm value
11 - external outputs
12 - save current configuration
13 - minimal encoder ticks per step
14 - maximal encoder ticks per step
15 - set/get microsteps settings
16 - set/get accel/decel (steps/s^2)
17 - set/get max speed (steps per sec)
18 - set/get min speed (steps per sec)
19 - get limiting speed for current microsteps
20 - set/get max steps (from zero)
21 - set/get max encoder's pulses per revolution
22 - set/get motorN flags
23 - end-switches reaction
24 - re-init motors after configuration changed
25 - set/get position (in steps)
26 - set relative steps, get remaining
27 - set relative steps @ lowest speed
28 - emergency stop motor (right now)
29 - smooth motor stopping
30 - emergency stop all motors
31 - find zero position & refresh counters
32 - get motor state
33 - set/get encoder's position
