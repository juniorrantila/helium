#pragma once
#include "System.h"
#include <Ty/ErrorOr.h>
#include <Ty/Forward.h>
#include <sys/uio.h>
#include <unistd.h>

namespace Core {

struct MappedFile;

struct File {
    static File& stdout()
    {
        static auto file = File::from(STDOUT_FILENO, false);
        return file;
    }

    static File& stderr()
    {
        static auto file = File::from(STDERR_FILENO, false);
        return file;
    }

    static ErrorOr<File> open_for_writing(StringView path,
        mode_t mode = 0666);
    static ErrorOr<File> open_for_writing(c_string path,
        mode_t mode = 0666);
    static File from(int fd, bool should_close)
    {
        return File(fd, should_close);
    }

    constexpr File(File&& other)
        : m_fd(other.m_fd)
    {
        other.invalidate();
    }

    constexpr File& operator=(File&& other)
    {
        m_fd = other.m_fd;
        other.invalidate();
        return *this;
    }

    ~File()
    {
        if (is_valid()) {
            if (m_should_close)
                close();
            invalidate();
        }
    }

    void close() const;

    ErrorOr<size_t> nonatomic_writev(struct iovec const*,
        size_t count) const;

    ErrorOr<size_t> write(void const* data, size_t size) const
    {
        return TRY(Core::System::write(m_fd, data, size));
    }

    ErrorOr<size_t> write(StringBuffer const& string) const
    {
        return TRY(Core::System::write(m_fd, string));
    }

    ErrorOr<size_t> write(MappedFile const& file) const
    {
        return TRY(Core::System::write(m_fd, file));
    }

    ErrorOr<size_t> write(StringView string) const
    {
        return TRY(Core::System::write(m_fd, string));
    }

    bool is_tty() const { return Core::System::isatty(m_fd); }

private:
    constexpr File() = default;

    static ErrorOr<size_t> writev_max_count();

    constexpr File(int fd, bool should_close)
        : m_fd(fd)
        , m_should_close(should_close)
    {
    }

    constexpr bool is_valid() const { return m_fd >= 0; }
    constexpr void invalidate() { m_fd = 0; }

    int m_fd { -1 };
    bool m_should_close { false };
};

}
