## Time-lapse as USB HID keyboard

This tool allow to get precision time of events: pressing switch,
changing distance measured by ultrasonic or infrared sensor,
laser light disappearing on photoresistor

Just connect board to any device with USB keyboard support, after some time
(needed to establish GPS connection and precision timer setup) it will simulate
keyboard on which somebody types time of sensors' state changing


#### Sensors supprorted
* SHARP GP2Y0A02YK - infrared distance-meter
* HCSR04 - ultrasonic distance-meter
* simple switch-button
* photoresistor + laser

To get precision time this tool use GPS module (NEO-6M)

#### Connection diagram
| Pin  | Function  |
| :--: | :-------- |
| PA0 | Infrared sensor data |
| PA1 | Laser photoresistor data |
| PA2 | GPS Rx (MCU Tx) |
| PA3 | GPS Tx (MCU Rx) |
| PA4 | GPS PPS signal |
| PA5 | Trigger (button) switch |
| PB10 | Ultrasonic "TRIG" pin |
| PB11 | Ultrasonic "ECHO" pin |

#### Powering devices
* To power up GPS module you can use +5V or +3.3V.
* Ultrasonic & Infrared sensors need +5V power.
* Photoresistor should be connected to +3.3V by one pin, another pin (data) should be pulled to ground by 1kOhm resisror
