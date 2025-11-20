#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "log.h"

int terse_logs = 0;

void my_log(const char *format, ...)
{
    if (terse_logs == 0) {
        time_t now;
        struct tm timeinfo;
        char timestring[32];

        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(timestring, sizeof(timestring), "%F %T", &timeinfo);

        fputs(timestring, stderr);
        fputc(' ', stderr);
    }

    fputs("tiny-ssh-honeypot: ", stderr);

    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fputc('\n', stderr);
}
