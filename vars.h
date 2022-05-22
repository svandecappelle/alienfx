#ifndef VARS_H
#define VARS_H
#include "vars.h"

#define debug 0

#define	ALIENWARE_VENDORID		0x187c
#define	ALIENWARE_PRODUCTID_M14XR2	0x0521

#define	OK			0
#define	LIBUSB_INIT_ERR		-1
#define	LIBUSB_OPEN_ERR		-2
#define	LIBUSB_SIZE_ERR		-3
#define	LIBUSB_WRITE_ERR	-4
#define	LIBUSB_READ_ERR		-5

#define SEND_REQUEST_TYPE 0x21
#define SEND_REQUEST      0x09
#define SEND_VALUE        0x202
#define SEND_INDEX        0x00
#define SEND_DATA_SIZE    9

#define	READ_REQUEST_TYPE	0xa1
#define	READ_REQUEST		0x01
#define	READ_VALUE		0x101
#define	READ_INDEX		0x0
#define	READ_DATA_SIZE		8

#define	MIN_SPEED		100
#define	MAX_SPEED		1000
#define	STEP_SPEED		100

#define	ZONE_KEYBOARD_LEFT		0x01
#define	ZONE_KEYBOARD_MIDDLE_LEFT		0x04
#define	ZONE_KEYBOARD_RIGHT		0x02
#define	ZONE_KEYBOARD_MIDDLE_RIGHT		0x08

#define ZONE_TOUCHPAD 0x0200
#define ZONE_SPEAKER_RIGHT 0x0040
#define ZONE_SPEAKER_LEFT 0x0020
#define ZONE_ALIEN_HEAD 0x0080
#define ZONE_MEDIABAR 0x1c00
#define ZONE_POWER_BUTTON 0x2000
#define ZONE_ALIEN_NAME 0x0100

typedef struct RegionColor {
    int region;
    int color[3];
} RegionColor;

typedef struct ChainValue {
    struct RegionColor *value;
    struct ChainValue *next;
} ChainValue;

typedef struct Chain {
    struct ChainValue *value;
    struct Chain *next;
} Chain;

#endif
