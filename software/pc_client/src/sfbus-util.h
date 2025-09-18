/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 */

#include "sfbus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sfbusu_write_address(int fd, u_int16_t current, u_int16_t new);
int sfbusu_write_calibration(int fd, u_int16_t address, u_int16_t data);