#pragma once

namespace Ty {

template <typename T>
T&& move(T& value)
{
    return static_cast<T&&>(value);
}

};

using namespace Ty;
