Self-made chiller control system

UART: 115200N1, not more than 100ms between data bytes in command
To turn ON human terminal (without timeout) send "####"

Protocol: [ command line ]
'[' clears earlier input
'\n', '\r', ' ', '\t' are ignored
All messages are asynchroneous!

Commands:
Ax - turn on(x=1)/off(x=0) external alarm or check its value
Cx - set cooler PWM to x
d  - (only when EBUG defined) go into debug commands
F  - get flow sensor rate for 5s period
Hx - set heater PWM to x
L  - check water level sensor value
Px - set pump PWM to x
R  - reset MCU
Tx - get NTC temperature for channel x
t  - get MCU temperature
V  - get Vdd value *100V

Debugging commands:
A - show raw ADC value (next letter is index, 0..5)
F - get flow_cntr value & PB state
T - show raw T values
w - test watchdog


Messages:
MCUTEMP10=x     - mcu temperature * 10 (degrC)
SOFTRESET=1     - software reset occured (msg @ start)
VDD100=x        - Vdd*100 (V)
WDGRESET=1      - watchdog reset occured (msg @ start)
