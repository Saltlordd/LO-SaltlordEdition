#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>

// Only the integer operations used by the three retail functions in this
// fixture. Guest addresses are sparse, so the test never allocates 4 GiB.
union PPCRegister
{
    uint64_t u64;
    int64_t s64;
    uint32_t u32;
    int32_t s32;
    uint8_t u8;
};

struct Condition
{
    bool lt{}, eq{}, gt{};
    template <typename T>
    void compare(T a, T b, uint32_t)
    {
        lt = a < b;
        eq = a == b;
        gt = a > b;
    }
};

struct PPCContext
{
    PPCRegister r0{}, r1{}, r2{}, r3{}, r4{}, r5{}, r6{}, r7{};
    PPCRegister r8{}, r9{}, r10{}, r11{}, r12{}, r13{}, r14{}, r15{};
    PPCRegister r16{}, r17{}, r18{}, r19{}, r20{}, r21{}, r22{}, r23{};
    PPCRegister r24{}, r25{}, r26{}, r27{}, r28{}, r29{}, r30{}, r31{};
    PPCRegister ctr{};
    uint64_t lr{};
    uint32_t xer{};
    Condition cr6{};
};

inline std::unordered_map<uint32_t, uint8_t> guest;
inline uint32_t Load32(uint32_t address)
{
    return (uint32_t(guest.at(address)) << 24) |
           (uint32_t(guest.at(address + 1)) << 16) |
           (uint32_t(guest.at(address + 2)) << 8) | guest.at(address + 3);
}
inline void Store32(uint32_t address, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        guest[address + i] = uint8_t(value >> (24 - i * 8));
}

#define PPC_FUNC(name) void name(PPCContext& ctx, uint8_t* base)
#define PPC_FUNC_IMPL(name) extern "C" PPC_FUNC(name)
#define PPC_FUNC_PROLOGUE()
#define PPC_LOAD_U8(address) guest.at(uint32_t(address))
#define PPC_LOAD_U32(address) Load32(uint32_t(address))
#define PPC_STORE_U32(address, value) Store32(uint32_t(address), uint32_t(value))
#define PPC_CALL_INDIRECT_FUNC(address) Dispatch(uint32_t(address), ctx, base)

PPC_FUNC(sub_829E54F0);
PPC_FUNC(sub_8229E0C8);
void Dispatch(uint32_t address, PPCContext& ctx, uint8_t* base);
void __savegprlr_25(PPCContext& ctx, uint8_t* base);
void __restgprlr_25(PPCContext& ctx, uint8_t* base);
void sub_8229DA68(PPCContext& ctx, uint8_t* base);
void sub_82323570(PPCContext& ctx, uint8_t* base);
