CAN/USB management of 2 relays and some other things
===================================================

The device can also work as USB-CAN converter.

CAN protocol is quite simple. The node receives data to selected CAN ID, transmission done with the 
ID = CANID + 0x100.


Pinout:
- PA0/1 - Relay0/1 
- PA2..5 - Button0..3 (user buttons)
- PA6/7 - ADC inputs through resistive divider (0..5 and 0..12V)
- PA8..10 - PWM0..2 outputs (opendrain through SI2300, 5 or 12V)
- PA13/14 - SWD; the SWD pin +3.3V connected through 22Ohm resistor
- PA11. PA12 - USB DM/DP
- PB0..PB7 - CAN ID 
- PB8, PB9 - CAN Rx/Tx
- PB12..15 - LED0..3 outputs (direct outputs without any protection!!!)
 
The pins LEDr0 and LEDr1 are indicated relay works (12V through 2.2kOhm resistor)


### Buttons standalone

BTN1 - switch relay1

BTN2 - switch relay 2

BTN3 - change PWM0: hold to turn ON or turn OFF; press shortly BTN1/BTN2 to increase/decrease PWM0 to 1, 
hold BTN1/BTN2 to inc/dec PWM0 to 25 (do as many presses as need).