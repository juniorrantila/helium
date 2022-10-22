#pragma once
#include "Hardware.h"

namespace Core {

struct Threads {
    static u32 in_machine()
    {
        return Hardware::the().threads();
    }
};

}
