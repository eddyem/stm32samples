/*
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>

#include "usb_lib.h"
#include "usb_descr.h"
#include "usb_dev.h"

static ep_t endpoints[STM32ENDPOINTS];

static uint16_t USB_Addr = 0;
static uint8_t ep0databuf[EP0DATABUF_SIZE], setupdatabuf[EP0DATABUF_SIZE];
static uint8_t ep0dbuflen = 0;
static config_pack_t *setup_packet = (config_pack_t*) setupdatabuf;
volatile uint8_t usbON = 0; // device is configured and active

static void wr0(const uint8_t *buf, uint16_t size){
    if(setup_packet->wLength < size) size = setup_packet->wLength; // shortened request
    if(size < endpoints[0].txbufsz){
        EP_WriteIRQ(0, buf, size);
        return;
    }
    while(size){
        uint16_t l = size;
        if(l > endpoints[0].txbufsz) l = endpoints[0].txbufsz;
        EP_WriteIRQ(0, buf, l);
        buf += l;
        size -= l;
        uint8_t needzlp = (l == endpoints[0].txbufsz) ? 1 : 0;
        if(size || needzlp){ // send last data buffer
            uint16_t epstatus = KEEP_DTOG(USB->EPnR[0]);
            // keep DTOGs, clear CTR_RX,TX, set TX VALID, leave stat_Rx
            USB->EPnR[0] = (epstatus & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX|USB_EPnR_STAT_RX))
                            ^ USB_EPnR_STAT_TX;
            uint32_t ctr = 1000000;
            while(--ctr && (USB->ISTR & USB_ISTR_CTR) == 0){IWDG->KR = IWDG_REFRESH;};
            if((USB->ISTR & USB_ISTR_CTR) == 0){
                return;
            }
            if(needzlp) EP_WriteIRQ(0, (uint8_t*)0, 0);
        }
    }
}

static inline void get_descriptor(){
    uint8_t descrtype = setup_packet->wValue >> 8,
            descridx = setup_packet->wValue & 0xff;
    switch(descrtype){
        case DEVICE_DESCRIPTOR:
            wr0(USB_DeviceDescriptor, sizeof(USB_DeviceDescriptor));
        break;
        case CONFIGURATION_DESCRIPTOR:
            wr0(USB_ConfigDescriptor, sizeof(USB_ConfigDescriptor));
        break;
        case STRING_DESCRIPTOR:
            if(descridx < iDESCR_AMOUNT) wr0((const uint8_t *)StringDescriptor[descridx], *((uint8_t*)StringDescriptor[descridx]));
            else EP_WriteIRQ(0, (uint8_t*)0, 0);
        break;
        case DEVICE_QUALIFIER_DESCRIPTOR:
            wr0(USB_DeviceQualifierDescriptor, USB_DeviceQualifierDescriptor[0]);
        break;
        default:
        break;
    }
}

static uint16_t configuration = 0; // reply for GET_CONFIGURATION (==1 if configured)
static inline void std_d2h_req(){
    uint16_t st = 0;
    switch(setup_packet->bRequest){
        case GET_DESCRIPTOR:
            get_descriptor();
        break;
        case GET_STATUS:
            EP_WriteIRQ(0, (uint8_t *)&st, 2); // send status: Bus Powered
        break;
        case GET_CONFIGURATION:
            EP_WriteIRQ(0, (uint8_t*)&configuration, 1);
        break;
        default:
        break;
    }
}

static inline void std_h2d_req(){
    switch(setup_packet->bRequest){
        case SET_ADDRESS:
            // new address will be assigned later - after  acknowlegement or request to host
            USB_Addr = setup_packet->wValue;
        break;
        case SET_CONFIGURATION:
            // Now device configured
            configuration = setup_packet->wValue;
            set_configuration(configuration);
            usbON = 1;
        break;
        default:
        break;
    }
}

void WEAK usb_standard_request(){
    uint8_t recipient = REQUEST_RECIPIENT(setup_packet->bmRequestType);
    uint8_t dev2host = (setup_packet->bmRequestType & 0x80) ? 1 : 0;
    switch(recipient){
        case REQ_RECIPIENT_DEVICE:
            if(dev2host){
                std_d2h_req();
                return;
            }else{
                std_h2d_req();
                EP_WriteIRQ(0, (uint8_t *)0, 0);
            }
        break;
        case REQ_RECIPIENT_INTERFACE:
            //EP_WriteIRQ(0, (const uint8_t*)"epi", 4);
        break;
        case REQ_RECIPIENT_ENDPOINT:
            if(setup_packet->bRequest == CLEAR_FEATURE){
                EP_WriteIRQ(0, (uint8_t *)0, 0);
            }else{
                //EP_WriteIRQ(0, (const uint8_t*)"epr", 4);
            }
        break;
        default:
            //EP_WriteIRQ(0, (const uint8_t*)"epd", 4);
        break;
    }
}

void WEAK usb_class_request(config_pack_t _U_ *req, uint8_t _U_ *data, unsigned int _U_ datalen){
    EP_WriteIRQ(0, (const uint8_t*)"cls", 4);
}

void WEAK usb_vendor_request(config_pack_t _U_ *packet, uint8_t _U_ *data, unsigned int _U_ datalen){
    EP_WriteIRQ(0, (const uint8_t*)"vnd", 4);
}

/*
bmRequestType: 76543210
7    direction: 0 - host->device, 1 - device->host
65   type: 0 - standard, 1 - class, 2 - vendor
4..0 getter: 0 - device, 1 - interface, 2 - endpoint, 3 - other
*/
/**
 * Endpoint0 (control) handler
 */
