// Driver regression: large rectangle lists must preserve cleared and untouched pixels.
#include <gpu/depth_clear_layout.h>
#include <plume_render_interface.h>
#include <cstdio>
#include <cstring>
#include <vector>
namespace plume {
    std::unique_ptr<RenderInterface> CreateD3D12Interface();
    std::unique_ptr<RenderInterface> CreateVulkanInterface();
}
static int LegacyDepthClear() {
    using namespace plume;
    constexpr uint32_t width = 1280, height = 736;
    auto api = CreateD3D12Interface();
    auto device = api->createDevice();
    auto queue = device->createCommandQueue(RenderCommandListType::DIRECT);
    for (const uint32_t count : {0u, 1u, 16u, 17u, 720u, 721u}) {
        auto depth = device->createTexture(RenderTextureDesc::Texture2D(
            width, height, 1, RenderFormat::D32_FLOAT, RenderTextureFlag::DEPTH_TARGET));
        auto fb = device->createFramebuffer(RenderFramebufferDesc(nullptr, 0, depth.get()));
        auto readback = device->createBuffer(RenderBufferDesc::ReadbackBuffer(width * height * 4));
        auto commands = queue->createCommandList();
        auto fence = device->createCommandFence();
        std::vector<RenderRect> rects;
        if (count == 720) {
            for (const auto& r : gpu::renderer::MapDepthClear(640, 2, {0, 0, 640, 360}, width, height, 0))
                rects.push_back({r.left, r.top, r.right, r.bottom});
            if (rects.size() != count) return 2;
        } else {
            for (uint32_t i = 0; i < count; ++i) {
                const int x = int((i % 320) * 4), y = int((i / 320) * 4);
                rects.push_back({x, y, x + 2, y + 2});
            }
        }
        std::vector<float> expected(width * height, count ? 1.0f : 0.25f);
        for (const auto& r : rects)
            for (int y = r.top; y < r.bottom; ++y)
                for (int x = r.left; x < r.right; ++x) expected[y * width + x] = 0.25f;
        commands->begin();
        commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(depth.get(), RenderTextureLayout::DEPTH_WRITE));
        commands->setFramebuffer(fb.get());
        commands->clearDepthStencil(true, false, 1, 0);
        std::printf("clear %u rectangles\n", count); std::fflush(stdout);
        commands->clearDepthStencil(true, false, 0.25f, 0, rects.data(), count);
        commands->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(depth.get(), RenderTextureLayout::COPY_SOURCE));
        commands->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(
            readback.get(), RenderFormat::R32_FLOAT, width, height, 1, width, 0),
            RenderTextureCopyLocation::Subresource(depth.get(), 0));
        commands->end();
        const RenderCommandList* lists[] = {commands.get()};
        queue->executeCommandLists(lists, 1, nullptr, 0, nullptr, 0, fence.get());
        queue->waitForCommandFence(fence.get());
        const auto values = static_cast<const float*>(readback->map());
        size_t errors = 0;
        for (size_t i = 0; i < expected.size(); ++i) errors += values[i] != expected[i];
        readback->unmap();
        std::printf("%u rectangles: %zu mismatches across %zu depth pixels\n", count, errors, expected.size());
        if (errors) return 1;
    }
    std::puts("PASS: full clear, batch boundaries, sparse isolation and 720 tile rectangles");
    return 0;
}

