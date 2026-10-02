#pragma once

#include "binary_cache.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <regex>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#ifdef _WIN32
#include <io.h>
#include <share.h>
#include <windows.h>
#else
#include <sys/file.h>
#include <unistd.h>
#endif

// The local compiled-shader cache: one append-only file per binary format in
// place of one file per shader. Records are keyed by the guest shader hash and a
// digest of the exact compiler input (the translated HLSL, which embeds the
// common prelude, plus entry, profile, options and compiler identity). The
// translator version is deliberately not part of the key, so a translator
// change recompiles only the shaders whose generated HLSL actually changed.
namespace xenos::cache {
using InputDigest = resources::Sha256Digest;

inline InputDigest CompileInputDigest(std::string_view hlsl, bool pixel, const Identity& identity) {
    std::string key = "lo-shader-compile-input-1:" + std::to_string(static_cast<unsigned>(identity.format)) +
        ":" + (pixel ? std::string("ps_6_0") : std::string("vs_6_0")) + ":main";
    for (const auto& value : {identity.compiler, identity.options})
        key += ":" + std::to_string(value.size()) + ":" + value;
    key += ":" + std::to_string(hlsl.size()) + ":";
    key.append(hlsl);
    return resources::Sha256(IdentityBytes(key));
}

class ShaderStore {
public:
    static constexpr char Magic[9] = "LOSTORE1";
    static constexpr uint32_t Schema = 1;
    static constexpr size_t HeaderBytes = 32;
    static constexpr size_t RecordHeaderBytes = 64;
    static constexpr uint32_t RecordMarker = 0x5253534c; // "LSSR"

    struct Stats {
        uint64_t records = 0, fileBytes = 0, liveBytes = 0, recovered = 0;
        bool writable = false;
    };

    static std::filesystem::path PathFor(const std::filesystem::path& cacheDir, Format format) {
        return cacheDir / (std::string("shaders_") + (format == Format::Dxil ? "dxil" : format == Format::Spirv ? "spirv" : "dxbc") + ".lostore");
    }

    ShaderStore() = default;
    ShaderStore(const ShaderStore&) = delete;
    ShaderStore& operator=(const ShaderStore&) = delete;
    ~ShaderStore() { Close(); }

    // Opens or creates the store. Another process holding it opens read-only.
    // A torn last record (crash during append) is dropped and the file truncated.
    bool Open(const std::filesystem::path& path, Format format, std::string* error = nullptr) {
        std::lock_guard lock(mutex_);
        CloseLocked();
        path_ = path;
        format_ = format;
        try {
            std::error_code ec;
            const bool exists = std::filesystem::exists(path, ec);
            file_ = OpenFile(path, exists ? "r+b" : "w+b", true);
            writable_ = file_ != nullptr;
            if (!file_ && exists) file_ = OpenFile(path, "rb", false);
            if (!file_) throw std::runtime_error("cannot open shader store");
            if (!exists || FileSize() == 0) {
                if (!writable_) throw std::runtime_error("empty shader store is locked by another process");
                WriteHeader();
            } else ReadHeader();
            Scan();
            return true;
        } catch (const std::exception& e) {
            if (error) *error = e.what();
            CloseLocked();
            return false;
        }
    }
    bool IsOpen() const { std::lock_guard lock(mutex_); return file_ != nullptr; }
    const std::filesystem::path& Path() const { return path_; }

    // A validated binary, or empty when absent, corrupt or of another format.
    std::vector<uint8_t> Find(bool pixel, uint64_t hash, const InputDigest& digest) {
        std::lock_guard lock(mutex_);
        if (!file_) return {};
        const auto it = index_.find(Key{pixel, hash, digest});
        if (it == index_.end()) return {};
        std::vector<uint8_t> binary(it->second.size);
        if (!Seek(it->second.offset) || std::fread(binary.data(), 1, binary.size(), file_) != binary.size() ||
            Fnv(binary) != it->second.checksum || !CompleteBinary(binary, format_))
            return {};
        used_.insert(it->first);
        return binary;
    }

    bool Add(bool pixel, uint64_t hash, const InputDigest& digest, std::span<const uint8_t> binary, std::string* error = nullptr) {
        std::lock_guard lock(mutex_);
        try {
            if (!file_ || !writable_) throw std::runtime_error("shader store is not writable");
            if (binary.empty() || binary.size() > MaxBinaryBytes || !CompleteBinary(binary, format_))
                throw std::runtime_error("invalid shader binary");
            const Key key{pixel, hash, digest};
            if (index_.contains(key)) { used_.insert(key); return true; }
            const auto offset = end_;
            if (!Seek(offset)) throw std::runtime_error("shader store seek failed");
            const auto header = RecordHeader(key, binary);
            if (std::fwrite(header.data(), 1, header.size(), file_) != header.size() ||
                std::fwrite(binary.data(), 1, binary.size(), file_) != binary.size() || std::fflush(file_))
                throw std::runtime_error("shader store write failed");
            end_ = offset + RecordHeaderBytes + binary.size();
            index_[key] = {offset + RecordHeaderBytes, uint32_t(binary.size()), Fnv(binary)};
            used_.insert(key);
            return true;
        } catch (const std::exception& e) {
            if (error) *error = e.what();
            return false;
        }
    }

