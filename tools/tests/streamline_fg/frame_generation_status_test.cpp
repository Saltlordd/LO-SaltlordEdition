#include "gpu/frame_generation_status.h"
#include <cstdio>
#include <stdexcept>

namespace {
unsigned checks = 0;
void Check(bool value, const char* reason) {
    ++checks;
    if (!value) throw std::runtime_error(reason);
}
}
int main() {
    try {
        using namespace gpu::video;
        using framegen::Provider;
        using Phase = FrameGenerationPhase;
        const framegen::Config off{};
        const framegen::Config dlss2{Provider::Dlss, framegen::Mode::Fixed, 1};
        const framegen::Config dlss4{Provider::Dlss, framegen::Mode::Fixed, 3};
        const framegen::Config fsr{Provider::Fsr, framegen::Mode::Fixed, 1};
        const auto vulkan = [](framegen::Config request) {
            FrameGenerationPhaseInputs in;
            in.backendSelected = true; in.providerFixedAtStartup = true; in.request = request;
            return in;
        };

        FrameGenerationPhaseInputs in;
        Check(DeriveFrameGenerationPhase(in) == Phase::Off, "no backend and FG off");
        in.request = dlss2;
        Check(DeriveFrameGenerationPhase(in) == Phase::Pending, "enabled request waits for backend selection");

        in = vulkan(dlss2);
        Check(DeriveFrameGenerationPhase(in) == Phase::RestartRequired, "Vulkan provider without a startup session restarts");
        in.startupFailure = Provider::Dlss;
        Check(DeriveFrameGenerationPhase(in) == Phase::Unavailable, "provider that failed at startup is not offered a restart");
        in.request = fsr;
        Check(DeriveFrameGenerationPhase(in) == Phase::RestartRequired, "a different provider was never tried");

        in = vulkan(dlss4);
        in.sessionProvider = Provider::Dlss; in.applied = dlss2;
        Check(DeriveFrameGenerationPhase(in) == Phase::Pending, "multiplier change applies in process");
        in.applied = dlss4;
        Check(DeriveFrameGenerationPhase(in) == Phase::Ready, "applied request with its session");
        in.failedRequest = dlss4;
        Check(DeriveFrameGenerationPhase(in) == Phase::Unavailable, "failed request is unavailable");

        in = vulkan(off);
        in.sessionProvider = Provider::Dlss;
        Check(DeriveFrameGenerationPhase(in) == Phase::Off, "FG off with a retained Vulkan session");
        in.failedRequest = off;
        Check(DeriveFrameGenerationPhase(in) == Phase::Off, "a failed Off request still reads Off");
        in.requestError = true;
        Check(DeriveFrameGenerationPhase(in) == Phase::Unavailable, "request error is unavailable");

        FrameGenerationPhaseInputs d3d12;
        d3d12.backendSelected = true; d3d12.request = fsr;
        d3d12.sessionProvider = Provider::Dlss; d3d12.applied = dlss2;
        d3d12.startupFailure = Provider::Fsr;
        Check(DeriveFrameGenerationPhase(d3d12) == Phase::Pending, "D3D12 switches providers in process");

        std::printf("FG status phase: %u checks passed\n", checks);
        return 0;
    } catch (const std::exception& e) { std::fprintf(stderr, "FAIL: %s\n", e.what()); return 1; }
}
