#include "logger.h"
#include <stdio.h>
#include <time.h>

static FILE* log_file = NULL;

static void ensure_log_file()
{
    if (log_file == NULL)
    {
        log_file = fopen("debug_log.txt", "a");
        if (log_file)
        {
            time_t now = time(0);
            char* dt = ctime(&now);
            fprintf(log_file, "\n=== Log started at: %s", dt);
            fflush(log_file);
        }
    }
}

void log_info(const char* message)
{
    ensure_log_file();
    if (!log_file) return;

    time_t now = time(0);
    char* dt = ctime(&now);
    dt[24] = '\0';

    fprintf(log_file, "[INFO] %s - %s\n", dt, message);
    fflush(log_file);
}

void log_error(const char* message)
{
    ensure_log_file();
    if (!log_file) return;

    time_t now = time(0);
    char* dt = ctime(&now);
    dt[24] = '\0';

    fprintf(log_file, "[ERROR] %s - %s\n", dt, message);
    fflush(log_file);
}

void log_warning(const char* message)
{
    ensure_log_file();
    if (!log_file) return;

    time_t now = time(0);
    char* dt = ctime(&now);
    dt[24] = '\0';

    fprintf(log_file, "[WARNING] %s - %s\n", dt, message);
    fflush(log_file);
}

void log_debug(const char* message)
{
    ensure_log_file();
    if (!log_file) return;

    time_t now = time(0);
    char* dt = ctime(&now);
    dt[24] = '\0';

    fprintf(log_file, "[DEBUG] %s - %s\n", dt, message);
    fflush(log_file);
}

void log_message(const char* message)
{
    ensure_log_file();
    if (!log_file) return;

    time_t now = time(0);
    char* dt = ctime(&now);
    dt[24] = '\0';

    fprintf(log_file, "[LOG] %s - %s\n", dt, message);
    fflush(log_file);
}

void log_clear()
{
    if (log_file)
    {
        fclose(log_file);
        log_file = NULL;
    }

    log_file = fopen("debug_log.txt", "w");
    if (log_file)
    {
        time_t now = time(0);
        char* dt = ctime(&now);
        fprintf(log_file, "=== Log started (fresh) at: %s", dt);
        fflush(log_file);
    }
}