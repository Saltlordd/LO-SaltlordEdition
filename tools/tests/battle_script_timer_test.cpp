// Linked with verbatim generated PPC functions and the production timer hook.
#include "ppc_context.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <sys/mman.h>
#endif

PPC_EXTERN_FUNC(sub_8238AC40);
PPC_EXTERN_FUNC(sub_82E74250);
PPC_EXTERN_FUNC(sub_8238ACC8);
PPC_EXTERN_FUNC(sub_82A9BF40);
PPC_EXTERN_FUNC(sub_8238AE08);
PPC_EXTERN_FUNC(sub_82A9DBB8);
PPC_EXTERN_FUNC(sub_82A9E3B8);

namespace {
constexpr uint32_t Stack = 0x1F000;
constexpr uint32_t Manager = 0x30000;
constexpr uint32_t Script = 0x40000;
constexpr uint32_t Active = 0x50000;
constexpr uint32_t Threads = 0x60000;
constexpr uint32_t CoreClock = 0x70000;
constexpr uint32_t Scale60 = 0x82000DA8;
unsigned forwardCalls = 0, initCalls = 0, releaseCalls = 0, operandCalls = 0;
bool releaseSucceeds = true;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

uint32_t FloatBits(float value) { return std::bit_cast<uint32_t>(value); }

PPCContext Context(uint32_t manager = Manager) {
    PPCContext ctx{};
    ctx.r1.u64 = Stack;
    ctx.r3.u64 = manager;
    return ctx;
}

void Call(PPCFunc* function, uint8_t* base, PPCContext& ctx) {
    function(ctx, base);
    Require(ctx.r1.u32 == Stack, "guest stack balanced");
}

void Initialize(uint8_t* base) {
    std::memset(base + Manager, 0, 0x50000);
    PPC_STORE_U32(Manager + 24, Active);
    PPC_STORE_U32(Active + 40, Threads);
    PPC_STORE_U32(Active + 56, 0);
    PPC_STORE_U32(Threads + 8, 0xFFFFFFFFu); // real wait opcode initializes this operand
    PPC_STORE_U32(Script + 28, 0x40000000u); // active script
    PPC_STORE_U32(Script + 12, 0); // no unrelated script slots in this fixture
    PPC_STORE_U32(CoreClock + 4, FloatBits(0));
    PPC_STORE_U8(CoreClock + 24, 1);
    PPC_STORE_U32(Scale60, FloatBits(60));
    auto ctx = Context();
    const unsigned before = initCalls;
    Call(sub_82A9DBB8, base, ctx);
    Require(initCalls == before + 1 && PPC_LOAD_U32(Manager + 44) == Script,
            "initializer forwarded once and installed script");
}

uint32_t Tick(uint8_t* base, double seconds) {
    auto ctx = Context();
    ctx.f1.f64 = seconds;
    const unsigned before = forwardCalls;
    Call(sub_8238ACC8, base, ctx);
    Require(forwardCalls == before + 1, "active update forwarded exactly once");
    return PPC_LOAD_U32(Script + 16);
}

uint32_t TickFromAbsoluteMs(uint8_t* base, float absoluteMs) {
    auto delta = Context(CoreClock);
    delta.f1.f64 = absoluteMs;
    Call(sub_8238AC40, base, delta);
    auto read = Context(CoreClock);
    Call(sub_82E74250, base, read);
    // The generated caller sub_8238A640 multiplies this millisecond delta by
    // 0.001 before passing seconds to sub_8238ACC8.
    const double seconds = double(float(read.f1.f64 * 0.001f));
    return Tick(base, seconds);
}

void Wait(uint8_t* base) {
    auto ctx = Context();
    Call(sub_82A9BF40, base, ctx);
}

void WaitAcrossRate(uint8_t* base, int fps, bool baseline) {
    Initialize(base);
    const unsigned beforeOperand = operandCalls;
    Wait(base);
    Require(operandCalls == beforeOperand + 1 && PPC_LOAD_U32(Threads + 8) == 60,
            "real wait opcode initialized 60-tick operand once");
    const int limit = fps * 2;
    int completed = 0;
    for (int frame = 1; frame <= limit; ++frame) {
        TickFromAbsoluteMs(base, float(double(frame) * 1000.0 / fps));
        Wait(base);
        if (PPC_LOAD_U32(Active + 52) == 3) { completed = frame; break; }
    }
    if (baseline && fps > 60) {
        Require(completed == 0 && PPC_LOAD_U32(Threads + 8) == 60,
                "unmodified 90/120 FPS guest wait must remain stuck at zero ticks");
    } else {
        Require(completed > 0, "timed wait completed");
        Require(double(completed) / fps >= 0.98 && double(completed) / fps <= 1.12,
                "timed wait duration follows one second of source time");
    }
}

void GuardAndResetCases(uint8_t* base) {
    Initialize(base);
    auto ctx = Context();
    ctx.lr = 0xDEADBEEFu;
    ctx.f31.f64 = 1.0 / 30;
    PPC_STORE_U32(Script + 16, 37);
    const unsigned before = forwardCalls;
    Call(sub_8238AE08, base, ctx);
    Require(forwardCalls == before + 1 && PPC_LOAD_U32(Script + 16) == 37,
            "non-target caller forwarded once without timer overwrite");

    PPC_STORE_U32(Manager + 44, 0);
    auto early = Context(); early.f1.f64 = 1.0 / 120;
    Call(sub_8238ACC8, base, early);
    Require(forwardCalls == before + 1, "empty script returned before timer hook");
    PPC_STORE_U32(Manager + 44, Script);
    PPC_STORE_U32(Script + 28, 0);
    early = Context(); early.f1.f64 = 1.0 / 120;
    Call(sub_8238ACC8, base, early);
    Require(forwardCalls == before + 1, "inactive script returned before timer hook");
    PPC_STORE_U32(Script + 28, 0x40000000u);
    PPC_STORE_U8(Manager + 4, 1);
    PPC_STORE_U32(Manager + 8, 0);
    early = Context(); early.f1.f64 = 1.0 / 120;
    Call(sub_8238ACC8, base, early);
    Require(forwardCalls == before + 1 && PPC_LOAD_U32(Manager + 8) == 1,
            "first-update early return did not visit timer hook");
    PPC_STORE_U8(Manager + 4, 0);
    Require(Tick(base, 1.0 / 120) == 0 && Tick(base, 1.0 / 120) == 1,
            "early returns did not accumulate hidden time");

    Initialize(base);
    Require(Tick(base, 1.0 / 120) == 0, "half tick retained");
    Require(Tick(base, 0) == 0 && Tick(base, 1.0 / 120) == 1,
            "zero delta preserves fractional progress");
    Initialize(base);
    Require(Tick(base, 1.0 / 120) == 0, "half tick before discontinuity");
    Require(Tick(base, -0.1) == uint32_t(-6),
            "negative delta preserves the generated signed tick");
    Require(Tick(base, 1.0 / 120) == 0, "negative delta clears stale fraction");
    Initialize(base);
    Tick(base, 1.0 / 120);
    Tick(base, std::numeric_limits<double>::quiet_NaN());
    Require(Tick(base, 1.0 / 120) == 0, "NaN delta clears stale fraction");
    Initialize(base);
    Tick(base, 1.0 / 120);
    Require(Tick(base, std::numeric_limits<double>::infinity()) == uint32_t(INT_MAX),
            "infinite delta preserves the generated saturation");
    Require(Tick(base, 1.0 / 120) == 0, "infinite delta clears stale fraction");
    Initialize(base);
    Tick(base, 1.0 / 120);
    Require(Tick(base, double(std::numeric_limits<int32_t>::max()) / 60.0 + 1) == uint32_t(INT_MAX),
            "overflow delta preserves the generated saturation");
    Require(Tick(base, 1.0 / 120) == 0, "overflow delta clears stale fraction");

    Initialize(base);
    Require(Tick(base, 1.0 / 120) == 0, "half tick before failed release");
    releaseSucceeds = false;
    ctx = Context();
    const unsigned releases = releaseCalls;
    Call(sub_82A9E3B8, base, ctx);
    Require(releaseCalls == releases + 1 && PPC_LOAD_U32(Manager + 44) == Script,
            "failed release forwarded once and kept script");
    Require(Tick(base, 1.0 / 120) == 1, "failed release preserved fraction");
    Require(Tick(base, 1.0 / 120) == 0, "fraction exists before successful release");
    releaseSucceeds = true;
    ctx = Context();
    Call(sub_82A9E3B8, base, ctx);
    Require(releaseCalls == releases + 2 && PPC_LOAD_U32(Manager + 44) == 0,
            "successful release forwarded once and cleared script");
    PPC_STORE_U32(Manager + 44, Script); // restore pointer without init hook
    Require(Tick(base, 1.0 / 120) == 0,
            "successful release cleared fraction before same-address reuse");

    Initialize(base);
    Require(Tick(base, 1.0 / 120) == 0, "fraction exists before reinitialization");
    ctx = Context();
    const unsigned initializers = initCalls;
    Call(sub_82A9DBB8, base, ctx); // same manager and script addresses
    Require(initCalls == initializers + 1, "reinitialization forwarded once");
    Require(Tick(base, 1.0 / 120) == 0,
            "same-address initialization cleared previous generation fraction");
}

void MixedCadence(uint8_t* base) {
    Initialize(base);
    Wait(base);
    const double pattern[] = {1.0 / 30, 1.0 / 120, 1.0 / 90, 1.0 / 60,
                              0.010, 0.015, 0.004, 0.022};
    double elapsed = 0;
    bool completed = false;
    for (int i = 0; i < 180; ++i) {
        elapsed += pattern[i % 8];
        TickFromAbsoluteMs(base, float(elapsed * 1000));
        Wait(base);
        if (PPC_LOAD_U32(Active + 52) == 3) { completed = true; break; }
    }
    Require(completed && elapsed >= 0.98 && elapsed <= 1.12,
            "mixed 30/60/90/120 FPS cadence and jitter keep one-second wait");
}
}

