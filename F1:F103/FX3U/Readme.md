A usefull thing made of chineese FX3U clone
===========================================

Works over RS-232 (default: 115200, 8N1), CAN (default: 250000 baud)
or MODBUS-RTU (default: 9600, 8N1).

You can see pinout table in file `hardware.c`.

## Serial protocol (each string ends with '\n').

Values in parentheses after flags command is its bit number in whole uint32_t.
E.g. to reset flag "f_relay_inverted" you can call `f_relay_inverted=0` or 
`flags2=0`.


```

commands format: parameter[number][=setter]
parameter [CAN idx] - help
--------------------------

CAN bus commands:
canbuserr - print all CAN bus errors (a lot of if not connected)
cansniff - switch CAN sniffer mode
s - send CAN message: ID 0..8 data bytes

Configuration:
bounce [14] - set/get anti-bounce timeout (ms, max: 1000)
canid [6] - set both (in/out) CAN ID / get in CAN ID
canidin [7] - get/set input CAN ID
canidout [8] - get/set output CAN ID
canspeed [5] - get/set CAN speed (bps)
dumpconf - dump current configuration
eraseflash [10] - erase all flash storage
f_relay_inverted (2) - inverted state between relay and inputs
f_send_esw_can (0) - change of IN will send status over CAN with `canidin`
f_send_relay_can (1) - change of IN will send also CAN command to change OUT with `canidout`
f_send_relay_modbus (3) - change of IN will send also MODBUS command to change OUT with `modbusidout` (only for master!)
flags [17] - set/get configuration flags (as one U32 without parameter or Nth bit with)
modbusid [20] - set/get modbus slave ID (1..247) or set it master (0)
modbusidout [21] - set/get modbus slave ID (0..247) to send relay commands
modbusspeed [22] - set/get modbus speed (1200..115200)
saveconf [9] - save configuration
usartspeed [15] - get/set USART1 speed

IN/OUT:
adc [4] - get raw ADC values for given channel
esw [12] - anti-bounce read inputs
eswnow [13] - read current inputs' state
led [16] - work with onboard LED
relay [11] - get/set relay state (0 - off, 1 - on)

Other commands:
inchannels [18] - get u32 with bits set on supported IN channels
mcutemp [3] - get MCU temperature (*10degrC)
modbus - send modbus request with format "slaveID fcode regaddr nregs [N data]", to send zeros you can omit rest of 'data'
modbusraw - send RAW modbus request (will send up to 62 bytes + calculated CRC)
outchannels [19] - get u32 with bits set on supported OUT channels
reset [1] - reset MCU
time [2] - get/set time (1ms, 32bit)
wdtest - test watchdog


```

Value in square brackets is CAN bus command code.

The INs are changed compared to original "FX3U" clone: instead of the absent IN8 I use the on-board 
button "RUN".

## CAN bus protocol

Default CAN speed is 250kbaud.  Default CAN ID: 1 and 2 for slave. 
All data in little-endian format!

BYTE -  MEANING

0, 1 - (uint16_t) - command code (value in square brackets upper);

2 - (uint8_t) - parameter number (e.g. ADC channel or X/Y channel number), 0..126 [ORed with 0x80 for setter], 127 means "no parameter";

3 - (uint8_t) - error code (only when device answers for requests);

4..7 - (int32_t) - data.

When device receives CAN packet with its ID or ID=0 ("broadcast" message) it check this packet, perform some action and sends answer
(usually - getter). If command can't be execute or have wrong data (bad command, bad parameter number etc) the device sends back
the same packet with error code inserted.

When runnming getter you can send only three bytes: command code and parameter number. Sending "no parameter" instead of parno
means in some commands "all data" (e.g. get/set all relays or get all inputs).

### CAN bus error codes

0 - `ERR_OK`       - all OK,

1 - `ERR_BADPAR`   - parameter is wrong,

2 - `ERR_BADVAL`   - value is wrong (e.g. out of range),

3 - `ERR_WRONGLEN` - wrong message length (for setter or for obligatory parameter number),

4 - `ERR_BADCMD`   - unknown command code,

5 - `ERR_CANTRUN`  - can't run given command due to bad parameters or other reason.

### CAN bus command codes

Number - enum from canproto.h - text command analog

0 - CMD_PING - ping

1- CMD_RESET - reset

2 - CMD_TIME - time

3 - CMD_MCUTEMP - mcutemp

4 - CMD_ADCRAW - adc

5 - CMD_CANSPEED - canspeed

6 - CMD_CANID - canid

7 - CMD_CANIDin - canidin

8 - CMD_CANIDout - canidout

9 - CMD_SAVECONF - saveconf

10 - CMD_ERASESTOR - eraseflash

11 - CMD_RELAY - relay

12 - CMD_GETESW - esw

13 - CMD_GETESWNOW - eswnow

14 - CMD_BOUNCE - bounce

15 - CMD_USARTSPEED - usartspeed

16 - CMD_LED - led

17 - CMD_FLAGS - flags

18 - CMD_INCHNLS - inchannels

19 - CMD_OUTCHNLS - outchannels

20 - CMD_MODBUSID - modbusid

