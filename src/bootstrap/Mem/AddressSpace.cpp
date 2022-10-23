#include "AddressSpace.h"
#include <Core/System.h>
#include <Ty/Defer.h>
#include <Ty/ErrorOr.h>
#include <sys/mman.h>

namespace Mem::Internal {

ErrorOr<void> init(uintptr_t base, uintptr_t size)
{
    auto page_size = TRY(Core::System::page_size());
    TRY(Core::System::mmap((void*)base, size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT
            | MAP_FIXED_NOREPLACE));
    bool should_unmap_region = true;
    Defer unmap_region = [&] {
        if (should_unmap_region) {
            (void)Core::System::munmap((void*)base, size);
        }
    };
    TRY(Core::System::mprotect((void*)(base + size - page_size),
        page_size, PROT_NONE));
    should_unmap_region = false;
    return {};
}

ErrorOr<void> deinit(uintptr_t base, uintptr_t size)
{
    TRY(Core::System::munmap((void*)base, size));
    return {};
}

}
