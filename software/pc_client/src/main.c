/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is free software (...)
 * You should have received a copy of the GNU Affero General Public License
 * along with this program (...)
 *
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial activities involving the split-flap software without
 * disclosing the source code of your own applications (...)
 *
 */

#include <fcntl.h> // Contains file controls like O_RDWR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()

#include "console.h"
#include "devicemgr.h"
#include "ftdi485.h"
#include "sfbus.h"


void printUsage(char *argv[])
{
    fprintf(stderr, "Usage: %s -p <tty> -c <command> [value]\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int opt = ' ';
    u_int16_t addr_int = 0;
    char *port = malloc(256);
    char *command = malloc(16);
    char *addr = malloc(16);
    char *data = malloc(256);
    command = "";
    addr = "";
    data = "";
    while ((opt = getopt(argc, argv, "p:c:a:d:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = optarg;
            break;
        case 'c':
            command = optarg;
            break;
        case 'a':
            addr = optarg;
            break;
        case 'd':
            data = optarg;
            break;
        default:
            printUsage(argv);
        }
    }
    if (access(port, F_OK) != 0)
    {
        fprintf(stderr, "Filedescriptor: %s does not exist or cannot be opened\n", port);
        printUsage(argv);
    }
    // parse address
    if (strlen(addr) == 0)
    {
        fprintf(stderr, "Please specify address\n");
        printUsage(argv);
    }
    else
    {
        addr_int = strtol(addr, NULL, 10);
    }

    // start program
    setvbuf(stdout, NULL, _IONBF, 0); // do not buffer stdout!!!!

    printf("Open device at %s\n", port);
    int fd = rs485_init(port, B19200); // setup rs485

    if (strcmp(command, "ping") == 0)
    {
        sfbus_ping(fd, addr_int);
    }
    else if (strcmp(command, "printf") == 0)
    {
        devicemgr_printText(data, 0, 0);
    }
    else if (strcmp(command, "r_eeprom") == 0)
    {
        char *buffer = malloc(64);
        sfbus_read_eeprom(fd, addr_int, buffer);
        printf("Read data: 0x");
        print_charHex(buffer, 5);
        printf("\n");
        free(buffer);
        exit(0);
    }
    else if (strcmp(command, "w_addr") == 0)
    {
        int n_addr = strtol(data, NULL, 10);
        int ret = sfbusu_write_address(fd, addr_int, n_addr);
        exit(ret);
    }
    else if (strcmp(command, "w_cal") == 0)
    {
        int n_addr = strtol(data, NULL, 10);
        int ret = sfbusu_write_calibration(fd, addr_int, n_addr);
        exit(ret);
    }
    else if (strcmp(command, "status") == 0)
    {
        double voltage = 0;
        u_int32_t counter = 0;
        u_int8_t status = sfbus_read_status(fd, addr_int, &voltage, &counter);
        printf("=======================\n");
        printf("Status register flags :\n");
        printf(" 00 -> errorTooBig : %i\n", (status >> 0) & 0x01);
        printf(" 01 -> noHome      : %i\n", (status >> 1) & 0x01);
        printf(" 02 -> fuseBlown   : %i\n", (status >> 2) & 0x01);
        printf(" 03 -> homeSense   : %i\n", (status >> 3) & 0x01);
        printf(" 04 -> powerDown   : %i\n", (status >> 4) & 0x01);
        printf(" 05 -> failSafe    : %i\n", (status >> 5) & 0x01);
        printf(" 06 -> busy        : %i\n", (status >> 6) & 0x01);
        printf("Driver-Voltage    : %.2fV\n", voltage);
        printf("Rotations         : %i\n", counter);

        exit(0);
    }
    else if (strcmp(command, "display") == 0)
    {
        int flap = strtol(data, NULL, 10);
        // read current eeprom status
        char *buffer_w = malloc(4);
        sfbus_display(fd, addr_int, flap);
        exit(0);
    }
    else if (strcmp(command, "display_fr") == 0)
    {
        int flap = strtol(data, NULL, 10);
        // read current eeprom status
        char *buffer_w = malloc(4);
        sfbus_display_full(fd, addr_int, flap);
        exit(0);
    }
    else if (strcmp(command, "reset") == 0)
    {
        sfbus_reset_device(fd, addr_int);
        exit(0);
    }
    else if (strcmp(command, "power_on") == 0)
    {
        sfbus_motor_power(fd, addr_int, 1);
        exit(0);
    }
    else if (strcmp(command, "power_off") == 0)
    {
        sfbus_motor_power(fd, addr_int, 0);
        exit(0);
    }
    else if (strcmp(command, "server") == 0)
    {
        start_console(fd);
    }
    else
    {
        fprintf(stderr, "Invalid command specified!\n");
        printUsage(argv);
    }
    return 0;
}