21 - CMD_MODBUSIDOUT - modbusidout

22 - CMD_MODBUSSPEED - modbusspeed

### Examples of CAN commands (bytes of data transmitted with given ID)

(all data in HEX)

Get current time: "02 00 00". Answer something like "02 00 00 00 de ad be ef", where last four bytes
is time value (in milliseconds) from powering on.

Set relay number 5: "0b 00 85 00 01 00 00 00", answer: "0b 00 05 00 01 00 00 00".

Set relays 0..3 and reset other: "0b 00 ff 00 07 00 00 00", answer: "0b 00 7f 00 07 00 00 00".

Changing flags is the same as for text command: with parameter number (0..31) it will only change given
bit value, without ("no par" - 0x7f) will change all bits like whole uint32_t.

## MODBUS-RTU protocol

The device can work as master or slave.  Default format is 9600-8N1. BIG ENDIAN (like standard requires).
Default device ID is 1 ans 2 for target of "relay command" (if ID would be changed to 0 and flag `f_send_relay_modbus` set.

To run in master mode you should set its modbus ID to zero. Command `modbus` lets you to send strict 
formal modbus packet in format "slaveID fcode regaddr nregs [N data]" (all are space-delimited numbers in
decimal, hexadecimal (e.g. 0xFF), octal (e.g. 075) or binary (e.g. 0b1100110) format.
Here "slaveID" - one byte; "fcode" - one byte; "regaddr" - two bytes big endian; "nregs" - two bytes big endian;
"N" - one byte; "data" - N bytes.
Optional data bytes allowed only for "multiple" functions. In case of simple setters "nregs" is two bytes data
sent to slave. 

The command `modbusraw` will not check your data, just add CRC and send into bus.

In master mode you can activate flag `f_send_relay_modbus`. In this case each time the IN state changes
device will send command with ID=`modbusidout` to change corresponding relays. So, like for CAN commands
you can bind several devices to transmit IN states of one to OUT states of another. 
If `modbusidout` is zero, master will send broadcasting command. Slaves non answer for broadcast, only making
required action.

The hardware realisation of modbus based on UART4. Both input and output works over DMA, signal of packet end
is IDLE interrupt. This device doesn't supports full modbus protocol realisation: no 3.5 idle frames as packet
end; no long packets (input buffer is 68 bytes, allowing no more that 67 bytes; output buffer is 64 bytes, allowing
no more that 64 bytes). Maximal modbus slave ID is 247. You can increase in/out buffers size changing value of
macros `MODBUSBUFSZI` and `MODBUSBUFSZO` in `modbusrtu.h`.

In slave mode device doesn't support whole CAN-bus commands range. Next I describe allowed commands.

There are five holding registers. "[R]" means read-only, "[W]" - write-only, "[RW]" - read and write.

0 - MR_RESET [W] - reset MCU.

1 - MR_TIME [RW] - read or set MCU time (milliseconds, uint32_t).

2 - MR_LED [RW] - read or change on-board LED state.

3 - MR_INCHANNELS [R] - get uint32_t value where each N-th bit means availability of N-th IN channel 
(e.g. if 9th channel is physically absent 9th bit would be 0).

### Supported functional codes

#### 01 - read coil
Read state of all relays. Obligatory regaddr="00 00", nregs="00 N", where "N" is 8-multiple
number (in case of 10-relay module: 8 or 16). You will reseive N/8 bytes of data with relays' status (e.g. most 
lest significant bit is state or relay0, next - relay1 and so on).

Example: "01 01 00 00 00 10" - read state of all relays. If only relay 10 active you will 
receive: "01 01 02 04 00".

Errors: "02" - "regaddr" isn't zero; "03" - N isn't multiple of 8 or too large.

#### 02 - read discrete input
Read state of all discrete inputs. Input/output parameters are the same like for "read coil".

Example: "01 02 00 00 00 08" - read 8 first INs. Answer if first 4 inputs active (disconnected):
"01 02 01 0f".

Errors: like for "read coil".

#### 03 - read holding register
You can read value of non write-only registers. You can read only one register by time.

Example: "01 03 00 01 00 01" - read time. Answer: "01 03 04 00 15 53 01", where 0x00155301 is 1397.505 seconds 
from device start.

Errors: "02" - "regaddr" is wrong, "03" - "regno" isn't 1.

#### 04 - read input register
Read "nregs" ADC channels starting from "regaddr" number.

Example: "01 04 00 05 00 04" - read channels 5..8.  
Answer: "01 04 08 08 6c 00 21 00 33 00 41" - got 0x86c (2156) for 5th channel and so on.

Errors: "02" - wrong starting number, "03" - wrong amount (zero or N+start > last channel available).

#### 05 - write coil
Change single relay state. "nregs" is value (0 - off, non-0 - on), "regaddr" is relay number.

Example: "01 05 00 03 00 01" - turn 3rd relay on. Answer: "01 05 00 03 00 01".

Errors: "02" - wrong relay number.

#### 06 - write holding register
Write data to non read-only register (reset MCU, change time value or turn LED on/off). 

Example: "01 06 00 02 00 01" - turn LED on. Answer: "01 06 00 02 00 01".

Errors: "02" - wrong register.

#### 0f - write multiple coils
Change state of all relays by once. Here "regaddr" should be "00 00", 
"nregs" should be multiple of 8, "N" should be equal ("nregs"+7)/8. Each data bit means nth relay state.

Example: "01 0f 00 00 00 08 01 ff" - turn on relays 0..7.
Answer: "01 0f 00 00 00 08".

Errors: "02" - "regaddr" isn't zero, "03" - wrong amount of relays, "07" - can't change relay values.


#### 10 - write multiple registers
You can write only four "registers" by once changing appropriate uint32_t value. 
The only "register" you can change is 01 - MR_TIME. "nregs" should be equal 1.

Example: "01 10 00 01 00 01 04 00 00 00 00" - clears Tms counter, starting time from zero.
Answer: "01 10 00 01 00 01".

Errors: "02" - wrong register.


### Error codes
01 - ME_ILLEGAL_FUNCION - The function code received in the request is not an authorized action for the slave.

02 - ME_ILLEGAL_ADDRESS - The data address received by the slave is not an authorized address for the slave.

03 - ME_ILLEGAL_VALUE - The value in the request data field is not an authorized value for the slave.

04 - ME_SLAVE_FAILURE - The slave fails to perform a requested action because of an unrecoverable error.

05 - ME_ACK - The slave accepts the request but needs a long time to process it.

06 - ME_SLAVE_BUSY - The slave is busy processing another command.

07 - ME_NACK - The slave cannot perform the programming request sent by the master.

08 - ME_PARITY_ERROR - Memory parity error: slave is almost dead.




# Short programming guide

## Adding a new value to flash storage

All storing values described in structure `user_conf` (`flash.h`). You can add there any new value
but be carefull with 32-bit alignment. Bit flags stored as union `confflags_t` combining 32-bit and 1-bit access.
After you add this new value don't forget to add setter/getter and string describing it in function `dumpconf`.

Text protocol allows you to work with flags by their semantic name. So to add some flag you should also modify
`proto.c`:

- add text constant with flag name;
- add address of this constant into `bitfields` array (according to bit order in flags);
- add appropriate enum into `text_cmd`;
- add appropriate string into `funclist`: pointer to string constant, enum field and help text;
- modify function `confflags` for setter/getter of new flag.


## Adding a new command

All base commands are processed in files `canproto.c` and `proto.c`. `modbusproto.c` is for modbus-specific commands.

To add CAN/serial command you should first add a field to anonimous enum in `canproto.h`, which will be number code of 
given CANbus command. Codes of serial-only commands are stored in enum `text_cmd` of file `proto.c`. 

### Add both CAN/serial command
- add enum in `canproto.c`;
- add string const with text name of this command in `proto.c`;
- add string to `funclist` with address of string const, enum and help;
- add command handler into `canproto.c` and describer into array `funclist` (index should be equal
to command code, struct consists from pointer to handler, minimal and maximal value and minimal data length
of can packet). If min==max then argument wouldn't be checked.

The handler returns one of `errcodes` and have as argument only pointer to `CAN_message` structure. 
So, serial command parser before call this handler creates CAN packet from user data.
Format of command is next: "cmd[X][=VAL]", where "cmd" is command text, "X" - optional parameter,
"VAL" - value for setter. So the packet would be "C C P 0 VAL0 VAL1 VAL2 VAL3", where
"C" - command code, "P" - parameter number (or 0x7f" if X is omit) OR'ed with 0x80 for setter,
VALx - xth byte (little endian) of user value.

For setting/getting uint32_t paramegers (especially configuration parameters) you can use handler `u32setget`.
For bit flags - `flagsetget`.

To work with bit-flags by particular name use `confflags` handler of `proto.c`.

### Add serial-only command
In this case there's no CAN handler. You work only with `proto.c`.
- add enum in `text_cmd`;
- add string const with text name of this command;
- add string to `funclist` with address of string const, enum and help;
- add command handler;
- add pointer to this handler into `textfunctions` array.

Handler also returns one of `errcodes`, but have next arguments:
- `const char *txt` - all text (excluding spaces in beginning) after command in user string;
- `text_cmd command` - number of command (useful when you have common handler for several commands).

## Working with modbus

All exceptions and functional codes described as enums in `modbusrtu.h`. 
To form request or responce use structs `modbus_request` and `modbus_responce`. `data` fields in this 
structs is big-endian storing bytes in order like they will be sent via RS-485.
Amount of data bytes should be not less then `datalen` value. For requests that don't need data, 
`data` may be NULL regardless `datalen` (for Fcode <= 6). `regno` is amount of registers or 
data written to register dependent on `Fcode`.
The responce struct of error codes have NULL in `data` and `datalen` is appropriate exception code.

All high-level commands are in `modbusproto.c`. To add new `register` you should edit `modbus_regusters`
enum in `modbusproto.h`.

The main parsing pipeline is `parse_modbus_request` in `modbusproto.c`. 
Here you can add parsing of new functional codes.

To work with new "registers" edit `readreg`, `writereg` or `writeregs`.
