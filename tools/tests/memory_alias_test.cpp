#include <kernel/guest_address_space.h>
#include <os/platform.h>
#include <cstdio>
#include <cstdint>
#include <initializer_list>

int main()
{
    // Repeat to exercise release and reuse, including the preferred host base.
    for (int iteration = 0; iteration < 2; ++iteration)
    {
        uint8_t* base = GuestAddressSpace::Allocate();
        if (!base)
        {
            const auto failure = GuestAddressSpace::GetFailureInfo();
            std::fprintf(stderr, "Guest address space allocation failed: %s, error=%u view=%d size=%llu offset=%llu\n",
                         GuestAddressSpace::FailureOperationName(failure.operation), failure.error, failure.viewIndex,
                         static_cast<unsigned long long>(failure.size), static_cast<unsigned long long>(failure.offset));
            return 1;
        }
        auto word = [base](uint32_t address) -> volatile uint32_t& {
            return *reinterpret_cast<volatile uint32_t*>(base + address);
        };
        bool ok = true;
        for (uint32_t offset : {0u, 0x1000u, 0x1FFFCu, 0x1234560u, 0x1FFFFFFCu})
        {
            word(0xA0000000u + offset) = 0x12345678;
            ok &= word(0xC0000000u + offset) == 0x12345678;
            word(0xC0000000u + offset) = 0;
            ok &= word(0xA0000000u + offset) == 0;
            // macOS leaves E inaccessible: 16 KiB pages cannot express its
            // 4 KiB offset (see guest_address_space_macos.cpp).
            if (!LO_PLATFORM_MACOS && offset >= 0x1000)
            {
                word(0xE0000000u + offset - 0x1000) = 0xABCDEF01;
                ok &= word(0xA0000000u + offset) == 0xABCDEF01;
                ok &= word(0xC0000000u + offset) == 0xABCDEF01;
            }
        }
        // A virtual page must not accidentally alias the physical page. Use the
        // first page past the null guard, which is one 16 KiB host page on macOS.
        constexpr uint32_t page = LO_PLATFORM_MACOS ? 0x4000 : 0x1000;
        word(page) = 0x87654321;
        word(0xA0000000 + page) = 0xDEADBEEF;
        ok &= word(page) == 0x87654321;

        // Occlusion-query round trip: CPU initializes through C, GPU writes
        // END through A, CPU subtracts BEGIN from END through C.
        constexpr uint32_t query = 0x01002000;
        word(0xC0000000 + query + 0x30) = 100;
        word(0xC0000000 + query + 0x10) = 0xFFFFFFFF;
        ok &= word(0xA0000000 + query + 0x10) == 0xFFFFFFFF;
        word(0xA0000000 + query + 0x10) = 65636;
        ok &= word(0xC0000000 + query + 0x10) - word(0xC0000000 + query + 0x30) == 65536;
        GuestAddressSpace::Release(base);
        if (!ok)
        {
            std::fprintf(stderr, "Physical alias coherence failed\n");
            return 1;
        }
    }
    std::puts("PASS: A/C coherence, E offset, virtual isolation, query round trip, release/reallocate");
    return 0;
}
