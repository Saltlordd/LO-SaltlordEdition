#pragma once
#include "temporal_math.h"
#include <cstdint>
#include <memory>
#include <string>

namespace plume { struct RenderDevice; struct RenderCommandList; struct RenderTexture; }
namespace gpu::ao {
enum class Mode : uint32_t { Off, Ssao, Gtao };
enum class Debug : uint32_t { None, Visibility, Normals, Depth };
struct Inputs {
    // Full, matching current-frame images; depth is R32_FLOAT reversed Z (0 sky).
    // Color is RGBA8, optional HDR companion is extended-gamma RGBA16_FLOAT.
    // Caller keeps sampled images alive through the submission fence.
    plume::RenderTexture* color = nullptr;
    plume::RenderTexture* depth = nullptr;
    plume::RenderTexture* hdrColor = nullptr;
    const temporal::Camera* camera = nullptr;
    uint32_t width = 0, height = 0;
    Mode mode = Mode::Off;
    Debug debug = Debug::None;
    double jitterX = 0, jitterY = 0;
    float radius = 40; // Guest world units, not calibrated metres.
    float strength = .6f;
};
struct Output {
    plume::RenderTexture* color = nullptr;
    plume::RenderTexture* hdrColor = nullptr;
    // Retain on every submission consuming this output, including after a Flush.
    // Retirement also requires the original production serial to have completed.
    std::shared_ptr<void> lifetime;
};
class AmbientOcclusion {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    AmbientOcclusion();
    ~AmbientOcclusion();
    bool Init(plume::RenderDevice*);
    // Inputs must be SHADER_READ. Returned images are SHADER_READ, do not alias
    // any input, and remain owned until ReleaseCompletedThrough. Unsupported
    // projection/invalid inputs return an empty result without recording draws.
    // No history, jitter generation, readback, queue submit, or GPU wait.
    Output Record(plume::RenderCommandList*, const Inputs&);
    uint64_t RecordedSerial() const;
    // Call only after the submission using this serial (and earlier ones) ends.
    void ReleaseCompletedThrough(uint64_t);
    const std::string& LastError() const;
};
}
