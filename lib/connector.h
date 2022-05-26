#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <libusb-1.0/libusb.h>

#include "utils/functions.h"

libusb_device_handle * connect_usb() {
    libusb_context* usbcontext;
    libusb_device_handle*	usbhandle;
    int ret = InitDevice(&usbcontext, &usbhandle, ALIENWARE_VENDORID, ALIENWARE_PRODUCTID_M14XR2);
    if (ret!=0) {
        printf("Unable to access to Alienware keyboard please try with 'sudo'\n");
        exit(1);
    }
    usbdetach(usbhandle);
    return usbhandle;
}

void set_zone_color(libusb_device_handle*	usbhandle, int zone, int r, int g, int b) {
    unsigned char data[9]={0x02,0x03,0x05,0x00,0x00,0x01,0x0f,0xf0,0x00};
    data[4]=(zone>>8)&0xff;
    data[5]=(zone>>0)&0xff;
    data[6]=(r<<4); //&0xf0;
    data[6]|=g; //&0xf);
    data[7]=(b<<4); //&0xf0;
    usbwrite(usbhandle,data,9);
    end(usbhandle);
    pge(usbhandle);
    reset(usbhandle);
    // TODO check why this method no seems to work as expected
    // wait(usbhandle);
}

#endif
