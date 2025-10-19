#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int log_level_global = 4;

const char *loglevel[] = {"TRACE","DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};

void init_logger(int log_level)
{
    log_level_global = log_level;
    log_message(LOG_INFO, "Set log level to %s", loglevel[log_level_global]);

}

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