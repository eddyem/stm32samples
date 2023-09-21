Shutter control 
===============

Works with bi-stable shutter. 

Pinout:
PB0 (pullup in) - hall (or reed switch) sensor input (active low) - opened shutter detector
PB11 (pullup in) - CCD or button input: open at low signal, close at high

PA3 (ADC in) - shutter voltage (approx 1/12 U)
PA5 (PP out) - TLE5205 IN2
PA6 (PP out) - TLE5205 IN1
PA7 (pullup in) - TLE5205 FB
PA10 (PP out) - USB pullup (active low)
PA11,12 - USB
PA13,14 - SWD


Commands:
