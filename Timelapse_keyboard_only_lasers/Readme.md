## Time-lapse as USB HID keyboard

This tool allow to get precision time of events: four lasers with sensors (0->1 event), pressing switch-button (1->0 event).

Just connect board to any device with USB keyboard support, after some time
(needed to establish GPS connection and precision timer setup) it will simulate
keyboard on which somebody types time of sensors' state changing


#### Sensors supprorted
* four lasers or other things, catch signal 0->1
* simple switch-button, catch signal 1->0

To get precision time this tool use GPS module (NEO-6M)

#### Connection diagram
| *Pin*  | *Function*  |
| :---: | :-------- |
| PA0  | Laser 1 |
| PA1  | Laser 2 |
| PA2  | GPS Rx (MCU Tx) |
| PA3  | GPS Tx (MCU Rx) |
| PA4  | GPS PPS signal |
| PA5  | Trigger (button) switch |
| PA6  | Input power measurement |
| PA7  | Laser 3 |
| PA8  | Laser 4 |
| PB9  | Beeper |
| * LEDS * |
| PA13 | Yellow LED1 - Laser1/4 found |
| PA15 | Yellow LED2 - PPS event |
| PB7  | Green LED1 - Laser3 found |
| PB8  | Green LED2 - power state: ON == 12V OK, BLINK == insufficient power |
| PB6  | Red LED1 - Laser2 found |
| PB7  | Red LED2 - GPS state: OFF == not ready, BLINK == need sync, ON == ready |


#### Powering devices
* To power up GPS module you can use +5V or +3.3V.
* Lasers & photosensors need 10..30V of power, I connect them to 12V
* Lasers' sensors connected to MCU through optocouples
