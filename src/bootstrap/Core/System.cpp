#include "System.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace Core::System {

ErrorOr<void> fsync(int fd)
{
    if (::fsync(fd) < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<Stat> fstat(int fd)
{
    struct stat st { };
    if (::fstat(fd, &st) < 0)
        return Error::from_errno();
    return Stat(st);
}

ErrorOr<size_t> write(int fd, void const* data, size_t size)
{
    auto bytes = ::write(fd, data, size);
    if (bytes < 0)
        return Error::from_errno();
    return bytes;
}

ErrorOr<size_t> write(int fd, StringView string)
{
    return TRY(write(fd, string.data, string.size));
}

ErrorOr<size_t> write(int fd, MappedFile const& file)
{
    return TRY(write(fd, file.m_data, file.m_size));
}

ErrorOr<size_t> write(int fd, StringBuffer const& string)
{
    return TRY(write(fd, string.data(), string.size()));
}

ErrorOr<size_t> writev(int fd, struct iovec const* iovec, int count)
{
    auto bytes_written = ::writev(fd, iovec, count);
    if (bytes_written < 0)
        return Error::from_errno();
    return bytes_written;
}

ErrorOr<void> munmap(void const* addr, size_t size)
{
    if (::munmap((void*)addr, size) < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<u8*> mmap(size_t size, int prot, int flags, int fd,
    off_t offset)
{
    return TRY(mmap(nullptr, size, prot, flags, fd, offset));
}

ErrorOr<void> mprotect(void* addr, size_t len, int prot)
{
    if (::mprotect(addr, len, prot) < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<u8*> mmap(void* addr, size_t size, int prot, int flags,
    int fd, off_t offset)
{
    auto* ptr = ::mmap(addr, size, prot, flags, fd, offset);
    if (ptr == MAP_FAILED)
        return Error::from_errno();
    return (u8*)ptr;
}

ErrorOr<void> remove(c_string path)
{
    if (::remove(path) < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<int> open(c_string path, int flags)
{
    if ((flags & O_CREAT) != 0) {
        return Error::from_string_literal(
            "O_CREAT should not be used with this function "
            "variant");
    }
    auto fd = ::open(path, flags);
    if (fd < 0)
        return Error::from_errno();
    return fd;
}

ErrorOr<int> open(c_string path, int flags, mode_t mode)
{
    auto fd = ::open(path, flags | O_CREAT, mode);
    if (fd < 0)
        return Error::from_errno();
    return fd;
}

ErrorOr<void> close(int fd)
{
    TRY(fsync(fd));
    if (::close(fd) < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<void> unlink(c_string path)
{
    if (::unlink(path) < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<int> mkstemps(char* template_, int suffixlen)
{
    int fd = ::mkstemps(template_, suffixlen);
    if (fd < 0)
        return Error::from_errno();
    return fd;
}

ErrorOr<int> mkstemps(char* template_)
{
    return TRY(mkstemps(template_, 0));
}

ErrorOr<pid_t> posix_spawnp(c_string file, c_string const* argv,
    c_string const* envp,
    posix_spawn_file_actions_t const* file_actions,
    posix_spawnattr_t const* attrp)
{
    pid_t pid = -1;
    auto rc = ::posix_spawnp(&pid, file, file_actions, attrp,
        (char**)argv, (char**)envp);
    if (rc != 0)
        return Error::from_errno(rc);
    if (pid < 0)
        return Error::from_errno();
    return pid;
}

ErrorOr<Status> waitpid(pid_t pid, int options)
{
    int status = 0;
    if (::waitpid(pid, &status, options) < 0)
        return Error::from_errno();
    return Status { .raw = status };
}

bool isatty(int fd) { return ::isatty(fd) == 1; }

ErrorOr<long> sysconf(int name)
{
    auto value = ::sysconf(name);
    if (value == -1)
        return Error::from_errno();
    return value;
}

ErrorOr<u32> page_size()
{
    static u32 size = 0;
    if (size == 0) [[unlikely]] {
        size = (u32)TRY(Core::System::sysconf(_SC_PAGESIZE));
    }
    return size;
}

}
