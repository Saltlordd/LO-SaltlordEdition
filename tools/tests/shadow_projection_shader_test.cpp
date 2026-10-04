// Translate and compile the UE3 deferred shadow projection pixel shader (PS
// d55a20d004031279, captured in Uhra) with its screen position rebuilt from
// SV_Position; no GPU or game data required.
#include <gpu/shader/xenos_translator.h>
#include <gpu/shader/xenos_shader_code.h>
#include <gpu/shader/dxc_compiler.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

// Guest microcode words in host order (the runtime byte-swaps guest memory).
static constexpr uint32_t kShadowProjectionPs[] = {
    0x02406006, 0x600c1200, 0x12000000, 0x04006012, 0x60181200, 0x12000095, 0x0000601e, 0x60241000,
    0x56000500, 0x0555602a, 0x60305600, 0x56000255, 0x00006036, 0x403c5600, 0x56000000, 0x00000000,
    0x4040c400, 0x22000000, 0x4c100100, 0x0000001b, 0xe2000000, 0xc8060000, 0x006cbc00, 0xc1010000,
    0xc8090000, 0x00b26d1a, 0x8b000000, 0x0c000001, 0x1f1ffff8, 0x00004000, 0xc8010000, 0x016cc61b,
    0x8b000101, 0x4c100000, 0x0000006c, 0xe2000000, 0xc8030001, 0x00c56c00, 0xc1000000, 0xc80f0000,
    0x006caaaa, 0x8b000405, 0xc80f0000, 0x00b1a70d, 0xab010300, 0xc80f0002, 0x006caa0d, 0xab010200,
    0x4c180000, 0x006cc6b1, 0xa302fe02, 0xc8030000, 0x00c76c00, 0xc1020000, 0xc80c0001, 0x00461b6c,
    0x2b0dfe00, 0xb0160103, 0x00b166c1, 0x80000c0d, 0xc8090003, 0x00c41b6c, 0x2b0cfe00, 0xb02f0103,
    0x00941141, 0x80030c0d, 0xc80f0004, 0x0034bb00, 0x80010d00, 0x10181081, 0x1f1ffff8, 0x00004000,
    0xb8181081, 0x1f1fffc7, 0x00004000, 0xec181061, 0x1f1ffe3f, 0x00004000, 0x44181061, 0x1f1ff1ff,
    0x00004000, 0xc80f0001, 0x00001b00, 0xc5010000, 0xc80f0001, 0x00e96c00, 0x8701fe00, 0xc8040000,
    0x0094b100, 0x8f01fd00, 0xc8020001, 0x00c6c600, 0x8500fd00, 0xc8010001, 0x00b1c600, 0x45fe0000,
    0xc8010001, 0x00b16c00, 0xc1010100, 0x70000000, 0x0000006c, 0xe2000001, 0xc80f0004, 0x18a07700,
    0x80000b00, 0xc80f0005, 0x18a07700, 0x80000a00, 0xc80f0003, 0x18a07700, 0x80000700, 0xc80f0006,
    0x18a07700, 0x80000600, 0xc80f0001, 0x18a07700, 0x80000900, 0xc80f0007, 0x18a07700, 0x80000800,
    0xec1820e1, 0x8f1ffff8, 0x80004000, 0x441820e1, 0x8f1fffc7, 0x80004000, 0xec182021, 0x8f1ffe3f,
    0x80004000, 0x44182021, 0x8f1ff1ff, 0x80004000, 0xec1810c1, 0x8f1ffff8, 0x80004000, 0x441810c1,
    0x8f1fffc7, 0x80004000, 0xec181061, 0x8f1ffe3f, 0x80004000, 0x44181061, 0x8f1ff1ff, 0x80004000,
    0xec1830a1, 0x8f1ffff8, 0x80004000, 0x441830a1, 0x8f1fffc7, 0x80004000, 0xec183081, 0x8f1ffe3f,
    0x80004000, 0x44183081, 0x8f1ff1ff, 0x80004000, 0xc80f0004, 0x18001b00, 0xc5030000, 0xc80f0001,
    0x18001b00, 0xc5010000, 0xc80f0002, 0x18001b00, 0xc5020000, 0x281f0103, 0x18006c6c, 0xa701fe02,
    0x282f0104, 0x18006cb1, 0xa704fe02, 0x28420100, 0x18a71bc6, 0xaf04fd02, 0x28880100, 0x18a71b1b,
    0xaf03fd02, 0xc8010000, 0x18a71b00, 0x8f01fd00, 0xc8010000, 0x181b6c00, 0xc0000000, 0xc8010000,
    0x186cb100, 0xc0000000, 0xc8010000, 0x1a6cc600, 0xc0000000, 0xc8040000, 0x186c6cc6, 0xab00fd00,
    0x08100000, 0x000000c6, 0xe2000000, 0xc8020000, 0x006cc600, 0xc1000000, 0xb8100000, 0x00000041,
    0xc20000ff, 0xc80f8000, 0x006c6cb1, 0xab000e00, 0x00000000, 0x00000000, 0x00000000,
};

