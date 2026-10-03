#include <stdafx.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unordered_map>

extern "C" PPC_FUNC(__imp__sub_82397198);
extern "C" PPC_FUNC(__imp__sub_82397840);

namespace
{
// The battle camera update (sub_82390A70, slot 3 of vtable 0x8201CE68) feeds
// its POV to a rotation smoother at camera + 1088 every frame: SetInput
// (sub_82397198, POV in r5, camera cut flag in r6), then Update
// (sub_82397840, frame delta in f1). It adds the smoother's offset to the POV
// rotation it hands to the view.
//
// Update keeps the last 100 frame deltas and rotations and, in sub_82398458,
// resamples them at 12.5 ms steps. Each step consumes at most one history
// entry and moves toward it with weight 0.0125 / (time left before that
// entry). That is an interpolation only while every entry spans at least one
// step, i.e. at 80 FPS or below; the game ran at 30. With shorter frames the
// weight grows past 1, the time left drops through zero, and the resampled
// rotations explode: after some attacks the view jumps between random yaw and
// pitch for up to two seconds at 90 and 120 FPS (#117).
//
// Run the smoother only once at least one step has passed since its last
// update and give it the accumulated delta, so every entry it stores spans a
// full step. In between, sub_82390A70 keeps adding the last offset to the
// current POV. At 80 FPS or below every frame still updates.
constexpr double ResampleStep = 0.0125;
// A smoother not fed for this long (between battles) updates on its next
// frame instead of applying an offset left from before.
constexpr auto FreshAfter = std::chrono::milliseconds(250);

struct Gate
{
    std::chrono::steady_clock::time_point lastInput{};
    double pending = 0;
    double delta = 0;
    bool skipUpdate = false;
};

std::mutex gateMutex;
std::unordered_map<uint32_t, Gate> gates;

bool Enabled()
{
    // LO_BATTLE_CAMERA_SMOOTHER_GATE=0 restores the per-frame updates for A/B runs.
    static const bool enabled = [] {
        const char* value = std::getenv("LO_BATTLE_CAMERA_SMOOTHER_GATE");
        return !value || std::strcmp(value, "0") != 0;
    }();
    return enabled;
}
}

PPC_FUNC(sub_82397198)
{
    if (!Enabled())
    {
        __imp__sub_82397198(ctx, base);
        return;
    }
    const double delta = std::isfinite(ctx.f1.f64) && ctx.f1.f64 > 0 ? ctx.f1.f64 : 0.0;
    const bool cut = (ctx.r6.u32 & 0xFF) != 0;
    const auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard lock(gateMutex);
        Gate& gate = gates[ctx.r3.u32];
        const bool fresh = now - gate.lastInput > FreshAfter;
        gate.lastInput = now;
        gate.pending += delta;
        gate.skipUpdate = !cut && !fresh && gate.pending < ResampleStep;
        if (gate.skipUpdate)
            return;
        // A cut refills the history with 1/30 s entries; the entry Update
        // adds for this frame must also span a full step.
        gate.delta = std::max(gate.pending, ResampleStep);
        gate.pending = 0;
    }
    __imp__sub_82397198(ctx, base);
}

PPC_FUNC(sub_82397840)
{
    if (Enabled())
    {
        std::lock_guard lock(gateMutex);
        const auto gate = gates.find(ctx.r3.u32);
        if (gate != gates.end())
        {
            if (gate->second.skipUpdate)
                return;
            ctx.f1.f64 = gate->second.delta;
        }
    }
    __imp__sub_82397840(ctx, base);
}
