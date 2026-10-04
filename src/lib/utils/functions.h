#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <unistd.h>
#include <libusb-1.0/libusb.h>

#include "vars.h"

struct RegionColor * add_region_color(struct Chain *colorChain, int region, char *color);

void set_zone_color(libusb_device_handle*	usbhandle, int zone, int r, int g, int b);
libusb_device_handle * connect_usb();
// Same as connect_usb() but returns NULL instead of exiting when the device is unavailable
libusb_device_handle * try_connect_usb();

#endif
