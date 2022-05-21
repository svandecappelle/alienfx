#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <string.h>
#include <unistd.h>

#include "vars.h"
#include "functions.h"

void convertToRgb(RegionColor *r, char *arg) {
  int i = 0;
  const char delim[2] = ", ";
  char *color = malloc(sizeof(arg));
  color = strcpy(color, arg);
  color = strtok(color, delim);
  if (color != NULL) {
    r->color[i] = atoi(color);
    i += 1;
  }
  while (i < 3) {
    color = strtok(NULL, delim);
    r->color[i] = atoi(color);
    i += 1;
  }
}

void addValue(struct Chain *c, struct RegionColor *val) {
  ChainValue *tmp = c->value;
  
  c->value = (struct ChainValue *) malloc(sizeof(struct ChainValue));
  c->value->value = val;
  if (tmp != NULL) {
    c->value->next = tmp;
  } else {
    c->value->next = NULL;
  }
}

struct RegionColor * addRegionColor(struct Chain *colorChain, int region, char *color) {
  static RegionColor c;
  c = (RegionColor) {
    .region = region,
    .color = {0, 0, 0}
  };
  convertToRgb(&c, color);
  addValue(colorChain, &c);
  return &c;
}

int main(int argc, char *argv[]) {
  printf("Starting AlienFX script\n");
  libusb_context* usbcontext;
  libusb_device_handle*	usbhandle;
  InitDevice(&usbcontext, &usbhandle, ALIENWARE_VENDORID, ALIENWARE_PRODUCTID_M14XR2);
  usbdetach(usbhandle);
  
  int opt;
  char *value;
  struct RegionColor *v;
  Chain *colorChain = (Chain *) malloc(sizeof(Chain));
  while ((opt = getopt(argc, argv, ":k:")) != -1) {
    value = (char *) malloc(sizeof(optarg));
    value = strcpy(value, optarg);    

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
	//addRegionColor(colorChain, ZONE_KEYBOARD_MIDDLE_LEFT, value); 
        //addRegionColor(colorChain, ZONE_KEYBOARD_MIDDLE_RIGHT, value);
        //addRegionColor(colorChain, ZONE_KEYBOARD_RIGHT, value);
        //addRegionColor(colorChain, ZONE_ALIEN_NAME, value); 
	break;
      case 'l':
        // value = (char *) malloc(sizeof(optarg));
        // value = strcpy(value, optarg);
	// printf("hahahhahahahaha\n");
        
	// v = addRegionColor(colorChain, ZONE_ALIEN_NAME, value);
    	// setZoneColor(usbhandle, v->region, v->color[0], v->color[1], v->color[2]);
        break;
      case 'n':
        printf("n");
        break;
      case 'a':
        printf("a");
        break;
      case ':':
        printf("option need a value\n");
        break;
      default:
        printf("unkown option: %d\n", opt);
        break;
    }
    free(value);
  }
  
  for(; optind < argc; optind++){     
    printf("extra arguments: %s\n", argv[optind]); 
  }
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
