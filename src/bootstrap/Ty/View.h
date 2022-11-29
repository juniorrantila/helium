#pragma once
#include "Base.h"
#include "Traits.h"

namespace Ty {

template <typename T>
struct View {
    constexpr View(T* data, usize size)
        : m_data(data)
        , m_size(size)
    {
    }

    T& operator[](usize index) { return m_data[index]; }

    T const& operator[](usize index) const { return m_data[index]; }

    constexpr T* begin() { return m_data; }
    constexpr T* end() { return &m_data[m_size]; }

    constexpr T const* begin() const { return m_data; }
    constexpr T const* end() const { return &m_data[m_size]; }

    constexpr usize size() const { return m_size; }
    constexpr T* data() { return m_data; }
    constexpr T const* data() const { return m_data; }

private:
    T* m_data;
    usize m_size;
};

template <typename T>
requires is_const<T>
struct View<T> {
    constexpr View(View<RemoveConst<T>> other)
        : m_data(other.data())
        , m_size(other.size())
    {
    }

    constexpr View(T* data, usize size)
        : m_data(data)
        , m_size(size)
    {
    }

    T const& operator[](usize index) const { return m_data[index]; }

    constexpr T const* begin() const { return m_data; }
    constexpr T const* end() const { return &m_data[m_size]; }

    constexpr usize size() const { return m_size; }
    constexpr T const* data() const { return m_data; }

private:
    T* m_data;
    usize m_size;
};

}

using namespace Ty;
