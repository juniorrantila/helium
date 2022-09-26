#include <Core/GenericVector.h>
#include <Core/Try.h>
#include <Types.h>
#include <stdlib.h>
#include <string.h>

namespace Core {

struct All { };

template <>
ErrorOrGenericVector<All> GenericVector<All>::generic_create(
    u8 element_size)
{
    u32 capacity = 1024;
    usize alloc_size = (usize)capacity * (usize)element_size;
    auto* data = (u8*)malloc(alloc_size);
    if (!data)
        return Error::from_errno();
    return GenericVector<All> {
        data,
        0,
        capacity,
        element_size,
    };
}

template <>
bool GenericVector<All>::is_valid() const
{
    return m_data != nullptr;
}

template <>
void GenericVector<All>::invalidate()
{
    m_data = nullptr;
}

template <>
void GenericVector<All>::destroy() const
{
    if (is_valid()) {
        free(m_data);
        const_cast<GenericVector*>(this)->invalidate();
    }
}

namespace {

u32 slot_index(GenericVector<All> vec, u32 index)
{
    return index * vec.m_element_size;
}

u8* current_slot(GenericVector<All> vec)
{
    return &vec.m_data[slot_index(vec, vec.m_size)];
}

u8* slot(GenericVector<All> vec, u32 index)
{
    return &vec.m_data[slot_index(vec, index)];
}

}

template <>
GenericId GenericVector<All>::generic_append(void const* __restrict value)
{
    if (m_size >= m_capacity) {
        (void)fprintf(stderr, "%d %d\n", m_size, m_capacity);
        usize capacity = (usize)m_capacity * 2;
        auto alloc_size = m_element_size * capacity;
        auto* data = (u8*)realloc(m_data, alloc_size);
        if (!data) {
            // FIXME: Propagate errors.
            __builtin_abort();
        }
        m_capacity = capacity;
        m_data = data;
    }

    memcpy(current_slot(*this), value, m_element_size);
    return GenericId { m_size++, m_element_size };
}

template <>
void GenericVector<All>::generic_at(void* __restrict return_value,
    GenericId id) const
{
    // FIXME: Verify id is valid
    memcpy(return_value, slot(*this, id.raw()), m_element_size);
}

template <>
void GenericVector<All>::generic_at_index(void* __restrict return_value,
    u32 index) const
{
    // FIXME: Verify id is valid
    memcpy(return_value, slot(*this, index), m_element_size);
}

template <>
void const* GenericVector<All>::generic_first() const
{
    return m_data;
}

template <>
void const* GenericVector<All>::generic_last() const
{
    usize end_index = (usize)m_size * (usize)m_element_size;
    return &m_data[end_index];
}

}
