#pragma once
#include "ErrorOr.h"
#include <Ty/StringView.h>

namespace Core {

struct MappedFile {
    char const* m_data;
    u32 m_size;
    int m_fd;

    MappedFile(MappedFile&& other)
        : m_data(other.m_data)
        , m_size(other.m_size)
        , m_fd(other.m_fd)
    {
        other.invalidate();
    }

    static ErrorOr<MappedFile> open(StringView path);
    static ErrorOr<MappedFile> open(c_string path);
    ~MappedFile();

    StringView view() const
    {
        return StringView(m_data, m_size);
    }


    bool is_valid() const { return m_data != nullptr; }
    void invalidate() { m_data = nullptr; }

private:
    constexpr MappedFile(c_string data, u32 size, int fd)
        : m_data(data)
        , m_size(size)
        , m_fd(fd)
    {
    }

    constexpr MappedFile() = default;

};

}
