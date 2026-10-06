#pragma once

#include <cstdint>

namespace gpu::scene_copy_promotion {

// Called after a guest draw has already uploaded its constants and indices.
// A failed reservation must bypass promotion, never rotate the GPU slot.
inline constexpr bool CanAppendConstants(uint64_t offset, uint64_t capacity,
    uint64_t bytes, uint32_t count = 2, uint64_t alignment = 256) {
    if (!alignment || (alignment & (alignment - 1)) || !bytes || offset > capacity) return false;
    for (uint32_t i = 0; i < count; ++i) {
        const uint64_t padding = (alignment - (offset & (alignment - 1))) & (alignment - 1);
        if (padding > capacity - offset) return false;
        offset += padding;
        if (bytes > capacity - offset) return false;
        offset += bytes;
    }
    return true;
}

// Check before borrowing either framebuffer attachment. A promoted color
// attachment cannot be paired with the parked low-resolution depth grid.
inline constexpr bool MustRestore(bool sameStorage, bool sameFrame, bool sameEpoch,
    bool needsDepthStencil, uint32_t requestedGuestHeight, uint32_t promotedGuestHeight) {
    return !sameStorage || !sameFrame || !sameEpoch || needsDepthStencil ||
        requestedGuestHeight > promotedGuestHeight;
}

struct PixelRect {
    uint32_t left = 0, top = 0, right = 0, bottom = 0;
    constexpr bool Empty() const { return right <= left || bottom <= top; }
    constexpr bool operator==(const PixelRect&) const = default;
};

// Host pixels that a full-target scene copy certainly replaces. The quad bounds
// are in guest pixels; one guest pixel of margin on every side absorbs edge
// rounding and half-pixel offsets, so the result never claims an unwritten pixel.
// The scissor is already in host pixels.
constexpr PixelRect CoveredPixels(double minX, double minY, double maxX, double maxY,
    double scaleX, double scaleY, PixelRect scissor, uint32_t width, uint32_t height) {
    if (!(scaleX > 0) || !(scaleY > 0) || !(maxX > minX) || !(maxY > minY)) return {};
    const auto ceilEdge = [](double value, uint32_t limit) -> uint32_t {
        if (!(value > 0)) return 0;
        if (value >= double(limit)) return limit;
        const auto whole = uint32_t(value);
        return double(whole) < value ? whole + 1 : whole;
    };
    const auto floorEdge = [](double value, uint32_t limit) -> uint32_t {
        if (!(value > 0)) return 0;
        return value >= double(limit) ? limit : uint32_t(value);
    };
    PixelRect covered{ceilEdge((minX + 1) * scaleX, width), ceilEdge((minY + 1) * scaleY, height),
        floorEdge((maxX - 1) * scaleX, width), floorEdge((maxY - 1) * scaleY, height)};
    covered.left = covered.left > scissor.left ? covered.left : scissor.left;
    covered.top = covered.top > scissor.top ? covered.top : scissor.top;
    covered.right = covered.right < scissor.right ? covered.right : scissor.right;
    covered.bottom = covered.bottom < scissor.bottom ? covered.bottom : scissor.bottom;
    return covered.Empty() ? PixelRect{} : covered;
}

struct PixelRects {
    PixelRect rects[4]{};
    uint32_t count = 0;
};

// The rest of a width x height target around `covered`, as at most four bands.
constexpr PixelRects UncoveredPixels(uint32_t width, uint32_t height, PixelRect covered) {
    PixelRects out;
    const auto add = [&](PixelRect rect) { if (!rect.Empty()) out.rects[out.count++] = rect; };
    if (covered.Empty() || covered.right > width || covered.bottom > height) {
        add({0, 0, width, height});
        return out;
    }
    add({0, 0, width, covered.top});
    add({0, covered.bottom, width, height});
    add({0, covered.top, covered.left, covered.bottom});
    add({covered.right, covered.top, width, covered.bottom});
    return out;
}

} // namespace gpu::scene_copy_promotion
