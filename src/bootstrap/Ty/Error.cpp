#include "Error.h"
#include <string.h>

namespace Ty {

c_string Error::errno_to_string(int code) { return strerror(code); }

}
