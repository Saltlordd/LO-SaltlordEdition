#include "gpu/shader/portable_shader_contract.h"
#include "merge.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>

int main(int argc,char** argv) try {
    const auto command = argc > 1 ? std::string_view(argv[1]) : std::string_view{};
    if (command == "merge") return Merge(argc, argv);
    const bool runtime = command == "verify-runtime";
    // Metal consumes SPIR-V compiled at -O1, a separate contract (cache::MetalOptions).
    const bool metal = runtime && argc == 5 && std::string_view(argv[4]) == "--metal";
    if ((runtime ? argc != 4 && !metal : argc != 3) ||
        (command != "inspect" && command != "verify" && !runtime)) {
        std::cerr << "Usage: LoShaderPackTool <inspect|verify> pack.lospv|pack.lospd\n"
                     "       LoShaderPackTool verify-runtime pack.lospv|pack.lospd xexdump-image.bin [--metal]\n"
                     "       LoShaderPackTool merge baseline.lospv xexdump-image.bin manifest.tsv output-dir\n";
        return 2;
    }
    const bool verified = command != "inspect";
    const auto path = std::filesystem::path(reinterpret_cast<const char8_t*>(argv[2]));
    xenos::portable_pack::Report r;
    if (runtime) {
        // The runtime contract hashes the image as parsed, before import binding,
        // which is what xexdump writes, so this is the value the game computes.
        const auto format = xenos::portable_pack::Reader::Inspect(path).format;
        std::ifstream in(std::filesystem::path(reinterpret_cast<const char8_t*>(argv[3])), std::ios::binary);
        std::vector<uint8_t> image(xenos::portable_pack::RuntimeXexBytes);
        if (!in.read(reinterpret_cast<char*>(image.data()), std::streamsize(image.size())))
            throw std::runtime_error("missing/short decrypted runtime image (use xexdump output)");
        const auto backend = format == xenos::portable_pack::PackFormat::Dxil ?
            xenos::cache::Backend::D3D12 : xenos::cache::Backend::Vulkan;
        if (metal && backend != xenos::cache::Backend::Vulkan)
            throw std::runtime_error("--metal applies to SPIR-V packs only");
        auto identity = xenos::cache::MakeIdentity(backend, "");
        if (metal) identity.options = xenos::cache::MetalOptions();
        const auto expected = xenos::portable_pack::RuntimeContract(image, identity, format);
        xenos::portable_pack::Reader reader(path, expected, format);
        reader.VerifyAll();
        r = reader.Info();
    } else r = xenos::portable_pack::Reader::Inspect(path, verified);
    std::cout<<"{\n  \"schema\": "<<xenos::portable_pack::Schema
        <<",\n  \"format\": \""<<(r.format == xenos::portable_pack::PackFormat::Spirv ? (metal ? "spirv-metal" : "spirv") : "dxil")<<"\""
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
