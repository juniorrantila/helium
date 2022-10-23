#include "Error.h"
#include <iostream>
#include <string.h>

c_string errno_to_string(int code) { return strerror(code); }

void Error$show(Error error)
{
    std::cerr << error.function << ": " << error.message << ' '
              << '[' << error.file << ':' << error.line_in_file
              << ']' << std::endl;
}