    Stats GetStats() const {
        std::lock_guard lock(mutex_);
        Stats s;
        s.records = index_.size();
        s.fileBytes = end_;
        for (const auto& [key, entry] : index_) if (used_.contains(key)) s.liveBytes += RecordHeaderBytes + entry.size;
        s.recovered = recovered_;
        s.writable = writable_;
        return s;
    }

    // Rewrites the store with only the records found or added this session,
    // when the rest wastes at least `minDeadBytes` and a quarter of the file.
    // Returns the number of records dropped (0 when nothing was rewritten).
    size_t CompactUnused(uint64_t minDeadBytes = 64ull << 20, std::string* error = nullptr) {
        std::lock_guard lock(mutex_);
        try {
            if (!file_ || !writable_ || used_.empty()) return 0;
            uint64_t live = HeaderBytes;
            for (const auto& [key, entry] : index_) if (used_.contains(key)) live += RecordHeaderBytes + entry.size;
            const uint64_t dead = end_ > live ? end_ - live : 0;
            if (dead < minDeadBytes || dead * 4 < end_) return 0;
            const auto temp = std::filesystem::path(path_.native() + std::filesystem::path(".compact").native());
            std::error_code ec;
            std::filesystem::remove(temp, ec);
            FILE* out = OpenFile(temp, "w+b", true);
            if (!out) throw std::runtime_error("cannot create compacted shader store");
            size_t dropped = 0;
            std::map<Key, Entry> kept;
            {
                struct Closer { FILE*& f; ~Closer() { if (f) std::fclose(f); } } closer{out};
                const auto header = HeaderBytesFor(format_);
                if (std::fwrite(header.data(), 1, header.size(), out) != header.size()) throw std::runtime_error("compaction write failed");
                uint64_t position = HeaderBytes;
                std::vector<uint8_t> binary;
                for (const auto& [key, entry] : index_) {
                    if (!used_.contains(key)) { ++dropped; continue; }
                    binary.resize(entry.size);
                    if (!Seek(entry.offset) || std::fread(binary.data(), 1, binary.size(), file_) != binary.size() ||
                        Fnv(binary) != entry.checksum) { ++dropped; continue; }
                    const auto recordHeader = RecordHeader(key, binary);
                    if (std::fwrite(recordHeader.data(), 1, recordHeader.size(), out) != recordHeader.size() ||
                        std::fwrite(binary.data(), 1, binary.size(), out) != binary.size())
                        throw std::runtime_error("compaction write failed");
                    kept[key] = {position + RecordHeaderBytes, entry.size, entry.checksum};
                    position += RecordHeaderBytes + entry.size;
                }
                if (std::fflush(out)) throw std::runtime_error("compaction flush failed");
            }
            out = nullptr;
            CloseLocked(false);
#ifdef _WIN32
            if (!MoveFileExW(temp.c_str(), path_.c_str(), MOVEFILE_REPLACE_EXISTING))
                throw std::runtime_error("compacted shader store publish failed");
#else
            std::filesystem::rename(temp, path_);
#endif
            file_ = OpenFile(path_, "r+b", true);
            writable_ = file_ != nullptr;
            if (!file_) throw std::runtime_error("cannot reopen compacted shader store");
            index_ = std::move(kept);
            for (auto it = used_.begin(); it != used_.end();) it = index_.contains(*it) ? std::next(it) : used_.erase(it);
            end_ = FileSize();
            return dropped;
        } catch (const std::exception& e) {
            if (error) *error = e.what();
            return 0;
        }
    }

    void Close() { std::lock_guard lock(mutex_); CloseLocked(); }

private:
    using Key = std::tuple<bool, uint64_t, InputDigest>;
    struct Entry { uint64_t offset = 0; uint32_t size = 0; uint64_t checksum = 0; };

    static uint64_t Fnv(std::span<const uint8_t> bytes) {
        uint64_t hash = 0xcbf29ce484222325ull;
        for (uint8_t byte : bytes) { hash ^= byte; hash *= 0x100000001b3ull; }
        return hash;
    }
    static void Put32(uint8_t* p, uint32_t v) { for (unsigned i = 0; i < 4; ++i) p[i] = uint8_t(v >> (8 * i)); }
    static void Put64(uint8_t* p, uint64_t v) { for (unsigned i = 0; i < 8; ++i) p[i] = uint8_t(v >> (8 * i)); }
    static uint32_t Get32(const uint8_t* p) { uint32_t v = 0; for (unsigned i = 0; i < 4; ++i) v |= uint32_t(p[i]) << (8 * i); return v; }
    static uint64_t Get64(const uint8_t* p) { uint64_t v = 0; for (unsigned i = 0; i < 8; ++i) v |= uint64_t(p[i]) << (8 * i); return v; }

