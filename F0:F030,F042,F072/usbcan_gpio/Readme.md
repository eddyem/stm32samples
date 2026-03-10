USB-CAN adapter
===============

Based on [USB-CAN](https://github.com/eddyem/stm32samples/tree/master/F0%3AF030%2CF042%2CF072/usbcan_ringbuffer).

Unlike original (only USB-CAN) adapter, in spite of it have GPIO contacts on board, this firmware allows to use both
USB-CAN and GPIO.

The old USB-CAN is available as earlier by /dev/USB-CANx, also you can see new device: /dev/USB-GPIOx. 
New interface allows you to configure GPIO and use it's base functions: in/out/ADC.


## DMA channels

DMA1 channel1: ADC.