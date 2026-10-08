#include "xex_probe.h"
#include <kernel/guest_address_space.h>
#include <image.h>
#include <cstring>
#include <fstream>
#include <memory>
#include <vector>
extern "C" {
#include <libavutil/sha.h>
#include <libavutil/mem.h>
}

namespace {
std::string Sha256(const uint8_t* data, size_t size) {
    auto* sha = av_sha_alloc();
    if (!sha) return {};
    uint8_t digest[32]{};
    if (av_sha_init(sha, 256) < 0) { av_free(sha); return {}; }
    av_sha_update(sha, data, size);
    av_sha_final(sha, digest);
    av_free(sha);
    const char* hex = "0123456789abcdef";
    std::string value;
    for (auto byte : digest) { value += hex[byte >> 4]; value += hex[byte & 15]; }
    return value;
}
}

XexProbeResult TestXexImage(const std::filesystem::path& file, void (*log)(const char*, ...)) {
    if (!std::filesystem::is_regular_file(file)) {
        log("XEX loader NOT EXERCISED: choose Disc 1 default.xex in the test launcher");
        return XexProbeResult::Missing;
    }
    // This bring-up probe accepts the exact supplied Disc 1 revision. Verify
    // before parsing, so neither a different image nor arbitrary bytes reach
    // the production parser under an untested game configuration.
    constexpr size_t inputSize = 6623232;
    constexpr const char* inputHash = "175ae53d109d480a83bebbd186e7b6871f7b03ce80af69ab388db2f747640de3";
    constexpr const char* imageHash = "cb756b46092e448923517bf660b44ad1a0651c2860882b43ae021f4ad4290f71";
    if (std::filesystem::file_size(file) != inputSize) {
        log("BLOCKED XEX: unsupported file size; expected=%zu", inputSize);
        return XexProbeResult::Rejected;
    }
    std::vector<uint8_t> bytes(inputSize);
    std::ifstream stream(file, std::ios::binary);
    if (!stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) {
        log("BLOCKED XEX: failed to read the complete executable"); return XexProbeResult::Rejected;
    }
    if (Sha256(bytes.data(), bytes.size()) != inputHash) {
        log("BLOCKED XEX: SHA-256 does not match the supplied Disc 1 revision"); return XexProbeResult::Rejected;
    }
    log("Disc 1 XEX input fingerprint=PASS; bytes=%zu", bytes.size());
    auto image = Image::ParseImage(bytes.data(), bytes.size());
    if (!image.data || image.base != 0x82000000 || image.size != 0x13C0000 || image.entry_point != 0x827CA440) {
        log("BLOCKED XEX: unexpected decrypted image layout"); return XexProbeResult::Rejected;
    }
    if (Sha256(image.data.get(), image.size) != imageHash) {
        log("BLOCKED XEX: decrypted image fingerprint differs from host loader"); return XexProbeResult::Rejected;
    }
    log("Production XEX loader: decrypt/decompress=PASS base=0x%zx size=0x%x entry=0x%zx sections=%zu symbols=%zu",
        image.base, image.size, image.entry_point, image.sections.size(), image.symbols.size());
    std::unique_ptr<uint8_t, decltype(&GuestAddressSpace::Release)> guest(GuestAddressSpace::Allocate(), GuestAddressSpace::Release);
    if (!guest) { log("BLOCKED XEX: guest address-space allocation failed"); return XexProbeResult::Rejected; }
    auto* target = guest.get() + image.base;
    std::memcpy(target, image.data.get(), image.size);
    if (Sha256(target, image.size) != imageHash) {
        log("BLOCKED XEX: mapped guest image fingerprint mismatch"); return XexProbeResult::Rejected;
    }
    log("Guest image mapping=PASS; %u bytes verified at base+0x%zx; no guest entry point was called", image.size, image.base);
    return XexProbeResult::Loaded;
}
