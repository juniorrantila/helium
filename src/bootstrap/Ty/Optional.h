#pragma once
#include "Base.h"
#include "Traits.h"
#include <new>

namespace Ty {

template <typename T>
struct Optional {

    constexpr Optional() = default;

    constexpr Optional(T value) requires is_trivially_copyable<T>
        : m_has_value(true)
    {
        new (storage()) T(value);
    }

    constexpr Optional(T&& value) requires(
        !is_trivially_copyable<T>)
        : m_has_value(true)
    {
        new (storage()) T(move(value));
    }

    constexpr Optional(Optional&& other)
        : m_has_value(other.has_value())
    {
        new (storage()) T(other.release_value());
    }

    constexpr ~Optional()
    {
        if (m_has_value)
            value().~T();
        m_has_value = false;
    }

    constexpr T& value() { return *storage(); }
    constexpr T const& value() const { return *storage(); }

    constexpr T release_value()
    {
        m_has_value = false;
        return move(value());
    }

    constexpr bool has_value() const { return m_has_value; }

    constexpr T* operator->() { return storage(); }

    constexpr T const* operator->() const { return storage(); }

private:
    constexpr T* storage()
    {
        return reinterpret_cast<T*>(m_storage);
    }
    constexpr T const* storage() const
    {
        return reinterpret_cast<T const*>(m_storage);
    }

    alignas(T) u8 m_storage[sizeof(T)];
    bool m_has_value { false };
};

template <typename T>
struct Optional<T*> {

    constexpr Optional() = default;

    constexpr Optional(T* value)
        : m_value(value)
    {
    }

    constexpr ~Optional() { m_value = nullptr; }

    constexpr T*& value() { return m_value; }
    constexpr T const* const& value() const { return m_value; }

    constexpr T* release_value()
    {
        auto value = m_value;
        m_value = nullptr;
        return value;
    }

    constexpr bool has_value() const { return m_value != nullptr; }

    constexpr T* operator->() { return value(); }

    constexpr T const* operator->() const { return value(); }

private:
    T* m_value { nullptr };
};

}

using namespace Ty;