static void EP0_Handler(){
    uint16_t epstatus = USB->EPnR[0]; // EP0R on input -> return this value after modifications
    uint8_t reqtype = REQUEST_TYPE(setup_packet->bmRequestType);
    //char x[] = "EP0Hx";
    int rxflag = RX_FLAG(epstatus);
    if(rxflag && SETUP_FLAG(epstatus)){
        switch(reqtype){
            case REQ_TYPE_STANDARD:
                usb_standard_request();
            break;
            case REQ_TYPE_CLASS:
                usb_class_request(setup_packet, NULL, 0);
            break;
            case REQ_TYPE_VENDOR:
                usb_vendor_request(setup_packet, NULL, 0);
            break;
            default:
                //x[4] = reqtype;
                //EP_WriteIRQ(0, (const uint8_t*)x, 5);
            break;
        }
    }else if(rxflag){ // got data over EP0 or host acknowlegement
        if(ep0dbuflen){
            switch(reqtype){
                case REQ_TYPE_CLASS:
                    usb_class_request(setup_packet, ep0databuf, ep0dbuflen);
                break;
                    case REQ_TYPE_VENDOR:
                    usb_vendor_request(setup_packet, ep0databuf, ep0dbuflen);
                break;
            }
            ep0dbuflen = 0;
        }
    }else if(TX_FLAG(epstatus)){ // package transmitted
        // now we can change address after enumeration
        if ((USB->DADDR & USB_DADDR_ADD) != USB_Addr){
            USB->DADDR = USB_DADDR_EF | USB_Addr;
            usbON = 0;
        }
    }
    epstatus = KEEP_DTOG(USB->EPnR[0]);
    if(rxflag) epstatus ^= USB_EPnR_STAT_TX; // start ZLP/data transmission
    else epstatus &= ~USB_EPnR_STAT_TX; // or leave unchanged
    // keep DTOGs, clear CTR_RX,TX, set RX VALID
    USB->EPnR[0] = (epstatus & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX)) ^ USB_EPnR_STAT_RX;
}

/**
 * Write data to EP buffer (called from IRQ handler)
 * @param number - EP number
 * @param *buf - array with data
 * @param size - its size
 */
void EP_WriteIRQ(uint8_t number, const uint8_t *buf, uint16_t size){
    if(size > endpoints[number].txbufsz) size = endpoints[number].txbufsz;
    uint16_t N2 = (size + 1) >> 1;
    // the buffer is 16-bit, so we should copy data as it would be uint16_t
    uint16_t *buf16 = (uint16_t *)buf;
#if defined USB1_16
    // very bad: what if `size` is odd?
    uint32_t *out = (uint32_t *)endpoints[number].tx_buf;
    for(int i = 0; i < N2; ++i, ++out){
        *out = buf16[i];
    }
#elif defined USB2_16
    // use memcpy instead?
    for(int i = 0; i < N2; i++){
        endpoints[number].tx_buf[i] = buf16[i];
    }
#else
#error "Define USB1_16 or USB2_16"
#endif
    USB_BTABLE->EP[number].USB_COUNT_TX = size;
}

