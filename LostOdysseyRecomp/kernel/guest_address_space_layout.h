#pragma once

#include "guest_address_space.h"
#include <cstddef>

// Shared by the per-platform GuestAddressSpace implementations. Not part of the
// public interface in guest_address_space.h.
namespace GuestAddressSpace
{
// Views: [0] virtual, [1] A physical, [2] C alias of A, [3] E alias of A one
// 4 KiB page later. Offsets index the shared backing store.
inline constexpr size_t kSize = 0x100000000ull;
inline constexpr size_t kBackingSize = 0xC0001000ull;
inline constexpr size_t kStarts[] = {0, 0xA0000000, 0xC0000000, 0xE0000000};
inline constexpr size_t kSizes[] = {0xA0000000, 0x20000000, 0x20000000, 0x20000000};
inline constexpr size_t kOffsets[] = {0, 0xA0000000, 0xA0000000, 0xA0001000};

void ClearFailure();
void RecordFailure(FailureOperation operation, uint32_t error, int32_t viewIndex,
                   const void* address, size_t size, size_t offset = 0,
                   uint32_t preferredReservationError = 0, uintptr_t backingHandle = 0);
}
