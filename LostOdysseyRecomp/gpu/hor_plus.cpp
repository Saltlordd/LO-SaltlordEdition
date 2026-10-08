#include <stdafx.h>

#include "gpu/renderer.h"
#include "gpu/movie_clear.h"
#include "gpu/aspect_layout.h"
#include "gpu/frame_plan.h"
#include "cpu/ppc_context.h"
#include "kernel/memory.h"

extern "C" PPC_FUNC(__imp__sub_82300E50);
extern "C" PPC_FUNC(__imp__sub_82303990);
extern "C" PPC_FUNC(__imp__sub_823E4B50);
extern "C" PPC_FUNC(__imp__sub_823EFC08);

namespace
{
constexpr float NativeAspect = 16.0f / 9.0f;

bool ValidRange(uint32_t address, uint32_t size)
{
    return g_memory.base && address >= 0x10000 && uint64_t(address) + size <= 0x100000000ull;
}

uint32_t LoadWord(uint32_t address)
{
    return ValidRange(address, 4) ? __builtin_bswap32(*reinterpret_cast<const uint32_t*>(g_memory.base + address)) : 0;
}

void StoreWord(uint32_t address, uint32_t value)
{
    if (ValidRange(address, 4))
        *reinterpret_cast<uint32_t*>(g_memory.base + address) = __builtin_bswap32(value);
}

float LoadFloat(uint32_t address) { return std::bit_cast<float>(LoadWord(address)); }
void StoreFloat(uint32_t address, float value) { StoreWord(address, std::bit_cast<uint32_t>(value)); }

gpu::aspect_layout::Scale OutputScale()
{
    const float aspect = gpu::renderer::ActiveOutputAspect();
#if defined(__ANDROID__)
    return gpu::aspect_layout::ForAspect(aspect);
#else
    return {std::isfinite(aspect) && aspect > NativeAspect ? NativeAspect / aspect : 1.0f, 1.0f};
#endif
}
float HorPlusScale() { return OutputScale().x; }

bool IsMainPerspectiveView(uint32_t view)
{
    // The common constructor is also used for subordinate orthographic views.
    // Both accepted callers build a main camera from a live family/target pair
    // and give it the full guest output rectangle.
    const uint32_t family = LoadWord(view);
    const uint32_t target = family ? LoadWord(family + 0x0C) : 0;
    const uint32_t displaySurface = LoadWord(0x83302A34);
    const float x = LoadFloat(view + 0x1C), y = LoadFloat(view + 0x20);
    const float width = LoadFloat(view + 0x24), height = LoadFloat(view + 0x28);
    const float projectionW = LoadFloat(view + 0x80 + 44), projectionQ = LoadFloat(view + 0x80 + 60);
    return displaySurface && target && LoadWord(target + 4) == displaySurface &&
        std::isfinite(x) && std::isfinite(y) && std::isfinite(width) && std::isfinite(height) &&
        x >= 0.0f && y >= 0.0f && width > 0.0f && height > 0.0f && x + width <= 1280.0f && y + height <= 720.0f &&
        std::isfinite(projectionW) && std::isfinite(projectionQ) && std::abs(projectionW) > 1e-6f && std::abs(projectionQ) < 1e-6f;
}

bool IsMainOutputView(uint32_t view)
{
    const uint32_t family = LoadWord(view);
    const uint32_t target = family ? LoadWord(family + 0x0C) : 0;
    const uint32_t displaySurface = LoadWord(0x83302A34);
    const float x = LoadFloat(view + 0x1C), y = LoadFloat(view + 0x20);
    const float width = LoadFloat(view + 0x24), height = LoadFloat(view + 0x28);
    return displaySurface && target && LoadWord(target + 4) == displaySurface &&
        std::isfinite(x) && std::isfinite(y) && std::isfinite(width) && std::isfinite(height) &&
        x >= 0.0f && y >= 0.0f && width > 0.0f && height > 0.0f && x + width <= 1280.0f && y + height <= 720.0f;
}

bool IsDisplayTarget(uint32_t target)
{
    const uint32_t displaySurface = LoadWord(0x83302A34);
    return displaySurface && target && LoadWord(target + 4) == displaySurface;
}

void ScaleProjectionColumn(uint32_t projection, const float k, uint32_t column = 0)
{
    if (k == 1.0f || !ValidRange(projection, 64)) return;
    for (uint32_t row = 0; row < 4; ++row)
    {
        const uint32_t offset = row * 16 + column * 4;
        StoreFloat(projection + offset, LoadFloat(projection + offset) * k);
    }
}

bool IsPrimaryCanvas(uint32_t canvas, uint32_t& matrix)
{
    const uint32_t stack = LoadWord(canvas + 0x0C);
    const uint32_t count = LoadWord(canvas + 0x10);
    if (!stack || !count || count > 64 || !ValidRange(stack, count * 64)) return false;
    matrix = stack + (count - 1) * 64;
    // Match the uncomposed canvas basis so translated, rotated and scaled HUD
    // transforms at the top of the stack remain eligible for composition.
    return IsDisplayTarget(LoadWord(canvas + 4)) && std::abs(LoadFloat(stack + 20) + 2.0f / 720.0f) < 1e-6f;
}

struct CanvasColumn
{
    std::array<uint32_t, 8> words{};
};

CanvasColumn ComposeCanvasColumn(uint32_t matrix, gpu::aspect_layout::Scale scale)
{
    CanvasColumn original;
    for (uint32_t row = 0; row < 4; ++row)
        for (uint32_t axis = 0; axis < 2; ++axis) {
            const uint32_t address = matrix + row * 16 + axis * 4;
            original.words[row * 2 + axis] = LoadWord(address);
            StoreFloat(address, gpu::aspect_layout::CanvasComponent(LoadFloat(address),
                LoadFloat(matrix + row * 16 + 12), axis ? scale.y : scale.x,
                axis ? -1.0f / 720.0f : 1.0f / 1280.0f));
        }
    return original;
}
void RestoreCanvasColumn(uint32_t matrix, const CanvasColumn& original)
{
    for (uint32_t row = 0; row < 4; ++row)
        for (uint32_t axis = 0; axis < 2; ++axis)
            StoreWord(matrix + row * 16 + axis * 4, original.words[row * 2 + axis]);
}

thread_local bool movieMainOutput = false;

struct MovieScope
{
    bool previous;
    explicit MovieScope(bool current) : previous(movieMainOutput) { movieMainOutput = previous || current; }
    ~MovieScope() { movieMainOutput = previous; }
};

void EmitMovieBars(uint32_t device, uint32_t surfaceInfo, uint32_t colorInfo,
                   float x, float y, float width, float height, float safeLeft, float safeRight)
{
    if (!ValidRange(device + 0x2884, 4)) return;
    const uint32_t values[gpu::movie_clear::WordCount] = {gpu::movie_clear::Magic, surfaceInfo, colorInfo,
        std::bit_cast<uint32_t>(x), std::bit_cast<uint32_t>(y), std::bit_cast<uint32_t>(width), std::bit_cast<uint32_t>(height),
        std::bit_cast<uint32_t>(safeLeft), std::bit_cast<uint32_t>(safeRight), gpu::movie_clear::Magic};
    gpu::frame_plan::EmitPrivatePacket(device, gpu::movie_clear::RegisterBase, values);
}
}

