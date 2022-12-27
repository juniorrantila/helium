#pragma once
#ifndef __aarch64__
#    error "trying to include aarch64 header"
#endif

#include <Ty/Base.h>

namespace Arch::Internal::Time {

inline u64 current_cycle()
{
    u64 value = 0;
    asm volatile("mrs %0, cntvct_el0" : "=r"(value));
    return value;
}

}
