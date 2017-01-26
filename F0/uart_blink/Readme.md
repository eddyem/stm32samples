Code for STM32F030F4 chinese board
- Toggle onboard green LED once per second.
- Echo received by USART1 data (not more than 64 bytes including '\n') on B115200:
	- if PA6 not connected to ground echo text directly
	- otherwice echo it reversly
- Light up LED on PA5 (opendrain) when receive data and turn it off when send.
