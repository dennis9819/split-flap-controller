/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 */

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