#include <gpu/fsr_upscaler.h>
#include <plume_d3d12.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <memory>
#include <stdexcept>

namespace plume { std::unique_ptr<RenderInterface> CreateD3D12Interface(); }

namespace {
using namespace plume;
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void Clear(RenderCommandList& commands, RenderFramebuffer& framebuffer,
    RenderTexture& texture, RenderColor value) {
    commands.barriers(RenderBarrierStage::GRAPHICS,
        RenderTextureBarrier(&texture, RenderTextureLayout::COLOR_WRITE));
    commands.setFramebuffer(&framebuffer);
    commands.clearColor(0, value);
}
}

int main() {
    try {
        auto api = plume::CreateD3D12Interface();
        Check(bool(api), "D3D12 interface");
        auto device = api->createDevice();
        Check(bool(device), "D3D12 device");
        auto& native = *static_cast<D3D12Device*>(device.get());
        auto queue = device->createCommandQueue(RenderCommandListType::DIRECT);
        auto prefix = queue ? queue->createCommandList() : nullptr;
        auto isolated = queue ? queue->createCommandList() : nullptr;
        auto continuation = queue ? queue->createCommandList() : nullptr;
        auto fence = device->createCommandFence();
        Check(queue && prefix && isolated && continuation && fence, "D3D12 queue resources");
        gpu::fsr::Controller controller;
        struct Drain {
            gpu::fsr::Controller& controller;
            ~Drain() { controller.ShutdownAfterGpuDrain(); }
        } drain{controller};
        uint64_t serial = 0;
        for (const auto quality : {gpu::upscaling::FsrQuality::NativeAA,
                                   gpu::upscaling::FsrQuality::Quality}) {
            constexpr uint32_t render = 64;
            const uint32_t outputSize = quality == gpu::upscaling::FsrQuality::NativeAA ? 64 : 96;
            auto color = device->createTexture(RenderTextureDesc::Texture2D(render, render, 1,
                RenderFormat::R8G8B8A8_UNORM, RenderTextureFlag::RENDER_TARGET));
            auto depth = device->createTexture(RenderTextureDesc::Texture2D(render, render, 1,
                RenderFormat::R32_FLOAT, RenderTextureFlag::RENDER_TARGET));
            auto motion = device->createTexture(RenderTextureDesc::Texture2D(render, render, 1,
                RenderFormat::R16G16_FLOAT, RenderTextureFlag::RENDER_TARGET));
            auto invalidity = device->createTexture(RenderTextureDesc::Texture2D(render, render, 1,
                RenderFormat::R8_UNORM, RenderTextureFlag::RENDER_TARGET));
            auto mask = device->createTexture(RenderTextureDesc::Texture2D(render, render, 1,
                RenderFormat::R8_UNORM, RenderTextureFlag::RENDER_TARGET));
            auto output = device->createTexture(RenderTextureDesc::Texture2D(outputSize, outputSize, 1,
                RenderFormat::R16G16B16A16_FLOAT, RenderTextureFlag::UNORDERED_ACCESS));
            const uint32_t pitchPixels = ((outputSize * 8 + 255) / 256) * 32;
            auto readback = device->createBuffer(RenderBufferDesc::ReadbackBuffer(
                uint64_t(pitchPixels) * outputSize * 8));
            Check(color && depth && motion && invalidity && mask && output && readback,
                "D3D12 FSR textures");
            const RenderTexture* colorAttachment[] = {color.get()};
            const RenderTexture* depthAttachment[] = {depth.get()};
            const RenderTexture* motionAttachment[] = {motion.get()};
            const RenderTexture* invalidityAttachment[] = {invalidity.get()};
            const RenderTexture* maskAttachment[] = {mask.get()};
            auto colorFb = device->createFramebuffer(RenderFramebufferDesc(colorAttachment, 1));
            auto depthFb = device->createFramebuffer(RenderFramebufferDesc(depthAttachment, 1));
            auto motionFb = device->createFramebuffer(RenderFramebufferDesc(motionAttachment, 1));
            auto invalidityFb = device->createFramebuffer(RenderFramebufferDesc(invalidityAttachment, 1));
            auto maskFb = device->createFramebuffer(RenderFramebufferDesc(maskAttachment, 1));
            Check(colorFb && depthFb && motionFb && invalidityFb && maskFb,
                "D3D12 FSR framebuffers");
            gpu::fsr::Config config{render, render, outputSize, outputSize, quality, 1};
            Check(controller.EnsureSession(native, config) == gpu::fsr::Status::Ready,
                "D3D12 FSR context ready");
            gpu::temporal::TemporalFrameInputs inputs{};
            inputs.plan.consumer = gpu::upscaling::TemporalConsumer::FsrSr;
            inputs.plan.requestedUpscaler = gpu::upscaling::Upscaler::Fsr;
            inputs.plan.deviceEpoch = inputs.plan.geometryEpoch = inputs.temporalEpoch = 1;
            inputs.plan.width = inputs.plan.height = render;
            inputs.renderFrameId = ++serial;
            inputs.color = {color.get(), {render, render}, 0, 0, render, render};
            inputs.depth = {depth.get(), {render, render}, 0, 0, render, render};
            inputs.motion = {motion.get(), {render, render}, 0, 0, render, render};
            inputs.motionInvalidity = {invalidity.get(), {render, render}, 0, 0, render, render};
            inputs.currentInputsComplete = true;
            inputs.colorEncoding = gpu::temporal::ColorEncoding::Sdr;
            inputs.depthConvention = gpu::temporal::DepthConvention::Reversed;
            inputs.motionState = quality == gpu::upscaling::FsrQuality::Quality ?
                gpu::temporal::MotionState::Hybrid : gpu::temporal::MotionState::Tracked;
            inputs.resetHistory = true;
            if (inputs.motionState == gpu::temporal::MotionState::Hybrid) {
                inputs.colorOrdinal = 10;
                inputs.fsrMask = {{mask.get(), {render, render}, 0, 0, render, render},
                    {inputs.renderFrameId, inputs.temporalEpoch, inputs.plan.geometryEpoch,
                     inputs.plan.deviceEpoch, inputs.colorOrdinal, 11, 12, color.get()},
                    gpu::temporal::FsrMaskSemantic::ConservativeTransparentAlpha,
                    gpu::temporal::FsrMaskCoverage::Partial};
            }
            gpu::fsr::FrameMetadata frame{};
            frame.cameraValid = true;
            frame.cameraNear = FLT_MAX;
            frame.cameraFar = 10.0f;
            frame.verticalFovRadians = 0.7f;
            frame.viewSpaceToMetersFactor = 1.0f;
            frame.frameTimeDeltaMilliseconds = 0.0f;
            frame.depthScale = 1.0f;
            frame.depthBias = 0.0f;
            prefix->begin();
            Clear(*prefix, *colorFb, *color, RenderColor(.4f, .2f, .1f, .75f));
            Clear(*prefix, *depthFb, *depth, RenderColor(.5f, 0, 0, 0));
            Clear(*prefix, *motionFb, *motion, RenderColor(0, 0, 0, 0));
            Clear(*prefix, *invalidityFb, *invalidity, RenderColor(0, 0, 0, 0));
            Clear(*prefix, *maskFb, *mask, RenderColor(.25f, 0, 0, 0));
            RenderTextureBarrier reads[] = {
                {color.get(), RenderTextureLayout::SHADER_READ},
                {depth.get(), RenderTextureLayout::SHADER_READ},
                {motion.get(), RenderTextureLayout::SHADER_READ},
                {invalidity.get(), RenderTextureLayout::SHADER_READ},
                {mask.get(), RenderTextureLayout::SHADER_READ}};
            prefix->barriers(RenderBarrierStage::ALL, nullptr, 0, reads, 5);
            prefix->barriers(RenderBarrierStage::ALL,
                RenderTextureBarrier(output.get(), RenderTextureLayout::GENERAL));
            prefix->end();
            const auto attempt = controller.RecordIsolated(*static_cast<D3D12CommandList*>(isolated.get()),
                config, inputs, frame, *static_cast<D3D12Texture*>(output.get()));
            Check(attempt.status == gpu::fsr::Status::Ready && attempt.useId,
                "D3D12 FSR dispatch recorded");
            continuation->begin();
            continuation->barriers(RenderBarrierStage::COPY,
                RenderTextureBarrier(output.get(), RenderTextureLayout::COPY_SOURCE));
            continuation->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(readback.get(),
                RenderFormat::R16G16B16A16_FLOAT, outputSize, outputSize, 1, pitchPixels),
                RenderTextureCopyLocation::Subresource(output.get()));
            continuation->end();
            const RenderCommandList* lists[] = {prefix.get(), isolated.get(), continuation.get()};
            auto& nativeFence = *static_cast<D3D12CommandFence*>(fence.get());
            const UINT64 expectedFence = nativeFence.fenceValue;
            queue->executeCommandLists(lists, 3, nullptr, 0, nullptr, 0, fence.get());
            controller.OnBatchSubmitted(attempt.useId, serial);
            queue->waitForCommandFence(fence.get());
            Check(nativeFence.d3d && nativeFence.d3d->GetCompletedValue() >= expectedFence,
                "D3D12 FSR output fence");
            controller.ReleaseCompletedThrough(serial);
            const auto* halves = static_cast<const uint16_t*>(readback->map());
            Check(halves != nullptr, "D3D12 FSR readback");
            const uint32_t center = outputSize / 2;
            const size_t offset = (size_t(center) * pitchPixels + center) * 4;
            // The output is linear RGB16F; gamma-encode on the CPU to compare against the 8-bit expectation.
            uint8_t pixels[3];
            for (int c = 0; c < 3; ++c) {
                const uint16_t h = halves[offset + c];
                const int exponent = (h >> 10) & 31;
                const float magnitude = exponent == 0 ? std::ldexp(float(h & 1023), -24) :
                    exponent == 31 ? 1.0f : std::ldexp(float(1024 | (h & 1023)), exponent - 25);
                const float linear = (h & 0x8000) ? 0.0f : std::min(magnitude, 1.0f);
                pixels[c] = uint8_t(std::pow(linear, 1.0f / 2.2f) * 255.0f + .5f);
            }
            std::printf("D3D12_FSR mode=%s output=%ux%u center=%u,%u,%u\n",
                quality == gpu::upscaling::FsrQuality::NativeAA ? "nativeaa" : "quality",
                outputSize, outputSize, pixels[0], pixels[1], pixels[2]);
            Check(pixels[0] > 50 && pixels[0] < 180 &&
                  pixels[1] > 20 && pixels[1] < 150 &&
                  pixels[2] > 10 && pixels[2] < 120, "D3D12 FSR produced expected color");
            readback->unmap();
            controller.ReleaseFeatureAfterGpuDrain();
        }
        std::puts("D3D12 FSR NativeAA and Quality executed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
