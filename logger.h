#ifndef LOGGER_H
#define LOGGER_H

void log_info(const char* message);
void log_error(const char* message);
void log_warning(const char* message);
void log_debug(const char* message);
void log_message(const char* message);
void log_clear();

#endif