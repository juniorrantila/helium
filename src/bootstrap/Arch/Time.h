#pragma once
#include <Ty/Base.h>
#if __x86_64__
#    include "x86_64/Time.h"
#elif __aarch64__
#    include "aarch64/Time.h"
#else
#    error "unimplemented"
#endif

namespace Arch {

struct Time {
    static u64 current_cycle()
    {
        return Internal::Time::current_cycle();
    }
};

}
