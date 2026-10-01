#pragma once
#include "../settings/config.h"
#include "vrr_policy.h"
#include "../../shared/frame_generation/environment.h"
#include <string>

namespace gpu::frame_generation {
// Build availability only. The SDK still checks the actual adapter and runtime.
inline constexpr bool D3D12CompiledProvider(framegen::Provider provider) {
#if defined(_WIN32) && defined(LO_ENABLE_D3D12_FG)
#ifdef FRAMEGEN_WITH_DLSS
    if (provider == framegen::Provider::Dlss) return true;
#endif
#ifdef FRAMEGEN_WITH_FSR
    if (provider == framegen::Provider::Fsr) return true;
#endif
#endif
    (void)provider;
    return false;
}

inline constexpr bool VulkanCompiledProvider(framegen::Provider provider) {
#if defined(_WIN32) && defined(LO_ENABLE_STREAMLINE_FG)
    if (provider == framegen::Provider::Dlss) return true;
#endif
#if defined(_WIN32) && defined(LO_ENABLE_VULKAN_FSR_FG)
    if (provider == framegen::Provider::Fsr) return true;
#endif
    (void)provider;
    return false;
}

inline constexpr bool CompiledProvider(settings::GraphicsBackend backend, framegen::Provider provider) {
#if defined(__APPLE__) && defined(LO_ENABLE_METALFX_FG)
    if (backend == settings::GraphicsBackend::Metal && provider == framegen::Provider::MetalFx) return true;
#endif
    return backend == settings::GraphicsBackend::Vulkan ? VulkanCompiledProvider(provider) :
        backend == settings::GraphicsBackend::D3D12 && D3D12CompiledProvider(provider);
}

// Streamline 2.14.1 supports fixed MFG on Vulkan; dynamic MFG is D3D12-only.
// Hardware limits are checked separately against slDLSSGGetState, never clamped.
inline const char* VulkanRequestError(const framegen::Config& config) {
    if (config.provider == framegen::Provider::Off || config.mode == framegen::Mode::Off) return nullptr;
    if (config.provider == framegen::Provider::Fsr)
        return config.mode == framegen::Mode::Fixed && config.generatedFrames == 1 ? nullptr : "Vulkan FSR FG requires fixed 2x";
    if (config.provider != framegen::Provider::Dlss) return "unknown Vulkan FG provider";
    if (config.mode != framegen::Mode::Fixed) return "Vulkan DLSS FG requires fixed mode; dynamic MFG is D3D12-only";
    if (!config.generatedFrames || config.generatedFrames >= framegen::kMaxMultiplier)
        return "Vulkan DLSS FG multiplier must be from 2 to 6";
    return nullptr;
}

// Explicit LO_FG_PROVIDER (or legacy LO_DLSS_FG) keeps its existing whole-request
// defaults. Individual LO_FG_* switches otherwise override persisted values.
inline framegen::EnvironmentSelection ResolveSelection(settings::GraphicsBackend backend, const settings::Config& saved,
    const char* provider, const char* mode, const char* multiplier,
    const char* targetFps, const char* legacyDlss, uint32_t refreshHz = 0) {
    const bool providerOverride = provider || legacyDlss;
    const char* savedProvider = saved.frameGenerationProvider == framegen::Provider::Dlss ? "dlss" :
        saved.frameGenerationProvider == framegen::Provider::Fsr ? "fsr" :
        saved.frameGenerationProvider == framegen::Provider::MetalFx ? "metalfx" : "off";
    const char* savedMode = saved.frameGenerationMode == framegen::Mode::Off ? "off" :
        saved.frameGenerationMode == framegen::Mode::Dynamic ? "dynamic" : "fixed";
    const auto savedMultiplier = std::to_string(saved.frameGenerationMultiplier);
    const auto savedTargetFps = std::to_string(saved.frameGenerationTargetFps);
    auto selection = framegen::ParseEnvironment(providerOverride ? provider : savedProvider,
        mode ? mode : (providerOverride ? nullptr : savedMode),
        multiplier ? multiplier : (providerOverride ? nullptr : savedMultiplier.c_str()),
        targetFps ? targetFps : (providerOverride ? nullptr : savedTargetFps.c_str()), legacyDlss);
    if (selection.Enabled() && backend == settings::GraphicsBackend::Vulkan)
        selection.error = VulkanRequestError(selection.config);
    if (selection.Enabled() && !CompiledProvider(backend, selection.config.provider))
        selection.error = "selected FG provider was not compiled for this backend/platform";
    if (selection.Enabled() && selection.config.mode == framegen::Mode::Dynamic)
        selection.config.targetFrameRate = vrr::DynamicTarget(selection.config.targetFrameRate,
            saved.variableRefreshRate, refreshHz);
    return selection;
}

inline framegen::EnvironmentSelection ResolveD3D12Selection(const settings::Config& saved,
    const char* provider, const char* mode, const char* multiplier,
    const char* targetFps, const char* legacyDlss, uint32_t refreshHz = 0) {
    return ResolveSelection(settings::GraphicsBackend::D3D12, saved,
        provider, mode, multiplier, targetFps, legacyDlss, refreshHz);
}

inline framegen::EnvironmentSelection ResolveVulkanSelection(const settings::Config& saved,
    const char* provider, const char* mode, const char* multiplier,
    const char* targetFps, const char* legacyDlss) {
    return ResolveSelection(settings::GraphicsBackend::Vulkan, saved,
        provider, mode, multiplier, targetFps, legacyDlss);
}
} // namespace gpu::frame_generation
