#include "gpu/frame_generation_settings.h"
#include <cstdio>
#include <cstdlib>

namespace {
unsigned checks = 0;
void Check(bool value, const char* message) {
    ++checks;
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
}

int main() {
    using namespace framegen;
    using namespace gpu::frame_generation;
    settings::Config saved;
    saved.graphicsBackend = settings::GraphicsBackend::Vulkan;
    Check(!ResolveVulkanSelection(saved, nullptr, nullptr, nullptr, nullptr, nullptr).Enabled(), "default off");
    saved.frameGenerationProvider = Provider::Dlss;
    saved.frameGenerationMultiplier = 4;
    const auto resolve = [&](const char* provider = nullptr, const char* mode = nullptr,
                             const char* multiplier = nullptr, const char* fps = nullptr, const char* legacy = nullptr) {
        return ResolveVulkanSelection(saved, provider, mode, multiplier, fps, legacy);
    };
    const bool compiled = VulkanCompiledProvider(Provider::Dlss);
#if defined(_WIN32) && defined(LO_ENABLE_STREAMLINE_FG)
    Check(compiled, "Windows Vulkan DLSS build enabled");
#else
    Check(!compiled, "uncompiled or non-Windows Vulkan provider stays unavailable");
#endif
    const bool fsrCompiled = VulkanCompiledProvider(Provider::Fsr);
#if defined(_WIN32) && defined(LO_ENABLE_VULKAN_FSR_FG)
    Check(fsrCompiled, "Windows Vulkan FSR FG build enabled independently of SR");
#else
    Check(!fsrCompiled, "FSR SR build never implies Vulkan FSR FG");
#endif
    Check(!VulkanCompiledProvider(Provider::Off), "Off is not an SDK provider");
    Check(CompiledProvider(saved.graphicsBackend, Provider::Dlss) == compiled, "menu uses backend availability");
    auto selection = resolve();
    Check(selection.Enabled() == compiled && selection.config.generatedFrames == 3, "saved Vulkan 4x request");
    Check(!resolve("off", nullptr, nullptr, nullptr, "1").Enabled(), "explicit Off beats legacy and saved On");
    Check(!resolve(nullptr, nullptr, nullptr, nullptr, "0").Enabled(), "legacy Off beats saved On");
    selection = resolve(nullptr, nullptr, nullptr, nullptr, "1");
    Check(selection.Enabled() == compiled && selection.config.generatedFrames == 1, "legacy On remains fixed 2x");
    selection = resolve("dlss");
    Check(selection.config.generatedFrames == 1, "explicit provider keeps whole-request defaults");
    Check(!resolve(nullptr, "dynamic").Enabled(), "Vulkan dynamic MFG rejected");
    Check(resolve("fsr", "fixed", "2").Enabled() == fsrCompiled, "Vulkan FSR FG 2x uses its own build flag");
    Check(!resolve("fsr", "dynamic", "2").Enabled(), "FSR dynamic mode rejected");
    Check(!resolve("fsr", "fixed", "3").Enabled(), "FSR multi-frame request rejected");
    for (const char* bad : {"0", "1", "7", "-2", "2.5", "garbage"})
        Check(!resolve(nullptr, nullptr, bad).Enabled(), "invalid multiplier rejected");
    for (const char* bad : {"-1", "nan", "inf"})
        Check(!resolve(nullptr, nullptr, nullptr, bad).Enabled(), "invalid target rejected");
    for (uint32_t multiplier = 2; multiplier <= kMaxMultiplier; ++multiplier) {
        saved.frameGenerationMultiplier = multiplier;
        selection = resolve();
        Check(selection.Enabled() == compiled && !VulkanRequestError(selection.config), "fixed 2x through 6x admitted by policy");
        Check(Select(selection.config, {true, multiplier - 1, false}).Enabled(), "exact hardware limit admitted");
        Check(!Select(selection.config, {true, multiplier - 2, false}).Enabled(), "above hardware limit rejected without clamping");
        Check(!Select(selection.config, {}).Enabled(), "missing runtime capability never enables FG");
    }
    saved.variableRefreshRate = true;
    saved.frameGenerationTargetFps = 240;
    saved.frameGenerationMode = Mode::Dynamic;
    Check(!resolve().Enabled(), "VRR cannot make dynamic Vulkan MFG supported");
    saved.frameGenerationMode = Mode::Off;
    Check(!resolve().Enabled() && !resolve().error, "persisted Off does not load runtime");
    selection = resolve(nullptr, "fixed", "2");
    Check(selection.Enabled() == compiled && saved.frameGenerationMode == Mode::Off, "override does not mutate saved settings");
    std::printf("PASS Vulkan FG selection: %u checks (compiled=%d)\n", checks, compiled);
}