// Recompiler mid-assembly hook at 82301DA0.  The projection copy to view+0x80
// has completed; the original constructor has not derived VP, inverse VP, or
// the frustum yet.  The caller LR was saved in the constructor frame.
void HorPlusViewProjection(PPCRegister& r1, PPCRegister& r31)
{
    const uint32_t caller = LoadWord(r1.u32 + 0x1D8);
    const float k = HorPlusScale();
    if ((caller == 0x82300AFC || caller == 0x82307BEC) && IsMainPerspectiveView(r31.u32))
    {
        ScaleProjectionColumn(r31.u32 + 0x80, k);
        ScaleProjectionColumn(r31.u32 + 0x80, OutputScale().y, 1);
    }
}

// This alternate player-frustum path builds its own VP and frustum after the
// projection helper returns.  Restrict the strong wrapper to that exact call.
PPC_FUNC(sub_82300E50)
{
    const uint32_t caller = uint32_t(ctx.lr);
    const uint32_t projection = ctx.r3.u32;
    const float k = HorPlusScale();
    __imp__sub_82300E50(ctx, base);
    if (caller == 0x82988684)
    {
        ScaleProjectionColumn(projection, k);
        ScaleProjectionColumn(projection, OutputScale().y, 1);
    }
}

// Keep the game HUD at 16:9 in a wider physical scene target.  The flush copies
// this state into either its immediate draw or queued command before returning.
PPC_FUNC(sub_82303990)
{
    const uint32_t canvas = ctx.r3.u32;
    uint32_t matrix = 0;
    const auto scale = OutputScale();
    const bool primary = !scale.IsIdentity() && IsPrimaryCanvas(canvas, matrix);
    CanvasColumn original;
    std::array<uint32_t, 4> scissor{};
    if (primary) {
        original = ComposeCanvasColumn(matrix, scale);
        for (uint32_t i = 0; i < 4; ++i) scissor[i] = LoadWord(canvas + 0x2C + i * 4);
        const auto fitted = gpu::aspect_layout::FitScissor(scissor, scale);
        for (uint32_t i = 0; i < 4; ++i) StoreWord(canvas + 0x2C + i * 4, fitted[i]);
    }
    __imp__sub_82303990(ctx, base);
    if (primary && ValidRange(matrix, 64)) {
        RestoreCanvasColumn(matrix, original);
        for (uint32_t i = 0; i < 4; ++i) StoreWord(canvas + 0x2C + i * 4, scissor[i]);
    }
}

