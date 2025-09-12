#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <string.h>
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()
#include <sys/types.h>
#include "sfbus.h"
#include <json-c/json_object.h>
#include <sys/types.h>
#include <json-c/json.h>
#include <ctype.h>

int devicemgr_readStatus(int device_id) ;
json_object *  devicemgr_printDetails(int device_id);
json_object * devicemgr_printDetailsAll();
int devicemgr_register(int rs485_descriptor, u_int16_t address, int x,int y);
void devicemgr_init();
int devicemgr_print(char* text);