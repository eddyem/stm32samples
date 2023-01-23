#pragma once

#include "usbhw.h"

// sizes of ringbuffers for outgoing and incoming data
#define RBOUTSZ     (512)
#define RBINSZ      (512)

#define newline() USB_putbyte('\n')

void USB_proc();
int USB_sendall();
int USB_send(const uint8_t *buf, int len);
int USB_putbyte(uint8_t byte);
int USB_sendstr(const char *string);
int USB_receive(uint8_t *buf, int len);
int USB_receivestr(char *buf, int len);
