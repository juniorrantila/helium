#pragma once
#include "Error.h"
#include <Types.h>
#include <string.h>
#ifdef __cplusplus
#    include "ErrorOr.h"
#    include "Try.h"
#endif

#ifdef __cplusplus
namespace Core {
#endif

#ifdef __cplusplus
template <typename Key, typename Value>
struct ErrorOrDenseMap;
#else
typedef struct DenseMap DenseMap;
typedef struct ErrorOrDenseMap ErrorOrDenseMap;
#endif

typedef bool (*GenericEquality)(void const* __restrict,
    void const* __restrict);
#ifdef __cplusplus
template <typename Key, typename Value>
#endif
struct DenseMap {
    GenericEquality M(equality);
    u8* M(keys);
    u8* M(values);
    u32 M(size) : 24;
    u8 M(key_size);
    u32 M(capacity) : 24;
    u8 M(value_size);

#ifdef __cplusplus
    static ErrorOrDenseMap<Key, Value> generic_create(u8 key_size,
        u8 value_size,
        GenericEquality equality) asm("DenseMap$generic_create");

    static ErrorOr<DenseMap<Key, Value>> create(
        bool (*equality)(Key const&, Key const&)
        = [](auto const& a, auto const& b) {
              return __builtin_memcmp(&a, &b, sizeof(Key)) == 0;
          })
    {
        auto generic_equality = (GenericEquality)equality;
        return TRY(generic_create(sizeof(Key), sizeof(Value),
            generic_equality));
    }
    void destroy() const asm("DenseMap$destroy");

    bool is_valid() const asm("DenseMap$is_valid");
    void invalidate() asm("DenseMap$invalidate");

    constexpr u32 size() const { return m_size; }
    constexpr bool is_empty() const { return m_size == 0; }

    GenericId generic_append(void const* __restrict key,
        void const* __restrict value) asm("DenseMap$generic_"
                                          "append");

    Id<Value> append(Key const& key, Value const& value)
    {
        return generic_append(&key, &value);
    }

    GenericId generic_find(void const* __restrict key) asm(
        "DenseMap$generic_find");
    Id<Value> find(Key const& key) { return generic_find(&key); }

    void generic_at(void* return_value, GenericId id) const
        asm("DenseMap$generic_at");

    Value operator[](Id<Value> id) const
    {
        Value value;
        generic_at(&value, id);
        return value;
    }
#endif
};

#ifndef __cplusplus
ErrorOrDenseMap DenseMap$generic_create(u8 key_size, u8 value_size,
    GenericEquality);
#    define DenseMap$create(Key, Value, equality)               \
        ({                                                      \
            let generic_equality = (GenericEquality)equality;   \
            DenseMap$generic_create(sizeof(Key), sizeof(Value), \
                generic_equality);                              \
        })

void DenseMap$destroy(DenseMap const*);
GenericId DenseMap$generic_append(DenseMap*,
    void const* __restrict key, void const* __restrict value);
#    define DenseMap$append(map, Key, Value, key, value) \
        ({                                               \
            Key k = (Key)(key);                          \
            Value v = (Value)(value);                    \
            DenseMap$generic_append(map, &k, &v);        \
        })

GenericId DenseMap$generic_find(DenseMap const* __restrict,
    void const* __restrict);
#    define DenseMap$find(map, Key, key)    \
        ({                                  \
            Key k = (Key)key;               \
            DenseMap$generic_find(map, &k); \
        })

void DenseMap$generic_at(DenseMap const*, void* __restrict,
    GenericId);
#    define DenseMap$at(map, Value, id)     \
        ({                                  \
            Value v;                        \
            DenseMap$generic_at(map, &v, id); \
            v;                              \
        })
#endif

#if __cplusplus
template <typename Key, typename Value>
#endif
struct ErrorOrDenseMap {
    union {
#if __cplusplus
        DenseMap<Key, Value> M(value);
#else
        DenseMap M(value);
#endif
        Error M(error);
    };
    bool M(is_error);

#ifdef __cplusplus
    ErrorOrDenseMap(Error error)
        : m_error(error)
        , m_is_error(true)
    {
    }

    template <typename K, typename V>
    ErrorOrDenseMap(DenseMap<K, V>&& value)
        : m_value(*(DenseMap<Key, Value>*)(&value))
        , m_is_error(false)
    {
    }

    constexpr Error const& error() const { return m_error; }
    constexpr Error release_error() const { return m_error; }
    DenseMap<Key, Value> release_value() const { return m_value; }
    constexpr bool is_error() const { return m_is_error; }
#endif
};

#ifdef __cplusplus
}
#endif
