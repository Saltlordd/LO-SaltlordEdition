// Synthetic scene-depth AO regression. No game assets or window are used.
#include <gpu/ambient_occlusion.h>
#include <plume_render_interface.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace plume {
std::unique_ptr<RenderInterface> CreateD3D12Interface();
std::unique_ptr<RenderInterface> CreateVulkanInterface();
}
namespace {
using namespace plume;
constexpr uint32_t Width = 64, Height = 64;
constexpr uint32_t SmallWidth = 32, SmallHeight = 32;
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
uint32_t Pixel(uint8_t gray, uint8_t alpha) {
    return uint32_t(gray) * 0x010101u | (uint32_t(alpha) << 24);
}
uint8_t Red(uint32_t pixel) { return uint8_t(pixel); }
uint8_t Alpha(uint32_t pixel) { return uint8_t(pixel >> 24); }
float DepthAt(float z) { return (100.0f / z - 1.0f) / 99.0f; }
struct Half4 { uint16_t r, g, b, a; };
static_assert(sizeof(Half4) == 8);
float HalfToFloat(uint16_t value) {
    const float sign = (value & 0x8000) ? -1.0f : 1.0f;
    const uint32_t exponent = (value >> 10) & 31u, fraction = value & 1023u;
    if (exponent == 0) return sign * std::ldexp(float(fraction), -24);
    if (exponent == 31) return fraction ? std::numeric_limits<float>::quiet_NaN() :
        sign * std::numeric_limits<float>::infinity();
    return sign * std::ldexp(float(1024 + fraction), int(exponent) - 25);
}

struct Harness {
    std::unique_ptr<RenderInterface> api;
    std::unique_ptr<RenderDevice> device;
    std::unique_ptr<RenderCommandQueue> queue;
    std::unique_ptr<RenderCommandList> commands;
    std::unique_ptr<RenderCommandFence> fence;
    gpu::ao::AmbientOcclusion ao;
    explicit Harness(bool vulkan) {
        api = vulkan ? CreateVulkanInterface() : CreateD3D12Interface();
        Check(bool(api), "GPU interface");
        device = api->createDevice();
        Check(bool(device), "GPU device");
        std::printf("AO backend: %s on %s\n", vulkan ? "Vulkan" : "D3D12", device->getDescription().name.c_str());
        queue = device->createCommandQueue(RenderCommandListType::DIRECT);
        commands = queue->createCommandList();
        fence = device->createCommandFence();
        Check(bool(queue) && bool(commands) && bool(fence), "GPU queue, list and fence");
        Check(ao.Init(device.get()), ao.LastError().c_str());
    }
    void Submit() {
        commands->end();
        const RenderCommandList* lists[] = {commands.get()};
        queue->executeCommandLists(lists, 1, nullptr, 0, nullptr, 0, fence.get());
        queue->waitForCommandFence(fence.get());
        ao.ReleaseCompletedThrough(ao.RecordedSerial());
    }
    std::unique_ptr<RenderTexture> Texture(uint32_t width, uint32_t height, RenderFormat format) {
        auto texture = device->createTexture(RenderTextureDesc::Texture2D(width, height, 1, format));
        Check(bool(texture), "fixture texture allocation");
        return texture;
    }
    void Upload(RenderTexture* texture, const void* data, uint32_t width, uint32_t height,
                RenderFormat format, uint32_t bytesPerPixel) {
        const uint32_t pitch = (width * bytesPerPixel + 255u) & ~255u;
        auto upload = device->createBuffer(RenderBufferDesc::UploadBuffer(uint64_t(pitch) * height));
        Check(bool(upload), "fixture upload allocation");
        auto* mapped = static_cast<uint8_t*>(upload->map());
        Check(mapped != nullptr, "fixture upload map");
        for (uint32_t y = 0; y < height; ++y)
            std::memcpy(mapped + size_t(y) * pitch,
                        static_cast<const uint8_t*>(data) + size_t(y) * width * bytesPerPixel,
                        size_t(width) * bytesPerPixel);
        upload->unmap();
        commands->begin();
        commands->barriers(RenderBarrierStage::COPY,
                           RenderTextureBarrier(texture, RenderTextureLayout::COPY_DEST));
        commands->copyTextureRegion(RenderTextureCopyLocation::Subresource(texture),
            RenderTextureCopyLocation::PlacedFootprint(upload.get(), format, width, height, 1, pitch / bytesPerPixel));
        Submit();
    }
    struct Read {
        std::unique_ptr<RenderBuffer> buffer;
        uint32_t width = 0, height = 0, pitch = 0, bytesPerPixel = 0;
    };
    Read QueueRead(RenderTexture* source, uint32_t width, uint32_t height, RenderFormat format,
                   uint32_t bytesPerPixel) {
        Read result;
        result.width = width; result.height = height; result.bytesPerPixel = bytesPerPixel;
        result.pitch = (width * bytesPerPixel + 255u) & ~255u;
        result.buffer = device->createBuffer(RenderBufferDesc::ReadbackBuffer(uint64_t(result.pitch) * height));
        Check(bool(result.buffer), "fixture readback allocation");
        commands->barriers(RenderBarrierStage::COPY,
                           RenderTextureBarrier(source, RenderTextureLayout::COPY_SOURCE));
        commands->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(result.buffer.get(), format,
            width, height, 1, result.pitch / bytesPerPixel), RenderTextureCopyLocation::Subresource(source));
        commands->barriers(RenderBarrierStage::GRAPHICS,
                           RenderTextureBarrier(source, RenderTextureLayout::SHADER_READ));
        return result;
    }
    template<class T> std::vector<T> Collect(Read& read) {
        Check(sizeof(T) == read.bytesPerPixel, "readback pixel size");
        auto* mapped = static_cast<uint8_t*>(read.buffer->map());
        Check(mapped != nullptr, "fixture readback map");
        std::vector<T> pixels(size_t(read.width) * read.height);
        for (uint32_t y = 0; y < read.height; ++y)
            std::memcpy(pixels.data() + size_t(y) * read.width, mapped + size_t(y) * read.pitch,
                        size_t(read.width) * sizeof(T));
        read.buffer->unmap();
        return pixels;
    }
};

