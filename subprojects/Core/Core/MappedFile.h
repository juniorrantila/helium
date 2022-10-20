#pragma once
#include <Types.h>
#include <Core/Error.h>

#ifdef __cplusplus
namespace Core {
#endif

struct MappedFileOrError;
typedef struct MappedFile {
    char const* M(data);
    u32 M(size);
    int M(fd);

#if __cplusplus
    MappedFile(MappedFile&& other) asm("MappedFile$move");

    static MappedFileOrError open(StringView path) asm(
        "MappedFile$open");
    static MappedFileOrError open(c_string path);
    ~MappedFile() asm("MappedFile$destroy");

    StringView view() const asm("MappedFile$view");

    bool is_valid() const asm("MappedFile$is_valid");
    void invalidate() asm("MappedFile$invalidate");

    constexpr MappedFile(c_string data, u32 size, int fd)
        : m_data(data)
        , m_size(size)
        , m_fd(fd)
    {
    }

    constexpr MappedFile() = default;

#endif
} MappedFile;

typedef struct MappedFileOrError {
    union {
        Error M(error);
        MappedFile M(value);
    };
    bool M(is_error);

#ifdef __cplusplus
    MappedFileOrError(Error error)
        : m_error(error)
        , m_is_error(true)
    {
    }

    MappedFileOrError(MappedFile&& value)
        : m_value(std::move(value))
        , m_is_error(false)
    {
    }

    ~MappedFileOrError()
    {
        if (m_is_error) {
            m_value.~MappedFile();
        }
    }

    constexpr Error const& error() const { return m_error; }
    constexpr Error release_error() const { return m_error; }
    MappedFile release_value() { return std::move(m_value); }
    constexpr bool is_error() const { return m_is_error; }
#endif
} MappedFileOrError;

#ifndef __cplusplus
MappedFileOrError MappedFile$open(StringView path) asm("MappedFile$open");
void MappedFile$move(MappedFile* dest, MappedFile* src) asm("MappedFile$move");
void MappedFile$destroy(MappedFile const*) asm("MappedFile$destroy");
StringView MappedFile$view(MappedFile const*) asm("MappedFile$view");
bool MappedFile$is_valid(MappedFile const*) asm("MappedFile$is_valid");
void MappedFile$invalidate(MappedFile*) asm("MappedFile$invalidate");
#endif

#if __cplusplus
}
#endif
