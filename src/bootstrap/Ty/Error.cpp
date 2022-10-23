#include "Error.h"
#include <iostream>
#include <string.h>

namespace Ty {

c_string Error::errno_to_string(int code) { return strerror(code); }

void Error::show() const
{
    std::cerr << function() << ": " << message() << ' ' << '['
              << file() << ':' << line_in_file() << ']'
              << std::endl;
}

}
