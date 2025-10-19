/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 * This section provides a simple logging functionality
 */

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int log_level_global = 4; // set default log level to WARNING

const char *loglevel[] = {"TRACE","DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};

/*
* Initialize logger with specified log level
*
* @param log_level: log level to set
*/
void init_logger(int log_level) 
{
    log_level_global = log_level;
    log_message(LOG_INFO, "Set log level to %s", loglevel[log_level_global]);

}

/*
* Log message with specified log level
*
* @param level: log level of message
* @param message: message format string
* @param ...: additional arguments for message format
*/
void log_message(int level, const char *message, ...)
{
    if (level >= log_level_global)
    {
        va_list args; // get arguments
        va_start(args, message);
        time_t now; // prepare time
        time(&now);
        char *ctime_no_newline = strtok(ctime(&now), "\n");
        printf("%s [%s]: ", ctime_no_newline, loglevel[level]); // print message
        vprintf(message, args);
        printf("\n");
    }
}

/*
* Log message header with specified log level
*
* @param level: log level of message
* @return 1 if message should be logged, 0 otherwise    
*/
int log_message_header(int level)
{
    if (level >= log_level_global)
    {
        time_t now; // prepare time
        time(&now);
        char *ctime_no_newline = strtok(ctime(&now), "\n");
        printf("%s [%s]: ", ctime_no_newline, loglevel[level]); // print message
        return 1;
    }
    return 0;
}