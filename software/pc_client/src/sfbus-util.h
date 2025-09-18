#include "sfbus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sfbusu_write_address(int fd, u_int16_t current, u_int16_t new);
int sfbusu_write_calibration(int fd, u_int16_t address, u_int16_t data);