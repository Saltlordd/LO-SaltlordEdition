#pragma once
#include <cstdint>
#include <filesystem>
#include <string>

namespace gpu::optiscaler {
struct LoadResult {
    bool requested = false;
    bool loaded = false; // DLL attachment only, not evidence of an active upscaler.
    uint32_t systemError = 0;
    std::filesystem::path path;
    std::string reason;
};

// Call before SDL/Vulkan/DXGI device creation. First-call result is immutable.
// Hooks remain resident until process exit, including across GPU recreation.
// Explicit LO_FG_PROVIDER=off keeps the game's FG swapchain owners inactive.
const LoadResult& Initialize(bool ngxCompiled);
} // namespace gpu::optiscaler
