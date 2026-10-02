#include <stdafx.h>
#include <cstdlib>
#include <iostream>

// Emitted from the actual generated guest functions by verify_guard.py.
// The C1 handler, its operand reader, and battle predicate are not reimplemented.
#include "native_bodies.inc"

namespace
{
    constexpr uint32_t Manager = 0x10000;
    constexpr uint32_t Frame = 0x20000;
    constexpr uint32_t Code = 0x21000;
    constexpr uint32_t Actor = 0x30000;
    constexpr uint32_t Touch = 0x30100;
    constexpr uint32_t Overlaps = 0x31000;
    constexpr uint32_t Bindings = 0x32000;
    constexpr uint32_t GameInfo = 0x40000;
    constexpr uint32_t VTable = 0x41000;
    constexpr uint32_t World = 0x50000;
    constexpr uint32_t Controller = 0x60000;
    constexpr uint32_t DestroyedFlag = 0x61000;
    constexpr uint32_t Request = 0x83263EA8;
    constexpr uint32_t Entry = 0x3A;
    constexpr uint32_t Rejected = 0x10F;
    constexpr uint64_t Caller = 0x829FDB90;
    std::array<uint64_t, 7> saved{};
    uint64_t savedLr{};
    unsigned checks{};

    void Require(bool ok, const char* message)
    {
        ++checks;
        if (!ok) throw std::runtime_error(message);
    }

    void Prepare(int32_t phase, bool touching, int32_t battleId = -1,
                 uint32_t currentScene = 0, uint32_t nextScene = 0)
    {
        guest.clear();
        Store32(0x83318744, World);
        Store32(World + 472, currentScene);
        Store32(World + 476, nextScene);
        Store32(GameInfo + 840, uint32_t(battleId));
        Store32(GameInfo, VTable);
        Store32(VTable + 844, 0x829E54F0);
        Store32(Request, uint32_t(phase));
        Store32(Manager + 5824, 16);
        Store32(Manager + 23840, Bindings);
        Store32(Bindings + 4, Actor);
        Store32(Bindings + 10 * 4, Touch);
        Store32(Actor + 212, Overlaps);
        Store32(Actor + 216, 1);
        Store32(Overlaps, touching ? Touch : 0x30200);
        Store32(Touch + 96, 0);
        Store32(Frame + 1172, Code);
        Store32(Frame + 1692, Entry);
        // C1 with actor operands 1 and 10, and a little-endian false target.
        constexpr uint8_t instruction[]{0xC1, 1, 0, 0, 0, 10, 0, 0, 0, 0x0F, 1};
        for (unsigned i = 0; i < sizeof(instruction); ++i)
            guest[Code + Entry + i] = instruction[i];
        guest[Code + Rejected] = 0; // The retail false target; not executed by this fixture.
        guest[Code + Entry + 11] = 0xDD;
        Store32(Controller + 0x5C8, 0x08000010);
        Store32(DestroyedFlag, 0);
        // C1's stwu writes this back-chain word as part of its normal prologue.
        Store32(0x70000 - 144, 0x70000);
    }

    PPCContext Context()
    {
        PPCContext ctx{};
        ctx.r1.u64 = 0x70000;
        ctx.r3.u64 = Manager;
        ctx.r4.u64 = Frame;
        ctx.lr = Caller;
        ctx.r25.u64 = 25;
        ctx.r26.u64 = 26;
        ctx.r27.u64 = 27;
        ctx.r28.u64 = 28;
        ctx.r29.u64 = 29;
        ctx.r30.u64 = 30;
        ctx.r31.u64 = 31;
        return ctx;
    }

    void RunC1(bool accepted)
    {
        auto before = guest;
        auto ctx = Context();
        __imp__sub_82A4FA38(ctx, nullptr);
        const uint32_t pc = accepted ? Entry + 11 : Rejected;
        Require(Load32(Frame + 1692) == pc, "C1 chose the wrong native branch");
        Store32(Frame + 1692, Entry);
        Require(guest == before, "C1 changed guest state beyond its PC");
        Require(ctx.r1.u32 == 0x70000 && ctx.lr == Caller, "C1 did not restore its stack/LR");
        Require(ctx.r25.u64 == 25 && ctx.r26.u64 == 26 && ctx.r27.u64 == 27 &&
                ctx.r28.u64 == 28 && ctx.r29.u64 == 29 && ctx.r30.u64 == 30 &&
                ctx.r31.u64 == 31, "C1 changed nonvolatile registers");
        Require(ctx.r3.u32 == 0, "C1's return convention changed");
    }

