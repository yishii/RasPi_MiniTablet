/*
  RaspberryPi mini tablet sub-mcu handler
  Ver 0.01

  Copyright Yasuhiro ISHII(ishii.yasuhiro@gmail.com)
  This program is distributed under the lisence of
  Apache 2.0.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <stdbool.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define I2C_SLAVE_ADDR 0x54

void syncInput(int fd);

typedef struct {
  unsigned short x;
  unsigned short y;
  unsigned short touch;
} CommunicationContainer;

typedef struct {
  bool tapped;
  int x;
  int y;
} TouchEvent;

bool getTouchEvent(int fd,TouchEvent* ev)
{
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg[1];
  int result;
  CommunicationContainer comm;
  memset((void*)&comm,0,sizeof(comm));

  msg[0].addr = I2C_SLAVE_ADDR;
  msg[0].flags = I2C_M_RD;
  msg[0].len = sizeof(comm);
  msg[0].buf = (void*)&comm;

  data.msgs = msg;
  data.nmsgs = 1;

  result = ioctl(fd,I2C_RDWR,&data);
  
  if(comm.touch == 1){
    ev->x = comm.x;
    ev->y = comm.y;
    ev->tapped = true;
    //printf("x,y = %5d,%5d\n",ev->x,ev->y);
  } else {
    ev->tapped = false;
  }

  return(true);
}

#if 1
void createTouchInterface(int fd)
{
  struct uinput_user_dev uidev;

  ioctl(fd,UI_SET_EVBIT,EV_KEY);
  ioctl(fd,UI_SET_KEYBIT,BTN_TOUCH);
  ioctl(fd,UI_SET_EVBIT,EV_ABS);
  ioctl(fd,UI_SET_ABSBIT,ABS_X);
  ioctl(fd,UI_SET_ABSBIT,ABS_Y);
  ioctl(fd,UI_SET_ABSBIT,ABS_PRESSURE);

  memset(&uidev,0,sizeof(uidev));

  strcpy(uidev.name,"RasPiTablet_TouchScreen");
  uidev.id.bustype = BUS_VIRTUAL;
  uidev.id.vendor = 0x1c;
  uidev.id.product = 0x01;
  uidev.id.version = 0x01;

  uidev.absmin[ABS_X] = 0;
  uidev.absmax[ABS_X] = 1023;
  uidev.absmin[ABS_Y] = 0;
  uidev.absmax[ABS_Y] = 1023;
  uidev.absfuzz[ABS_X] = 0;
  uidev.absfuzz[ABS_Y] = 0;
  uidev.absflat[ABS_X] = 0;
  uidev.absflat[ABS_Y] = 0;

  uidev.absmin[ABS_PRESSURE] = 0;
  uidev.absmax[ABS_PRESSURE] = 1024;

  if(write(fd,&uidev,sizeof(uidev)) < 0){
    printf("Write err\n");
  }

  if(ioctl(fd,UI_DEV_CREATE/*,NULL*/) < 0){
    printf("UI_DEV_CREATE err\n");
  }
}
#else
void createTouchInterface(int fd)
{
  struct uinput_user_dev uidev;

  memset(&uidev,0,sizeof(uidev));

  strcpy(uidev.name,"TouchScreen");
  uidev.id.bustype = BUS_USB;//BUS_VIRTUAL;
  uidev.id.vendor = 0x1c;
  uidev.id.product = 0x01;
  uidev.id.version = 0x01;
  uidev.absmin[ABS_X] = 0;
  uidev.absmax[ABS_X] = 479;
  uidev.absmin[ABS_Y] = 0;
  uidev.absmax[ABS_Y] = 271;

  if(write(fd,&uidev,sizeof(uidev)) < 0){
    printf("Write err\n");
  }

  ioctl(fd,UI_SET_EVBIT,EV_ABS);
  ioctl(fd,UI_SET_ABSBIT,ABS_X);
  ioctl(fd,UI_SET_ABSBIT,ABS_Y);
  //ioctl(fd,UI_SET_ABSBIT,ABS_PRESSURE);
  ioctl(fd,UI_SET_EVBIT,EV_SYN);

  ioctl(fd,UI_SET_EVBIT,EV_KEY);
  ioctl(fd,UI_SET_KEYBIT,BTN_LEFT);
  ioctl(fd,UI_SET_KEYBIT,BTN_TOUCH);

  if(ioctl(fd,UI_DEV_CREATE,NULL) < 0){
    printf("UI_DEV_CREATE err\n");
  }
}
#endif

void setTouchEvent(int fd,TouchEvent* touch_event)
{
  struct input_event ev;

  memset((void*)&ev,0,sizeof(struct input_event));

  memset(&ev,0,sizeof(ev));
    
  gettimeofday(&ev.time,NULL);
  ev.type = EV_ABS;
  ev.code = ABS_X;
  ev.value = touch_event->x;
  //printf("%d\n",ev.value);
  if(write(fd,&ev,sizeof(ev)) < 0){
    printf("write err\n");
  }
  //gettimeofday(&ev.time,NULL);
  ev.type = EV_ABS;
  ev.code = ABS_Y;
  ev.value = touch_event->y;
  //printf("%d\n",ev.value);
  if(write(fd,&ev,sizeof(ev)) < 0){
    printf("write err\n");
  }

  syncInput(fd);
}

void setTapEvent(int fd,bool tap)
{
  struct input_event ev;

  memset((void*)&ev,0,sizeof(struct input_event));
  gettimeofday(&ev.time,NULL);
  ev.type = EV_KEY;
  ev.code = BTN_TOUCH;
  ev.value = tap == true ? 1 : 0;
  if(write(fd,&ev,sizeof(ev)) < 0){
    printf("write err\n");
  }

  ev.type = EV_ABS;
  ev.code = ABS_PRESSURE;
  ev.value = tap == true ? 1024 : 0;
  if(write(fd,&ev,sizeof(ev)) < 0){
    printf("write err\n");
  }

  syncInput(fd);
}


void syncInput(int fd)
{
  struct input_event ev;

  memset((void*)&ev,0,sizeof(struct input_event));
  
  gettimeofday(&ev.time,NULL);
  ev.type = EV_SYN;
  ev.code = SYN_REPORT;
  ev.value = 0;

  if(write(fd,&ev,sizeof(ev)) < 0){
    printf("write err\n");
  }
}

int main(void)
{
  int fd;
  int ui_fd;
  TouchEvent event;
  bool event_tapped_prev = false;

  fd = open("/dev/i2c-1",O_RDWR);
  if(fd < 0){
    printf("i2c open error\n");
    return(-1);
  }

  ui_fd = open("/dev/uinput",O_WRONLY | O_NONBLOCK);
  if(ui_fd < 0){
    printf("uinput device open error\n");
    return(-2);
  }

  createTouchInterface(ui_fd);

  while(1) {
    getTouchEvent(fd,&event);
    if(event.tapped){
      setTouchEvent(ui_fd,&event);
    }
    setTapEvent(ui_fd,event.tapped);

    event_tapped_prev = event.tapped;
  }

  close(fd);
}
