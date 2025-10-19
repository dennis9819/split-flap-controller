#ifndef LOGGER_H
#define LOGGER_H

void log_message(int level, const char *message, ...);
int log_message_header(int level);
void init_logger(int log_level);

enum logLevelEnum
{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
};
#endif /* LOG_H */
