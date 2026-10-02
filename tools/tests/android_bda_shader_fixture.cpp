#include "LostOdysseyRecomp/gpu/shader/common_hlsl.h"

#include <iostream>

int main()
{
    // Emit the production prelude as C++ compiled it for Android. The vertex
    // entry point keeps the address load live in the generated SPIR-V.
    std::cout << "#define XE_SAMPLE(t, s, coordinates) t.Sample(s, coordinates)\n";
    std::cout << xenos::kShaderCommonHlsl;
    std::cout << R"HLSL(
void main(uint vertexId : SV_VertexID, out float4 position : SV_Position)
{
    uint offset = XeVfetchOffset(95u) + vertexId * 16u;
    position = XeVF_32_32_32_32_FLOAT(xeVertexArena, offset, true, false);
}
)HLSL";
}
