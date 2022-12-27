#pragma once
#ifndef __x86_64__
#    error "trying to include x86_64 specific header"
#endif

#include <Ty/Base.h>

#ifndef ALWAYS_INLINE
#    define ALWAYS_INLINE __attribute__((always_inline)) inline
#endif
#define SYSCALL_CLOBBER_LIST "rcx", "r11", "memory"

namespace Arch {

ALWAYS_INLINE iptr syscall_impl(iptr __number)
{
    iptr retcode;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1)
{
    iptr retcode;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2)
{
    iptr retcode;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1), "S"(__arg2)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3)
{
    iptr retcode;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1), "S"(__arg2),
                 "d"(__arg3)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3, iptr __arg4)
{
    iptr retcode;
    register iptr r10 __asm__("r10") = __arg4;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1), "S"(__arg2),
                 "d"(__arg3), "r"(r10)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3, iptr __arg4, iptr __arg5)
{
    iptr retcode;
    register iptr r10 __asm__("r10") = __arg4;
    register iptr r8 __asm__("r8") = __arg5;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1), "S"(__arg2),
                 "d"(__arg3), "r"(r10), "r"(r8)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3, iptr __arg4, iptr __arg5, iptr __arg6)
{
    iptr retcode;
    register iptr r10 __asm__("r10") = __arg4;
    register iptr r8 __asm__("r8") = __arg5;
    register iptr r9 __asm__("r9") = __arg6;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1), "S"(__arg2),
                 "d"(__arg3), "r"(r10), "r"(r8), "r"(r9)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}
#undef SYSCALL_CLOBBER_LIST

}
