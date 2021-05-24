#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void check_alloc(void* p, const char* api)
{
    if (!p) {
        perror(api);
        exit(EXIT_FAILURE);
    }
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((malloc, returns_nonnull))
#endif
char* my_strdup(const char *s)
{
    char* retval = strdup(s);
    check_alloc(retval, "strdup");
    return retval;
}
