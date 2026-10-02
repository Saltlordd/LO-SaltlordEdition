#pragma once

#include <string>
#include <string_view>

namespace gpu::hdr_scene {

// The verified Lost Odyssey tone-map shader encodes display gamma after this
// saturation. Retain its exposure, DoF, bloom and fade, but let values above
// SDR white reach an FP16 attachment. Fail closed if translation changes.
inline bool RewriteTonemap(std::string& hlsl)
{
    constexpr std::string_view compressed =
        "xePV.xyz = saturate(r1.xyz * XeConst(9).www + r0.xyz);";
    constexpr std::string_view expanded =
        "xePV.xyz = max(r1.xyz * XeConst(9).www + r0.xyz, 0.0);";
    constexpr std::string_view finalClamp =
        "oC0 = clamp(oC0, ((xeFlags & 32u) != 0u) ? -xeColorMax : float4(0.0, 0.0, 0.0, 0.0), xeColorMax);";
    constexpr std::string_view fp16Clamp =
        "oC0.rgb = clamp(oC0.rgb, 0.0, 65504.0); oC0.a = saturate(oC0.a);";
    const auto first = hlsl.find(compressed);
    const auto last = hlsl.find(finalClamp);
    if (first == std::string::npos || last == std::string::npos ||
        hlsl.find(compressed, first + compressed.size()) != std::string::npos ||
        hlsl.find(finalClamp, last + finalClamp.size()) != std::string::npos)
        return false;
    hlsl.replace(last, finalClamp.size(), fp16Clamp);
    hlsl.replace(first, compressed.size(), expanded);
    return true;
}

} // namespace gpu::hdr_scene
