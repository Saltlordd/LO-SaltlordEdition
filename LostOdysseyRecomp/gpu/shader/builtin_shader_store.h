#pragma once

#include "shader_store.h"
#include <functional>

// Host (builtin) shaders, compiled from HLSL in the runtime itself, live in a
// small store of their own per translator version and binary format, in place
// of the old `builtin/` folder with one file per shader. A record's key is the
// FNV hash of the HLSL, entry and profile, plus the identity key that the
// per-shader file name ended with, so an old file moves in without its source.
namespace xenos::cache {
inline const char* FormatName(Format format) {
    return format == Format::Dxil ? "dxil" : format == Format::Spirv ? "spirv" : "dxbc";
}

inline std::filesystem::path BuiltinStorePath(const std::filesystem::path& cacheDir, Format format,
    unsigned translatorVersion = Version) {
    return cacheDir / ("builtin_v" + std::to_string(translatorVersion) + "_" + FormatName(format) + ".lostore");
}

inline std::optional<InputDigest> DigestFromHex(std::string_view hex) {
    if (hex.size() != 64) return std::nullopt;
    InputDigest digest{};
    for (size_t i = 0; i < digest.size(); ++i) {
        unsigned value = 0;
        for (const char c : hex.substr(i * 2, 2)) {
            value <<= 4;
            if (c >= '0' && c <= '9') value |= unsigned(c - '0');
            else if (c >= 'a' && c <= 'f') value |= unsigned(c - 'a' + 10);
            else return std::nullopt;
        }
        digest[i] = uint8_t(value);
    }
    return digest;
}

inline InputDigest BuiltinDigest(const Identity& identity) { return *DigestFromHex(IdentityKey(identity)); }

// A per-shader file of the old `builtin/` folder (cache::FileName).
struct LegacyBuiltinFile {
    bool pixel = false;
    uint64_t hash = 0;
    unsigned version = 0;
    Format format = Format::Dxil;
    std::string identityKey;
    bool temporary = false; // left by an interrupted write
};
inline std::optional<LegacyBuiltinFile> ParseLegacyBuiltinFile(const std::string& name) {
    static const std::regex pattern(R"(^(vs|ps)_([0-9a-f]{16})_v(\d+)_b\d+_f\d+_([0-9a-f]{64})(\.spv|\.dxil|\.dxbc)(\.tmp-\d+-\d+)?$)");
    std::smatch match;
    if (!std::regex_match(name, match, pattern)) return std::nullopt;
    LegacyBuiltinFile file;
    file.pixel = match[1].str() == "ps";
    file.hash = std::stoull(match[2].str(), nullptr, 16);
    file.version = unsigned(std::stoul(match[3].str()));
    file.identityKey = match[4].str();
    const auto extension = match[5].str();
    file.format = extension == ".spv" ? Format::Spirv : extension == ".dxil" ? Format::Dxil : Format::Dxbc;
    file.temporary = match[6].matched;
    return file;
}

struct BuiltinMigration {
    size_t moved = 0;      // valid current files now in a store
    size_t discarded = 0;  // older translator versions, interrupted writes, invalid files
    size_t kept = 0;       // files left in place (unknown names, or no writable store)
    uint64_t bytesFreed = 0;
    bool folderRemoved = false;
    std::string error;
};

// Moves the `builtin/` folder into the stores `storeFor(format)` returns
// (nullptr: leave that format's files). Every valid file is added and the
// stores synced before any file is deleted, and the folder goes only when
// it is empty, so an interrupted move is completed by the next call. Files
// with other names are never deleted.
inline BuiltinMigration MigrateBuiltinFolder(const std::filesystem::path& folder,
    const std::function<ShaderStore*(Format)>& storeFor) {
    BuiltinMigration result;
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) return result;
    std::vector<std::pair<std::filesystem::path, uint64_t>> doomed;
    std::set<ShaderStore*> written;
    for (std::filesystem::directory_iterator it(folder, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code sizeError;
        const uint64_t size = it->is_regular_file(sizeError) ? uint64_t(it->file_size(sizeError)) : 0;
        const auto parsed = ParseLegacyBuiltinFile(it->path().filename().string());
        if (!parsed || !it->is_regular_file(sizeError)) { ++result.kept; continue; }
        if (parsed->temporary || parsed->version != Version) {
            doomed.emplace_back(it->path(), size);
            ++result.discarded;
            continue;
        }
        ShaderStore* store = storeFor(parsed->format);
        if (!store) { ++result.kept; continue; }
        const auto binary = ReadBinary(it->path(), parsed->pixel, parsed->hash, parsed->identityKey, parsed->format);
        if (binary.empty()) {
            // Unreadable for the runtime as well; it would be compiled again.
            doomed.emplace_back(it->path(), size);
            ++result.discarded;
            continue;
        }
        std::string error;
        if (!store->Add(parsed->pixel, parsed->hash, *DigestFromHex(parsed->identityKey), binary, &error)) {
            if (result.error.empty()) result.error = error;
            ++result.kept;
            continue;
        }
        written.insert(store);
        doomed.emplace_back(it->path(), size);
        ++result.moved;
    }
    if (ec) {
        result.error = "cannot list " + folder.string() + ": " + ec.message();
        return result;
    }
    for (auto* store : written)
        if (!store->Sync()) {
            result.error = "shader store sync failed";
            return result;
        }
    for (const auto& [path, size] : doomed) {
        std::error_code removeError;
        if (std::filesystem::remove(path, removeError)) result.bytesFreed += size;
        else ++result.kept;
    }
    if (!result.kept) result.folderRemoved = std::filesystem::remove(folder, ec);
    return result;
}

// Host-shader stores of older translator versions can never be read again.
inline size_t RemoveOldBuiltinStores(const std::filesystem::path& cacheDir, uint64_t* bytesFreed = nullptr) {
    static const std::regex pattern(R"(^builtin_v(\d+)_(dxil|spirv|dxbc)\.lostore(\.compact)?$)");
    size_t removed = 0;
    std::error_code ec;
    std::vector<std::filesystem::path> doomed;
    for (std::filesystem::directory_iterator it(cacheDir, ec), end; !ec && it != end; it.increment(ec)) {
        const auto name = it->path().filename().string();
        std::smatch match;
        if (std::regex_match(name, match, pattern) && std::stoul(match[1].str()) != Version) doomed.push_back(it->path());
    }
    for (const auto& path : doomed) {
        std::error_code sizeError, removeError;
        const auto size = std::filesystem::file_size(path, sizeError);
        if (std::filesystem::remove(path, removeError)) {
            ++removed;
            if (bytesFreed && !sizeError) *bytesFreed += uint64_t(size);
        }
    }
    return removed;
}
} // namespace xenos::cache
