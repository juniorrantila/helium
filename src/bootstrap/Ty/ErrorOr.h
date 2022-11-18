#pragma once
#include "Error.h"
#include "Move.h"
#include "Optional.h"
#include "Traits.h"

namespace Ty {

template <typename T, typename E>
struct [[nodiscard]] ErrorOr {
    constexpr ErrorOr(ErrorOr&& other)
        : m_state(other.m_state)
    {
        switch (m_state) {
        case State::Error: m_error = other.release_error(); break;
        case State::Value: m_value = other.release_value(); break;
        case State::Moved: break;
        }
        other.m_state = State::Moved;
    }

    constexpr ErrorOr(T&& value) requires(!is_trivially_copyable<T>)
        : m_value(move(value))
        , m_state(State::Value)
    {
    }

    constexpr ErrorOr(T value) requires(is_trivially_copyable<T>)
        : m_value(value)
        , m_state(State::Value)
    {
    }

    constexpr ErrorOr(E&& error) requires(!is_trivially_copyable<E>)
        : m_error(move(error))
        , m_state(State::Error)
    {
    }

    constexpr ErrorOr(E error) requires(is_trivially_copyable<E>)
        : m_error(error)
        , m_state(State::Error)
    {
    }

    template <typename EE>
    constexpr ErrorOr(EE error) requires(
        !is_same<E, Error> && is_constructible<E, EE>)
        : m_error(E(error))
        , m_state(State::Error)
    {
    }

    constexpr ErrorOr()
        : m_value(T())
        , m_state(State::Value)
    {
    }

    constexpr ~ErrorOr()
    {
        switch (m_state) {
        case State::Error: m_error.~E(); break;
        case State::Value: m_value.~T(); break;
        case State::Moved: break;
        }
    }

    constexpr bool has_value() const
    {
        return m_state == State::Value;
    }
    constexpr bool is_error() const
    {
        return m_state == State::Error;
    }

    constexpr T release_value()
    {
        m_state = State::Moved;
        return move(m_value);
    }

    constexpr E release_error()
    {
        m_state = State::Moved;
        return move(m_error);
    }

    constexpr T const& value() const { return m_value; }
    constexpr E const& error() const { return m_error; }

    template <typename F>
    decltype(auto) or_else(F callback)
    {
        if (is_error())
            return callback();
        using Return = decltype(callback());
        return Return(release_value());
    }

    template <typename U>
    constexpr decltype(auto) or_else(U value) requires(
        !requires(U value) { value(); })
    {
        if (is_error())
            return value;
        return U(release_value());
    }

    template <typename U>
    ErrorOr<U, E> on_success(U success_value)
    {
        if (is_error())
            return release_error();
        return success_value;
    }

    constexpr void ignore() const { }

private:
    union {
        T m_value;
        E m_error;
    };
    enum class State : u8 {
        Value,
        Error,
        Moved,
    };
    State m_state;
};

template <typename E>
struct [[nodiscard]] ErrorOr<void, E> {
    constexpr ErrorOr() = default;

    constexpr ErrorOr(E error) requires is_trivially_copyable<E>
        : m_error(error)
    {
    }

    constexpr ErrorOr(E&& error) requires(!is_trivially_copyable<E>)
        : m_error(move(error))
    {
    }

    template <typename EE>
    constexpr ErrorOr(EE error) requires(
        !is_same<E, Error> && is_constructible<E, EE>)
        : m_error(EE(error))
    {
    }

    constexpr bool has_value() const { return !is_error(); }
    constexpr bool is_error() const { return m_error.has_value(); }

    constexpr void release_value() const { }
    constexpr E release_error() { return m_error.release_value(); }

    constexpr E const& error() const { return m_error.value(); }

    template <typename F>
    decltype(auto) or_else(F callback) const
    {
        if (is_error())
            return callback();
        using Return = decltype(callback());
        return Return();
    }

    template <typename U>
    constexpr decltype(auto) or_else(U value) const
        requires(!requires(U value) { value(); })
    {
        if (is_error())
            return value;
        return U();
    }

    template <typename U>
    ErrorOr<U, E> on_success(U success_value)
    {
        if (is_error())
            return release_error();
        return success_value;
    }

    constexpr void ignore() const { }

private:
    Optional<E> m_error {};
};

}

using namespace Ty;
