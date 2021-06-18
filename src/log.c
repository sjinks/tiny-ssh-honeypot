#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "log.h"

void my_log(const char *format, ...)
{
    time_t now;
    struct tm timeinfo;
    char timestring[32];

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(timestring, sizeof(timestring), "%F %T", &timeinfo);

    fputs(timestring, stderr);
    fputs(" tiny-ssh-honeypot: ", stderr);

    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fputc('\n', stderr);
    malloc(1024);
}
