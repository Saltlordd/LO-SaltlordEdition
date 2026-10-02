// One-file local shader store (gpu/shader/shader_store.h): keys, persistence,
// torn-tail recovery, integrity, locking and compaction. No GPU or DXC.
#include <gpu/shader/shader_store.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace {
unsigned checks = 0, failures = 0;
void Check(bool ok, const char* what) {
    ++checks;
    if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", what); }
}
namespace fs = std::filesystem;
using xenos::cache::Format;
using xenos::cache::ShaderStore;

// Smallest framing CompleteSpirv accepts: header, OpMemoryModel, OpEntryPoint,
// then OpNop padding. `salt` varies the id bound so binaries differ.
std::vector<uint8_t> Spirv(uint32_t salt, size_t padding = 0) {
    std::vector<uint32_t> words{0x07230203, 0x00010000, 0, 1 + salt, 0,
        (3u << 16) | 14, 0, 1, (4u << 16) | 15, 0, 1, 0x6e69616d};
    words.insert(words.end(), padding, 1u << 16);
    std::vector<uint8_t> bytes(words.size() * 4);
    std::memcpy(bytes.data(), words.data(), bytes.size());
    return bytes;
}
xenos::cache::InputDigest Digest(std::string_view hlsl, bool pixel = false) {
    return xenos::cache::CompileInputDigest(hlsl, pixel, xenos::cache::MakeIdentity(xenos::cache::Backend::Vulkan, "dxc-test"));
}
fs::path FreshDirectory() {
    const auto dir = fs::temp_directory_path() / ("lo-shader-store-test-" + std::to_string(std::rand()) + "-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(dir);
    return dir;
}

void DigestKeys() {
    auto identity = xenos::cache::MakeIdentity(xenos::cache::Backend::Vulkan, "dxc-1.8");
    const auto base = xenos::cache::CompileInputDigest("float4 main() : SV_Target { return 0; }", true, identity);
    auto bumped = identity;
    bumped.translatorVersion += 7;
    Check(xenos::cache::CompileInputDigest("float4 main() : SV_Target { return 0; }", true, bumped) == base,
        "a translator version bump keeps the compile-input digest of unchanged HLSL");
    auto options = identity;
    options.options += ";O1";
    Check(xenos::cache::CompileInputDigest("float4 main() : SV_Target { return 0; }", true, options) != base,
        "compiler options are part of the compile input");
    auto compiler = identity;
    compiler.compiler = "dxc-1.9";
    Check(xenos::cache::CompileInputDigest("float4 main() : SV_Target { return 0; }", true, compiler) != base,
        "the compiler identity is part of the compile input");
    Check(xenos::cache::CompileInputDigest("float4 main() : SV_Target { return 0; }", false, identity) != base,
        "the shader stage is part of the compile input");
    Check(xenos::cache::CompileInputDigest("float4 main() : SV_Target { return 1; }", true, identity) != base,
        "any HLSL change changes the compile input");
}

void PersistenceAndRecovery() {
    const auto dir = FreshDirectory();
    const auto path = ShaderStore::PathFor(dir, Format::Spirv);
    {
        ShaderStore store;
        std::string error;
        Check(store.Open(path, Format::Spirv, &error), "a new store opens");
        Check(store.Add(false, 0x1111, Digest("a"), Spirv(1)), "add vertex record");
        Check(store.Add(true, 0x1111, Digest("a", true), Spirv(2)), "add pixel record with the same hash");
        Check(store.Add(false, 0x2222, Digest("b"), Spirv(3, 1000)), "add a larger record");
        const auto size = fs::file_size(path);
        Check(store.Add(false, 0x1111, Digest("a"), Spirv(1)) && fs::file_size(path) == size, "a duplicate add does not grow the file");
        Check(!store.Add(false, 0x3333, Digest("c"), std::vector<uint8_t>(64, 0)), "an incomplete binary is refused");
    }
    {
        ShaderStore store;
        Check(store.Open(path, Format::Spirv), "the store reopens");
        Check(store.GetStats().records == 3, "three records after reopening");
        Check(store.Find(false, 0x1111, Digest("a")) == Spirv(1), "vertex record round trips");
        Check(store.Find(true, 0x1111, Digest("a", true)) == Spirv(2), "pixel record with the same hash is separate");
        Check(store.Find(false, 0x2222, Digest("b")) == Spirv(3, 1000), "large record round trips");
        Check(store.Find(false, 0x1111, Digest("changed")).empty(), "another compile input misses");
    }
    // A crash in the middle of an append leaves a torn record at the end.
    const auto goodSize = fs::file_size(path);
    {
        std::ofstream torn(path, std::ios::binary | std::ios::app);
        const auto header = std::vector<char>(40, 'x');
        torn.write(header.data(), std::streamsize(header.size()));
    }
    {
        ShaderStore store;
        Check(store.Open(path, Format::Spirv), "a store with a torn tail opens");
        Check(store.GetStats().recovered == 40 && fs::file_size(path) == goodSize, "the torn tail is dropped and truncated");
        Check(store.Find(false, 0x2222, Digest("b")) == Spirv(3, 1000), "records before the torn tail survive");
        Check(store.Add(false, 0x4444, Digest("d"), Spirv(4)), "appending after recovery works");
    }
    {
        ShaderStore store;
        Check(store.Open(path, Format::Spirv) && store.GetStats().records == 4, "the recovered store keeps every record");
        Check(store.Find(false, 0x4444, Digest("d")) == Spirv(4), "the post-recovery record round trips");
    }
    // Flip one byte inside the first record's binary.
    {
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        file.seekp(ShaderStore::HeaderBytes + ShaderStore::RecordHeaderBytes + 20);
        file.put(char(0x5a));
    }
    {
        ShaderStore store;
        Check(store.Open(path, Format::Spirv), "a store with a damaged binary opens");
        Check(store.Find(false, 0x1111, Digest("a")).empty(), "the damaged binary fails its checksum");
        Check(store.Find(false, 0x2222, Digest("b")) == Spirv(3, 1000), "other records are unaffected");
    }
    ShaderStore wrong;
    Check(!wrong.Open(path, Format::Dxil), "a SPIR-V store does not open as DXIL");
    std::error_code ec;
    fs::remove_all(dir, ec);
}

void SecondInstanceIsReadOnly() {
    const auto dir = FreshDirectory();
    const auto path = ShaderStore::PathFor(dir, Format::Spirv);
    ShaderStore first, second;
    Check(first.Open(path, Format::Spirv) && first.GetStats().writable, "the first instance writes");
    Check(first.Add(false, 0x5555, Digest("e"), Spirv(5)), "the first instance adds");
    Check(second.Open(path, Format::Spirv), "a second instance opens");
    Check(!second.GetStats().writable, "the second instance is read-only");
    Check(second.Find(false, 0x5555, Digest("e")) == Spirv(5), "the second instance reads");
    Check(!second.Add(false, 0x6666, Digest("f"), Spirv(6)), "the second instance cannot append");
    first.Close();
    second.Close();
    std::error_code ec;
    fs::remove_all(dir, ec);
}

void Compaction() {
    const auto dir = FreshDirectory();
    const auto path = ShaderStore::PathFor(dir, Format::Spirv);
    {
        ShaderStore store;
        Check(store.Open(path, Format::Spirv), "store for compaction opens");
        for (uint32_t i = 0; i < 6; ++i)
            Check(store.Add(false, 0x7000 + i, Digest("g" + std::to_string(i)), Spirv(10 + i, 2000)), "add compaction record");
    }
    {
        ShaderStore store;
        Check(store.Open(path, Format::Spirv), "store reopens for a new session");
        Check(store.CompactUnused(1) == 0, "nothing is compacted before a shader was used");
        Check(store.Find(false, 0x7001, Digest("g1")) == Spirv(11, 2000), "session uses record 1");
        Check(store.Find(false, 0x7004, Digest("g4")) == Spirv(14, 2000), "session uses record 4");
        Check(store.CompactUnused(1ull << 40) == 0, "compaction respects the dead-byte threshold");
        const auto before = fs::file_size(path);
        Check(store.CompactUnused(1) == 4, "four unused records are dropped");
        Check(fs::file_size(path) < before / 2, "the compacted file shrinks");
        Check(store.Find(false, 0x7004, Digest("g4")) == Spirv(14, 2000), "a kept record still reads after compaction");
        Check(store.Add(false, 0x7100, Digest("h"), Spirv(20)), "appending after compaction works");
    }
    {
        ShaderStore store;
        Check(store.Open(path, Format::Spirv) && store.GetStats().records == 3, "the compacted store holds the kept and new records");
        Check(store.Find(false, 0x7000, Digest("g0")).empty(), "a dropped record is gone");
        Check(store.Find(false, 0x7100, Digest("h")) == Spirv(20), "the record added after compaction persists");
    }
    std::error_code ec;
    fs::remove_all(dir, ec);
}

void LegacyNames() {
    const std::string digest(64, 'a');
    const auto spv = xenos::cache::ParseLegacyShaderCacheFile("vs_00112233445566ff_v26_b1_f1_" + digest + ".spv");
    Check(spv && spv->version == 26 && spv->extension == ".spv", "a current per-shader SPIR-V name parses");
    const auto tmp = xenos::cache::ParseLegacyShaderCacheFile("ps_00112233445566ff_v24_b0_f0_" + digest + ".dxil.tmp-49528630729700-0");
    Check(tmp && tmp->version == 24 && tmp->extension == ".dxil", "a leftover temporary DXIL name parses");
    Check(!xenos::cache::ParseLegacyShaderCacheFile("vs_00112233445566ff.bin"), "learned sources are not per-shader caches");
    Check(!xenos::cache::ParseLegacyShaderCacheFile("startup_vk12_v1.bundle"), "the startup bundle is not a per-shader cache");
    Check(!xenos::cache::ParseLegacyShaderCacheFile("vs_00112233445566ff_v26_b1_f1_" + digest + ".spv.failed"), "failure diagnostics are kept");
    Check(!xenos::cache::ParseLegacyShaderCacheFile("shaders_spirv.lostore"), "the store itself is not legacy");
}
} // namespace

int main() {
    std::srand(unsigned(std::chrono::steady_clock::now().time_since_epoch().count()));
    DigestKeys();
    PersistenceAndRecovery();
    SecondInstanceIsReadOnly();
    Compaction();
    LegacyNames();
    if (failures) {
        std::fprintf(stderr, "shader store: %u of %u checks failed\n", failures, checks);
        return 1;
    }
    std::printf("PASS: %u shader store checks (keys, persistence, torn tail, integrity, locking, compaction, legacy names)\n", checks);
    return 0;
}
