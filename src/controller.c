#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <string.h>
#include <unistd.h>

#include "lib/utils/vars.h"
#include "lib/connector.h"

extern int opterr;
extern char *optarg;

int main(int argc, char *argv[]) {
    printf("Starting AlienFX script\n");
    libusb_device_handle* usbhandle = connect_usb();
    int opt = -1;
    struct RegionColor *v;
    char *value;
    Chain *colorChain = (Chain *) malloc(sizeof(Chain));

    while ((opt = getopt(argc, argv, "k:l:s:m:t:")) > 0) {
        value=malloc(strlen(optarg));
        strcpy(value, optarg);

        switch (opt) {
            case 'k':
                v = addRegionColor(colorChain, ZONE_KEYBOARD_LEFT, value);
                setZoneColor(usbhandle, v->region, v->color[0], v->color[1], v->color[2]);

                v = addRegionColor(colorChain, ZONE_KEYBOARD_MIDDLE_LEFT, value);
                setZoneColor(usbhandle, v->region, v->color[0], v->color[1], v->color[2]);

                v = addRegionColor(colorChain, ZONE_KEYBOARD_MIDDLE_RIGHT, value);
                setZoneColor(usbhandle, v->region, v->color[0], v->color[1], v->color[2]);

                v = addRegionColor(colorChain, ZONE_KEYBOARD_RIGHT, value);
                setZoneColor(usbhandle, v->region, v->color[0], v->color[1], v->color[2]);
                break;
            case 'l':
                v = addRegionColor(colorChain, ZONE_ALIEN_NAME, value);
                setZoneColor(usbhandle, v->region, v->color[0], v->color[1], v->color[2]);
                break;
            case 's':
                v = addRegionColor(colorChain, ZONE_SPEAKER_LEFT, value);
                setZoneColor(usbhandle, v->region, v->color[0], v->color[1], v->color[2]);

                v = addRegionColor(colorChain, ZONE_SPEAKER_RIGHT, value);
                setZoneColor(usbhandle, v->region, v->color[0], v->color[1], v->color[2]);
                break;
            case 't':
                v = addRegionColor(colorChain, ZONE_TOUCHPAD, value);
                setZoneColor(usbhandle, v->region, v->color[0], v->color[1], v->color[2]);
                break;
            case 'm':
                v = addRegionColor(colorChain, ZONE_MEDIABAR, value);
                setZoneColor(usbhandle, v->region, v->color[0], v->color[1], v->color[2]);
                break;
            default: /* '?' */
                exit(EXIT_FAILURE);
        }
        free(value);
    }

    if (optind < argc) {
        printf("name=%s\n", argv[optind]);
    }

    printf("All colors wanted affected\n");

    /*setZoneColor(usbhandle, ZONE_KEYBOARD_LEFT, r, g, b);
      setZoneColor(usbhandle, ZONE_KEYBOARD_MIDDLE_LEFT, r, g, b);
      setZoneColor(usbhandle, ZONE_KEYBOARD_MIDDLE_RIGHT, r, g, b);
      setZoneColor(usbhandle, ZONE_KEYBOARD_RIGHT, r, g, b);
      setZoneColor(usbhandle, ZONE_TOUCHPAD, r, g, b);
      setZoneColor(usbhandle, ZONE_SPEAKER_LEFT, r, g, b);
      setZoneColor(usbhandle, ZONE_SPEAKER_RIGHT, r, g, b);
      setZoneColor(usbhandle, ZONE_POWER_BUTTON, r, g, b);
      setZoneColor(usbhandle, ZONE_MEDIABAR, r, g, b);
      setZoneColor(usbhandle, ZONE_ALIEN_NAME, r, g, b);
      setZoneColor(usbhandle, ZONE_ALIEN_HEAD, r, g, b);
      */
    return 0;
}
