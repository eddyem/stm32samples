#include "hardware.h"
#include "proto.h"
#include "usb.h"

#define MAXSTRLEN    RBINSZ

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[MAXSTRLEN+1];
    if(StartHSE()){
        SysTick_Config((uint32_t)72000); // 1ms
    }else{
        StartHSI();
        SysTick_Config((uint32_t)48000); // 1ms
    }
    hw_setup();
    USB_setup();
    uint32_t ctr = Tms;
    while(1){
        if(Tms - ctr > 499){
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
        }
        USB_proc();
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l){
            const char *ans = parse_cmd(inbuff);
            if(ans) USB_sendstr(ans);
        }
    }
}
