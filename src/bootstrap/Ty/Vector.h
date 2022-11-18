#pragma once
#include "Base.h"
#include "ErrorOr.h"
#include "Id.h"
#include "Move.h"
#include "ReverseIterator.h"
#include "Try.h"
#include "View.h"

void* he_malloc(usize);
void* he_realloc(void*, usize);
void he_free(void*);

#ifndef ALWAYS_INLINE
#    define ALWAYS_INLINE [[gnu::always_inline]]
#endif
#ifndef FLATTEN
#    define FLATTEN [[gnu::flatten]]
#endif

namespace Ty {

template <typename T>
struct Vector {
    static constexpr ErrorOr<Vector<T>> create(
        u32 starting_capacity = inline_capacity)
    {
        if (starting_capacity <= inline_capacity)
            return Vector();
        auto* data = (T*)he_malloc(sizeof(T) * starting_capacity);
        if (!data)
            return Error::from_errno();
        return Vector { data, starting_capacity };
    }

    constexpr Vector()
        : m_capacity(inline_capacity)
    {
    }

    constexpr Vector(Vector&& other)
        : m_data(other.m_data)
        , m_size(other.m_size)
        , m_capacity(other.m_capacity)
    {
        if (!other.is_hydrated()) {
            for (u32 i = 0; i < m_size; i++) {
                new (&inline_buffer()[i])
                    T(move(other.inline_buffer()[i]));
                other.inline_buffer()[i].~T();
            }
        }
        other.invalidate();
    }

    constexpr ~Vector()
    {
        if (is_valid()) {
            destroy_elements();
            if (is_hydrated())
                he_free(m_data);
            invalidate();
        }
    }

    ALWAYS_INLINE constexpr Id<T> unchecked_append(
        T&& value) requires(!is_trivially_copyable<T>)
    {
        new (current_slot()) T(move(value));
        return Id<T>(m_size++);
    }

    ALWAYS_INLINE constexpr Id<T> unchecked_append(
        T value) requires(is_trivially_copyable<T>)
    {
        new (current_slot()) T(value);
        return Id<T>(m_size++);
    }

    ALWAYS_INLINE constexpr ErrorOr<Id<T>> append(
        T&& value) requires(!is_trivially_copyable<T>)
    {
        TRY(expand_if_needed());
        return unchecked_append(move(value));
    }

    ALWAYS_INLINE constexpr ErrorOr<Id<T>> append(T value) requires(
        is_trivially_copyable<T>)
    {
        TRY(expand_if_needed());
        return unchecked_append(value);
    }

    ALWAYS_INLINE constexpr ErrorOr<void> ensure_capacity(
        u32 capacity)
    {
        if (m_capacity < capacity)
            TRY(expand(capacity));
        return {};
    }

    ALWAYS_INLINE constexpr ErrorOr<void> reserve(u32 elements)
    {
        if (elements > 0)
            TRY(expand(m_capacity + elements));
        return {};
    }

    FLATTEN constexpr View<T> view() { return { m_data, m_size }; }
    FLATTEN constexpr View<T const> view() const
    {
        return { m_data, m_size };
    }

    FLATTEN constexpr T const& at(u32 index) const
    {
        return data()[index];
    }

    FLATTEN constexpr T const& at(Id<T> id) const
    {
        return at(id.raw());
    }

    FLATTEN constexpr T const& operator[](u32 index) const
    {
        return at(index);
    }

    FLATTEN constexpr T const& operator[](Id<T> id) const
    {
        return at(id);
    }

    FLATTEN constexpr T& operator[](u32 index)
    {
        return data()[index];
    }

    FLATTEN constexpr T& operator[](Id<T> id)
    {
        return data()[id.raw()];
    }

    FLATTEN constexpr T* begin() { return data(); }
    FLATTEN constexpr T* end() { return &data()[m_size]; }

    FLATTEN constexpr T const* begin() const { return data(); }
    FLATTEN constexpr T const* end() const
    {
        return &data()[m_size];
    }

    ReverseIterator<T const> in_reverse() const
    {
        return { begin(), end() };
    }

    ReverseIterator<T> in_reverse() { return { begin(), end() }; }

    FLATTEN constexpr T* data()
    {
        return m_data ?: inline_buffer();
    }

    FLATTEN constexpr T const* data() const
    {
        return m_data ?: inline_buffer();
    }

    constexpr T const& last() const { return data()[m_size - 1]; }
    constexpr T& last() { return data()[m_size - 1]; }

    ALWAYS_INLINE constexpr u32 size() const { return m_size; }

    constexpr bool is_empty() const { return m_size == 0; }

    constexpr bool is_valid() const { return m_size != 0xFFFFFF; }

private:
    constexpr static auto inline_capacity = 8;

    static constexpr u32 storage_size()
    {
        return sizeof(T) * inline_capacity;
    }

    static constexpr u32 storage_alignment() { return alignof(T); }

    ErrorOr<void> expand_hydrate(u32 capacity)
    {
        auto* data = (T*)he_malloc(capacity * sizeof(T));
        if (!data)
            return Error::from_errno();
        __builtin_memcpy(data, inline_buffer(), storage_size());
        m_capacity = capacity;
        m_data = data;

        return {};
    }

    ALWAYS_INLINE constexpr bool is_hydrated() const
    {
        return m_data != nullptr;
    }

    constexpr ErrorOr<void> expand(u32 capacity)
    {
        if (!is_hydrated()) {
            TRY(expand_hydrate(capacity));
            return {};
        }
        auto* data = (T*)he_realloc(m_data, capacity * sizeof(T));
        if (!data)
            return Error::from_errno();
        m_capacity = capacity;
        m_data = data;

        return {};
    }

    constexpr void destroy_elements() const
        requires(!is_trivially_destructible<T>)
    {
        for (u32 i = 0; i < m_size; i++)
            data()[i].~T();
    }

    ALWAYS_INLINE constexpr void destroy_elements() const
        requires(is_trivially_destructible<T>)
    {
    }

    ALWAYS_INLINE constexpr void invalidate() { m_size = 0xFFFFFF; }

    FLATTEN constexpr T* current_slot() { return &data()[m_size]; }

    ALWAYS_INLINE constexpr ErrorOr<void> expand_if_needed()
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
        , m_capacity(capacity)
    {
    }

    ALWAYS_INLINE T* inline_buffer()
    {
        return reinterpret_cast<T*>(m_storage);
    }

    ALWAYS_INLINE T const* inline_buffer() const
    {
        return reinterpret_cast<T const*>(m_storage);
    }

    alignas(storage_alignment()) u8 m_storage[storage_size()];
    T* m_data { nullptr };
    u32 m_size { 0 };
    u32 m_capacity { 0 };
};

}

using namespace Ty;
