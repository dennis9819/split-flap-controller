#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <string.h>
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()
#include <sys/types.h>

#define RS485TX 0
#define RS485RX 1

int rs485_init(char *device, int baud);
int rs485_trdir(int rs485_fd, int level);