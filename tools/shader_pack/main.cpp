#include "gpu/shader/portable_shader_contract.h"
#include "gpu/shader/portable_shader_pack_location.h"
#include "merge.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>

// The runtime contract hashes the image as parsed, before import binding,
// which is what xexdump writes, so this is the value the game computes.
static std::vector<uint8_t> ReadRuntimeImage(const char* argument) {
    std::ifstream in(std::filesystem::path(reinterpret_cast<const char8_t*>(argument)), std::ios::binary);
    std::vector<uint8_t> image(xenos::portable_pack::RuntimeXexBytes);
    if (!in.read(reinterpret_cast<char*>(image.data()), std::streamsize(image.size())))
        throw std::runtime_error("missing/short decrypted runtime image (use xexdump output)");
    return image;
}

int main(int argc,char** argv) try {
    const auto command = argc > 1 ? std::string_view(argv[1]) : std::string_view{};
    if (command == "merge") return Merge(argc, argv);
    if (command == "contract" && (argc == 3 || (argc == 4 && std::string_view(argv[3]) == "--android"))) {
        const bool androidContract = argc == 4;
        if (androidContract && !xenos::VertexFetchUsesDeviceAddress)
            throw std::runtime_error("--android requires a pack tool built with LO_ANDROID_SHADER_PACK_HOST=ON");
        // The contract each renderer's startup download looks up in index.json.
        namespace pack = xenos::portable_pack;
        const auto image = ReadRuntimeImage(argv[2]);
        if (androidContract) {
            std::cout << "{\n  \"android-vulkan\": \""
                      << xenos::resources::Sha256Hex(pack::FlavorContract(image, pack::Flavor::Vulkan))
                      << "\"\n}\n";
            return 0;
        }
        const char* separator = "{\n";
        for (const auto flavor : {pack::Flavor::Vulkan, pack::Flavor::D3D12, pack::Flavor::Metal}) {
            std::cout << separator << "  \"" << pack::FlavorName(flavor) << "\": \""
                      << xenos::resources::Sha256Hex(pack::FlavorContract(image, flavor)) << "\"";
            separator = ",\n";
        }
        std::cout << "\n}\n";
        return 0;
    }
    const bool runtime = command == "verify-runtime";
    // Metal consumes SPIR-V compiled at -O1, a separate contract (cache::MetalOptions).
    const bool metal = runtime && argc == 5 && std::string_view(argv[4]) == "--metal";
    const bool android = runtime && argc == 5 && std::string_view(argv[4]) == "--android";
    if ((runtime ? argc != 4 && !metal && !android : argc != 3) ||
        (command != "inspect" && command != "verify" && !runtime)) {
        std::cerr << "Usage: LoShaderPackTool <inspect|verify> pack.lospv|pack.lospd\n"
                     "       LoShaderPackTool verify-runtime pack.lospv|pack.lospd xexdump-image.bin [--metal|--android]\n"
                     "       LoShaderPackTool contract xexdump-image.bin [--android]\n"
                     "       LoShaderPackTool merge baseline.lospv xexdump-image.bin manifest.tsv output-dir\n";
        return 2;
    }
    const bool verified = command != "inspect";
    if (android && !xenos::VertexFetchUsesDeviceAddress)
        throw std::runtime_error("--android requires a pack tool built with LO_ANDROID_SHADER_PACK_HOST=ON");
    const auto path = std::filesystem::path(reinterpret_cast<const char8_t*>(argv[2]));
    xenos::portable_pack::Report r;
    if (runtime) {
        const auto format = xenos::portable_pack::Reader::Inspect(path).format;
        const auto image = ReadRuntimeImage(argv[3]);
        const auto backend = format == xenos::portable_pack::PackFormat::Dxil ?
            xenos::cache::Backend::D3D12 : xenos::cache::Backend::Vulkan;
        if (metal && backend != xenos::cache::Backend::Vulkan)
            throw std::runtime_error("--metal applies to SPIR-V packs only");
        if (android && backend != xenos::cache::Backend::Vulkan)
            throw std::runtime_error("--android applies to SPIR-V packs only");
        auto identity = xenos::cache::MakeIdentity(backend, "");
        if (metal) identity.options = xenos::cache::MetalOptions();
        const auto expected = xenos::portable_pack::RuntimeContract(image, identity, format);
        xenos::portable_pack::Reader reader(path, expected, format);
        reader.VerifyAll();
        r = reader.Info();
    } else r = xenos::portable_pack::Reader::Inspect(path, verified);
    std::cout<<"{\n  \"schema\": "<<xenos::portable_pack::Schema
        <<",\n  \"format\": \""<<(r.format == xenos::portable_pack::PackFormat::Spirv ? (metal ? "spirv-metal" : android ? "spirv-android" : "spirv") : "dxil")<<"\""
        <<",\n  \"contract\": \""<<xenos::resources::Sha256Hex(r.contract)<<"\""
        <<",\n  \"records\": "<<r.records<<",\n  \"unique_binaries\": "<<r.uniqueBinaries
        <<",\n  \"blocks\": "<<r.blocks<<",\n  \"failures_omitted\": "<<r.failuresOmitted
        <<",\n  \"reconstructed_hlsl_bytes_omitted\": "<<r.hlslBytesOmitted
        <<",\n  \"diagnostic_bytes_omitted\": "<<r.diagnosticBytesOmitted
        <<",\n  \"binary_bytes_before_dedup\": "<<r.binaryBytes
        <<",\n  \"binary_bytes_after_dedup\": "<<r.uniqueBinaryBytes
        <<",\n  \"compressed_payload_bytes\": "<<r.compressedBytes
        <<",\n  \"index_bytes\": "<<r.indexBytes
        <<",\n  \"file_bytes\": "<<r.fileBytes
        <<",\n  \"file_mib\": "<<std::fixed<<std::setprecision(3)<<double(r.fileBytes)/1048576.0
        <<",\n  \"all_payloads_verified\": "<<(verified?"true":"false")
        <<",\n  \"runtime_compatibility_verified\": "<<(runtime?"true":"false")<<"\n}\n";
    return 0;
} catch(const std::exception& e) {std::cerr<<"shader pack: "<<e.what()<<'\n';return 1;}
