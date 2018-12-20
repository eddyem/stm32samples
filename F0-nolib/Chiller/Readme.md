Self-made chiller control system

UART: 115200N1, not more than 100ms between data bytes in command
To turn ON human terminal (without timeout) send "####"

Protocol: [ command line ]
'[' clears earlier input
'\n', '\r', ' ', '\t' are ignored
All messages are asynchroneous!

Commands:
d - (only when EBUG defined) go into debug commands
R - reset MCU


Debugging commands:
w - test watchdog
A - show raw ADC value (next letter is index, 0..5)

Messages:
MCUTEMP10=x     - mcu temperature * 10 (degrC)
SOFTRESET=1     - software reset occured (msg @ start)
VDD100=x        - Vdd*100 (V)
WDGRESET=1      - watchdog reset occured (msg @ start)
