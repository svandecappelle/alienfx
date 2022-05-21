#include "vars.h"
#include <unistd.h>

int InitDevice(libusb_context** usbcontext,libusb_device_handle** usbhandle,unsigned short idVendor,unsigned short idProduct)
{
	if (0==libusb_init(usbcontext)) {
    // libusb_set_debug(*usbcontext, 3);	
		if ((*usbhandle=libusb_open_device_with_vid_pid(*usbcontext,idVendor,idProduct))) {
			fprintf(stderr,"device opened\n");
		} else {
			return LIBUSB_OPEN_ERR;
		}
	} else {
		return	LIBUSB_INIT_ERR;
	}
	return OK;
}

void usbdetach(libusb_device_handle* usbhandle) {
	if (libusb_kernel_driver_active(usbhandle,0))
		libusb_detach_kernel_driver(usbhandle,0);

}

void usbattach(libusb_device_handle* usbhandle) {
	libusb_attach_kernel_driver(usbhandle,0);
}

int usbsetdelay(libusb_device_handle* usbhandle,unsigned int delay) {
	unsigned char data[9];
	unsigned int retval;
	int i;
	delay=(delay/STEP_SPEED)*STEP_SPEED;	// quantize to step multiple

	if (delay<MIN_SPEED) delay=MIN_SPEED;
	if (delay>MAX_SPEED) delay=MAX_SPEED;

	data[0]=0x02;
	data[1]=0x0e;	//COMMAND_SET_SPEED;
	data[2]=(delay>>8)&0xff;
	data[3]=(delay>>0)&0xff;
	data[4]=0;
	data[5]=0;
	data[6]=0;
	data[7]=0;
	data[8]=0;
	
	fprintf(stderr,"write> ");
	for (i=0;i<SEND_DATA_SIZE;i++) 
	{
		fprintf(stderr,"%02x ",0xff&((unsigned int)data[i]));
	}
	fprintf(stderr,"\n");
	retval=libusb_control_transfer(usbhandle,
					SEND_REQUEST_TYPE,
					SEND_REQUEST,
					SEND_VALUE,
					SEND_INDEX,
					data,
					SEND_DATA_SIZE,
					0);

	if (retval!=9)
		return LIBUSB_WRITE_ERR;

	return	OK;
}


int usbwrite(libusb_device_handle* usbhandle,unsigned char* data,unsigned short len) {
  int retval;
  int i;
  if (len!=SEND_DATA_SIZE)
    return LIBUSB_SIZE_ERR;


  fprintf(stderr,"write> ");
  for (i=0;i<SEND_DATA_SIZE;i++) {
    fprintf(stderr,"%02x ",0xff&((unsigned int)data[i]));
  }
  fprintf(stderr,"\n");

  retval=libusb_control_transfer(
    usbhandle,
    SEND_REQUEST_TYPE,
    SEND_REQUEST,
    SEND_VALUE,
    SEND_INDEX,
    data,
    SEND_DATA_SIZE,
    0
  );
  if (retval!=SEND_DATA_SIZE)
    return LIBUSB_WRITE_ERR;
  usleep(9000);	
  return OK;
}

int usbread(libusb_device_handle* usbhandle, unsigned char* data, unsigned int len) {
  unsigned char buf[READ_DATA_SIZE];
  int readbytes;
  int i;

  if (len!=READ_DATA_SIZE)
    return LIBUSB_SIZE_ERR;
	
  readbytes=libusb_control_transfer(
    usbhandle,
    READ_REQUEST_TYPE,
    READ_REQUEST,
    READ_VALUE,
    READ_INDEX,
    buf,
    READ_DATA_SIZE,
    0
  );
  if (readbytes!=READ_DATA_SIZE)
    return LIBUSB_READ_ERR;
  fprintf(stderr,"read>  ");
  for (i=0;i<READ_DATA_SIZE;i++) {
    data[i]=buf[i];
    fprintf(stderr,"%02x ",0xff&((unsigned int)data[i]));
  }
  fprintf(stderr,"\n");
  return OK;
}


void reset(libusb_device_handle* usbhandle) {
  unsigned char data[9]={0x02,0x07,0x04,0x00,0x00,0x00,0x00,0x00,0x00};
  usbwrite(usbhandle,data,9);
}
void end(libusb_device_handle* usbhandle) {
  unsigned char data[9]={0x02,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  usbwrite(usbhandle,data,9);
}

void pge(libusb_device_handle* usbhandle) {
  unsigned char data[9]={0x02,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  usbwrite(usbhandle,data,9);
}

void chk(libusb_device_handle* usbhandle) {
  unsigned char data[9]={0x02,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  usbwrite(usbhandle,data,9);
}

void wait(libusb_device_handle* usbhandle) {
  unsigned char rply[8];
  int ready=0;
  while (!ready) {
    chk(usbhandle);
    usbread(usbhandle,rply,8);
    if (rply[0]==0x11) ready=1;
  }
}

void setZoneColor(libusb_device_handle*	usbhandle, int zone, int r, int g, int b) {
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
