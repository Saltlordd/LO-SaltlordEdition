#include "gpu/scene_copy_promotion_policy.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace gpu::scene_copy_promotion;
namespace {
unsigned checks = 0;
void Require(bool value, const char* message) {
    ++checks;
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
}
int main() {
    Require(CanAppendConstants(0, 512, 256), "two exact-fit constant blocks");
    Require(!CanAppendConstants(1, 512, 256), "alignment can exhaust ring");
    Require(CanAppendConstants(1, 768, 256), "alignment with sufficient space");
    Require(!CanAppendConstants(256, 511, 256, 1), "single block cannot wrap");
    Require(!CanAppendConstants(513, 512, 1), "invalid offset rejected");
    Require(!CanAppendConstants(0, 512, 0), "zero-byte reservation rejected");
    Require(!CanAppendConstants(0, 512, 256, 2, 0), "zero alignment rejected");
    Require(!CanAppendConstants(0, 512, 256, 2, 3), "non-power-of-two alignment rejected");
    constexpr auto max = std::numeric_limits<uint64_t>::max();
    Require(!CanAppendConstants(max - 4, max, 8, 1), "alignment cannot overflow");
    Require(!CanAppendConstants(0, max, max, 2), "combined size cannot overflow");
    Require(CanAppendConstants(max - 255, max, 255, 1), "large exact-fit reservation");
    // Exercise every frame/epoch/storage/depth combination used by the
    // renderer preflight. A Flush changes none of these and preserves mapping.
    for (unsigned flags = 0; flags < 16; ++flags) {
        const bool sameStorage = flags & 1, sameFrame = flags & 2;
        const bool sameEpoch = flags & 4, depthStencil = flags & 8;
        const bool keep = sameStorage && sameFrame && sameEpoch && !depthStencil;
        Require(MustRestore(sameStorage, sameFrame, sameEpoch, depthStencil, 736, 736) == !keep,
            "storage/frame/epoch/depth preflight");
    }
    Require(!MustRestore(true, true, true, false, 720, 736), "padded target retains smaller viewport");
    Require(MustRestore(true, true, true, false, 768, 736), "growing extent restores parked grid");
    // 1280x720 copy into a 1280x736 guest target promoted to 4K (3840x2208).
    const PixelRect fullScissor{0, 0, 3840, 2208};
    const auto covered = CoveredPixels(0, 0, 1280, 720, 3, 3, fullScissor, 3840, 2208);
    Require(covered == PixelRect{3, 3, 3837, 2157}, "one guest pixel margin inside the quad");
    const auto rest = UncoveredPixels(3840, 2208, covered);
    Require(rest.count == 4 && rest.rects[0] == PixelRect{0, 0, 3840, 3} &&
        rest.rects[1] == PixelRect{0, 2157, 3840, 2208} && rest.rects[2] == PixelRect{0, 3, 3, 2157} &&
        rest.rects[3] == PixelRect{3837, 3, 3840, 2157}, "bands cover everything outside the copy");
    uint64_t area = uint64_t(covered.right - covered.left) * (covered.bottom - covered.top);
    for (uint32_t i = 0; i < rest.count; ++i)
        area += uint64_t(rest.rects[i].right - rest.rects[i].left) * (rest.rects[i].bottom - rest.rects[i].top);
    Require(area == 3840ull * 2208, "covered plus bands tile the target exactly");
    Require(CoveredPixels(0, 0, 1280, 720, 1.5, 1.5, {0, 0, 1920, 1104}, 1920, 1104) == PixelRect{2, 2, 1918, 1078},
        "fractional scales round inward");
    Require(CoveredPixels(0.5f, 0, 1279.5f, 720, 3, 3, fullScissor, 3840, 2208) == PixelRect{5, 3, 3835, 2157},
        "half-pixel quad bounds stay conservative");
    Require(CoveredPixels(0, 0, 1280, 720, 3, 3, {0, 0, 1000, 2208}, 3840, 2208).right == 1000,
        "scissor limits the covered pixels");
    Require(CoveredPixels(0, 0, 1280, 720, 0, 3, fullScissor, 3840, 2208).Empty(), "invalid scale covers nothing");
    Require(CoveredPixels(0, 0, 1.5, 720, 3, 3, fullScissor, 3840, 2208).Empty(), "narrow quad covers nothing");
    const auto all = UncoveredPixels(3840, 2208, PixelRect{});
    Require(all.count == 1 && all.rects[0] == fullScissor, "nothing covered resamples the whole target");
    const auto clipped = UncoveredPixels(3840, 2208, PixelRect{0, 0, 3841, 2208});
    Require(clipped.count == 1 && clipped.rects[0] == fullScissor, "out-of-range coverage resamples the whole target");
    Require(UncoveredPixels(3840, 2208, fullScissor).count == 0, "full coverage needs no resample");
    std::cout << checks << " promotion policy checks passed\n";
}
