#include "gpu/shader/portable_shader_contract.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace pack = xenos::portable_pack;
namespace fs = std::filesystem;
using Bytes = std::vector<uint8_t>;
static void Write(const fs::path& file, const Bytes& bytes) {
    std::ofstream stream(file, std::ios::binary);
    stream.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
    if (!stream) throw std::runtime_error("fixture write failed");
}
// SPIR-V framing only. No claim of driver validation for this CPU fixture.
static Bytes Binary() {
    const uint32_t words[] = {0x07230203, 0x00010500, 0, 16, 0,
        (3u << 16) | 14, 0, 1, (5u << 16) | 15, 0, 1, 0x6e69616d, 0};
    Bytes bytes;
    for (auto word : words) for (unsigned i=0; i<4; ++i) bytes.push_back(uint8_t(word >> (8*i)));
    return bytes;
}
// DXIL container framing only, as in portable_shader_pack_test.cpp.
static Bytes Dxil() {
    const uint32_t words[] = {0x43425844, 0, 0, 0, 0, 0, 72, 1, 36,
        0x4c495844, 28, 0x10060, 7, 0x4c495844, 0x100, 16, 4, 0xdec04342};
    Bytes bytes;
    for (auto word : words) for (unsigned i=0; i<4; ++i) bytes.push_back(uint8_t(word >> (8*i)));
    return bytes;
}
int main(int argc, char** argv) try {
    if (argc != 2) throw std::runtime_error("expected fixture directory");
    const fs::path root = reinterpret_cast<const char8_t*>(argv[1]);
    fs::create_directories(root);
    Bytes image(pack::RuntimeXexBytes, 0);
    for (size_t i=0; i<image.size(); ++i) image[i] = uint8_t(i * 17);
    const auto identity = xenos::cache::MakeIdentity(xenos::cache::Backend::Vulkan, "");
    const auto contract = pack::RuntimeContract(image);
    const auto direct = pack::Contract(identity.translatorVersion, identity.options, identity.variant,
        xenos::kShaderCommonHlsl, xenos::resources::variants::DiscoveryIdentity, image);
    if (contract != direct) throw std::runtime_error("shared contract drift");
    const auto dxilContract = pack::RuntimeContract(image,
        xenos::cache::MakeIdentity(xenos::cache::Backend::D3D12, ""), pack::PackFormat::Dxil);
    if (dxilContract == contract) throw std::runtime_error("DXIL and SPIR-V contracts collide");
    bool shortRejected = false;
    try { (void)pack::RuntimeContract(Bytes(pack::RuntimeXexBytes - 1, 0)); }
    catch (const std::runtime_error&) { shortRejected = true; }
    if (!shortRejected) throw std::runtime_error("contract accepted a short executable prefix");
    Write(root / "image.bin", image);
    // A source whose renderer-byte FNV key is already in the synthetic pack,
    // allowing the merge CLI to exercise source/hash validation without DXC.
    Bytes source(12, 0);
    source[0]=1; source[4]=2;
    uint64_t sourceHash=0xcbf29ce484222325ull;
    for(auto byte:source) {sourceHash^=byte;sourceHash*=0x100000001b3ull;}
    Write(root / "source.bin", source);
    xenos::TranslatedShader shader;
    for (const auto* producer : {"matching", "explicit"}) {
        pack::Writer writer(root / (std::string(producer) + ".lospv"), contract, producer);
        writer.Add(1, shader, Binary()); writer.Add(sourceHash, shader, Binary()); writer.Finish();
    }
    { pack::Writer writer(root / "matching.lospd", dxilContract, "matching-dxil", pack::PackFormat::Dxil);
      writer.Add(1, shader, Dxil()); writer.Finish(); }
    image[600] ^= 1;
    Write(root / "wrong-image.bin", image);
    { pack::Writer writer(root / "wrong-contract.lospv", pack::RuntimeContract(image), "incompatible");
      writer.Add(1, shader, Binary()); writer.Finish(); }
    Write(root / "short-image.bin", Bytes{1,2,3});
    std::cout << "PASS shared runtime contract and packaging fixtures generated\n";
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
