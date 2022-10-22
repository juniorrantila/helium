#include "Defer.h"
#include "MappedFile.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "System.h"

namespace Core {

MappedFileOrError MappedFile::open(StringView path)
{
    auto* path_string = __builtin_strndup(path.data, path.size);
    Defer free_path_string = [=] {
        __builtin_free(path_string);
    };
    return TRY(open(path_string));
}

MappedFileOrError MappedFile::open(c_string path)
{
    auto fd = TRY(Core::System::open(path, O_RDONLY));
    auto should_close_file = true;
    Defer close_file = [=] {
        if (should_close_file)
            (void)Core::System::close(fd);
    };
    auto file_stat = TRY(Core::System::fstat(fd));
    if (!file_stat.is_regular())
        return Error::from_string_literal("file is not a regular file");
    u32 size = file_stat.size();
    auto* data = TRY(Core::System::mmap(size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd));
    should_close_file = false;
    return MappedFile((c_string)data, size, fd);
}

MappedFile::~MappedFile()
{
    if (is_valid()) {
        munmap((void*)m_data, m_size);
        (void)Core::System::close(m_fd);
        invalidate();
    }
}

bool MappedFile::MappedFile::is_valid() const
{
    return m_data != nullptr;
}

void MappedFile::invalidate()
{
    m_data = nullptr;
}

MappedFile::MappedFile(MappedFile&& other)
    : m_data(other.m_data)
    , m_size(other.m_size)
    , m_fd(other.m_fd)
{
    other.invalidate();
}

StringView MappedFile::view() const
{
    return StringView(m_data, m_size);
}

}
