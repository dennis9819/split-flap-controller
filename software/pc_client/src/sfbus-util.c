/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 */

#include "sfbus-util.h"
int sfbusu_write_address(int fd, u_int16_t current, u_int16_t new)
{
    if (new < 1)
    {
        printf("Please specify new address > 0 with -d\n");
        return -1;
    }
    // read current eeprom status
    char *buffer_w = malloc(64);
    char *buffer_r = malloc(64);
    if (sfbus_read_eeprom(fd, current, buffer_w) < 0)
    {
        fprintf(stderr, "Error reading eeprom\n");
        return 1;
    }
    // modify current addr
    u_int16_t n_addr_16 = new;
    memcpy(buffer_w, &n_addr_16, 2);
    if (sfbus_write_eeprom(fd, current, buffer_w, buffer_r) < 0)
    {
        fprintf(stderr, "Error writing eeprom\n");
        return 1;
    }

    return 0;
}

int sfbusu_write_calibration(int fd, u_int16_t address, u_int16_t data)
{
    // read current eeprom status
    char *buffer_w = malloc(64);
    char *buffer_r = malloc(64);
    if (sfbus_read_eeprom(fd, address, buffer_w) < 0)
    {
        fprintf(stderr, "Error reading eeprom\n");
        return 1;
    }
    // modify current calibration
    memcpy(buffer_w + 2, &data, 2);
    if (sfbus_write_eeprom(fd, address, buffer_w, buffer_r) < 0)
    {
        fprintf(stderr, "Error writing eeprom\n");
        return 1;
    }

    return 0;
}
