#pragma once
#include <Core/Error.h>
#include <Types.h>
#include <string.h>
#if __cplusplus
#    include <Core/ErrorOr.h>
#    include <Core/Try.h>
#endif

#ifdef __cplusplus
namespace Core {
#endif

#if __cplusplus
template <typename T = u8>
struct ErrorOrVector;
#else
typedef struct Vector Vector;
typedef struct ErrorOrVector ErrorOrVector;
#endif

#if __cplusplus
template <typename T>
#endif
struct Vector {
    u8* M(data);
    u32 M(size);
    u32 M(capacity) : 24;
    u8 M(element_size);

#if __cplusplus
    static ErrorOrVector<T> generic_create(u8 element_size) asm(
        "Vector$generic_create");

    static ErrorOr<Vector<T>> create()
    {
        return TRY(generic_create(sizeof(T)));
    }
    void destroy() const asm("Vector$destroy");

    bool is_valid() const asm("Vector$is_valid");
    void invalidate() asm("Vector$invalidate");

    constexpr u32 size() const { return m_size; }
    constexpr bool is_empty() const { return m_size == 0; }

    GenericId generic_append(void const* __restrict value) asm(
        "Vector$generic_append");

    Id<T> append(T const& value) { return generic_append(&value); }

    void generic_at(void* return_value, GenericId id) const
        asm("Vector$generic_at");

    T operator[](Id<T> id) const
    {
        T value;
        generic_at(&value, id);
        return value;
    }

    void const* generic_first() const asm("Vector$generic_first");
    void const* generic_last() const asm("Vector$generic_last");

    constexpr T const* begin() const
    {
        return (T const*)m_data;
    }

    constexpr T const* end() const
    {
        usize end_index = (usize)m_size * (usize)m_element_size;
        return (T const*)&m_data[end_index];
    }
#endif
};

#ifndef __cplusplus
ErrorOrVector Vector$generic_create(u8 element_size);
#    define Vector$create(T) Vector$generic_create(sizeof(T))

void Vector$destroy(Vector const*);
GenericId Vector$generic_append(Vector*, void const* __restrict);
#    define Vector$append(vec, T, value)    \
        ({                                  \
            T x = (T)(value);               \
            Vector$generic_append(vec, &x); \
        })

void Vector$generic_at(Vector const*, void* __restrict, GenericId);
#    define Vector$at(vec, T, id)           \
        ({                                  \
            T x;                            \
            Vector$generic_at(vec, &x, id); \
            x;                              \
        })

void const* Vector$generic_first(Vector const*);
#    define Vector$first(vec, T) ((T*)Vector$generic_first(vec))

void const* Vector$generic_last(Vector const*);
#    define Vector$last(vec, T) ((T*)Vector$generic_last(vec))
#endif

#if __cplusplus
template <typename T>
#endif
struct ErrorOrVector {
#if __cplusplus
    union {
        Vector<T> M(value);
        Error M(error);
    };
#else
    union {
        Vector M(value);
        Error M(error);
    };
#endif
    bool M(is_error);

#ifdef __cplusplus
    ErrorOrVector(Error error)
        : m_error(error)
        , m_is_error(true)
    {
    }

    template <typename U>
    ErrorOrVector(Vector<U>&& value)
        : m_value(*(Vector<T>*)(&value))
        , m_is_error(false)
    {
    }

    constexpr Error const& error() const { return m_error; }
    constexpr Error release_error() const { return m_error; }
    Vector<T> release_value() const { return m_value; }
    constexpr bool is_error() const { return m_is_error; }
#endif
};

#ifdef __cplusplus
}
#endif
