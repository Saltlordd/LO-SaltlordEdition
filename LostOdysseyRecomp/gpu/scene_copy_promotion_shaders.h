#pragma once

#include <string>

namespace gpu::scene_copy_promotion {
// Shared with the standalone GPU fixture: do not validate a copied substitute
// for the production shader. The constants match SharedConstants::transfer.
inline constexpr const char* VertexShader = R"HLSL(
void main(uint id : SV_VertexID, out float4 pos : SV_Position) {
    float2 uv = float2((id << 1) & 2, id & 2);
    pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}
)HLSL";
inline constexpr const char* Common = R"HLSL(
Texture2D<float4> base : register(t0, space1);
#ifdef __spirv__
struct XePushConstants { uint64_t vs; uint64_t sharedAddress; uint64_t ps; };
[[vk::push_constant]] ConstantBuffer<XePushConstants> xePush;
#define xePromotion vk::RawBufferLoad<uint4>(xePush.sharedAddress + 240)
#else
cbuffer XeShared : register(b1, space0) {
    uint4 pad0[2]; uint4 pad1[8]; float4 pad2; float4 pad3;
    float4 pad4; float4 pad5; float4 xeColorMax; uint4 xePromotion;
};
#endif
)HLSL";
inline const std::string RgbaShader = std::string(Common) + R"HLSL(
float4 main(float4 pos : SV_Position) : SV_Target {
    uint4 p = xePromotion;
    return base.Load(int3(int2(pos.xy * float2(asfloat(p.x), asfloat(p.y))), 0));
}
)HLSL";
// Drawn into the promoted target itself with an RGB-only write mask and a
// scissor limited to the SR output: the guest copy's alpha and the guest tile
// padding outside the SR output keep their values. xePromotion.z != 0 marks
// linear SR output (FSR), which is display-encoded here.
inline const std::string RgbShader = std::string(Common) + R"HLSL(
Texture2D<float4> sr : register(t1, space1);
float4 main(float4 pos : SV_Position) : SV_Target {
    float3 rgb = sr.Load(int3(int2(pos.xy), 0)).rgb;
    if (xePromotion.z != 0) rgb = pow(saturate(rgb), (1.0 / 2.2).xxx);
    return float4(rgb, 1.0);
}
)HLSL";
} // namespace gpu::scene_copy_promotion
