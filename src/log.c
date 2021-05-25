#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "log.h"

void my_log(const char *format, ...)
{
    time_t now;
    struct tm* timeinfo;
    char timestring[32];

    time(&now);
    timeinfo = localtime(&now);
    strftime(timestring, sizeof(timestring), "%Y-%m-%d %H:%M:%S", timeinfo);

    fprintf(
        stderr, "%s %s[%d]: ",
        timestring,
        "ssh-honeypotd",
        getpid()
    );

    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fputc('\n', stderr);
}