static int CoalescedVulkan() {
    using namespace plume;
    using namespace gpu::renderer;
    constexpr uint32_t guestWidth = 1280, guestHeight = 736, scale = 3;
    constexpr uint32_t width = guestWidth * scale, height = guestHeight * scale;
    auto api = CreateVulkanInterface();
    if (!api) return 2;
    auto device = api->createDevice();
    if (!device) return 2;
    auto queue = device->createCommandQueue(RenderCommandListType::DIRECT);
    if (!queue) return 2;
    auto depth = device->createTexture(RenderTextureDesc::Texture2D(
        width, height, 1, RenderFormat::D32_FLOAT, RenderTextureFlag::DEPTH_TARGET));
    if (!depth) return 2;
    auto fb = device->createFramebuffer(RenderFramebufferDesc(nullptr, 0, depth.get()));
    auto readback = device->createBuffer(RenderBufferDesc::ReadbackBuffer(uint64_t(width) * height * 4));
    auto commands = queue->createCommandList();
    auto fence = device->createCommandFence();
    if (!fb || !readback || !commands || !fence) return 2;
    for (unsigned fixture = 0; fixture < 2; ++fixture) {
        std::vector<DepthClearRect> original;
        if (fixture == 0) {
            original = MapDepthClear(640, 2, {0, 0, 640, 360}, guestWidth, guestHeight, 0);
            if (original.size() != 720) return 2;
        } else {
            // A partial clear with a central hole, split edges and overlapping
            // top-edge tiles. Clearing its bounding box would fail readback.
            original = {{80, 40, 160, 72}, {160, 40, 240, 72}, {240, 40, 320, 72},
                {144, 40, 208, 72}, {80, 72, 112, 128}, {80, 128, 112, 180},
                {288, 72, 320, 128}, {288, 128, 320, 180},
                {112, 148, 176, 180}, {176, 148, 240, 180}, {240, 148, 288, 180}};
        }
        // Build expected coverage from the original rectangles, not the
        // merged output, then independently inspect every scaled depth pixel.
        std::vector<uint8_t> expected(guestWidth * guestHeight);
        for (const auto& r : original)
            for (int y = r.top; y < r.bottom; ++y)
                for (int x = r.left; x < r.right; ++x) expected[size_t(y) * guestWidth + x] = 1;
        auto merged = original;
        CoalesceDepthClearRects(merged);
        if (merged.empty()) return 2; // Zero API rectangles would clear the whole target.
        if (fixture == 0 && (merged.size() != 1 || merged[0].left != 0 || merged[0].top != 0 ||
            merged[0].right != 1280 || merged[0].bottom != 720)) return 2;
        std::vector<RenderRect> rects;
        for (const auto& r : merged)
            rects.push_back({r.left * int32_t(scale), r.top * int32_t(scale),
                r.right * int32_t(scale), r.bottom * int32_t(scale)});
        commands->begin();
        commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(depth.get(), RenderTextureLayout::DEPTH_WRITE));
        commands->setFramebuffer(fb.get());
        commands->clearDepthStencil(true, false, 1, 0);
        commands->clearDepthStencil(true, false, 0.25f, 0, rects.data(), uint32_t(rects.size()));
        commands->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(depth.get(), RenderTextureLayout::COPY_SOURCE));
        commands->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(
            readback.get(), RenderFormat::R32_FLOAT, width, height, 1, width, 0),
            RenderTextureCopyLocation::Subresource(depth.get(), 0));
        commands->end();
        const RenderCommandList* lists[]{commands.get()};
        queue->executeCommandLists(lists, 1, nullptr, 0, nullptr, 0, fence.get());
        queue->waitForCommandFence(fence.get());
        const auto values = static_cast<const float*>(readback->map());
        if (!values) return 2;
        size_t errors = 0, untouchedBottomErrors = 0;
        for (uint32_t y = 0; y < height; ++y) for (uint32_t x = 0; x < width; ++x) {
            const float wanted = expected[size_t(y / scale) * guestWidth + x / scale] ? 0.25f : 1.0f;
            const float value = values[size_t(y) * width + x];
            errors += value != wanted;
            if (y >= 2160) untouchedBottomErrors += value != 1.0f;
        }
        readback->unmap();
        std::printf("Vulkan coalesced %s: %zu -> %zu rectangles, %ux%u depth, %zu mismatches, bottom 48 rows errors=%zu\n",
            fixture == 0 ? "full 3840x2160" : "partial with hole", original.size(), merged.size(),
            width, height, errors, untouchedBottomErrors);
        if (errors || untouchedBottomErrors) return 1;
    }
    std::puts("PASS: Vulkan coalesced full/partial clears preserve exact coverage and padded depth rows");
    return 0;
}