PPC_FUNC(sub_823EFC08)
{
    MovieScope scope(IsMainOutputView(ctx.r6.u32));
    __imp__sub_823EFC08(ctx, base);
}

// FLO_MOVIE is the only call at this helper return address.  Compress its
// destination into the centred 16:9 safe region; OverlayColor and postprocess
// calls retain their full-width coverage.
PPC_FUNC(sub_823E4B50)
{
    const bool movie = movieMainOutput && uint32_t(ctx.lr) == 0x823EFF88;
    // The outer movie call receives its device in r4, but its 0x823EFF00
    // caller moves that value to r3 immediately before this helper.
    const uint32_t device = ctx.r3.u32;
    const float originalX = float(ctx.f1.f64), originalY = float(ctx.f2.f64);
    const float originalWidth = float(ctx.f3.f64), originalHeight = float(ctx.f4.f64);
    float safeLeft = originalX, safeRight = originalX + originalWidth;
    if (movie)
    {
        const auto axes = OutputScale();
        ctx.f2.f64 = gpu::aspect_layout::FitBoundary(originalY, 720.0f, axes.y);
        ctx.f4.f64 = originalHeight * axes.y;
        const float scale = axes.x;
        if (scale != 1.0f)
        {
            safeLeft = (1.0f - scale) * 640.0f + originalX * scale;
            safeRight = safeLeft + originalWidth * scale;
            ctx.f1.f64 = safeLeft;
            ctx.f3.f64 = safeRight - safeLeft;
        }
    }
    __imp__sub_823E4B50(ctx, base);
    if (movie && safeRight > safeLeft)
        EmitMovieBars(device, LoadWord(device + 0x2880), LoadWord(device + 0x2884),
            originalX, originalY, originalWidth, originalHeight, safeLeft, safeRight);
}
