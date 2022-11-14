#include "System.h"
#include "Syscall.h"
#include <stdio.h> // remove()
#include <stdlib.h>

namespace Core::System {

ErrorOr<void> fsync(int fd)
{
    auto rv = syscall(Syscall::fsync, fd);
    if (rv < 0)
        return Error::from_syscall(rv);
    return {};
}

ErrorOr<Stat> fstat(int fd)
{
    struct stat st { };
    if (::fstat(fd, &st) < 0)
        return Error::from_errno();
    return Stat(st);
}

ErrorOr<usize> write(int fd, void const* data, usize size)
{
    auto rv = syscall(Syscall::write, fd, data, size);
    if (rv < 0)
        return Error::from_syscall(rv);
    return rv;
}

ErrorOr<usize> write(int fd, StringView string)
{
    return TRY(write(fd, string.data, string.size));
}

ErrorOr<usize> write(int fd, MappedFile const& file)
{
    return TRY(write(fd, file.m_data, file.m_size));
}

ErrorOr<usize> write(int fd, StringBuffer const& string)
{
    return TRY(write(fd, string.data(), string.size()));
}

ErrorOr<usize> writev(int fd, struct iovec const* iovec, int count)
{
    auto rv = syscall(Syscall::writev, fd, iovec, count);
    if (rv < 0)
        return Error::from_syscall(rv);
    return rv;
}

ErrorOr<u8*> mmap(void* addr, usize size, int prot, int flags,
    int fd, long offset)
{
    auto rv = syscall(Syscall::mmap, addr, size, prot, flags, fd,
        offset);
    if (rv < 0)
        return Error::from_syscall(rv);
    return (u8*)rv;
}

ErrorOr<u8*> mmap(usize size, int prot, int flags, int fd,
    long offset)
{
    return TRY(mmap(nullptr, size, prot, flags, fd, offset));
}

ErrorOr<void> munmap(void const* addr, usize size)
{
    auto rv = syscall(Syscall::munmap, addr, size);
    if (rv < 0)
        return Error::from_syscall(rv);
    return {};
}

ErrorOr<void> mprotect(void* addr, usize len, int prot)
{
    auto rv = syscall(Syscall::mprotect, addr, len, prot);
    if (rv < 0)
        return Error::from_syscall(rv);
    return {};
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
    auto fd = syscall(Syscall::open, path, flags, 0);
    if (fd < 0)
        return Error::from_syscall(fd);
    return (int)fd;
}

ErrorOr<int> open(c_string path, int flags, mode_t mode)
{
    auto fd = syscall(Syscall::open, path, flags | O_CREAT, mode);
    if (fd < 0)
        return Error::from_syscall(fd);
    return (int)fd;
}

ErrorOr<void> close(int fd)
{
    TRY(fsync(fd));
    auto rv = syscall(Syscall::close, fd);
    if (rv < 0)
        return Error::from_syscall(rv);
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

#define TIOCGETD 0x5424

bool isatty(int fd)
{
    int line_discipline = 0x1234abcd;
    // This gets the line discipline of the terminal. When called on
    // something that isn't a terminal it doesn't change
    // `line_discipline` and returns -1.
    auto rv
        = syscall(Syscall::ioctl, fd, TIOCGETD, &line_discipline);
    return rv == 0;
}

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

Optional<c_string> getenv(c_string name)
{
    c_string env = ::getenv(name);
    if (!env)
        return {};
    return env;
}

}
