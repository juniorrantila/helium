#include "MappedFile.h"
#include "System.h"
#include <Ty/Defer.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Core {

ErrorOr<MappedFile> MappedFile::open(StringView path)
{
    auto path_buffer
        = TRY(StringBuffer::create(path.size + "\0"sv.size));
    TRY(path_buffer.write(path, "\0"sv));
    return TRY(open(path_buffer.view().data));
}

ErrorOr<MappedFile> MappedFile::open(c_string path)
{
    auto fd = TRY(Core::System::open(path, O_RDONLY));
    auto should_close_file = true;
    Defer close_file = [&] {
        if (should_close_file)
            Core::System::close(fd).ignore();
    };
    auto file_stat = TRY(Core::System::fstat(fd));
    if (!file_stat.is_regular())
        return Error::from_string_literal(
            "file is not a regular file");
    u32 size = file_stat.size();
    auto* data = TRY(Core::System::mmap(size,
        PROT_READ | PROT_WRITE, MAP_PRIVATE, fd));
    should_close_file = false;
    return MappedFile((c_string)data, size, fd);
}

MappedFile::~MappedFile()
{
    if (is_valid()) {
        munmap((void*)m_data, m_size);
        Core::System::close(m_fd).ignore();
        invalidate();
    }
}

}
