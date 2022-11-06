#pragma once
#include "MappedFile.h"
#include <Ty/ErrorOr.h>
#include <Ty/StringBuffer.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef _GNU_SOURCE
extern char** environ;
#endif

namespace Core::System {

ErrorOr<usize> write(int fd, MappedFile const& file);
ErrorOr<usize> write(int fd, StringBuffer const& string);
ErrorOr<usize> write(int fd, StringView string);
ErrorOr<usize> write(int fd, void const* data, usize size);
ErrorOr<usize> writev(int fd, struct iovec const* iovec, int count);
ErrorOr<void> fsync(int fd);
ErrorOr<void> munmap(void const* addr, usize size);
ErrorOr<u8*> mmap(usize size, int prot, int flags, int fd = -1,
    off_t offset = 0);
ErrorOr<u8*> mmap(void* addr, usize size, int prot, int flags,
    int fd = -1, off_t offset = 0);
ErrorOr<void> mprotect(void* addr, usize len, int prot);

struct Stat {
    constexpr Stat(struct stat st)
        : raw(st)
    {
    }

    constexpr bool is_regular() const
    {
        return S_ISREG(raw.st_mode);
    }
    constexpr off_t size() const { return raw.st_size; }

    struct stat raw;
};
ErrorOr<Stat> fstat(int fd);

ErrorOr<int> mkstemps(char* template_);
ErrorOr<int> mkstemps(char* template_, int suffixlen);
ErrorOr<int> open(c_string path, int flags);
ErrorOr<int> open(c_string path, int flags, mode_t mode);
ErrorOr<void> close(int fd);
ErrorOr<void> remove(c_string path);
ErrorOr<void> unlink(c_string path);

ErrorOr<pid_t> posix_spawnp(c_string file, c_string const* argv,
    c_string const* envp = environ,
    posix_spawn_file_actions_t const* file_actions = nullptr,
    posix_spawnattr_t const* attrp = nullptr);

struct Status {
    constexpr bool did_exit() const { return WIFEXITED(raw); }
    constexpr int exit_status() const { return WEXITSTATUS(raw); }

    int raw;
};
ErrorOr<Status> waitpid(pid_t pid, int options = 0);

bool isatty(int fd);

ErrorOr<long> sysconf(int name);

ErrorOr<u32> page_size();

}
