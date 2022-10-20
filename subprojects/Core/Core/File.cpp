#include <Core/Defer.h>
#include <Core/File.h>
#include <Core/System.h>
#include <Core/Try.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

namespace Core {

void File::close() const { ::close(m_fd); }

ErrorOr<File> File::open_for_writing(StringView path, mode_t mode)
{
    auto* path_str = strndup(path.data, path.size);
    Defer free_path_str = [=] {
        free(path_str);
    };
    if (!path_str)
        return Error::from_errno();
    return open_for_writing(path_str, mode);
}

ErrorOr<File> File::open_for_writing(c_string path, mode_t mode)
{
    auto fd = TRY(Core::System::open(path, O_WRONLY, mode));
    return File(fd, true);
}

ErrorOr<size_t> File::writev_max_count()
{
    static ssize_t max_count = 0;
    if (static bool first = true; first) [[unlikely]] {
        max_count = TRY(Core::System::sysconf(_SC_IOV_MAX));
        first = false;
    }
    return max_count;
}

namespace {
constexpr auto min(auto a, auto b) { return a < b ? a : b; }
}

ErrorOr<size_t> File::nonatomic_writev(struct iovec const* iovec,
    size_t count) const
{
    size_t total = 0;
    auto batch_size = TRY(writev_max_count());
    for (size_t i = 0; i < count; i += batch_size) {
        auto size = (int)min(count - i, batch_size);
        auto bytes_written
            = TRY(Core::System::writev(m_fd, &iovec[i], size));
        if (__builtin_add_overflow(total, bytes_written, &total))
            return Error::from_string_literal("overflow");
    }
    return total;
}

}
