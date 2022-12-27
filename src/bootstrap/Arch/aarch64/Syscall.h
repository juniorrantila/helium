#pragma once
#ifndef __aarch64__
#    error "trying to include aarch64 specific header"
#endif

#include <Ty/Base.h>

#ifndef ALWAYS_INLINE
#    define ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

#define REGISTER_DECL_0                        \
    register long x8 __asm__("x8") = __number; \
    register long x0 __asm__("x0")

#define REGISTER_DECL_1                        \
    register long x8 __asm__("x8") = __number; \
    register long x0 __asm__("x0") = __arg1

#define REGISTER_DECL_2 \
    REGISTER_DECL_1;    \
    register long x1 __asm__("x1") = __arg2

#define REGISTER_DECL_3 \
    REGISTER_DECL_2;    \
    register long x2 __asm__("x2") = __arg3

#define REGISTER_DECL_4 \
    REGISTER_DECL_3;    \
    register long x3 __asm__("x3") = __arg4

#define REGISTER_DECL_5 \
    REGISTER_DECL_4;    \
    register long x4 __asm__("x4") = __arg5

#define REGISTER_DECL_6 \
    REGISTER_DECL_5;    \
    register long x5 __asm__("x5") = __arg6

#define SYSCALL_INSTR(input_constraint) \
    asm volatile("svc 0"                \
                 : "=r"(x0)             \
                 : input_constraint     \
                 : "memory", "cc")

#define REGISTER_CONSTRAINT_0 "r"(x8)
#define REGISTER_CONSTRAINT_1 REGISTER_CONSTRAINT_0, "r"(x0)
#define REGISTER_CONSTRAINT_2 REGISTER_CONSTRAINT_1, "r"(x1)
#define REGISTER_CONSTRAINT_3 REGISTER_CONSTRAINT_2, "r"(x2)
#define REGISTER_CONSTRAINT_4 REGISTER_CONSTRAINT_3, "r"(x3)
#define REGISTER_CONSTRAINT_5 REGISTER_CONSTRAINT_4, "r"(x4)
#define REGISTER_CONSTRAINT_6 REGISTER_CONSTRAINT_5, "r"(x5)

namespace Arch {

ALWAYS_INLINE iptr syscall_impl(iptr __number)
{
    REGISTER_DECL_0;
    SYSCALL_INSTR(REGISTER_CONSTRAINT_0);
    return x0;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1)
{
    REGISTER_DECL_1;
    SYSCALL_INSTR(REGISTER_CONSTRAINT_1);
    return x0;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2)
{
    REGISTER_DECL_2;
    SYSCALL_INSTR(REGISTER_CONSTRAINT_2);
    return x0;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3)
{
    REGISTER_DECL_3;
    SYSCALL_INSTR(REGISTER_CONSTRAINT_3);
    return x0;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3, iptr __arg4)
{
    REGISTER_DECL_4;
    SYSCALL_INSTR(REGISTER_CONSTRAINT_4);
    return x0;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3, iptr __arg4, iptr __arg5)
{
    REGISTER_DECL_5;
    SYSCALL_INSTR(REGISTER_CONSTRAINT_5);
    return x0;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3, iptr __arg4, iptr __arg5, iptr __arg6)
{
    REGISTER_DECL_6;
    SYSCALL_INSTR(REGISTER_CONSTRAINT_6);
    return x0;
}

}

#undef REGISTER_DECL_0
#undef REGISTER_DECL_1
#undef REGISTER_DECL_2
#undef REGISTER_DECL_3
#undef REGISTER_DECL_4
#undef REGISTER_DECL_5
#undef REGISTER_DECL_6
#undef SYSCALL_INSTR
#undef REGISTER_CONSTRAINT_0
#undef REGISTER_CONSTRAINT_1
#undef REGISTER_CONSTRAINT_2
#undef REGISTER_CONSTRAINT_3
#undef REGISTER_CONSTRAINT_4
#undef REGISTER_CONSTRAINT_5
#undef REGISTER_CONSTRAINT_6
