#pragma once
#include "Error.h"
#include <Types.h>
#include <string.h>
#if __cplusplus
#    include "ErrorOr.h"
#    include "Try.h"
#endif

#ifdef __cplusplus
namespace Core {
#endif

#if __cplusplus
template <typename T = u8>
struct ErrorOrGenericVector;
#else
typedef struct GenericVector GenericVector;
typedef struct ErrorOrGenericVector ErrorOrGenericVector;
#endif

#if __cplusplus
template <typename T>
#endif
struct GenericVector {
    u8* M(data);
    u32 M(size);
    u32 M(capacity) : 24;
    u8 M(element_size);

#if __cplusplus
private:
    constexpr GenericVector() = default;
    constexpr GenericVector(u8* data, u32 size, u32 capacity,
        u8 element_size)
        : m_data(data)
        , m_size(size)
        , m_capacity(capacity)
        , m_element_size(element_size)
    {
    }

public:
    static ErrorOrGenericVector<T> generic_create(
        u8 element_size) asm("GenericVector$generic_create");

    static ErrorOr<GenericVector<T>> create()
    {
        return TRY(generic_create(sizeof(T)));
    }
    void destroy() const asm("GenericVector$destroy");

    bool is_valid() const asm("GenericVector$is_valid");
    void invalidate() asm("GenericVector$invalidate");

    constexpr u32 size() const { return m_size; }
    constexpr bool is_empty() const { return m_size == 0; }

    GenericId generic_append(void const* __restrict value) asm(
        "GenericVector$generic_append");

    Id<T> append(T const& value) { return generic_append(&value); }

    void generic_at(void* return_value, GenericId id) const
        asm("GenericVector$generic_at");

    void generic_at_index(void* return_value, u32 index) const
        asm("GenericVector$generic_at_index");

    T& operator[](Id<T> id) { return ((T*)m_data)[id.raw()]; }

    T const& operator[](u32 index) const
    {
        return ((T const*)m_data)[index];
    }

    void const* generic_first() const
        asm("GenericVector$generic_first");
    void const* generic_last() const
        asm("GenericVector$generic_last");

    constexpr T const* begin() const { return (T const*)m_data; }

    constexpr T const* end() const
    {
        usize end_index = (usize)m_size * (usize)m_element_size;
        return (T const*)&m_data[end_index];
    }

    constexpr T* begin() { return (T*)m_data; }

    constexpr T* end()
    {
        usize end_index = (usize)m_size * (usize)m_element_size;
        return (T*)&m_data[end_index];
    }
#endif
};

#ifndef __cplusplus
ErrorOrVector GenericVector$generic_create(u8 element_size);
#    define GenericVector$create(T) \
        GenericVector$generic_create(sizeof(T))

void GenericVector$destroy(Vector const*);
GenericId GenericVector$generic_append(Vector*,
    void const* __restrict);
#    define GenericVector$append(vec, T, value)    \
        ({                                         \
            T x = (T)(value);                      \
            GenericVector$generic_append(vec, &x); \
        })

void GenericVector$generic_at(Vector const*, void* __restrict,
    GenericId);
#    define GenericVector$at(vec, T, id)           \
        ({                                         \
            T x;                                   \
            GenericVector$generic_at(vec, &x, id); \
            x;                                     \
        })

void GenericVector$generic_at_index(Vector const*, void* __restrict,
    u32);
#    define GenericVector$at(vec, T, index)                 \
        ({                                                  \
            T x;                                            \
            GenericVector$generic_at_index(vec, &x, index); \
            x;                                              \
        })

void const* GenericVector$generic_first(Vector const*);
#    define GenericVector$first(vec, T) \
        ((T*)Vector$generic_first(vec))

void const* GenericVector$generic_last(Vector const*);
#    define GenericVector$last(vec, T) \
        ((T*)Vector$generic_last(vec))
#endif

#if __cplusplus
template <typename T>
#endif
struct ErrorOrGenericVector {
#if __cplusplus
    union {
        GenericVector<T> M(value);
        Error M(error);
    };
#else
    union {
        GenericVector M(value);
        Error M(error);
    };
#endif
    bool M(is_error);

#ifdef __cplusplus
    ErrorOrGenericVector(Error error)
        : m_error(error)
        , m_is_error(true)
    {
    }

    template <typename U>
    ErrorOrGenericVector(GenericVector<U>&& value)
        : m_value(*(GenericVector<T>*)(&value))
        , m_is_error(false)
    {
    }

    constexpr Error const& error() const { return m_error; }
    constexpr Error release_error() const { return m_error; }
    GenericVector<T> release_value() const { return m_value; }
    constexpr bool is_error() const { return m_is_error; }
#endif
};

#ifdef __cplusplus
}
#endif
