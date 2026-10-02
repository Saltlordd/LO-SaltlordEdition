#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "../gpu/backend_selection.h"

namespace updater::shader_pack
{
struct StartupRequest
{
    gpu::backend::Backend configuredBackend = gpu::backend::Backend::D3D12;
    uint32_t uiLanguage = 0;
    std::span<const uint8_t> unboundXex; // XexLoader::UnboundIdentityPrefix()
};

// Runs after the update check and before the renderer prepares shaders. When no
// installed pack matches the configured renderer, offers the published one and
// installs it. Never fails startup: the renderer compiles locally without it.
// Returns one line for the log.
std::string PrepareAtStartup(const StartupRequest &request);
}
