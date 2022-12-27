#pragma once
#ifndef __x86_64__
#    error "trying to include x86_64 header"
#endif
#include <Ty/Base.h>

namespace Arch::Internal::Time {

inline u64 current_cycle() { return __rdtsc(); }

}