/**
 * Write data to EP buffer (called outside IRQ handler)
 * @param number - EP number
 * @param *buf - array with data
 * @param size - its size
 */
void EP_Write(uint8_t number, const uint8_t *buf, uint16_t size){
    EP_WriteIRQ(number, buf, size);
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[number]);
    // keep DTOGs, clear CTR_TX & set TX VALID to start transmission
    USB->EPnR[number] = (epstatus & ~(USB_EPnR_CTR_TX)) ^ USB_EPnR_STAT_TX;
}

/*
 * Copy data from EP buffer into user buffer area
 * @param *buf - user array for data
 * @return amount of data read
 */
int EP_Read(uint8_t number, uint8_t *buf){
    int sz = endpoints[number].rx_cnt;
    if(!sz) return 0;
    endpoints[number].rx_cnt = 0;
#if defined USB1_16
    int n = (sz + 1) >> 1;
    uint32_t *in = (uint32_t*)endpoints[number].rx_buf;
    uint16_t *out = (uint16_t*)buf;
    for(int i = 0; i < n; ++i, ++in)
        out[i] = *(uint16_t*)in;
#elif defined USB2_16
    // use memcpy instead?
    for(int i = 0; i < sz; ++i)
        buf[i] = endpoints[number].rx_buf[i];
#else
#error "Define USB1_16 or USB2_16"
#endif
    return sz;
}


static uint16_t lastaddr = LASTADDR_DEFAULT;
/**
 * Endpoint initialisation
 * @param number - EP num (0...7)
 * @param type - EP type (EP_TYPE_BULK, EP_TYPE_CONTROL, EP_TYPE_ISO, EP_TYPE_INTERRUPT)
 * @param txsz - transmission buffer size @ USB/CAN buffer
 * @param rxsz - reception buffer size @ USB/CAN buffer
 * @param uint16_t (*func)(ep_t *ep) - EP handler function
 * @return 0 if all OK
 */
int EP_Init(uint8_t number, uint8_t type, uint16_t txsz, uint16_t rxsz, void (*func)(ep_t ep)){
    if(number >= STM32ENDPOINTS) return 4; // out of configured amount
    if(txsz > USB_BTABLE_SIZE || rxsz > USB_BTABLE_SIZE) return 1; // buffer too large
    if(lastaddr + txsz + rxsz >= USB_BTABLE_SIZE/ACCESSZ) return 2; // out of btable
    USB->EPnR[number] = (type << 9) | (number & USB_EPnR_EA);
    USB->EPnR[number] ^= USB_EPnR_STAT_RX | USB_EPnR_STAT_TX_1;
    if(rxsz & 1 || rxsz > USB_BTABLE_SIZE) return 3; // wrong rx buffer size
    uint16_t countrx = 0;
    if(rxsz < 64) countrx = rxsz / 2;
    else{
        if(rxsz & 0x1f) return 3; // should be multiple of 32
        countrx = 31 + rxsz / 32;
    }
    USB_BTABLE->EP[number].USB_ADDR_TX = lastaddr;
    endpoints[number].tx_buf = (uint16_t *)(USB_BTABLE_BASE + lastaddr * ACCESSZ);
    endpoints[number].txbufsz = txsz;
    lastaddr += txsz;
    USB_BTABLE->EP[number].USB_COUNT_TX = 0;
    USB_BTABLE->EP[number].USB_ADDR_RX = lastaddr;
    endpoints[number].rx_buf = (uint8_t *)(USB_BTABLE_BASE + lastaddr * ACCESSZ);
    lastaddr += rxsz;
    USB_BTABLE->EP[number].USB_COUNT_RX = countrx << 10;
    endpoints[number].func = func;
    return 0;
}

