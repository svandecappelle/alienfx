#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <unistd.h>
#include <libusb-1.0/libusb.h>

#include "vars.h"

// Raw 9 byte packet write / 8 byte status read, return OK (0) on success
int usbwrite(libusb_device_handle* usbhandle, unsigned char* data, unsigned short len);
int usbread(libusb_device_handle* usbhandle, unsigned char* data, unsigned int len);

void set_zone_color(libusb_device_handle*	usbhandle, int zone, int r, int g, int b);
libusb_device_handle * connect_usb();
// Same as connect_usb() but returns NULL instead of exiting when the device is unavailable
libusb_device_handle * try_connect_usb();

#endif