static void Require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

int main()
{
    try {
        using namespace xenos;
        const auto shadow = TranslateShader(kShadowProjectionPs, uint32_t(std::size(kShadowProjectionPs)), true);
        Require(shadow.errors.empty(), "shadow projection translation failed");
        const auto& hlsl = shadow.hlsl;
        const auto divide = hlsl.find("\tps = clamp(rcp(r0.w), FLT_MIN, FLT_MAX);");
        const auto rebuild = hlsl.find("float2 xeScreenUv = iPos.xy / float2(xeScreenDims);");
        const auto scaleBias = hlsl.find("\txePV.xw = r0.zy * XeConst(0).yx + XeConst(0).zw;");
        Require(divide != std::string::npos && rebuild != std::string::npos && scaleBias != std::string::npos,
            "screen position is not rebuilt from SV_Position");
        Require(divide < rebuild && rebuild < scaleBias, "rebuild is not between the divide and the depth UV");
        Require(hlsl.find("tex2D_0.GetDimensions(xeScreenDims.x, xeScreenDims.y);") != std::string::npos,
            "rebuild does not use the host scene-depth size");
        Require(hlsl.find("all(iPos.xy < float2(xeScreenDims))") != std::string::npos &&
            hlsl.find("<= 0.0625))") != std::string::npos, "rebuild has no texture-bounds and mapping guard");
        Require(hlsl.find("float2 xeScreenUv", rebuild + 1) == std::string::npos, "rebuild inserted more than once");
        for (auto format : {ShaderBinaryFormat::Dxil, ShaderBinaryFormat::Spirv}) {
            const auto compiled = CompileHlsl(hlsl, "main", "ps_6_0", format);
            if (!compiled.ok) throw std::runtime_error(compiled.errors);
        }

        // A pixel shader without the screen position prologue stays untouched.
        std::array<uint32_t, 6> code{};
        ControlFlowExecInstruction cf{};
        cf.address = 1; cf.count = 1; cf.sequence = 1; cf.opcode = ControlFlowOpcode::ExecEnd;
        std::memcpy(code.data(), &cf, 6);
        TextureFetchInstruction fetch{};
        fetch.opcode = FetchOpcode::TextureFetch;
        fetch.dimension = TextureDimension::Texture2D;
        fetch.dstSwizzle = 0x688; fetch.srcSwizzle = 4;
        std::memcpy(code.data() + 3, &fetch, 12);
        const auto plain = TranslateShader(code.data(), uint32_t(code.size()), true);
        Require(plain.errors.empty(), "plain translation failed");
        Require(plain.hlsl.find("xeScreenDims") == std::string::npos, "unrelated pixel shader was rewritten");
        std::puts("shadow projection shader checks passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
