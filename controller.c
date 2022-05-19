#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#include "vars.h"
#include "functions.h"

int main(void) {
  fprintf(stdout, "Hello"); 
  try();
}

void try() {
	libusb_context*		usbcontext;
	libusb_device_handle*	usbhandle;
	int retval;
	int i,j,r,g,b;
	int ready;
	retval = InitDevice(&usbcontext, &usbhandle, ALIENWARE_VENDORID, ALIENWARE_PRODUCTID_M14XR2);
	usbdetach(usbhandle);
  r = 15;
  g = 0;
  b = 0;

  setZoneColor(usbhandle, ZONE_KEYBOARD_LEFT, r, g, b);
  setZoneColor(usbhandle, ZONE_KEYBOARD_MIDDLE_LEFT, r, g, b);
  setZoneColor(usbhandle, ZONE_KEYBOARD_MIDDLE_RIGHT, r, g, b);
  setZoneColor(usbhandle, ZONE_KEYBOARD_RIGHT, r, g, b);
  setZoneColor(usbhandle, ZONE_TOUCHPAD, r, g, b);
}
