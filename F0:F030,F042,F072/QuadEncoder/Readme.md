Quadrature encoder
=============================

## GPIO

## UART 
115200N1, not more than 100ms between data bytes in command.
To turn ON human terminal (without timeout) send "####".

## Protocol
All commands are in brackets: `[ command line ]`.

'[' clears earlier input; '\n', '\r', ' ', '\t' are ignored.
All messages are asynchronous!

## Commands
[D] - get rotation direction
[R] - reset
[T] - get encoder position

When you rotate encoder you will see speed of its rotation in quaters of pulses per 10ms.
