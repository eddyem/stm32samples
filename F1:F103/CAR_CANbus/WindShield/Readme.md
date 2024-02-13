Manage windshields over CAN
===========================

## Pinout

### Sorted by pin number

| **Pin #** | **Pin name ** | **function** | **settings**           | **comment **                            |
| --------- | ------------- | ------------ | ---------------------- | --------------------------------------- |
| 1         | VBAT          | 3v3          |                        |                                         |
| 2         | PC13          | NC           |                        |                                         |
| 3         | PC14          | NC           |                        |                                         |
| 4         | PC15          | NC           |                        |                                         |
| 5         | OCS_IN        | 8MHz         | default                |                                         |
| 6         | OSC_OUT       | 8MHz         | default                |                                         |
| 7         | NRST          | NRST conn    |                        | reset MCU                               |
| 8         | VSSA          | gnd          |                        |                                         |
| 9         | VDDA          | 3v3          |                        |                                         |
| 10        | PA0           | Vsen         | Ain                    | motors current                          |
| 11        | PA1           | Power_EN     | slow PP (1)            | enable 5V power DC-DC                   |
| 12        | PA2           | L_UP         | slow PP (0)            | turn on left up semibridge              |
| 13        | PA3           | R_UP         | slow PP (0)            | turn on right up semibridge             |
| 14        | PA4           | NC           |                        |                                         |
| 15        | PA5           | NC           |                        |                                         |
| 16        | PA6           | R_DOWN       |       TIM3_ch1 PWM out | PWM of right down semibridge            |
| 17        | PA7           | L_DOWN       |       TIM3_ch2 PWM out | PWM of left up semibridge               |
| 18        | PB0           | NC           |                        |                                         |
| 19        | PB1           | NC           |                        |                                         |
| 20        | PB2           | NC           |                        |                                         |
| 21        | PB10          | NC           |                        |                                         |
| 22        | PB11          | NC           |                        |                                         |
| 23        | VSS1          | gnd          |                        |                                         |
| 24        | VDD1          | 3v3          |                        |                                         |
| 25        | PB12          | UP_DIR       | in pulldown            | high when got command to move window up |
| 26        | PB13          | DOWN_DIR     | in pulldown            | -//- down (when connected to old wires) |
| 27        | PB14          | UP_BTN       | in pullup              | button control - move up                |
| 28        | PB15          | DOWN_BTN     | in pullup              | -//- down                               |
| 29        | PA8           | NC           |                        |                                         |
| 30        | PA9           | USART Tx     | AFPP                   | USART commands                          |
| 31        | PA10          | USART Rx     | AF floating in         | (test and so on)                        |
| 32        | PA11          | DOWN_SW      | in pullup              | down Hall switch                        |
| 33        | PA12          | UP_SW        | in pullup              | upper Hall switch                       |
| 34        | PA13          | SWDIO        |                        | program/debug                           |
| 35        | VSS2          | gnd          |                        |                                         |
| 36        | VDD2          | 3v3          |                        |                                         |
| 37        | PA14          | SWCLK        |                        | program/debug                           |
| 38        | PA15          | NC           |                        |                                         |
| 39        | PB3           | NC           |                        |                                         |
| 40        | PB4           | NC           |                        |                                         |
| 41        | PB5           | NC           |                        |                                         |
| 42        | PB6           | NC           |                        |                                         |
| 43        | PB7           | NC           |                        |                                         |
| 44        | BOOT0         | BOOT conn    |                        | bootloader activate                     |
| 45        | PB8           | CAN Rx       | in floating or AF flin | CAN bus                                 |
| 46        | PB9           | CAN Tx       | AFPP                   | -//-                                    |
| 47        | VSS3          | gnd          |                        |                                         |
| 48        | VDD3          | 3v3          |                        |                                         |


## Sorted by pin name

| **Pin #** | **Pin name ** | **function** | **settings**           | **comment **                            |
| --------- | ------------- | ------------ | ---------------------- | --------------------------------------- |
| 44        | BOOT0         | BOOT conn    |                        | bootloader activate                     |
| 10        | PA0           | Vsen         | Ain                    | motors current                          |
| 11        | PA1           | Power_EN     | slow PP (1)            | enable 5V power DC-DC                   |
| 12        | PA2           | L_UP         | slow PP (0)            | turn on left up semibridge              |
| 13        | PA3           | R_UP         | slow PP (0)            | turn on right up semibridge             |
| 16        | PA6           | R_DOWN       |       TIM3_ch1 PWM out | PWM of right down semibridge            |
| 17        | PA7           | L_DOWN       |       TIM3_ch2 PWM out | PWM of left up semibridge               |
| 30        | PA9           | USART Tx     | AFPP                   | USART commands                          |
| 31        | PA10          | USART Rx     | AF floating in         | (test and so on)                        |
| 32        | PA11          | DOWN_SW      | in pullup              | down Hall switch                        |
| 33        | PA12          | UP_SW        | in pullup              | upper Hall switch                       |
| 34        | PA13          | SWDIO        |                        | program/debug                           |
| 37        | PA14          | SWCLK        |                        | program/debug                           |
| 45        | PB8           | CAN Rx       | in floating or AF flin | CAN bus                                 |
| 46        | PB9           | CAN Tx       | AFPP                   | -//-                                    |
| 25        | PB12          | UP_DIR       | in pulldown            | high when got command to move window up |
| 26        | PB13          | DOWN_DIR     | in pulldown            | -//- down (when connected to old wires) |
| 27        | PB14          | UP_BTN       | in pullup              | button control - move up                |
| 28        | PB15          | DOWN_BTN     | in pullup              | -//- down                               |
| 5         | OCS_IN        | 8MHz         | default                |                                         |
| 6         | OSC_OUT       | 8MHz         | default                |                                         |
| 7         | NRST          | NRST conn    |                        | reset MCU                               |
| 8         | VSSA          | gnd          |                        |                                         |
| 9         | VDDA          | 3v3          |                        |                                         |

To format pretty tables [use](https://josh.fail/2022/pure-bash-markdown-table-generator/) `markdown-table -5 -s"|" < STM32F103C.md >> Readme.md`.
