/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cdcacm.h"
#include "user_proto.h"
#include "main.h"

// Buffer for USB Tx
static uint8_t USB_Tx_Buffer[USB_TX_DATA_SIZE];
static uint8_t USB_Tx_ptr = 0;
// connection flag
uint8_t USB_connected = 0;
static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5740,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

char usbdatabuf[USB_RX_DATA_SIZE]; // buffer for received data
int usbdatalen = 0;  // lenght of received data

/*
 * This notification endpoint isn't implemented. According to CDC spec its
 * optional, but its absence causes a NULL pointer dereference in Linux
 * cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x82,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	 },
};

static const struct usb_interface_descriptor comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 0,

	.endpoint = comm_endp,

	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors),
}};

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = data_endp,
}};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = data_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Organisation, author",
	"device",
	"version",
};

// default line coding: B115200, 1stop, 8bits, parity none
struct usb_cdc_line_coding linecoding = {
	.dwDTERate   = 115200,
	.bCharFormat = USB_CDC_1_STOP_BITS,
	.bParityType = USB_CDC_NO_PARITY,
	.bDataBits   = 8,
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

/**
 * This function runs every time it gets a request for control parameters get/set
 * parameter SET_LINE_CODING used to change USART1 parameters: if you want to
 * change them, just connect through USB with required parameters
 */
static int cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)){
	(void)complete;
	(void)buf;
	(void)usbd_dev;
	char local_buf[10];
	struct usb_cdc_line_coding  lc;

	switch (req->bRequest) {
	case SET_CONTROL_LINE_STATE:{
		if(req->wValue){ // terminal is opened
			USB_connected = 1;
		}else{ // terminal is closed
			USB_connected = 0;
		}
		/*
		 * This Linux cdc_acm driver requires this to be implemented
		 * even though it's optional in the CDC spec, and we don't
		 * advertise it in the ACM functional descriptor.
		 */
		struct usb_cdc_notification *notif = (void *)local_buf;
		/* We echo signals back to host as notification. */
		notif->bmRequestType = 0xA1;
		notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
		notif->wValue = 0;
		notif->wIndex = 0;
		notif->wLength = 2;
		local_buf[8] = req->wValue & 3;
		local_buf[9] = 0;
		usbd_ep_write_packet(usbd_dev, 0x83, local_buf, 10);
	}break;
	case SET_LINE_CODING:
		if (!len || (*len != sizeof(struct usb_cdc_line_coding)))
			return 0;
		memcpy((void *)&lc, (void *)*buf, *len);
		// Mark & Space parity don't support by hardware, check it
		if(lc.bParityType == USB_CDC_MARK_PARITY || lc.bParityType == USB_CDC_SPACE_PARITY){
			return 0; // error
		}else{
//			memcpy((void *)&linecoding, (void *)&lc, sizeof(struct usb_cdc_line_coding));
//			UART_setspeed(USART1, &linecoding);
		}
	break;
	case GET_LINE_CODING: // return linecoding buffer
		if(len && *len == sizeof(struct usb_cdc_line_coding))
			memcpy((void *)*buf, (void *)&linecoding, sizeof(struct usb_cdc_line_coding));
		//usbd_ep_write_packet(usbd_dev, 0x83, (char*)&linecoding, sizeof(linecoding));
	break;
	default:
		return 0;
	}
	return 1;
}

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep){
	(void)ep;
	int len = usbd_ep_read_packet(usbd_dev, 0x01, usbdatabuf + usbdatalen, USB_RX_DATA_SIZE - usbdatalen);
	usbdatalen += len;
	if(usbdatalen >= USB_RX_DATA_SIZE){ // buffer overflow - drop all its contents
		usbdatalen = 0;
	}
}

static void cdcacm_data_tx_cb(usbd_device *usbd_dev, uint8_t ep){
	(void)ep;
	(void)usbd_dev;

	usb_send_buffer();
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;
	(void)usbd_dev;

	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, USB_RX_DATA_SIZE, cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, USB_TX_DATA_SIZE, cdcacm_data_tx_cb);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				cdcacm_control_request);
}

static usbd_device *current_usb = NULL;

usbd_device *USB_init(){
	current_usb = usbd_init(&stm32f103_usb_driver, &dev, &config,
		usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	if(!current_usb) return NULL;
	usbd_register_set_config_callback(current_usb, cdcacm_set_config);
	return current_usb;
}

mutex_t send_block_mutex = MUTEX_UNLOCKED;
/**
 * Put byte into USB buffer to send
 * @param byte - a byte to put into a buffer
 */
void usb_send(uint8_t byte){
	mutex_lock(&send_block_mutex);
	USB_Tx_Buffer[USB_Tx_ptr++] = byte;
	mutex_unlock(&send_block_mutex);
	if(USB_Tx_ptr == USB_TX_DATA_SIZE){ // buffer can be overflowed - send it!
		usb_send_buffer();
	}
}

/**
 * Send all data in buffer over USB
 * this function runs when buffer is full or on SysTick
 */
void usb_send_buffer(){
	if(MUTEX_LOCKED == mutex_trylock(&send_block_mutex)) return;
	if(USB_Tx_ptr){
		if(current_usb && USB_connected){
			// usbd_ep_write_packet return 0 if previous packet isn't transmit yet
			while(USB_Tx_ptr != usbd_ep_write_packet(current_usb, 0x82, USB_Tx_Buffer, USB_Tx_ptr));
			usbd_poll(current_usb);
		}
		USB_Tx_ptr = 0;
	}
	mutex_unlock(&send_block_mutex);
}
