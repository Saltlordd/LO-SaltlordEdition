#pragma once

#include <plume_render_interface.h>

namespace gpu::shader
{
// Devices that consume the runtime's SPIR-V output and its Vulkan descriptor
// layout: Vulkan natively, and Metal through plume's SPIR-V to MSL translation.
// Everything else (D3D12) takes DXIL.
inline bool UsesSpirv(const plume::RenderDevice* device)
{
    const auto format = device->getCapabilities().shaderFormat;
    return format == plume::RenderShaderFormat::SPIRV || format == plume::RenderShaderFormat::METAL;
}
}