static int ShadowAtlas(bool vulkan) {
    using namespace plume;
    using namespace gpu::renderer;
    auto api = vulkan ? CreateVulkanInterface() : CreateD3D12Interface();
    auto device = api ? api->createDevice() : nullptr;
    auto queue = device ? device->createCommandQueue(RenderCommandListType::DIRECT) : nullptr;
    if (!queue) return 2;
    for (const uint32_t scale : {1u, 2u, 4u}) {
        constexpr uint32_t guestWidth = 880, guestHeight = 896;
        const uint32_t width = guestWidth * scale, height = guestHeight * scale;
        const uint32_t rowPixels = (width + 63u) & ~63u;
        auto depth = device->createTexture(RenderTextureDesc::Texture2D(
            width, height, 1, RenderFormat::D32_FLOAT, RenderTextureFlag::DEPTH_TARGET));
        auto fb = depth ? device->createFramebuffer(RenderFramebufferDesc(nullptr, 0, depth.get())) : nullptr;
        auto readback = device->createBuffer(RenderBufferDesc::ReadbackBuffer(uint64_t(rowPixels) * height * 4));
        auto commands = queue->createCommandList();
        auto fence = device->createCommandFence();
        if (!depth || !fb || !readback || !commands || !fence) return 2;
        // The shadow clear uses half the atlas pitch. Its right tile maps to
        // x=[480,800), y=[0,432); the left tile and padded rows must survive.
        auto guestRects = MapDepthClear(440, 2, {240, 0, 400, 216}, guestWidth, guestHeight, 0);
        CoalesceDepthClearRects(guestRects);
        if (guestRects.empty()) return 2;
        std::vector<RenderRect> rects;
        for (const auto& r : guestRects)
            rects.push_back({r.left * int(scale), r.top * int(scale), r.right * int(scale), r.bottom * int(scale)});
        commands->begin();
        commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(depth.get(), RenderTextureLayout::DEPTH_WRITE));
        commands->setFramebuffer(fb.get());
        commands->clearDepthStencil(true, false, 0.75f, 0);
        commands->clearDepthStencil(true, false, 0.25f, 0, rects.data(), uint32_t(rects.size()));
        commands->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(depth.get(), RenderTextureLayout::COPY_SOURCE));
        commands->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(
            readback.get(), RenderFormat::R32_FLOAT, width, height, 1, rowPixels, 0),
            RenderTextureCopyLocation::Subresource(depth.get(), 0));
        commands->end();
        const RenderCommandList* lists[]{commands.get()};
        queue->executeCommandLists(lists, 1, nullptr, 0, nullptr, 0, fence.get());
        queue->waitForCommandFence(fence.get());
        const auto* values = static_cast<const float*>(readback->map());
        if (!values) return 2;
        size_t errors = 0;
        for (uint32_t y = 0; y < height; ++y) for (uint32_t x = 0; x < width; ++x) {
            const bool rightTile = x >= 480 * scale && x < 800 * scale && y < 432 * scale;
            errors += values[size_t(y) * rowPixels + x] != (rightTile ? 0.25f : 0.75f);
        }
        readback->unmap();
        std::printf("%s shadow atlas %ux: %ux%u, %zu pixel mismatches\n", vulkan ? "Vulkan" : "D3D12", scale, width, height, errors);
        if (errors) return 1;
    }
    std::puts("PASS: shadow 1x/2x/4x clears preserve neighboring atlas tiles and padding");
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 1) return LegacyDepthClear();
    if (argc == 2 && std::strcmp(argv[1], "--shadow") == 0) return ShadowAtlas(false);
    if (argc == 3 && std::strcmp(argv[1], "--vulkan") == 0 &&
        std::strcmp(argv[2], "--shadow") == 0) return ShadowAtlas(true);
    if (argc == 3 && std::strcmp(argv[1], "--vulkan") == 0 &&
        std::strcmp(argv[2], "--coalesced-only") == 0) return CoalescedVulkan();
    std::fprintf(stderr, "usage: depth_clear_gpu_test [--shadow | --vulkan --shadow | --vulkan --coalesced-only]\n");
    return 2;
}
