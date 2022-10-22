#pragma once
#include "Error.h"
#include <optional>
#include <type_traits>

namespace Core {

template <typename T, typename E = Error>
struct [[nodiscard]] ErrorOr {
    constexpr ErrorOr(ErrorOr&& other)
        : m_state(other.m_state)
    {
        switch(m_state) {
            case State::Error: m_error = other.release_error(); break;
            case State::Value: m_value = other.release_value(); break;
            case State::Moved: break;
        }
        other.m_state = State::Moved;
    }

    constexpr ErrorOr(T&& value) requires(
        !std::is_trivially_copyable_v<T>)
        : m_value(std::move(value))
        , m_state(State::Value)
    {
    }

    constexpr ErrorOr(T value) requires(
        std::is_trivially_copyable_v<T>)
        : m_value(value)
        , m_state(State::Value)
    {
    }

    constexpr ErrorOr(E&& error) requires(
        !std::is_trivially_copyable_v<E>)
        : m_error(std::move(error))
        , m_state(State::Error)
    {
    }

    constexpr ErrorOr(E error) requires(
        std::is_trivially_copyable_v<E>)
        : m_error(error)
        , m_state(State::Error)
    {
    }

    template <typename EE>
    constexpr ErrorOr(EE error) requires(
        !std::is_same_v<E, Error> && std::is_constructible_v<E, EE>)
        : m_error(E(error))
        , m_state(State::Error)
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

    constexpr bool is_error() const { return m_state == State::Error; }

    T release_value()
    {
        m_state = State::Moved;
        return std::move(m_value);
    }
    E release_error()
    { 
        m_state = State::Moved;
        return std::move(m_error);
    }

    T const& value() const { return m_value; }
    E const& error() const { return m_error; }

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

    constexpr ErrorOr(E error)
        : m_error(error)
    {
    }

    template <typename EE>
    constexpr ErrorOr(EE error) requires(
        !std::is_same_v<E, Error> && std::is_constructible_v<E, EE>)
        : m_error(EE(error))
    {
    }

    constexpr bool is_error() const { return m_error.has_value(); }

    void release_value() const { }
    E release_error() { return std::move(m_error.value()); }

    E const& error() const { return m_error.value(); }

private:
    std::optional<E> m_error {};
};

}