    static std::array<uint8_t, HeaderBytes> HeaderBytesFor(Format format) {
        std::array<uint8_t, HeaderBytes> h{};
        std::memcpy(h.data(), Magic, 8);
        Put32(h.data() + 8, Schema);
        Put32(h.data() + 12, static_cast<uint32_t>(format));
        return h;
    }
    static std::array<uint8_t, RecordHeaderBytes> RecordHeader(const Key& key, std::span<const uint8_t> binary) {
        std::array<uint8_t, RecordHeaderBytes> h{};
        Put32(h.data(), RecordMarker);
        Put32(h.data() + 4, std::get<0>(key) ? 1u : 0u);
        Put64(h.data() + 8, std::get<1>(key));
        std::memcpy(h.data() + 16, std::get<2>(key).data(), 32);
        Put32(h.data() + 48, uint32_t(binary.size()));
        Put64(h.data() + 56, Fnv(binary));
        return h;
    }

    static FILE* OpenFile(const std::filesystem::path& path, const char* mode, bool exclusive) {
#ifdef _WIN32
        std::wstring wmode(mode, mode + std::strlen(mode));
        return _wfsopen(path.c_str(), wmode.c_str(), exclusive ? _SH_DENYWR : _SH_DENYNO);
#else
        FILE* f = std::fopen(path.c_str(), mode);
        if (f && exclusive && flock(fileno(f), LOCK_EX | LOCK_NB) != 0) { std::fclose(f); return nullptr; }
        return f;
#endif
    }
    bool Seek(uint64_t offset) {
#ifdef _WIN32
        return _fseeki64(file_, static_cast<long long>(offset), SEEK_SET) == 0;
#else
        return fseeko(file_, static_cast<off_t>(offset), SEEK_SET) == 0;
#endif
    }
    uint64_t FileSize() {
        std::error_code ec;
        const auto size = std::filesystem::file_size(path_, ec);
        return ec ? 0 : uint64_t(size);
    }
    void WriteHeader() {
        const auto header = HeaderBytesFor(format_);
        if (!Seek(0) || std::fwrite(header.data(), 1, header.size(), file_) != header.size() || std::fflush(file_))
            throw std::runtime_error("shader store header write failed");
    }
    void ReadHeader() {
        std::array<uint8_t, HeaderBytes> h{};
        if (!Seek(0) || std::fread(h.data(), 1, h.size(), file_) != h.size() || std::memcmp(h.data(), Magic, 8) ||
            Get32(h.data() + 8) != Schema || Get32(h.data() + 12) != static_cast<uint32_t>(format_))
            throw std::runtime_error("shader store format or schema mismatch");
    }
    void Scan() {
        const uint64_t size = FileSize();
        uint64_t offset = HeaderBytes;
        std::array<uint8_t, RecordHeaderBytes> h{};
        while (offset + RecordHeaderBytes <= size) {
            if (!Seek(offset) || std::fread(h.data(), 1, h.size(), file_) != h.size()) break;
            const uint32_t length = Get32(h.data() + 48);
            if (Get32(h.data()) != RecordMarker || Get32(h.data() + 4) > 1 || !length || length > MaxBinaryBytes ||
                offset + RecordHeaderBytes + length > size) break;
            Key key{Get32(h.data() + 4) == 1, Get64(h.data() + 8), {}};
            std::memcpy(std::get<2>(key).data(), h.data() + 16, 32);
            index_[key] = {offset + RecordHeaderBytes, length, Get64(h.data() + 56)};
            offset += RecordHeaderBytes + length;
        }
        end_ = offset;
        if (offset < size) {
            recovered_ = size - offset;
            if (writable_) {
                std::fflush(file_);
#ifdef _WIN32
                if (_chsize_s(_fileno(file_), static_cast<long long>(offset)) != 0) writable_ = false;
#else
                if (ftruncate(fileno(file_), static_cast<off_t>(offset)) != 0) writable_ = false;
#endif
            }
        }
    }
    void CloseLocked(bool clearIndex = true) {
        if (file_) { std::fclose(file_); file_ = nullptr; }
        writable_ = false;
        if (clearIndex) { index_.clear(); used_.clear(); end_ = 0; recovered_ = 0; }
    }

    mutable std::mutex mutex_;
    std::filesystem::path path_;
    Format format_ = Format::Dxil;
    FILE* file_ = nullptr;
    bool writable_ = false;
    std::map<Key, Entry> index_;
    std::set<Key> used_;
    uint64_t end_ = 0, recovered_ = 0;
};

// Per-shader cache files from before the store (cache::FileName), any
// translator version and format, including leftovers of interrupted writes.
struct LegacyCacheFile { unsigned version = 0; std::string extension; };
inline std::optional<LegacyCacheFile> ParseLegacyShaderCacheFile(const std::string& name) {
    static const std::regex pattern(R"(^(vs|ps)_[0-9a-f]{16}_v(\d+)_b\d+_f\d+_[0-9a-f]{64}(\.spv|\.dxil|\.dxbc)(\.tmp-\d+-\d+)?$)");
    std::smatch match;
    if (!std::regex_match(name, match, pattern)) return std::nullopt;
    return LegacyCacheFile{unsigned(std::stoul(match[2].str())), match[3].str()};
}
} // namespace xenos::cache
