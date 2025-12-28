#include <stm32f3.h>
#include <string.h>

#include "flash.h"
#include "strfunc.h"
#include "usb_dev.h"
#include "version.inc"

static const char *const sOKn = "OK\n", *const sERRn = "ERR\n";

//#define LOCBUFFSZ       (32)
// local buffer for I2C data to send
//static uint8_t locBuffer[LOCBUFFSZ];
extern volatile uint32_t Tms;

const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/InterfaceBoard build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "d - dump flash\n"
        "ix - rename interface number x (0..6)\n"
        "Ex - erase full storage (witout x) or only page x\n"
        "F - reinit configurations from flash\n"
        "R - soft reset\n"
        "S - store new parameters into flash\n"
        "T - print current Tms\n"
;

static void dumpflash(){
    USB_sendstr("userconf_sz="); USB_sendstr(u2str(the_conf.userconf_sz));
    USB_sendstr("\ncurrentconfidx="); USB_sendstr(i2str(currentconfidx));
    USB_putbyte('\n');
    for(int i = 0; i < InterfacesAmount; ++i){
        USB_sendstr("interface"); USB_putbyte('0' + i);
        USB_putbyte('=');
        int l = the_conf.iIlengths[i] / 2;
        char *ptr = (char*) the_conf.iInterface[i];
        for(int j = 0; j < l; ++j){
            USB_putbyte(*ptr);
            ptr += 2;
        }
        USB_putbyte('\n');
    }
}

static void setiface(const char *str){
    if(!str || !*str) goto err;
    uint32_t N;
    const char *nxt = getnum(str, &N);
    if(!nxt || nxt == str || N >= InterfacesAmount) goto err;
    nxt = strchr(nxt, '=');
    if(!nxt || !*(++nxt)) goto err;
    nxt = omit_spaces(nxt);
    if(!nxt || !*nxt) goto err;
    int l = strlen(nxt);
    if(l > MAX_IINTERFACE_SZ) goto err;
    the_conf.iIlengths[N] = (uint8_t) l * 2;
    char *ptr = (char*)the_conf.iInterface[N];
    for(int i = 0; i < l; ++i){
        *ptr++ = *nxt++;
        *ptr++ = 0;
    }
    USB_sendstr(sOKn);
    return;
err:
    USB_sendstr(sERRn);
}

static const char* erpg(const char *str){
    uint32_t N;
    if(str == getnum(str, &N)) return sERRn;
    if(erase_storage(N)) return sERRn;
    return sOKn;
}

const char *parse_cmd(const char *buf){
    if(!buf || !*buf) return NULL;
    if(strlen(buf) > 1){
        // "long" commands
        char c = *buf++;
        switch(c){
            case 'E':
                return erpg(buf);
            case 'i':
                setiface(buf);
                return NULL;
            break;
            default:
                return buf-1; // echo wrong data
        }
    }
    // "short" commands
    switch(*buf){
        case 'd':
            dumpflash();
        break;
        case 'E':
            if(erase_storage(-1)) return sERRn;
            return sOKn;
        break;
        case 'F':
            flashstorage_init();
            return sOKn;
        case 'R':
            NVIC_SystemReset();
            return NULL;
        case 'S':
            if(store_userconf()) return sERRn;
            return sOKn;
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
