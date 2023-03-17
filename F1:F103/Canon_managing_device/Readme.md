# Canon lens management.

Management of some Canon Lens. Checked: EF85, EF200 and EF400.

[Documentation about first "crack" methods](https://github.com/eddyem/canon-lens).

## Pinout

* PA4 - lens detect (low active);
* PA5,6,7 - SPI SCK, MISO (should have external pullup), MOSI;
* PA8 - enable lens power (high active);
* PA9 - !OC - overcurrent detected (low active);
* PA10 - USB DP pullup (low active);
* PA11,12 - USB DM, DP;
* PA13,14 - SWD IO, CLK;
* PB8,9 - CAN Rx,Tx;
* PC13 - USB/CAN (solder jumper to turn on CAN instead of USB, zero - CAN).

## USB commands
Protocol have a string form, each string ends with '\n'. You should wait an answer for previous command before sending next, or have risk to miss all the rest commands in one packet.

### Base commands

> **0**  move to smallest foc value (e.g. 2.5m)

> **1**  move to largest foc value (e.g. infinity)

> **a**  move focus to given ABSOLUTE position or get current value (without number)

> **d**  open/close diaphragm by 1 step (+/-), open/close fully (o/c) (no way to know it current status)

> **f**  move focus to given RELATIVE position

> **h**  turn on hand focus management

> **i**  get lens information

> **l**  get lens model

> **r**  get regulators' state

### Debugging or configuration commands

> **A**  set (!0) or reset (0) autoinit

> **C**  set CAN speed (25-3000 kbaud)

> **D**  set CAN ID (11 bit)

> **E**  erase full flash storage

> **F**  change SPI flags (F f val), f== l-LSBFIRST, b-BR [18MHz/2^(b+1)], p-CPOL, h-CPHA

> **G**  get SPI status

> **I**  reinit SPI

> **L**  'flood' message (same as `S` but every 250ms until next command)

> **P**  dump current config

> **R**  software reset

> **S**  send data over SPI

> **T**  show Tms value

> **X**  save current config to flash
