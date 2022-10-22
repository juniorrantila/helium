#include "DenseMap.h"
#include "Try.h"
#include <Types.h>
#include <stdlib.h>
#include <string.h>

namespace Core {

struct All { };
using AllDenseMaps = DenseMap<All, All>;

template <>
ErrorOrDenseMap<All, All> AllDenseMaps::generic_create(u8 key_size,
    u8 value_size, GenericEquality equality)
{
    u32 capacity = 1024;

    usize key_alloc_size = (usize)capacity * (usize)key_size;
    auto* keys = (u8*)malloc(key_alloc_size);
    if (!keys)
        return Error::from_string_literal(strerror(errno));

    usize value_alloc_size = (usize)capacity * (usize)value_size;
    auto* values = (u8*)malloc(value_alloc_size);
    if (!values)
        return Error::from_string_literal(strerror(errno));

    return DenseMap<All, All> { .M(equality) = equality,
        .M(keys) = keys,
        .M(values) = values,
        .M(size) = 0,
        .M(key_size) = key_size,
        .M(capacity) = capacity,
        .M(value_size) = value_size };
}

template <>
bool AllDenseMaps::is_valid() const
{
    return M(keys) && M(values);
}

template <>
void AllDenseMaps::invalidate()
{
    M(keys) = nullptr;
    M(values) = nullptr;
}

template <>
void AllDenseMaps::destroy() const
{
    if (is_valid()) {
        free(M(keys));
        free(M(values));
        const_cast<DenseMap*>(this)->invalidate();
    }
}

namespace {

u32 key_slot_index(AllDenseMaps map, u32 index)
{
    return index * map.M(key_size);
}

u8* current_key_slot(AllDenseMaps map)
{
    return &map.M(keys)[key_slot_index(map, map.M(size))];
}

u8* key_slot(AllDenseMaps map, u32 index)
{
    return &map.M(keys)[key_slot_index(map, index)];
}

u32 value_slot_index(AllDenseMaps map, u32 index)
{
    return index * map.M(value_size);
}

u8* current_value_slot(AllDenseMaps map)
{
    return &map.M(values)[value_slot_index(map, map.M(size))];
}

u8* value_slot(AllDenseMaps map, u32 index)
{
    return &map.M(values)[value_slot_index(map, index)];
}

}

template <>
GenericId AllDenseMaps::generic_append(void const* __restrict key,
    void const* __restrict value)
{
    if (m_size + 1 >= m_capacity) {
        usize capacity = (usize)m_capacity * 2;

        auto key_alloc_size = M(key_size) * capacity;
        auto* keys = (u8*)realloc(M(keys), key_alloc_size);
        if (!keys) {
            // FIXME: Propagate errors.
            __builtin_abort();
        }

        auto value_alloc_size = M(value_size) * capacity;
        auto* values = (u8*)realloc(M(values), value_alloc_size);
        if (!values) {
            // FIXME: Propagate errors.
            __builtin_abort();
        }

        M(keys) = keys;
        M(values) = values;
    }

    memcpy(current_key_slot(*this), key, M(key_size));
    memcpy(current_value_slot(*this), value, M(value_size));

    return GenericId {
        M(size)++,
        M(value_size),
    };
}

template <>
GenericId AllDenseMaps::generic_find(void const* __restrict key)
{
    for (u32 i = 0; i < M(size); i++) {
        auto const* try_key = key_slot(*this, i);
        if (M(equality)(key, try_key))
            return GenericId { i, M(value_size) };
    }
    return GenericId::invalid();
}

template <>
void AllDenseMaps::generic_at(void* __restrict return_value,
    GenericId id) const
{
    // FIXME: Verify id is valid
    memcpy(return_value, value_slot(*this, id.raw()),
        M(value_size));
}

}
