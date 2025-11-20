#include <stdarg.h>
#include <syslog.h>
#include "log.h"

void my_log(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsyslog(LOG_INFO, format, ap);
    va_end(ap);
}