    void ComparePredicate(uint64_t caller, uint32_t expectedResult)
    {
        auto original = Context();
        original.r3.u64 = GameInfo;
        original.lr = caller;
        auto patched = original;
        __imp__sub_829E54F0(original, nullptr);
        auto before = guest;
        sub_829E54F0(patched, nullptr);
        Require(patched.r3.u32 == expectedResult, "predicate result mismatch");
        original.r3.u64 = expectedResult;
        Require(patched.r1.u64 == original.r1.u64 && patched.lr == original.lr &&
                patched.r10.u64 == original.r10.u64 && patched.r11.u64 == original.r11.u64 &&
                patched.cr6.eq == original.cr6.eq && patched.cr6.lt == original.cr6.lt &&
                patched.cr6.gt == original.cr6.gt, "wrapper changed original predicate context");
        Require(guest == before, "predicate wrapper wrote guest memory");
    }
}

void __savegprlr_25(PPCContext& ctx, uint8_t*)
{
    saved = {ctx.r25.u64, ctx.r26.u64, ctx.r27.u64, ctx.r28.u64,
             ctx.r29.u64, ctx.r30.u64, ctx.r31.u64};
    savedLr = ctx.r12.u64;
}
void __restgprlr_25(PPCContext& ctx, uint8_t*)
{
    ctx.r25.u64 = saved[0]; ctx.r26.u64 = saved[1]; ctx.r27.u64 = saved[2];
    ctx.r28.u64 = saved[3]; ctx.r29.u64 = saved[4]; ctx.r30.u64 = saved[5];
    ctx.r31.u64 = saved[6]; ctx.lr = savedLr;
}
void sub_8229DA68(PPCContext& ctx, uint8_t*) { ctx.r3.u64 = World; }
void sub_82323570(PPCContext& ctx, uint8_t*) { ctx.r3.u64 = GameInfo; }
void sub_8229E0C8(PPCContext& ctx, uint8_t* base) { __imp__sub_8229E0C8(ctx, base); }
void Dispatch(uint32_t address, PPCContext& ctx, uint8_t* base)
{
    Require(address == 0x829E54F0, "unexpected C1 virtual dispatch");
    sub_829E54F0(ctx, base);
}

int main()
try
{
    for (int32_t phase : {-1, 0, 1, 2})
    {
        for (bool touching : {false, true})
        {
            Prepare(phase, touching);
            RunC1(touching && phase <= 0);
        }
        Prepare(phase, true);
        ComparePredicate(0x82A4FBA4, phase > 0 ? 1 : 0);
        // Item-event updates and the VM scheduler must keep retail semantics.
        ComparePredicate(0x829FAC54, 0);
        ComparePredicate(0x829FE334, 0);
        Prepare(phase, true, 123);
        RunC1(false);
        ComparePredicate(0x82A4FBA4, 1);
        ComparePredicate(0x829FAC54, 1);
        ComparePredicate(0x829FE334, 1);
        Prepare(phase, true, -1, 3);
        RunC1(false);
        ComparePredicate(0x82A4FBA4, 1);
        ComparePredicate(0x829FAC54, 1);
        Prepare(phase, true, -1, 0, 3);
        RunC1(false);
        ComparePredicate(0x82A4FBA4, 1);
        ComparePredicate(0x829FE334, 1);
    }
    // Conditions recover after the verified retail completion/cancel reset.
    // Controller animation/EndTouch recovery is a separate static check.
    Prepare(1, true);
    RunC1(false);
    Store32(Request, 0);
    RunC1(true);
    Prepare(2, true);
    RunC1(false);
    Store32(Request, 0);
    RunC1(true);
    std::cout << "field interaction: " << checks << " checks passed\n";
    return EXIT_SUCCESS;
}
catch (const std::exception& e)
{
    std::cerr << "field interaction: " << e.what() << '\n';
    return EXIT_FAILURE;
}