extern "C" PPC_FUNC(__imp__sub_8238AE08) { ++forwardCalls; }
extern "C" PPC_FUNC(__imp__sub_82A9DBB8) {
    ++initCalls;
    PPC_STORE_U32(ctx.r3.u32 + 44, Script);
}
extern "C" PPC_FUNC(__imp__sub_82A9E3B8) {
    ++releaseCalls;
    if (releaseSucceeds) PPC_STORE_U32(ctx.r3.u32 + 44, 0);
}
PPC_FUNC(__savegprlr_29) {
    PPC_STORE_U32(ctx.r1.u32 - 8, ctx.r12.u32);
    PPC_STORE_U64(ctx.r1.u32 - 16, ctx.r31.u64);
    PPC_STORE_U64(ctx.r1.u32 - 24, ctx.r30.u64);
    PPC_STORE_U64(ctx.r1.u32 - 32, ctx.r29.u64);
}
PPC_FUNC(__restgprlr_29) {
    ctx.lr = PPC_LOAD_U32(ctx.r1.u32 - 8);
    ctx.r31.u64 = PPC_LOAD_U64(ctx.r1.u32 - 16);
    ctx.r30.u64 = PPC_LOAD_U64(ctx.r1.u32 - 24);
    ctx.r29.u64 = PPC_LOAD_U64(ctx.r1.u32 - 32);
}
PPC_FUNC(sub_823A5058) {}
PPC_FUNC(sub_82B07848) {}
PPC_FUNC(sub_8238B4A8) {}
PPC_FUNC(sub_8238B5A0) {}
PPC_FUNC(sub_8238B850) {}
PPC_FUNC(sub_8238B900) {}
PPC_FUNC(sub_8238C708) {}
PPC_FUNC(sub_8238BE38) { ++operandCalls; ctx.r3.u64 = 60; }

