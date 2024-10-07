Shutter control 
===============

Works with bi-stable shutter. 
You can find the device in `/dev/shutterX` (symlink to `/dev/ttyUSBX`).

After powered on, the device will try to close shutter (during one second). 

The CCD (or manual) controlled input allows to open/close shutter by changing its level (depending on `ccdactive` parameter). 
If current configuration can't open or close the shutter immediately, it will be opened/closed as soon as possible, while the input holds its level.

When the shutter is closed, the device will send over USB a message like
```
exptime=10000
shutter=closed
```
with calculated value of opened time and current shutter state. 
If the error occured and shutter can't be closed, user will read `exp=cantclose`, shutter will come into error state. You should close the shutter by external command in that case.

After the shutter completely opened, user can read a message `shutter=opened`.

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

Any wrong long message will echo back. Any wrong short command will show help list.

## Shutter control
Commands '0' ...  '3' should be used only for debugging purposes.
To open/close shutter use only 'O', 'C' and 'E' commands.

When opening or closing shutter you will first receive an answer: `OK` if command could be done or `ERR` if there's insufficient voltage on capacitor or shutter is absent.
After opened the message `shutter=opened` will appear. After closing you will receive messages `exptime=xxx` (when `xxx` is approx. exp. time in milliseconds) and `shutter=closed`.

Command 'E' could return `OK`, `ERR` or `ERRNUM`/`I32OVERFLOW` in wrong number format (number could be decimal, 0x.. - hexadecimal, b.. - binary or 0.. - octal).
"Status" command in this case will return `shutter=exposing` and `expfor=xxx` - the given exposition time.

When exposition starts you will receive message `OK` and `shutter=opened`. After its end you'll got `exptime=...`, `shutter=closed`.
If shutter can't be closed, you will give a lots of `exp=cantclose` and different error messages until problem be solved. To stop this error messages give command 'O'.

## Different commands
* 'A' will show raw values for all ADC channels: 0. - capacitor voltage, 1 - MCU temperature, 2 - MCU Vdd. You will give messages like `adcX=val`.

* 't' - `mcut=val`, where val = T*10 degrC.

* 'T' - `tms=val`, val in ms.

* 'v' - `vdd=val`, val in V*100

* 'V' - `voltage=val`, val in V*100

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
shtrvdiv=25
```

* `userconf_sz` - "magick" number to determine first non-empty record in flash storage. The aligned size of one record in bytes.
* `ccdactive` - is level of 'CCD' input to open shutter (1 to open on high and 0 to open on low signal), change this value with command `c`.
* `hallactive` - the same for Hall sensor or reed switch mounted on shutter to indicate opened state, change with `h`.
* `minvoltage` - voltage level on discharged capacitor (V*100 Volts), when shutter is in opened/closed state, the power will be off from coils reaching this level. Can be from 1V to 10V. Change with `<`.
* `workvoltage` - minimal level (V*100 Volts) of charged capacitor to start process of opening/closing, you can open/close shutter only with higher voltage. Can be from 5V to 100V. Change with `>`.
* `shuttertime` - maximal time (milliseconds) of holding active voltage on shutter's coils. Can be from 5ms to 1s. Change with `#`.
* `waitingtime` - waiting time for the shutter to finish its job. The expose time can't be less than this value. Can be from 5ms to 1s. Change with `$`.
* `shtrvmul` - multiplier to calculate capacitor voltage from voltage on ADC input of MCU. Vcap = V*shtrvmul/shtrvdiv. Can be from 1 to 65535. Change with `*`.
* `shtrvdiv` - divider of capacitor voltage calculation. Can be from 1 to 65535. Change with `/`.

All changes of configuration processed in RAM and acts immediately after changed. If you want to store changes, use command `s`.