Shutter control 
===============

Works with bi-stable shutter. 
You can find the device in `/dev/shutterX` (symlink to `/dev/ttyUSBX`).

## Pinout

**PB0** (pullup in) - hall (or reed switch) sensor input (active low) - opened shutter detector

**PB11** (pullup in) - CCD or button input: open at low signal, close at high

**PA3** (ADC in) - shutter voltage (approx 1/12 U)

**PA5** (PP out) - TLE5205 IN2

**PA6** (PP out) - TLE5205 IN1

**PA7** (pullup in) - TLE5205 FB

**PA10** (PP out) - USB pullup (active low)

**PA11**, **PA12** - USB

**PA13**, **PA14** - SWD

## Commands

###    debugging options:
* '0' - shutter OPE
* '1' - shutter CLO
* '2' - shutter OFF
* '3' - shutter HIZ
* 'W' - test watchdog

###    configuration:
* '< n' - voltage on discharged capacitor (*100)
* '> n' - voltage on fully charged capacitor (*100)
* '# n' - duration of electrical pulse to open/close shutter (ms)
* '$ n' - duration of mechanical work to completely open/close shutter (ms)
* '* n' - shutter voltage multiplier
* '/ n' - shutter voltage divider (V=Vadc*M/D)
* 'c n' - open shutter when CCD ext level is n (0/1)
* 'd' - dump current config
* 'e' - erase flash storage
* 'h n' - shutter is opened when hall level is n (0/1)
* 's' - save configuration into flash

###    common control:
* 'A' - get raw ADC values
* 'C' - close shutter / abort exposition
* 'E n' - expose for n milliseconds
* 'O' - open shutter
* 'R' - software reset
* 'S' - get shutter state; also hall and ccd inputs state (1 - active)
* 't' - get MCU temperature (/10degC)
* 'T' - get Tms
* 'v' - get Vdd (/100V)
* 'V' - get shutter voltage (/100V)


If you will enter wrong long message, will receive its echo back. Any wrong short command will show help list.

## Shutter control
Commands '0', '1' and '2' should be used only for debugging purposes.
To open/close shutter use only 'O', 'C' and 'E' commands.

When opening or closing shutter you will first receive an answer: `OK` if command could be done or `ERR` if there's insufficient voltage on capacitor or shutter is absent.
After opened the message `shutter=opened` will appear. After closing you will receive messages `exptime=xxx` (when `xxx` is approx. exp. time in milliseconds) and `shutter=closed`.

Command 'E' could return `OK`, `ERR` or `ERRNUM`/`I32OVERFLOW` in wrong number format (number could be decimal, 0x.. - hexadecimal, b.. - binary or 0.. - octal).

When exposition starts you will receive message `OK` and `shutter=opened`. After its end you'll got `exptime=...`, `shutter=closed`.
If shutter can't be closed, you will give a lots of "exp=cantclose" and different error messages until problem be solved. To stop this error messages give command 'O'.

## Different commands
* 'A' will show raw values for all ADC channels: 0. - capacitor voltage, 1 - MCU temperature, 2 - MCU Vdd. You will give messages like `adcX=val`.

* 't' - `mcut=val`, where val = T*10 degrC.

* 'T' - `tms=val`, val in ms.

* 'v' - `vdd=val`, val in V*100

* 'V' - "voltage=val", val in V*100

* 'S' - several answers:
  * `shutter=`: `closed`, `opened`, `error`, `process`, `wait` or `exposing` - shutter state 
  * `expfor=...` (only when exposing by command Exxx) - show given exposition time
  * `exptime=...` (only when shutter is opened) - show time since opening
  * `regstate=`: `open`, `close`, `off` or `hiZ` - TLE5205 outputs state
  * `fbstate=`: `0` or `1` - TLE5205 FB out state (1 - error: insufficient voltage or shutter absent)
  * `hall=`: `0` or `1` - 1 for opened shutter, 0 for closed
  * `ccd=`: `0` or `1` - 1 for active (closed contacts) state of "CCD" input

## Configuration
All configuration stored in MCU flash memory, to dump current config just enter command 'd' and you will give an answer like
```
userconf_sz=16
ccdactive=1
hallactive=0
minvoltage=400
workvoltage=700
shuttertime=20
waitingtime=30
shtrvmul=143
shtrdiv=25
```

* ``
* `ccdactive` - is level of 'CCD' input to open shutter (1 to open on high and 0 to open on low signal)
* `hallactive`