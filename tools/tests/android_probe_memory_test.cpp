#include "../android_probe/memory_probe.h"

#include <dirent.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <iostream>
#include <string>

namespace
{
int OpenFdCount()
{
    DIR* const directory = opendir("/proc/self/fd");
    assert(directory);
    int count = 0;
    while (readdir(directory))
        ++count;
    closedir(directory);
    return count;
}

bool PreferredBaseMapped()
{
    unsigned char present = 0;
    const int result = mincore(reinterpret_cast<void*>(0x100000000ull),
                               static_cast<size_t>(sysconf(_SC_PAGESIZE)), &present);
    assert(result == 0 || errno == ENOMEM);
    return result == 0;
}

void* ReservePreferredPage(size_t page)
{
    void* const preferred = reinterpret_cast<void*>(0x100000000ull);
    constexpr int flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_FIXED_NOREPLACE
    void* result = mmap(preferred, page, PROT_READ | PROT_WRITE,
                        flags | MAP_FIXED_NOREPLACE, -1, 0);
    if (result == MAP_FAILED && errno != EINVAL)
        return MAP_FAILED;
    if (result != MAP_FAILED)
        return result;
#endif
    // A plain hint cannot replace an existing mapping. Reject a relocated map.
    return mmap(preferred, page, PROT_READ | PROT_WRITE, flags, -1, 0);
}
}

int main()
{
    assert(!PreferredBaseMapped());
    const int fdCount = OpenFdCount();
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        const std::string report = lo::android_probe::ProbeMemory();
        std::cout << report;
        assert(report.find("memory.fixed_4g_reservation=PASS") != std::string::npos);
        assert(report.find("memory.memfd=PASS") != std::string::npos);
        assert(report.find("memory.alias_ac=PASS") != std::string::npos);
        if (sysconf(_SC_PAGESIZE) == 4096)
        {
            assert(report.find("memory.alias_e=PASS") != std::string::npos);
            assert(report.find("memory.status=PASS") != std::string::npos);
        }
        else if (4096 % sysconf(_SC_PAGESIZE) != 0)
        {
            assert(report.find("memory.alias_e=UNSUPPORTED") != std::string::npos);
            assert(report.find("memory.status=UNSUPPORTED") != std::string::npos);
        }
        assert(!PreferredBaseMapped());
        assert(OpenFdCount() == fdCount);
    }

    const size_t page = static_cast<size_t>(sysconf(_SC_PAGESIZE));
    void* const occupied = ReservePreferredPage(page);
    assert(occupied != MAP_FAILED);
    if (occupied != reinterpret_cast<void*>(0x100000000ull))
    {
        munmap(occupied, page);
        assert(false && "preferred address unavailable for conflict test");
    }
    volatile unsigned char* const sentinel = static_cast<unsigned char*>(occupied);
    sentinel[0] = 0x5A;
    const std::string conflict = lo::android_probe::ProbeMemory();
    std::cout << conflict;
    assert(conflict.find("memory.fixed_4g_reservation=FAIL") != std::string::npos);
    assert(conflict.find("memory.status=FAIL") != std::string::npos);
    assert(PreferredBaseMapped());
    assert(sentinel[0] == 0x5A);
    assert(OpenFdCount() == fdCount);
    assert(munmap(occupied, page) == 0);
    assert(!PreferredBaseMapped());
}
