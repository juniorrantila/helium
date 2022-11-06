#pragma once
#include "AddressSpace.h"

namespace Mem {

template <typename T, typename AddressSpace>
struct NullablePtr {
    using address_space = AddressSpace;
    static constexpr auto null = address_space::size - 1;

    constexpr NullablePtr(nullptr_t)
        : m_index(null)
    {
    }

    static constexpr NullablePtr from_raw(T* raw)
    {
        return { ((uptr)(raw)-address_space::base) };
    }

    static constexpr NullablePtr from(u32 index)
    {
        return NullablePtr(index);
    }

    explicit constexpr operator bool() const
    {
        return m_index != null;
    }

    constexpr bool operator==(NullablePtr other) const
    {
        return m_index == other.m_index;
    }

    constexpr bool operator==(nullptr_t) const
    {
        return m_index == null;
    }

    constexpr T& operator*() { return *raw(); }
    constexpr T const& operator*() const { return *raw(); }

    constexpr T* operator->() { return raw(); }
    constexpr T const* operator->() const { return raw(); }

    constexpr T* raw()
    {
        return (T*)(address_space::base + m_index);
    }
    constexpr T const* raw() const
    {
        return (T*)(address_space::base + m_index);
    }

private:
    constexpr NullablePtr(u32 index)
        : m_index(index)
    {
    }

    u32 m_index;
};

template <typename T>
using NullableLoPtr = NullablePtr<T, LoRam>;

template <typename T>
using NullableHiPtr = NullablePtr<T, HiRam>;

}
