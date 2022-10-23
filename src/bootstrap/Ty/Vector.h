#pragma once
#include "Base.h"
#include "ErrorOr.h"
#include "Id.h"
#include "Try.h"
#include "View.h"
#include <type_traits>

namespace Ty {

template <typename T>
struct Vector {
    static constexpr ErrorOr<Vector<T>> create(
        u32 starting_capacity = 32)
    {
        auto* data
            = (T*)__builtin_malloc(sizeof(T) * starting_capacity);
        if (!data)
            return Error::from_errno();
        return Vector { data, starting_capacity };
    }

    constexpr Vector(Vector&& other)
        : m_data(other.m_data)
        , m_size(other.m_size)
        , m_capacity(other.m_capacity)
    {
        other.invalidate();
    }

    constexpr ~Vector()
    {
        if (is_valid()) {
            destroy_elements();
            __builtin_free(m_data);
            invalidate();
        }
    }

    constexpr Id<T> unchecked_append(T&& value) requires(
        !std::is_trivially_copyable_v<T>)
    {
        new (current_slot()) T(std::move(value));
        return Id<T>(m_size++);
    }

    constexpr Id<T> unchecked_append(T value) requires(
        std::is_trivially_copyable_v<T>)
    {
        new (current_slot()) T(value);
        return Id<T>(m_size++);
    }

    constexpr ErrorOr<Id<T>> append(T&& value) requires(
        !std::is_trivially_copyable_v<T>)
    {
        TRY(expand_if_needed());
        return unchecked_append(std::move(value));
    }

    constexpr ErrorOr<Id<T>> append(T value) requires(
        std::is_trivially_copyable_v<T>)
    {
        TRY(expand_if_needed());
        return unchecked_append(value);
    }

    constexpr ErrorOr<void> ensure_capacity(u32 capacity)
    {
        if (m_capacity < capacity)
            TRY(expand(capacity));
        return {};
    }

    constexpr ErrorOr<void> reserve(u32 elements)
    {
        if (elements > 0)
            TRY(expand(m_capacity + elements));
        return {};
    }

    constexpr View<T> view() { return { m_data, m_size }; }
    constexpr View<T const> view() const
    {
        return { m_data, m_size };
    }

    constexpr T const& at(u32 index) const { return m_data[index]; }

    constexpr T const& at(Id<T> id) const { return at(id.raw()); }

    constexpr T const& operator[](u32 index) const
    {
        return at(index);
    }

    constexpr T const& operator[](Id<T> id) const { return at(id); }

    constexpr T& operator[](u32 index) { return m_data[index]; }

    constexpr T& operator[](Id<T> id) { return m_data[id.raw()]; }

    constexpr T* begin() { return m_data; }
    constexpr T* end() { return &m_data[m_size]; }

    constexpr T const* begin() const { return m_data; }
    constexpr T const* end() const { return &m_data[m_size]; }

    constexpr T* data() { return m_data; }
    constexpr T const* data() const { return m_data; }
    constexpr u32 size() const { return m_size; }

    constexpr bool is_empty() const { return m_size == 0; }

    constexpr bool is_valid() const { return m_data != nullptr; }

private:
    constexpr ErrorOr<void> expand(u32 capacity)
    {
        auto* data
            = (T*)__builtin_realloc(m_data, capacity * sizeof(T));
        if (!data)
            return Error::from_errno();
        m_capacity = capacity;
        m_data = data;

        return {};
    }

    constexpr void destroy_elements() const
        requires(!std::is_trivially_destructible_v<T>)
    {
        for (u32 i = 0; i < m_size; i++)
            data()[i].~T();
    }

    constexpr void destroy_elements() const
        requires(std::is_trivially_destructible_v<T>)
    {
    }

    constexpr void invalidate() { m_data = nullptr; }

    constexpr T* current_slot() { return &m_data[m_size]; }

    constexpr ErrorOr<void> expand_if_needed()
    {
        if (m_size >= m_capacity)
            TRY(expand());
        return {};
    }

    constexpr ErrorOr<void> expand()
    {
        TRY(expand(m_capacity * 1.5));
        return {};
    }

    constexpr Vector(T* data, u32 capacity)
        : m_data(data)
        , m_size(0)
        , m_capacity(capacity)
    {
    }

    T* m_data { nullptr };
    u32 m_size { 0 };
    u32 m_capacity { 0 };
};

}

using namespace Ty;
