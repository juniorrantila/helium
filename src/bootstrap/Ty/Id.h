#pragma once
#include "Base.h"

namespace Ty {

template <typename T>
struct Id {
    constexpr Id() = default;

    constexpr explicit Id(u32 raw_id)
        : m_raw(raw_id)
    {
    }

    constexpr u32 raw() const { return m_raw; }

    static constexpr Id<T> invalid() { return Id<T> { 0xFFFFFF }; }
    constexpr bool is_valid() const { return m_raw != 0xFFFFFF; }

private:
    u32 m_raw { 0 };
};

}

using namespace Ty;