gpu::temporal::Matrix Projection() {
    return {1,0,0,0, 0,1,0,0, 0,0,100.0/99.0,1, 0,0,-100.0/99.0,0};
}
double Mean(const std::vector<uint32_t>& pixels, uint32_t x0, uint32_t x1,
            uint32_t y0, uint32_t y1, uint32_t width) {
    double sum = 0;
    for (uint32_t y = y0; y < y1; ++y)
        for (uint32_t x = x0; x < x1; ++x) sum += Red(pixels[size_t(y) * width + x]);
    return sum / double((x1 - x0) * (y1 - y0));
}
}

int main(int argc, char** argv) {
    try {
        const bool vulkan = argc == 2 && std::string(argv[1]) == "--vulkan";
        Check(argc == 1 || vulkan, "usage: ambient_occlusion_gpu_test [--vulkan]");
        Harness gpu(vulkan);
        auto camera = gpu::temporal::Camera::Create(Projection(), {0,0,Width,Height});
        auto smallCamera = gpu::temporal::Camera::Create(Projection(), {0,0,SmallWidth,SmallHeight});
        auto orthographic = gpu::temporal::Camera::Create(
            {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}, {0,0,Width,Height});
        Check(bool(camera) && bool(smallCamera) && bool(orthographic), "camera fixtures");
        auto color = gpu.Texture(Width, Height, RenderFormat::R8G8B8A8_UNORM);
        auto alternate = gpu.Texture(Width, Height, RenderFormat::R8G8B8A8_UNORM);
        auto depth = gpu.Texture(Width, Height, RenderFormat::R32_FLOAT);
        auto hdr = gpu.Texture(Width, Height, RenderFormat::R16G16B16A16_FLOAT);
        auto smallColor = gpu.Texture(SmallWidth, SmallHeight, RenderFormat::R8G8B8A8_UNORM);
        auto smallDepth = gpu.Texture(SmallWidth, SmallHeight, RenderFormat::R32_FLOAT);
        std::vector<uint32_t> colors(Width * Height), alternateColors(Width * Height);
        std::vector<float> depths(Width * Height, DepthAt(20));
        std::vector<Half4> hdrColors(Width * Height, {0x4000,0x4000,0x4000,0x3800});
        for (uint32_t y = 0; y < Height; ++y) for (uint32_t x = 0; x < Width; ++x) {
            const uint8_t alpha = uint8_t((x * 13 + y * 7) & 255);
            colors[size_t(y) * Width + x] = Pixel(192, alpha);
            alternateColors[size_t(y) * Width + x] = Pixel(96, alpha);
        }
        gpu.Upload(color.get(), colors.data(), Width, Height, RenderFormat::R8G8B8A8_UNORM, 4);
        gpu.Upload(alternate.get(), alternateColors.data(), Width, Height, RenderFormat::R8G8B8A8_UNORM, 4);
        gpu.Upload(hdr.get(), hdrColors.data(), Width, Height, RenderFormat::R16G16B16A16_FLOAT, 8);
        gpu.Upload(depth.get(), depths.data(), Width, Height, RenderFormat::R32_FLOAT, 4);
        std::vector<uint32_t> smallColors(SmallWidth * SmallHeight, Pixel(160, 91));
        std::vector<float> smallDepths(SmallWidth * SmallHeight, DepthAt(20));
        gpu.Upload(smallColor.get(), smallColors.data(), SmallWidth, SmallHeight, RenderFormat::R8G8B8A8_UNORM, 4);
        gpu.Upload(smallDepth.get(), smallDepths.data(), SmallWidth, SmallHeight, RenderFormat::R32_FLOAT, 4);
        gpu::ao::Inputs input{color.get(), depth.get(), hdr.get(), &*camera, Width, Height,
            gpu::ao::Mode::Off};
        gpu.commands->begin();
        const auto originalSerial = gpu.ao.RecordedSerial();
        Check(!gpu.ao.Record(gpu.commands.get(), input).color && gpu.ao.RecordedSerial() == originalSerial,
              "Off records no AO output or submission serial");
        input.mode = gpu::ao::Mode::Ssao;
        input.camera = &*orthographic;
        Check(!gpu.ao.Record(gpu.commands.get(), input).color && gpu.ao.RecordedSerial() == originalSerial,
              "unsupported projection records no AO output or submission serial");
        input.camera = &*camera;
        input.width = 0;
        Check(!gpu.ao.Record(gpu.commands.get(), input).color && gpu.ao.RecordedSerial() == originalSerial,
              "invalid extent records no AO output or submission serial");
        gpu.Submit();
        input.width = Width; input.debug = gpu::ao::Debug::Visibility; input.radius = 8; input.strength = 1;
        auto render = [&](gpu::ao::Mode mode) {
            input.mode = mode;
            gpu.commands->begin();
            for (auto* image : {color.get(), depth.get(), hdr.get()})
                gpu.commands->barriers(RenderBarrierStage::GRAPHICS,
                                       RenderTextureBarrier(image, RenderTextureLayout::SHADER_READ));
            auto output = gpu.ao.Record(gpu.commands.get(), input);
            Check(output.color != nullptr && output.hdrColor != nullptr, gpu.ao.LastError().c_str());
            auto read = gpu.QueueRead(output.color, Width, Height, RenderFormat::R8G8B8A8_UNORM, 4);
            gpu.Submit();
            return gpu.Collect<uint32_t>(read);
        };
        const auto flatSsao = render(gpu::ao::Mode::Ssao);
        const auto flatGtao = render(gpu::ao::Mode::Gtao);
        Check(Mean(flatSsao, 12,52,12,52,Width) > 220 &&
              Mean(flatGtao, 12,52,12,52,Width) > 210,
              "flat plane has near-white SSAO/GTAO visibility");
        for (uint32_t y = 0; y < Height; ++y)
            for (uint32_t x = 0; x < Width; ++x)
                depths[size_t(y) * Width + x] = DepthAt(17.0f + float(x) * .06f + float(y) * .03f);
        gpu.Upload(depth.get(), depths.data(), Width, Height, RenderFormat::R32_FLOAT, 4);
        const auto slopedSsao = render(gpu::ao::Mode::Ssao);
        const auto slopedGtao = render(gpu::ao::Mode::Gtao);
        Check(Mean(slopedSsao, 8,56,8,56,Width) > 205 &&
              Mean(slopedGtao, 8,56,8,56,Width) > 195,
              "smooth sloped plane does not self-occlude strongly");
        Check(Mean(slopedSsao, 0,4,0,Height,Width) > 170 &&
              Mean(slopedGtao, Width-4,Width,0,Height,Width) > 170,
              "viewport edges remain stable on a sloped plane");
        depths.assign(Width * Height, DepthAt(20));
        for (uint32_t y = 0; y < Height; ++y)
            for (uint32_t x = 0; x < Width; ++x)
                if (x >= 12 && x < 30 && y >= 10 && y < 54) depths[size_t(y) * Width + x] = DepthAt(16);
        depths[5 * Width + 5] = 0; // Sky and malformed depth must preserve color.
        depths[6 * Width + 6] = std::numeric_limits<float>::quiet_NaN();
        gpu.Upload(depth.get(), depths.data(), Width, Height, RenderFormat::R32_FLOAT, 4);
        const auto cornerSsao = render(gpu::ao::Mode::Ssao);
        const auto cornerGtao = render(gpu::ao::Mode::Gtao);
        const double ssaoEdge = Mean(cornerSsao, 31,39,20,44,Width);
        const double gtaoEdge = Mean(cornerGtao, 31,39,20,44,Width);
        const double ssaoFar = Mean(cornerSsao, 48,56,20,44,Width);
        const double gtaoFar = Mean(cornerGtao, 48,56,20,44,Width);
        Check(ssaoEdge + 3 < ssaoFar && gtaoEdge + 3 < gtaoFar,
              "step creates local SSAO and GTAO contact shading");
        Check(std::abs(ssaoEdge - gtaoEdge) > 2,
              "SSAO and GTAO produce distinct measured visibility");
        input.debug = gpu::ao::Debug::None; input.mode = gpu::ao::Mode::Gtao;
        gpu.commands->begin();
        for (auto* image : {color.get(), depth.get(), hdr.get()})
            gpu.commands->barriers(RenderBarrierStage::GRAPHICS,
                                   RenderTextureBarrier(image, RenderTextureLayout::SHADER_READ));
        const auto composed = gpu.ao.Record(gpu.commands.get(), input);
        Check(composed.color && composed.hdrColor, gpu.ao.LastError().c_str());
        auto colorRead = gpu.QueueRead(composed.color, Width, Height, RenderFormat::R8G8B8A8_UNORM, 4);
        auto hdrRead = gpu.QueueRead(composed.hdrColor, Width, Height, RenderFormat::R16G16B16A16_FLOAT, 8);
        gpu.Submit();
        const auto colorOut = gpu.Collect<uint32_t>(colorRead);
        const auto hdrOut = gpu.Collect<Half4>(hdrRead);
        for (size_t i = 0; i < colorOut.size(); ++i)
            Check(Alpha(colorOut[i]) == Alpha(colors[i]) && hdrOut[i].a == 0x3800,
                  "AO preserves exact RGBA8 and FP16 alpha");
        for (const size_t index : {size_t(5 * Width + 5), size_t(6 * Width + 6)}) {
            Check(colorOut[index] == colors[index] && hdrOut[index].r == 0x4000 &&
                  hdrOut[index].g == 0x4000 && hdrOut[index].b == 0x4000,
                  "sky and NaN depth preserve scene color without pollution");
        }
        const size_t contact = 32 * Width + 33;
        double hdrMask = 0, rgbaMask = 0;
        for (uint32_t y = 20; y < 44; ++y) for (uint32_t x = 31; x < 39; ++x) {
            const size_t index = size_t(y) * Width + x;
            hdrMask += HalfToFloat(hdrOut[index].r) / 2.0f;
            rgbaMask += double(Red(colorOut[index])) / 192.0;
        }
        hdrMask /= 8 * 24; rgbaMask /= 8 * 24;
        Check(HalfToFloat(hdrOut[contact].r) > 1.0f && rgbaMask < 1 &&
              std::abs(hdrMask - rgbaMask) < .04,
              "HDR values above 1 retain the same AO visibility mask as RGBA8");
        // Record two outputs and a resize before the first submission completes.
        // Their distinct resources must keep the first draw's descriptors intact.
        gpu.commands->begin();
        for (auto* image : {color.get(), alternate.get(), depth.get(), smallColor.get(), smallDepth.get()})
            gpu.commands->barriers(RenderBarrierStage::GRAPHICS,
                                   RenderTextureBarrier(image, RenderTextureLayout::SHADER_READ));
        input.hdrColor = nullptr;
        input.color = color.get();
        auto first = gpu.ao.Record(gpu.commands.get(), input);
        input.color = alternate.get();
        auto second = gpu.ao.Record(gpu.commands.get(), input);
        input.color = smallColor.get(); input.depth = smallDepth.get(); input.camera = &*smallCamera;
        input.width = SmallWidth; input.height = SmallHeight;
        auto resized = gpu.ao.Record(gpu.commands.get(), input);
        Check(first.color && second.color && resized.color && first.color != second.color &&
              first.color != resized.color && second.color != resized.color,
              "unfinished AO serials keep separate output allocations through resize");
        auto firstRead = gpu.QueueRead(first.color, Width, Height, RenderFormat::R8G8B8A8_UNORM, 4);
        auto secondRead = gpu.QueueRead(second.color, Width, Height, RenderFormat::R8G8B8A8_UNORM, 4);
        auto resizedRead = gpu.QueueRead(resized.color, SmallWidth, SmallHeight, RenderFormat::R8G8B8A8_UNORM, 4);
        gpu.Submit();
        const auto firstOut = gpu.Collect<uint32_t>(firstRead);
        const auto secondOut = gpu.Collect<uint32_t>(secondRead);
        const auto resizedOut = gpu.Collect<uint32_t>(resizedRead);
        Check(Alpha(firstOut[contact]) == Alpha(colors[contact]) &&
              Alpha(secondOut[contact]) == Alpha(alternateColors[contact]) &&
              Red(firstOut[contact]) > Red(secondOut[contact]) &&
              Alpha(resizedOut[16 * SmallWidth + 16]) == 91 &&
              std::abs(int(Red(resizedOut[16 * SmallWidth + 16])) - 160) < 20,
              "pending serials preserve source identity and resized output");
        // A completed production serial is still in use when a later submission
        // samples its output. Only the output lease can make it reusable.
        gpu::ao::AmbientOcclusion lifetimeAo;
        Check(lifetimeAo.Init(gpu.device.get()), lifetimeAo.LastError().c_str());
        auto submitLifetime = [&] {
            gpu.commands->end();
            const RenderCommandList* lists[] = {gpu.commands.get()};
            gpu.queue->executeCommandLists(lists, 1, nullptr, 0, nullptr, 0, gpu.fence.get());
            gpu.queue->waitForCommandFence(gpu.fence.get());
            lifetimeAo.ReleaseCompletedThrough(lifetimeAo.RecordedSerial());
        };
        gpu::ao::Inputs lifeInput{color.get(), depth.get(), nullptr, &*camera, Width, Height,
            gpu::ao::Mode::Gtao};
        lifeInput.radius = 8; lifeInput.strength = 1;
        gpu.commands->begin();
        for (auto* image : {color.get(), alternate.get(), depth.get()})
            gpu.commands->barriers(RenderBarrierStage::GRAPHICS,
                                   RenderTextureBarrier(image, RenderTextureLayout::SHADER_READ));
        auto held = lifetimeAo.Record(gpu.commands.get(), lifeInput);
        Check(held.color && held.lifetime, lifetimeAo.LastError().c_str());
        RenderTexture* heldTexture = held.color;
        auto heldRead = gpu.QueueRead(held.color, Width, Height, RenderFormat::R8G8B8A8_UNORM, 4);
        submitLifetime();
        const auto heldBaseline = gpu.Collect<uint32_t>(heldRead);
        std::vector<gpu::ao::Output> otherOutputs;
        for (unsigned i = 0; i < 4; ++i) {
            gpu.commands->begin();
            lifeInput.color = alternate.get();
            auto other = lifetimeAo.Record(gpu.commands.get(), lifeInput);
            Check(other.color && other.lifetime && other.color != heldTexture,
                  "completed AO output stays allocated while its consumer lease is held");
            otherOutputs.push_back(std::move(other));
            submitLifetime();
        }
        gpu.commands->begin();
        auto retainedRead = gpu.QueueRead(heldTexture, Width, Height, RenderFormat::R8G8B8A8_UNORM, 4);
        submitLifetime();
        Check(gpu.Collect<uint32_t>(retainedRead) == heldBaseline,
              "later same-size submissions cannot overwrite a leased AO output");
        held.lifetime.reset();
        lifetimeAo.ReleaseCompletedThrough(lifetimeAo.RecordedSerial());
        gpu.commands->begin();
        lifeInput.color = color.get();
        auto recycled = lifetimeAo.Record(gpu.commands.get(), lifeInput);
        Check(recycled.color == heldTexture,
              "completed AO output can be reused after the consumer releases its lease");
        submitLifetime();
        std::printf("PASS: synthetic AO %s GPU readback; no game-scene acceptance\n",
                    vulkan ? "Vulkan" : "D3D12");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
}
