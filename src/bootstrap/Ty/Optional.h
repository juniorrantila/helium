#pragma once
#include "Base.h"
#include "Forward.h"
#include "Move.h"
#include "New.h"
#include "Traits.h"

namespace Ty {

template <typename T>
struct [[nodiscard]] Optional {

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

    constexpr ~Optional() { clear_if_needed(); }

    constexpr Optional& operator=(
        T value) requires is_trivially_copyable<T>
    {
        clear_if_needed();
        new (storage()) T(value);
        m_has_value = true;
        return *this;
    }

    constexpr Optional& operator=(T&& value) requires(
        !is_trivially_copyable<T>)
    {
        clear_if_needed();
        new (storage()) T(move(value));
        m_has_value = true;
        return *this;
    }

    constexpr Optional& operator=(
        Optional value) requires is_trivially_copyable<T>
    {
        clear_if_needed();
        new (this) Optional(value);
        return *this;
    }

    constexpr Optional& operator=(Optional&& value) requires(
        !is_trivially_copyable<T>)
    {
        clear_if_needed();
        new (this) Optional(move(value));
        m_has_value = true;
        return *this;
    }

    template <typename E>
    constexpr ErrorOr<T, E> or_throw(
        E error) requires is_trivially_copyable<E>
    {
        if (m_has_value)
            return release_value();
        return error;
    }

    template <typename E>
    constexpr ErrorOr<T, E> or_throw(E&& error) requires(
        !is_trivially_copyable<E>)
    {
        if (m_has_value)
            return release_value();
        return error;
    }

    template <typename F>
    constexpr decltype(auto) or_else(F callback) requires
        requires(F callback)
    {
        callback();
    }
    {
        if (!has_value())
            return callback();
        using Return = decltype(callback());
        return Return(release_value());
    }

    template <typename U>
    constexpr decltype(auto) or_else(U value) requires(
        !requires(U value) { value(); })
    {
        if (!has_value())
            return value;
        return U(release_value());
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

    constexpr T operator*() { return release_value(); }

    explicit constexpr operator bool() { return has_value(); }

private:
    constexpr void clear_if_needed()
    {
        if (m_has_value)
            clear();
    }

    constexpr void clear()
    {
        value().~T();
        m_has_value = false;
    }

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
requires(HasInvalid<T>) struct [[nodiscard]] Optional<T> {
    constexpr Optional() { new (storage()) T(T::Invalid); }

    constexpr Optional(T value) requires is_trivially_copyable<T>
    {
        new (storage()) T(value);
    }

    constexpr Optional(T&& value) requires(
        !is_trivially_copyable<T>)
    {
        new (storage()) T(move(value));
    }

    constexpr Optional(Optional&& other)
    {
        new (storage()) T(other.release_value());
    }

    constexpr ~Optional() { clear(); }

    constexpr Optional& operator=(
        T value) requires is_trivially_copyable<T>
    {
        clear();
        new (storage()) T(value);
        return *this;
    }

    constexpr Optional& operator=(T&& value) requires(
        !is_trivially_copyable<T>)
    {
        clear();
        new (storage()) T(move(value));
        return *this;
    }

    constexpr Optional& operator=(
        Optional value) requires is_trivially_copyable<T>
    {
        clear();
        new (this) Optional(value);
        return *this;
    }

    constexpr Optional& operator=(Optional&& value) requires(
        !is_trivially_copyable<T>)
    {
        clear();
        new (this) Optional(move(value));
        return *this;
    }

    template <typename E>
    constexpr ErrorOr<T, E> or_throw(
        E error) requires is_trivially_copyable<E>
    {
        if (has_value())
            return release_value();
        return error;
    }

    template <typename E>
    constexpr ErrorOr<T, E> or_throw(E&& error) requires(
        !is_trivially_copyable<E>)
    {
        if (has_value())
            return release_value();
        return error;
    }

    template <typename F>
    constexpr decltype(auto) or_else(F callback) requires
        requires(F callback)
    {
        callback();
    }
    {
        if (!has_value())
            return callback();
        using Return = decltype(callback());
        return Return(release_value());
    }

    template <typename U>
    constexpr decltype(auto) or_else(U value) requires(
        !requires(U value) { value(); })
    {
        if (!has_value())
            return value;
        return U(release_value());
    }

    constexpr T& value() { return *storage(); }
    constexpr T const& value() const { return *storage(); }

    constexpr T release_value() { return move(value()); }

    constexpr bool has_value() const
    {
        return value() != T::Invalid;
    }

    constexpr T* operator->() { return storage(); }
    constexpr T const* operator->() const { return storage(); }

    constexpr T operator*() { return release_value(); }

    explicit constexpr operator bool() { return has_value(); }

private:
    constexpr void clear() { value().~T(); }

    constexpr T* storage()
    {
        return reinterpret_cast<T*>(m_storage);
    }
    constexpr T const* storage() const
    {
        return reinterpret_cast<T const*>(m_storage);
    }

    alignas(T) u8 m_storage[sizeof(T)];
};

template <typename T>
struct [[nodiscard]] Optional<T*> {

    constexpr Optional() = default;

    constexpr Optional(T* value)
        : m_value(value)
    {
    }

    constexpr ~Optional() { m_value = nullptr; }

    constexpr Optional& operator=(Optional other)
    {
        m_value = other.m_value;
        return *this;
    }

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

    constexpr T operator*() { return release_value(); }

    explicit constexpr operator bool() { return has_value(); }

    template <typename E>
    constexpr ErrorOr<T, E> or_throw(
        E error) requires is_trivially_copyable<E>
    {
        if (has_value())
            return release_value();
        return error;
    }

    template <typename E>
    constexpr ErrorOr<T, E> or_throw(E&& error) requires(
        !is_trivially_copyable<E>)
    {
        if (has_value())
            return release_value();
        return error;
    }

    template <typename F>
    constexpr decltype(auto) or_else(F callback)
    {
        if (!has_value())
            return callback();
        using Return = decltype(callback());
        return Return(release_value());
    }

private:
    T* m_value { nullptr };
};

}

using namespace Ty;
