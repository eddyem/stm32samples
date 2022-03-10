Code for STM32F030F4 chinese board

Echo received by USART data as morze code (@ PA6, TIM3CH1)

USART Rx by interrupts, Tx by DMA, Morze by timer3 with DMA @ each letter
- Toggle onboard green LED once per second.
- Echo received by USART1 data (not more than 64 bytes including '\n') on B115200
- Morze text by dimmer
