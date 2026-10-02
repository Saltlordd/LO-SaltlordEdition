#include <gpu/hdr_scene_shader.h>

#include <fstream>
#include <iterator>
#include <string>

int main(int argc, char** argv)
{
    std::string source =
        "xePV.xyz = saturate(r1.xyz * XeConst(9).www + r0.xyz);\n"
        "ps = clamp(log2(r1.x), FLT_MIN, FLT_MAX);\n"
        "oC0 = clamp(oC0, ((xeFlags & 32u) != 0u) ? -xeColorMax : float4(0.0, 0.0, 0.0, 0.0), xeColorMax);\n";
    if (!gpu::hdr_scene::RewriteTonemap(source)) return 1;
    if (source.find("xePV.xyz = max(r1.xyz * XeConst(9).www + r0.xyz, 0.0);") == std::string::npos ||
        source.find("ps = clamp(log2(r1.x), FLT_MIN, FLT_MAX);") == std::string::npos ||
        source.find("oC0.rgb = clamp(oC0.rgb, 0.0, 65504.0); oC0.a = saturate(oC0.a);") == std::string::npos)
        return 2;
    const auto rewritten = source;
    if (gpu::hdr_scene::RewriteTonemap(source) || source != rewritten) return 3;

    std::string ambiguous =
        "xePV.xyz = saturate(r1.xyz * XeConst(9).www + r0.xyz);\n"
        "xePV.xyz = saturate(r1.xyz * XeConst(9).www + r0.xyz);\n"
        "oC0 = clamp(oC0, ((xeFlags & 32u) != 0u) ? -xeColorMax : float4(0.0, 0.0, 0.0, 0.0), xeColorMax);";
    const auto original = ambiguous;
    if (gpu::hdr_scene::RewriteTonemap(ambiguous) || ambiguous != original) return 4;

    // Optional local capture checks the real translated shader without making
    // generated evidence a build or CI dependency.
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file) return 5;
        std::string captured(std::istreambuf_iterator<char>{file}, {});
        if (!gpu::hdr_scene::RewriteTonemap(captured)) return 6;
    }
}
