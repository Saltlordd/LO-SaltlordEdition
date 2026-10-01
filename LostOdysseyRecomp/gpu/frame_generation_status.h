#pragma once
#include "../../shared/frame_generation/core.h"
#include <cstdint>
#include <optional>

namespace gpu::video {
enum class FrameGenerationPhase : uint8_t { Off, Pending, Ready, Unavailable, RestartRequired };

// Inputs of the menu-facing FG phase, captured under the settings lock.
struct FrameGenerationPhaseInputs {
    bool backendSelected = false;
    // Vulkan installs each SDK's WSI hooks at startup, so another provider
    // needs a restart; D3D12 and Metal switch providers in process.
    bool providerFixedAtStartup = false;
    framegen::Config request{};
    bool requestError = false;
    framegen::Config applied{};
    framegen::Provider sessionProvider = framegen::Provider::Off;
    std::optional<framegen::Config> failedRequest;
    // Provider whose session failed to start in this process. A restart cannot
    // help until something outside the game changes, so it is not offered.
    framegen::Provider startupFailure = framegen::Provider::Off;
};

inline FrameGenerationPhase DeriveFrameGenerationPhase(const FrameGenerationPhaseInputs& in) {
    using Phase = FrameGenerationPhase;
    const bool providerOff = in.request.provider == framegen::Provider::Off;
    const bool enabled = !providerOff && in.request.mode != framegen::Mode::Off;
    if (in.requestError || (in.failedRequest && *in.failedRequest == in.request))
        return providerOff && !in.requestError ? Phase::Off : Phase::Unavailable;
    if (in.providerFixedAtStartup && enabled && in.sessionProvider != in.request.provider)
        return in.request.provider == in.startupFailure ? Phase::Unavailable : Phase::RestartRequired;
    if (!in.backendSelected || in.request != in.applied)
        return providerOff && !in.backendSelected ? Phase::Off : Phase::Pending;
    return in.applied.provider == framegen::Provider::Off ? Phase::Off : Phase::Ready;
}
} // namespace gpu::video
