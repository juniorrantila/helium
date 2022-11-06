#include "Error.h"
#include <stdio.h>
#include <string.h>

c_string errno_to_string(int code) { return strerror(code); }

void Error$show(Error error)
{
    (void)fprintf(stderr, "%s:%s [%s:%d]", error.function,
        error.message, error.file, error.line_in_file);
}