int main(int argc, char** argv) {
    const bool baseline = argc == 2 && std::strcmp(argv[1], "--baseline") == 0;
    uint8_t* base = nullptr;
#ifdef _WIN32
    base = static_cast<uint8_t*>(VirtualAlloc(nullptr, size_t(1) << 32, MEM_RESERVE, PAGE_NOACCESS));
    Require(base != nullptr, "reserve 4 GiB guest address space");
    Require(VirtualAlloc(base + 0x10000, 0x80000, MEM_COMMIT, PAGE_READWRITE), "commit fixture objects");
    Require(VirtualAlloc(base + 0x82000000, 0x10000, MEM_COMMIT, PAGE_READWRITE), "commit guest scale constant");
#else
    base = static_cast<uint8_t*>(mmap(nullptr, size_t(1) << 32, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0));
    Require(base != MAP_FAILED, "reserve 4 GiB guest address space");
#endif
    try {
        if (baseline) {
            for (int fps : {90, 120}) WaitAcrossRate(base, fps, true);
        } else {
            for (int fps : {30, 60, 90, 120}) WaitAcrossRate(base, fps, false);
        }
        if (!baseline) {
            GuardAndResetCases(base);
            MixedCadence(base);
        }
        std::printf("PASS battle script timer: %s, native AC40/E74250/ACC8/BF40, forwards=%u\n",
                    baseline ? "baseline control" : "production hook", forwardCalls);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL battle script timer: %s\n", e.what());
        return 1;
    }
#ifdef _WIN32
    VirtualFree(base, 0, MEM_RELEASE);
#else
    munmap(base, size_t(1) << 32);
#endif
}
