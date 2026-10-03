#include "memory_probe.h"

#include "../../LostOdysseyRecomp/kernel/guest_address_space_layout.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <sstream>

#if defined(__linux__)
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace lo::android_probe
{
#if defined(__linux__)
namespace
{
constexpr uintptr_t kPreferredBase = 0x100000000ull;

struct ProbeResources
{
    void* reservation = MAP_FAILED;
    int backing = -1;

    ~ProbeResources()
    {
        if (backing >= 0)
            close(backing);
        if (reservation != MAP_FAILED)
            munmap(reservation, GuestAddressSpace::kSize);
    }
};

void* ReservePreferred()
{
    constexpr int flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_FIXED_NOREPLACE
    void* result = mmap(reinterpret_cast<void*>(kPreferredBase), GuestAddressSpace::kSize,
                        PROT_NONE, flags | MAP_FIXED_NOREPLACE, -1, 0);
    if (result == MAP_FAILED && errno != EINVAL)
        return MAP_FAILED;
    if (result != MAP_FAILED)
        return result;
    // An older kernel may reject MAP_FIXED_NOREPLACE. A non-fixed hint cannot
    // replace another mapping and is accepted only if it returns the exact VA.
#endif
    return mmap(reinterpret_cast<void*>(kPreferredBase), GuestAddressSpace::kSize,
                PROT_NONE, flags, -1, 0);
}

void WriteError(std::ostringstream& out, const char* stage, int error)
{
    out << "memory." << stage << "=FAIL errno=" << error << " (" << std::strerror(error) << ")\n";
    out << "memory.status=FAIL\n";
}
}
#endif

std::string ProbeMemory()
{
    std::ostringstream out;
#if !defined(__linux__)
    out << "memory.status=UNSUPPORTED (requires Linux/Android mmap and memfd)\n";
    return out.str();
#else
    errno = 0;
    const long pageValue = sysconf(_SC_PAGESIZE);
    if (pageValue <= 0)
    {
        WriteError(out, "page_size", errno);
        return out.str();
    }
    const auto page = static_cast<size_t>(pageValue);
    out << "memory.page_size=" << page << '\n';
    const size_t eDelta = GuestAddressSpace::kOffsets[3] - GuestAddressSpace::kOffsets[1];
    const bool eAligned = (eDelta % page) == 0;
    out << "memory.e_view_alignment=" << (eAligned ? "PASS" : "UNSUPPORTED")
        << " (backing offset delta=" << eDelta << ")\n";

    ProbeResources resources;
    resources.reservation = ReservePreferred();
    if (resources.reservation == MAP_FAILED)
    {
        WriteError(out, "fixed_4g_reservation", errno);
        return out.str();
    }
    if (reinterpret_cast<uintptr_t>(resources.reservation) != kPreferredBase)
    {
        out << "memory.fixed_4g_reservation=FAIL (preferred address unavailable)\n";
        out << "memory.status=FAIL\n";
        return out.str();
    }
    out << "memory.fixed_4g_reservation=PASS (virtual address only)\n";

#if defined(SYS_memfd_create)
    resources.backing = static_cast<int>(syscall(SYS_memfd_create, "lo-android-memory-probe", 0));
#else
    errno = ENOSYS;
#endif
    if (resources.backing < 0)
    {
        WriteError(out, "memfd", errno);
        return out.str();
    }
    // Normalize the production backing offsets to zero. The same relative
    // A/C/E layout can then be checked without sizing a multi-GiB memfd.
    const size_t acLength = eAligned ? page + eDelta : page;
    if (ftruncate(resources.backing, static_cast<off_t>(acLength)) != 0)
    {
        WriteError(out, "memfd_resize", errno);
        return out.str();
    }
    out << "memory.memfd=PASS (sparse backing size=" << acLength << ")\n";

    auto* const base = static_cast<uint8_t*>(resources.reservation);
    for (size_t view = 1; view < 3; ++view)
    {
        void* const address = base + GuestAddressSpace::kStarts[view];
        // MAP_FIXED only replaces pages within the reservation owned above.
        if (mmap(address, acLength, PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_FIXED, resources.backing, 0) != address)
        {
            WriteError(out, view == 1 ? "map_a" : "map_c", errno);
            return out.str();
        }
    }

    constexpr size_t sentinelOffset = 0x80;
    volatile uint8_t* const a = base + GuestAddressSpace::kStarts[1];
    volatile uint8_t* const c = base + GuestAddressSpace::kStarts[2];
    a[sentinelOffset] = 0x35;
    const bool aToC = c[sentinelOffset] == 0x35;
    c[sentinelOffset] = 0xCA;
    const bool cToA = a[sentinelOffset] == 0xCA;
    if (!aToC || !cToA)
    {
        out << "memory.alias_ac=FAIL (shared writes disagree)\n";
        out << "memory.status=FAIL\n";
        return out.str();
    }
    out << "memory.alias_ac=PASS (bidirectional sentinel writes)\n";

    if (!eAligned)
    {
        out << "memory.alias_e=UNSUPPORTED (4 KiB backing shift is not host-page aligned)\n";
        out << "memory.status=UNSUPPORTED\n";
        return out.str();
    }

    void* const eAddress = base + GuestAddressSpace::kStarts[3];
    if (mmap(eAddress, page, PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_FIXED, resources.backing, static_cast<off_t>(eDelta)) != eAddress)
    {
        WriteError(out, "map_e", errno);
        return out.str();
    }
    volatile uint8_t* const e = static_cast<uint8_t*>(eAddress);
    a[eDelta + sentinelOffset] = 0x63;
    const bool aToE = e[sentinelOffset] == 0x63;
    e[sentinelOffset] = 0x9C;
    const bool eToA = a[eDelta + sentinelOffset] == 0x9C && c[eDelta + sentinelOffset] == 0x9C;
    if (!aToE || !eToA)
    {
        out << "memory.alias_e=FAIL (shifted writes disagree)\n";
        out << "memory.status=FAIL\n";
        return out.str();
    }
    out << "memory.alias_e=PASS (shifted sentinel writes)\n";
    out << "memory.status=PASS (small-map feasibility only; production full mapping and protection untested)\n";
    return out.str();
#endif
}
}
