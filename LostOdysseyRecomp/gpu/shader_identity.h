#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace gpu::shader_identity
{
    // The command-word hash is only an index. Compare all copied microcode bytes
    // before reusing the renderer-byte hash, including on command-hash collisions.
    class Cache
    {
        struct Entry { std::vector<uint32_t> words; uint64_t hash; };
        std::unordered_map<uint64_t, std::vector<Entry>> entries;
        size_t bytes = 0;
        size_t limit;

    public:
        explicit Cache(size_t byteLimit = 16 * 1024 * 1024) : limit(byteLimit) {}

        uint64_t Get(uint64_t commandHash, const uint32_t* words, uint32_t count)
        {
            const size_t size = size_t(count) * sizeof(uint32_t);
            if (auto it = entries.find(commandHash); it != entries.end())
                for (const auto& entry : it->second)
                    if (entry.words.size() == count && (!size || !std::memcmp(entry.words.data(), words, size)))
                        return entry.hash;

            uint64_t hash = 0xcbf29ce484222325ull;
            const auto* data = reinterpret_cast<const uint8_t*>(words);
            for (size_t i = 0; i < size; ++i) { hash ^= data[i]; hash *= 0x100000001b3ull; }
            // Bound this optional CPU cache independently of GPU shader lifetime.
            const size_t cost = size + sizeof(Entry) + 64;
            if (cost > limit) return hash;
            if (cost > limit - bytes) { entries.clear(); bytes = 0; }
            Entry entry{{}, hash};
            if (count) entry.words.assign(words, words + count);
            entries[commandHash].push_back(std::move(entry));
            bytes += cost;
            return hash;
        }
    };

    // In-memory index of a microcode snapshot: FNV-1a over the words in four
    // interleaved lanes, folded in lane order (shorter multiply chains).
    inline uint64_t CommandWordHash(const uint32_t* words, uint32_t count)
    {
        uint64_t lane[4] = {0xcbf29ce484222325ull, 0xcbf29ce484222325ull ^ 1, 0xcbf29ce484222325ull ^ 2, 0xcbf29ce484222325ull ^ 3};
        for (uint32_t i = 0; i < count; ++i)
            lane[i & 3] = (lane[i & 3] ^ words[i]) * 0x100000001b3ull;
        uint64_t hash = lane[0];
        for (int k = 1; k < 4; ++k) hash = (hash ^ lane[k]) * 0x100000001b3ull;
        return hash;
    }

    // IM_LOAD captures microcode on the command-processor thread. Guest memory
    // may change at any time, but draws consume this owned snapshot until the
    // next load. Revalidate all incoming bytes at each load, then resolve the
    // renderer identity only once for that snapshot, even across cache eviction.
    class CapturedShader
    {
        std::vector<uint32_t> words;
        uint64_t commandHash = 0;
        uint64_t rendererHash = 0;
        bool rendererHashValid = false;

    public:
        bool Load(const uint32_t* source, uint32_t count)
        {
            if (!count || count > 0x10000) return false;
            const size_t size = size_t(count) * sizeof(uint32_t);
            if (words.size() == count && !std::memcmp(words.data(), source, size))
                return false;
            words.assign(source, source + count);
            commandHash = CommandWordHash(words.data(), count);
            rendererHashValid = false;
            return true;
        }

        const uint32_t* Words() const { return words.empty() ? nullptr : words.data(); }
        uint32_t Count() const { return uint32_t(words.size()); }
        uint64_t CommandHash() const { return commandHash; }

        template<class IdentityCache>
        uint64_t RendererHash(IdentityCache& cache)
        {
            if (!rendererHashValid) {
                rendererHash = cache.Get(commandHash, Words(), Count());
                rendererHashValid = true;
            }
            return rendererHash;
        }
    };
}
