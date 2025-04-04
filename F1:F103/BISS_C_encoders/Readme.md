Get data from 2 encoders by BISS-C protocol
===========================================

This device works with two BISS-C encoders (resolution from 8 to 32 bit).

If you want to test readout from device, run `./testDev /dev/encoder_X0` or `./testDev /dev/encoder_X0`.

** Encoder cable pinout:

* 1 - NC or shield
* 2 - CLK_A - positive of SSI clock out
* 3 - CLC_B - negative of SSI clock out
* 4 - NC or shield
* 5 - +5V - 5V power for sensor (at least 250mA)
* 6 - DATA_A - positive of data in
* 7 - DATA_B - negative of data in
* 8 - NC
* 9 - Gnd - common ground


** Device interfaces

After connection you will see device 0483:5740 with three CDC interfaces. Each interface
have its own `iInterface` field, by default they are: 

* encoder_cmd - configure/command/debugging interface
* encoder_X - X sensor output
* encoder_Y - Y sensor output

Add to udev-rules file `99-myHW.rules` which will create symlinks to each interface in `/dev/` directory.
You can change all three `iInterface` values and store them in device' flash memory.

The readout of encoders depends on settings. If you save in flash `autom=1`, readout of both encoders
will repeat each `amperiod` milliseconds. Also you always can ask for readout sending any '\n'-terminated
data into encoder's interface or running commands `readenc`, `readX` or `readY` in command interface.


** Protocol

The device have simple text protocol, each text string should be closed by '\n'.
Base format is 'param [ = value]', where 'param' could be command to run some procedure
or getter, 'value' is setter.

Answer for all getters is 'param=value'. Here are answers for setters and procedures:

* OK - all OK
* FAIL - procedure failed to run
* BADCMD - your entered wrong command
* BADPAR - parameter of setter is out of allowable range

Some procedures (like 'help' or 'readenc') returns a lot of data after calling.


*** Base commands on command interface

These are commands for directrly work with SPI interfaces:

* readenc - read both encoders once
* readX - read X encoder once
* readY - read Y encoder once
* help - show full help
* reset - reset MCU
* spideinit - deinit SPI
* spiinit - init SPI
* spistat - get status of both SPI interfaces


*** Configuration commands

This set of commands allows to change current configuration (remember: each time SPI configuration changes
you need to run `spiinit`) and store it into flash memory.

* autom - turn on or off automonitoring
* amperiod - period of monitoring, 1..255 milliseconds
* BR - change SPI BR register (1 - 18MHz ... 7 - 281kHz)
* CPHA - change CPHA value (0/1)
* CPOL - change CPOL value (0/1)
* dumpconf - dump current configuration
* encbits - set encoder data bits amount (26/32)
* encbufsz - change encoder auxiliary buffer size (8..32 bytes)
* erasestorage - erase full storage or current page (=n)
* maxzeros - maximal amount of zeros in preamble
* minzeros - minimal amount of zeros in preamble
* setiface1 - set name of first (command) interface
* setiface2 - set name of second (axis X) interface
* setiface3 - set name of third (axis Y) interface
* storeconf - store configuration in flash memory

Here is example of default configuration ouput (`dumpconf`):

```
userconf_sz=108
currentconfidx=-1
setiface1=
setiface2=
setiface3=
autom=0
amperiod=1
BR=4
CPHA=0
CPOL=1
encbits=26
encbufsz=12
maxzeros=50
minzeros=4
```

`userconf_sz` is some type of "magick sequence" to find start of last record in flash memory.
`currentconfidx` shows number of record (-1 means that the storage is empty and you see default values).
Empty field of `setifaceX` means default interface name.


*** Debugging commands

Some of these commands could be usefull when you're trying to play with settings or want to measure maximal
readout speed for encoders (when each reading starts immediately after parsing previous result).

* dummy - dummy integer setter/getter
* fin - reinit flash (e.g. to restore last configuration)
* sendx - send text string to X encoder's terminal
* sendy - send text string to Y encoder's terminal
* testx - test X-axis throughput
* testy - test Y-axis throughput

