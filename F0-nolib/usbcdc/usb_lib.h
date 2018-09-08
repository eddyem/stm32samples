#pragma once
#ifndef __USB_LIB_H__
#define __USB_LIB_H__

#include "usb_defs.h"

//Максимальное количесво конечных точек
#define MAX_ENDPOINTS                   2
// bRequest, standard; for bmRequestType == 0x80
#define GET_STATUS                      0x00
#define GET_DESCRIPTOR                  0x06
#define GET_CONFIGURATION               0x08    //не реализован
// for bmRequestType == 0
#define CLEAR_FEATURE                   0x01
#define SET_FEATURE                     0x03    //не реализован
#define SET_ADDRESS                     0x05
#define SET_DESCRIPTOR                  0x07    //не реализован
#define SET_CONFIGURATION               0x09
// for bmRequestType == 0x81, 1 or 0xB2
#define GET_INTERFACE                   0x0A    //не реализован
#define SET_INTERFACE                   0x0B    //не реализован
#define SYNC_FRAME                      0x0C    //не реализован

// Class-Specific Control Requests
#define SEND_ENCAPSULATED_COMMAND   0x00
#define GET_ENCAPSULATED_RESPONSE   0x01
#define SET_COMM_FEATURE            0x02
#define GET_COMM_FEATURE            0x03
#define CLEAR_COMM_FEATURE          0x04
#define SET_LINE_CODING             0x20
#define GET_LINE_CODING             0x21
#define SET_CONTROL_LINE_STATE      0x22
#define SEND_BREAK                  0x23

/* Line Coding Structure from CDC spec 6.2.13
struct usb_cdc_line_coding {
    __le32  dwDTERate;
    __u8    bCharFormat;
#define USB_CDC_1_STOP_BITS         0
#define USB_CDC_1_5_STOP_BITS           1
#define USB_CDC_2_STOP_BITS         2

    __u8    bParityType;
#define USB_CDC_NO_PARITY           0
#define USB_CDC_ODD_PARITY          1
#define USB_CDC_EVEN_PARITY         2
#define USB_CDC_MARK_PARITY         3
#define USB_CDC_SPACE_PARITY            4

    __u8    bDataBits;
} __attribute__ ((packed));
*/

// wValue
#define DEVICE_DESCRIPTOR               0x100
#define CONFIGURATION_DESCRIPTOR        0x200
#define STRING_LANG_DESCRIPTOR          0x300
#define STRING_MAN_DESCRIPTOR           0x301
#define STRING_PROD_DESCRIPTOR          0x302
#define STRING_SN_DESCRIPTOR            0x303
#define DEVICE_QALIFIER_DESCRIPTOR      0x600
//Макросы установки/очистки битов в EPnR регистрах
#define CLEAR_DTOG_RX(R)                (R & USB_EPnR_DTOG_RX) ? R : (R & (~USB_EPnR_DTOG_RX))
#define SET_DTOG_RX(R)                  (R & USB_EPnR_DTOG_RX) ? (R & (~USB_EPnR_DTOG_RX)) : R
#define TOGGLE_DTOG_RX(R)               (R | USB_EPnR_DTOG_RX)
#define KEEP_DTOG_RX(R)                 (R & (~USB_EPnR_DTOG_RX))
#define CLEAR_DTOG_TX(R)                (R & USB_EPnR_DTOG_TX) ? R : (R & (~USB_EPnR_DTOG_TX))
#define SET_DTOG_TX(R)                  (R & USB_EPnR_DTOG_TX) ? (R & (~USB_EPnR_DTOG_TX)) : R
#define TOGGLE_DTOG_TX(R)               (R | USB_EPnR_DTOG_TX)
#define KEEP_DTOG_TX(R)                 (R & (~USB_EPnR_DTOG_TX))
#define SET_VALID_RX(R)                 ((R & USB_EPnR_STAT_RX) ^ USB_EPnR_STAT_RX)   | (R & (~USB_EPnR_STAT_RX))
#define SET_NAK_RX(R)                   ((R & USB_EPnR_STAT_RX) ^ USB_EPnR_STAT_RX_1) | (R & (~USB_EPnR_STAT_RX))
#define SET_STALL_RX(R)                 ((R & USB_EPnR_STAT_RX) ^ USB_EPnR_STAT_RX_0) | (R & (~USB_EPnR_STAT_RX))
#define KEEP_STAT_RX(R)                 (R & (~USB_EPnR_STAT_RX))
#define SET_VALID_TX(R)                 ((R & USB_EPnR_STAT_TX) ^ USB_EPnR_STAT_TX)   | (R & (~USB_EPnR_STAT_TX))
#define SET_NAK_TX(R)                   ((R & USB_EPnR_STAT_TX) ^ USB_EPnR_STAT_TX_1) | (R & (~USB_EPnR_STAT_TX))
#define SET_STALL_TX(R)                 ((R & USB_EPnR_STAT_TX) ^ USB_EPnR_STAT_TX_0) | (R & (~USB_EPnR_STAT_TX))
#define KEEP_STAT_TX(R)                 (R & (~USB_EPnR_STAT_TX))
#define CLEAR_CTR_RX(R)                 (R & (~USB_EPnR_CTR_RX))
#define CLEAR_CTR_TX(R)                 (R & (~USB_EPnR_CTR_TX))
#define CLEAR_CTR_RX_TX(R)              (R & (~(USB_EPnR_CTR_TX | USB_EPnR_CTR_RX)))
//Состояния соединения USB
#define USB_DEFAULT_STATE               0
#define USB_ADRESSED_STATE              1
#define USB_CONFIGURE_STATE             2
//Типы конечных точек
#define EP_TYPE_BULK                    0x00
#define EP_TYPE_CONTROL                 0x01
#define EP_TYPE_ISO                     0x02
#define EP_TYPE_INTERRUPT               0x03

//Тип для расшифровки конфигурационного пакета
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} config_pack_t;
//Структура состояний конечных точек
typedef struct __ep_t{
    uint16_t *tx_buf;
    uint8_t *rx_buf;
    uint16_t (*func)();
    uint16_t status;
    unsigned rx_cnt : 10;
    unsigned tx_flag : 1;
    unsigned rx_flag : 1;
    unsigned setup_flag : 1;
} ep_t;
//Статус и адрес соединения USB
typedef struct {
    uint8_t USB_Status;
    uint16_t USB_Addr;
} usb_dev_t;

//Инициализация USB
void USB_Init();
//Получить статус соединения USB
uint8_t USB_GetState();
//Инициализация конечной точки
void EP_Init(uint8_t number, uint8_t type, uint16_t addr_tx, uint16_t addr_rx, uint16_t (*func)(ep_t ep));
//Запись массива в буфер конечной точки из прерывания
void EP_WriteIRQ(uint8_t number, const uint8_t *buf, uint16_t size);
//Запись массива в буфер конечной точки извне прерывания
void EP_Write(uint8_t number, const uint8_t *buf, uint16_t size);
//Чтение массива из буфера конечной точки
void EP_Read(uint8_t number, uint8_t *buf);

#endif // __USB_LIB_H__
