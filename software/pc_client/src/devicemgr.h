#include "sfbus.h"
#include <ctype.h>
#include <errno.h> // Error integer and strerror() function
#include <fcntl.h> // Contains file controls like O_RDWR
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()

int devicemgr_readStatus(int device_id);
int devicemgr_readCalib(int device_id);
void devicemgr_printDetails(int device_id, json_object *root);
void devicemgr_printDetailsAll(json_object *root);
int devicemgr_register(int rs485_descriptor, u_int16_t address, int x, int y, int nid);
void devicemgr_init();
int devicemgr_print(char *text);
int devicemgr_refresh();
int devicemgr_save(char *file);
void devicemgr_printText(char *text, int x, int y);
void devicemgr_printFlap(int flap, int x, int y);