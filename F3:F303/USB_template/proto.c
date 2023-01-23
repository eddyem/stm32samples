#include <stm32f3.h>
#include <string.h>

#include "strfunc.h"
#include "usb.h"
#include "version.inc"

//#define LOCBUFFSZ       (32)
// local buffer for I2C data to send
//static uint8_t locBuffer[LOCBUFFSZ];
extern volatile uint32_t Tms;

const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/USB_template build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "T - print current Tms\n"
;
/*
// read N numbers from buf, @return 0 if wrong or none
TRUE_INLINE uint16_t readNnumbers(const char *buf){
    uint32_t D;
    const char *nxt;
    uint16_t N = 0;
    while((nxt = getnum(buf, &D)) && nxt != buf && N < LOCBUFFSZ){
        buf = nxt;
        locBuffer[N++] = (uint8_t) D&0xff;
        USND("add byte: "); USND(uhex2str(D&0xff)); USND("\n");
    }
    USND("Send "); USND(u2str(N)); USND(" bytes\n");
    return N;
}
*/


const char *parse_cmd(const char *buf){
    // "long" commands
  /*  switch(*buf){
        case '':
        break;
    }*/
    // "short" commands
    if(buf[1]) return buf; // echo wrong data
    switch(*buf){
        case 'T':
            USB_sendstr("T=");
            USB_sendstr(u2str(Tms));
            newline();
        break;
        default: // help
            USB_sendstr(helpstring);
        break;
    }
    return NULL;
}
