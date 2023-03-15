# Canon lens management.

Management of some Canon Lens.

## Pinout

* PA4 - lens detect (zero active);
* PA5,6,7 - SPI SCK, MISO, MOSI;
* PA10 - USB DP pullup (zero active);
* PA11,12 - USB DM, DP;
* PA13,14 - SWD IO, CLK;
* PB8,9 - CAN Rx,Tx;
* PC13 - USB/CAN (solder jumper to turn on CAN instead of USB, zero - CAN).

Protocol have a string form, each string ends with '\n'. You should wait an answer for previous command before sending next,
or have risk to miss all the rest commands in one packet.

## USB commands:

0 - move to smallest foc value (e.g. 2.5m)
1 - move to largest foc value (e.g. infinity)
d - open/close diaphragm by 1 step (+/-), open/close fully (o/c) (no way to know it current status)
f - get focus state or move it to given relative position
h - turn on hand focus management
i - get lens information
l - get lens model
r - get regulators' state
                debugging commands:
F - change SPI flags (F f val), f== l-LSBFIRST, b-BR [18MHz/2^(b+1)], p-CPOL, h-CPHA
G - get SPI status
I - reinit SPI
R - software reset
S - send data over SPI
T - show Tms value


