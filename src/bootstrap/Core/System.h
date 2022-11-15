#pragma once
#include "MappedFile.h"
#include <Ty/ErrorOr.h>
#include <Ty/IOVec.h>
#include <Ty/StringBuffer.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>

extern "C" {
extern char** environ;
}

#ifndef STDOUT_FILENO
#    define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#    define STDERR_FILENO 2
#endif

namespace Core::System {

#define MAP_GROWSDOWN 0x00100
#define MAP_DENYWRITE 0x00800
#define MAP_EXECUTABLE 0x01000
#define MAP_LOCKED 0x02000
#define MAP_NORESERVE 0x04000
#define MAP_POPULATE 0x08000
#define MAP_NONBLOCK 0x10000
#define MAP_STACK 0x20000
#define MAP_HUGETLB 0x40000
#define MAP_SYNC 0x80000
#define MAP_FIXED_NOREPLACE 0x100000

#ifdef __linux__
#    define PROT_READ 0x1
#    define PROT_WRITE 0x2
#    define PROT_EXEC 0x4
#    define PROT_NONE 0x0
#    define PROT_GROWSDOWN 0x01000000
#    define PROT_GROWSUP 0x02000000

#    define MAP_SHARED 0x01
#    define MAP_PRIVATE 0x02
#    define MAP_SHARED_VALIDATE 0x03
#    define MAP_TYPE 0x0f

#    define MAP_FIXED 0x10
#    define MAP_FILE 0
#    define MAP_ANONYMOUS 0x20
#    define MAP_ANON MAP_ANONYMOUS
#    define MAP_HUGE_SHIFT 26
#    define MAP_HUGE_MASK 0x3f
#    define MAP_32BIT 0x40

#    define MS_ASYNC 1
#    define MS_SYNC 4
#    define MS_INVALIDATE 2

#    define MADV_NORMAL 0
#    define MADV_RANDOM 1
#    define MADV_SEQUENTIAL 2
#    define MADV_WILLNEED 3
#    define MADV_DONTNEED 4
#    define MADV_FREE 8
#    define MADV_REMOVE 9
#    define MADV_DONTFORK 10
#    define MADV_DOFORK 11
#    define MADV_MERGEABLE 12
#    define MADV_UNMERGEABLE 13
#    define MADV_HUGEPAGE 14
#    define MADV_NOHUGEPAGE 15
#    define MADV_DONTDUMP 16
#    define MADV_DODUMP 17
#    define MADV_WIPEONFORK 18
#    define MADV_KEEPONFORK 19
#    define MADV_COLD 20
#    define MADV_PAGEOUT 21
#    define MADV_POPULATE_READ 22
#    define MADV_POPULATE_WRITE 23
#    define MADV_HWPOISON 100

#    define MCL_CURRENT 1
#    define MCL_FUTURE 2
#    define MCL_ONFAULT 4

#    define O_ACCMODE 0003
#    define O_RDONLY 00
#    define O_WRONLY 01
#    define O_RDWR 02
#    define O_CREAT 0100
#    define O_EXCL 0200
#    define O_NOCTTY 0400
#    define O_TRUNC 01000
#    define O_APPEND 02000
#    define O_NONBLOCK 04000
#    define O_SYNC 04010000
#    define O_ASYNC 020000
#else
#    warning "flags unimplemented"
#endif

ErrorOr<usize> write(int fd, MappedFile const& file);
ErrorOr<usize> write(int fd, StringBuffer const& string);
ErrorOr<usize> write(int fd, StringView string);
ErrorOr<usize> write(int fd, void const* data, usize size);
ErrorOr<usize> writev(int fd, IOVec const* iovec, int count);
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

    constexpr bool is_executable() const
    {
        auto flag = S_IXGRP | S_IXOTH | S_IXUSR;
        return (raw.st_mode & flag) > 0;
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

Optional<c_string> getenv(StringView name);

ErrorOr<bool> has_program(StringView name);

[[noreturn]] void exit(int code);

}
