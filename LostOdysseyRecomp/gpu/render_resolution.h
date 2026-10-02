#pragma once
#include <algorithm>
#include <cstdint>

namespace gpu::resolution {
struct Size {
    uint32_t width = 1280, height = 720;
    bool operator==(const Size&) const = default;
};
enum class TargetRole : uint8_t { Unknown, Scene, Fixed };
// The guest remains 1280x720. Host scene targets follow the output aspect;
// the camera and Canvas hooks preserve scene and UI proportions separately.
// A mode sets the 16:9 content area: wider outputs keep its height and widen,
// taller ones keep its width and grow taller, so that area keeps its pixels.
// A minimized/uninitialized output uses the native size, never a zero allocation.
inline constexpr uint32_t MaxTallHeight = 4320; // bounds portrait rasters
inline constexpr Size ResolveInternalSize(uint32_t mode, uint32_t outputWidth, uint32_t outputHeight) {
    const bool hasOutput = outputWidth && outputHeight;
    const bool tall = hasOutput && uint64_t(outputWidth) * 9 < uint64_t(outputHeight) * 16;
    const auto widthForHeight = [&](uint32_t height) {
        return (std::max)(1u, uint32_t((uint64_t(height) * outputWidth + outputHeight / 2) / outputHeight));
    };
    const auto forWidth = [&](uint32_t width) -> Size {
        const uint32_t height = (std::max)(1u, uint32_t((uint64_t(width) * outputHeight + outputWidth / 2) / outputWidth));
        return height <= MaxTallHeight ? Size{width, height} : Size{widthForHeight(MaxTallHeight), MaxTallHeight};
    };
    const auto forMode = [&](uint32_t height) -> Size {
        if (!hasOutput) return {uint32_t((uint64_t(height) * 16) / 9), height};
        return tall ? forWidth(uint32_t((uint64_t(height) * 16) / 9)) : Size{widthForHeight(height), height};
    };
    switch (mode) {
    case 720: case 1080: case 1440: case 2160: return forMode(mode);
    default:
        if (!hasOutput) return {};
        // Follow the output, capping the 16:9 area at 2160 rows.
        if (tall) return forWidth((std::clamp)(outputWidth, 1u, 3840u));
        const uint32_t height = (std::clamp)(outputHeight, 1u, 2160u);
        return {widthForHeight(height), height};
    }
}
// Map boundaries, rather than independently rounding an origin and a width:
// adjacent rectangles then share precisely the same physical pixel boundary.
inline constexpr uint32_t Scale(uint32_t guest, uint32_t internalHeight) {
    return uint32_t((uint64_t(guest) * internalHeight + 360) / 720);
}
inline constexpr uint32_t ScaleX(uint32_t guest, uint32_t internalWidth) {
    return uint32_t((uint64_t(guest) * internalWidth + 640) / 1280);
}
inline constexpr Size TargetSize(uint32_t pitch, uint32_t height, Size internal) {
    // Square EDRAM surfaces include shadow maps and luminance reductions. Their
    // texture resolution is independent of the scene's output resolution.
    if (pitch == height || ScaleX(pitch, internal.width) > 16384 || Scale(height, internal.height) > 16384)
        return {};
    return internal;
}
// Catalogued scene surfaces use both dimensions of the CPU frame plan. Fixed
// surfaces retain their guest texels. Until the catalog proves a role, retain
// the historical isotropic height scale rather than widening by output aspect.
inline constexpr Size TargetSizeForRole(TargetRole role, uint32_t pitch, uint32_t height, Size plan) {
    switch (role) {
    case TargetRole::Scene: return plan;
    case TargetRole::Fixed: return {};
    case TargetRole::Unknown: {
        // Scale by the plan's 16:9 area, which is its full height only on
        // outputs at least as wide as 16:9.
        const uint32_t area = (std::min)(plan.height, uint32_t((uint64_t(plan.width) * 9) / 16));
        return TargetSize(pitch, height, { uint32_t((uint64_t(area) * 16) / 9), area });
    }
    }
    return {};
}
// A plan may carry a sub-720 official DLSS input. Only catalogued Scene targets
// receive that size; Fixed and Unknown retain their legacy mappings and never
// invent an aspect ratio from a rounded recommended input.
inline constexpr Size TargetSizeForPlan(TargetRole role, uint32_t pitch, uint32_t height, Size input, Size legacy) {
    return role == TargetRole::Scene ? input : TargetSizeForRole(role, pitch, height, legacy);
}
inline constexpr uint32_t TargetHeight(uint32_t pitch, uint32_t height, uint32_t internalHeight) {
    return TargetSize(pitch, height, {uint32_t((uint64_t(internalHeight) * 16) / 9), internalHeight}).height;
}
}
