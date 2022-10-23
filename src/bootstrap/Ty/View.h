#pragma once
#include "Base.h"
#include <type_traits>

namespace Ty {

template <typename T>
struct View {
    constexpr View(T* data, size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    T& operator[](size_t index) { return m_data[index]; }

    T const& operator[](size_t index) const
    {
        return m_data[index];
    }

    constexpr T* begin() { return m_data; }
    constexpr T* end() { return &m_data[m_size]; }

    constexpr T const* begin() const { return m_data; }
    constexpr T const* end() const { return &m_data[m_size]; }

    constexpr size_t size() const { return m_size; }
    constexpr T* data() { return m_data; }
    constexpr T const* data() const { return m_data; }

private:
    T* m_data;
    size_t m_size;
};

template <typename T>
requires std::is_const_v<T>
struct View<T> {
    constexpr View(T const* data, size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    T const& operator[](size_t index) const
    {
        return m_data[index];
    }

    constexpr T const* begin() const { return m_data; }
    constexpr T const* end() const { return &m_data[m_size]; }

    constexpr size_t size() const { return m_size; }
    constexpr T const* data() const { return m_data; }

private:
    T* m_data;
    size_t m_size;
};

}

using namespace Ty;
