#pragma once

#if defined(LO_GPU_PLUME)
#include "temporal_frame_inputs.h"
#include "upscaling_plan.h"

#include <cstdint>
#include <memory>
#include <optional>

namespace plume { struct D3D12CommandList; struct D3D12Device; struct D3D12Texture; }

// Intel XeSS Super Resolution on the D3D12 backend. libxess.dll is loaded at
// runtime; without it, or on other backends/platforms, the provider reports
// Unavailable and the frame plan keeps ordinary rendering.
namespace gpu::xess {

enum class Status : uint8_t { Ready, Unavailable, NeedsReconfigure, Failed, DeviceLost, InputUnavailable };

struct Config {
    uint32_t renderWidth = 0, renderHeight = 0;
    uint32_t outputWidth = 0, outputHeight = 0;
    // Persisted FSR quality IDs; each maps to the same-named XeSS preset.
    upscaling::FsrQuality quality = upscaling::FsrQuality::Quality;
    uint64_t deviceEpoch = 0;
    bool depthInverted = true;
    bool operator==(const Config&) const = default;
};

struct Attempt {
    Status status = Status::Unavailable;
    uint64_t useId = 0;
    std::optional<int32_t> sdkResult, hrResult;
};

// Owns the XeSS context. The caller owns command submission, its serial, the
// isolated command list and the output texture.
class Controller {
public:
    Controller();
    ~Controller();
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    upscaling::OutputSizing QuerySizing(const plume::D3D12Device& device, const upscaling::SizingKey& key);
    Status EnsureSession(plume::D3D12Device& device, const Config& config);
    Attempt RecordIsolated(plume::D3D12CommandList& commands, const Config& config,
        const temporal::TemporalFrameInputs& inputs, plume::D3D12Texture& output);
    void OnBatchSubmitted(uint64_t useId, uint64_t serial);
    void OnBatchDiscarded(uint64_t useId);
    void ReleaseCompletedThrough(uint64_t serial);
    bool HasFeatureState() const;
    void ReleaseFeatureAfterGpuDrain();
    void ShutdownAfterGpuDrain();
    void AbandonUsesAfterDeviceLoss();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gpu::xess
#endif
