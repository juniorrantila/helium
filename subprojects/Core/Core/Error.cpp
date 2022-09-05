#include <Core/Error.h>
#include <iostream>

namespace Core {

void Error::show() const {
    std::cerr << function() << ": "
              << message() << ' '
              << '[' << file()
              << ':' << line_in_file()
              << ']' << std::endl;
}

}