// standard IRQ handler
void USB_IRQ(){
    if(USB->ISTR & USB_ISTR_RESET){
        usbON = 0;
        // Reinit registers
        USB->CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM | USB_CNTR_SUSPM | USB_CNTR_WKUPM;
        // Endpoint 0 - CONTROL
        // ON USB LS size of EP0 may be 8 bytes, but on FS it should be 64 bytes!
        lastaddr = LASTADDR_DEFAULT;
        // clear address, leave only enable bit
        USB->DADDR = USB_DADDR_EF;
        if(EP_Init(0, EP_TYPE_CONTROL, USB_EP0BUFSZ, USB_EP0BUFSZ, EP0_Handler)){
            return;
        }
        USB->ISTR = ~USB_ISTR_RESET;
    }
    if(USB->ISTR & USB_ISTR_CTR){
        // EP number
        uint8_t n = USB->ISTR & USB_ISTR_EPID;
        // copy status register
        uint16_t epstatus = USB->EPnR[n];
        // copy received bytes amount
        endpoints[n].rx_cnt = USB_BTABLE->EP[n].USB_COUNT_RX & 0x3FF; // low 10 bits is counter
        // check direction
        if(USB->ISTR & USB_ISTR_DIR){ // OUT interrupt - receive data, CTR_RX==1 (if CTR_TX == 1 - two pending transactions: receive following by transmit)
            if(n == 0){ // control endpoint
                if(epstatus & USB_EPnR_SETUP){ // setup packet -> copy data to conf_pack
                    EP_Read(0, setupdatabuf);
                    // interrupt handler will be called later
                }else if(epstatus & USB_EPnR_CTR_RX){ // data packet -> push received data to ep0databuf
                    ep0dbuflen = EP_Read(0, ep0databuf);
                }
            }
        }
        // call EP handler
        if(endpoints[n].func) endpoints[n].func(endpoints[n]);
    }
    if(USB->ISTR & USB_ISTR_SUSP){ // suspend -> still no connection, may sleep
        usbON = 0;
#ifndef STM32F0
        USB->CNTR |= USB_CNTR_FSUSP | USB_CNTR_LP_MODE;
#else
        USB->CNTR |= USB_CNTR_FSUSP | USB_CNTR_LPMODE;
#endif
        USB->ISTR = ~USB_ISTR_SUSP;
    }
    if(USB->ISTR & USB_ISTR_WKUP){ // wakeup
#ifndef STM32F0
        USB->CNTR &= ~(USB_CNTR_FSUSP | USB_CNTR_LP_MODE); // clear suspend flags
#else
        USB->CNTR &= ~(USB_CNTR_FSUSP | USB_CNTR_LPMODE);
#endif
        USB->ISTR = ~USB_ISTR_WKUP;
    }
}


// here we suppose that all PIN settings done in hw_setup earlier
void USB_setup(){
#if defined STM32F3
    NVIC_DisableIRQ(USB_LP_IRQn);
    // remap USB LP & Wakeup interrupts to 75 and 76 - works only on pure F303
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // enable tacting of SYSCFG
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_USB_IT_RMP;
#elif defined STM32F1
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);
#elif defined STM32F0
    NVIC_DisableIRQ(USB_IRQn);
    RCC->APB1ENR |= RCC_APB1ENR_CRSEN;
    RCC->CFGR3 &= ~RCC_CFGR3_USBSW; // reset USB
    RCC->CR2 |= RCC_CR2_HSI48ON; // turn ON HSI48
    uint32_t tmout = 16000000;
    while(!(RCC->CR2 & RCC_CR2_HSI48RDY)){if(--tmout == 0) break;}
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
    CRS->CFGR &= ~CRS_CFGR_SYNCSRC;
    CRS->CFGR |= CRS_CFGR_SYNCSRC_1; // USB SOF selected as sync source
    CRS->CR |= CRS_CR_AUTOTRIMEN; // enable auto trim
    CRS->CR |= CRS_CR_CEN; // enable freq counter & block CRS->CFGR as read-only
    RCC->CFGR |= RCC_CFGR_SW;
#endif
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    //??
    USB->CNTR   = USB_CNTR_FRES; // Force USB Reset
    for(uint32_t ctr = 0; ctr < 72000; ++ctr) nop(); // wait >1ms
    USB->CNTR   = 0;
    USB->BTABLE = 0;
    USB->DADDR  = 0;
    USB->ISTR   = 0;
    USB->CNTR   = USB_CNTR_RESETM | USB_CNTR_WKUPM; // allow only wakeup & reset interrupts
#if defined STM32F3
    NVIC_EnableIRQ(USB_LP_IRQn);
#elif defined STM32F1
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
#elif defined STM32F0
    USB->BCDR |= USB_BCDR_DPPU;
    NVIC_EnableIRQ(USB_IRQn);
#endif
}


#if defined STM32F3
void usb_lp_isr() __attribute__ ((alias ("USB_IRQ")));
#elif defined STM32F1
void usb_lp_can_rx0_isr() __attribute__ ((alias ("USB_IRQ")));
#elif defined STM32F0
void usb_isr() __attribute__ ((alias ("USB_IRQ")));
#endif
