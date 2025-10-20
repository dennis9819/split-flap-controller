/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
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
#include "logging/logger.h"
#include "sfbus.h"

extern char *optarg;

void printUsage(char *argv[])
{
    fprintf(stderr, "Usage: %s -p <tty> -c <command> [value]\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{

    // initialize logger
    init_logger(LOG_TRACE);
    log_message(LOG_INFO, "Starting split-flap pc client %s", "v1.0.0");
    log_message(LOG_INFO, "(c) 2024-2025 GuniaLabs (www.dennisgunia.de)");

    // parse arguments
    int opt = ' ';
    char *port = NULL, *config_file = NULL, *log_level = NULL;

    while ((opt = getopt(argc, argv, "p:c:l:")) != -1) // check options
    {
        switch (opt)
        {
        case 'p':
            port = optarg;
            break;
        case 'c':
            config_file = optarg;
            break;
        case 'l':
            log_level = optarg;
            break;
        default:
            printUsage(argv);
        }
    }


    if (log_level != NULL) // if log level specified
    {
        if (strlen(log_level) > 0)
        {                                                     // if log level specified
            long inputLogLevel = strtol(log_level, NULL, 10); // parse log level
            init_logger(inputLogLevel);                       // re-init logger with new level
        }
    }

    if (config_file == NULL) // if config file not specified
    {
        log_message(LOG_CRITICAL, "Please specify config file\n");
        printUsage(argv);
    }
    else
    {
        if (strlen(config_file) == 0) // if config file path empty
        {
            log_message(LOG_CRITICAL, "Please specify config file\n");
        }
        else
        {
            if (access(config_file, F_OK) != 0) // check if config file exists and can be opened
            {
                log_message(LOG_CRITICAL, "Config file: %s does not exist or cannot be opened\n", config_file);
                printUsage(argv);
            }
            else
            {
                log_message(LOG_INFO, "Use device configuration at '%s'", config_file);
            }
        }
    }

    if (port == NULL) // if port not specified
    {
        log_message(LOG_CRITICAL, "Please specify serial port\n");
        printUsage(argv);
    }
    else
    {
        if (strlen(port) == 0) // if port path empty
        {
            log_message(LOG_CRITICAL, "Please specify serial port\n");
            printUsage(argv);
        }
        else
        {
            if (access(port, F_OK) != 0) // check if port exists and can be opened
            {
                log_message(LOG_CRITICAL, "Serial port: %s does not exist or cannot be opened\n", port);
                printUsage(argv);
            }
            else
            {
                log_message(LOG_INFO, "Use serial port at '%s'", port);
            }
        }
    }

    int fd = rs485_init(port, B57600); // setup rs485
    start_console(fd, config_file);    // start console
    return 0;
}