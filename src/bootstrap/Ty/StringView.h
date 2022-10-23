#pragma once
#include "Base.h"
#include <iostream>
#include <string_view>

namespace Ty {

struct StringView {
    char const* data;
    u32 size;

    constexpr StringView(std::string_view other)
        : data(other.data())
        , size(other.size())
    {
    }

    constexpr StringView() = default;

    static constexpr StringView from_c_string(c_string data)
    {
        return StringView(data, __builtin_strlen(data));
    }

    constexpr StringView(char const* data, u32 size)
        : data(data)
        , size(size)
    {
    }

    constexpr bool operator==(StringView other) const
    {
        if (size != other.size)
            return false;
        if (data == other.data)
            return true;
        bool same = true;
        // clang-format off
        while (other.size --> 0)
            same &= data[other.size] == other.data[other.size];
        // clang-format on
        return same;
    }

    constexpr bool is_empty() const { return size == 0; }

    constexpr char const& operator[](u32 index) const
    {
        return data[index];
    }

    constexpr u32 unchecked_copy_to(char* other, u32 size) const
    {
        if (data == other)
            return size;
        for (u32 i = 0; i < size; i++)
            other[i] = data[i];
        return size;
    }

    constexpr u32 unchecked_copy_to(char* __restrict other) const
    {
        return strncpy(other, *this);
    }

private:
    static constexpr u32 strncpy(char* __restrict to,
        StringView from)
    {
        if (from.data == to)
            return from.size;
        for (u32 i = 0; i < from.size; i++)
            to[i] = from[i];
        return from.size;
    }
};

constexpr StringView operator""sv(c_string data, size_t size)
{
    return StringView(data, size);
}

inline std::ostream& operator<<(std::ostream& os, StringView view)
{
    return std::operator<<(os,
        std::string_view(view.data, view.size));
}

}

using namespace Ty;