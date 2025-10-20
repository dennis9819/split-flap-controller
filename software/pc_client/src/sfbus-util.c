/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 */

#include "sfbus-util.h"
#include "logging/logger.h"

/*
* Write new address to device
*
* @param fd: rs485 file descriptor
* @param current: current device address
* @param new: new device address
* @return 0 on success, -1 on error
*/
int sfbusu_write_address(int fd, u_int16_t current, u_int16_t new)
{
    log_message(LOG_INFO, "Writing new address 0x%04X to device with current address 0x%04X", new, current);
    if (new < 1)
    {
        log_message(LOG_ERROR, "Cannot write address: Please specify new address > 0 with -d");
        return -1;
    }
    // read current eeprom status
    char *buffer_w = malloc(SFBUS_MAX_BUFFER_SIZE);
    char *buffer_r = malloc(SFBUS_MAX_BUFFER_SIZE);
    if (sfbus_read_eeprom(fd, current, buffer_w) < 0)
    {
        log_message(LOG_ERROR, "Cannot write address: Error reading eeprom");
        return 1;
    }
    // modify current addr
    u_int16_t n_addr_16 = new;
    memcpy(buffer_w, &n_addr_16, 2);
    if (sfbus_write_eeprom(fd, current, buffer_w, buffer_r) < 0)
    {
        log_message(LOG_ERROR, "Cannot write address: Error writing eeprom");
        return 1;
    }
    return 0;
}

/*
* Write new calibration data to device
*
* @param fd: rs485 file descriptor
* @param address: device address
* @param data: calibration data
* @return 0 on success, -1 on error
*/
int sfbusu_write_calibration(int fd, u_int16_t address, u_int16_t data)
{
    log_message(LOG_INFO, "Writing new calibration 0x%04X to device at address 0x%04X", data, address);
    // read current eeprom status
    char *buffer_w = malloc(SFBUS_MAX_BUFFER_SIZE);
    char *buffer_r = malloc(SFBUS_MAX_BUFFER_SIZE);
    if (sfbus_read_eeprom(fd, address, buffer_w) < 0)
    {
        log_message(LOG_ERROR, "Cannot write calibration: Error reading eeprom");
        return 1;
    }
    // modify current calibration
    memcpy(buffer_w + 2, &data, 2);
    if (sfbus_write_eeprom(fd, address, buffer_w, buffer_r) < 0)
    {
        log_message(LOG_ERROR, "Cannot write address: Error writing eeprom");
        return 1;
    }

    return 0;
}
