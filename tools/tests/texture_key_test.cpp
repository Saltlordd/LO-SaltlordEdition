#include "gpu/texture_key.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

using gpu::texture_cache::Key;
using gpu::texture_cache::KeyHash;

static size_t checks = 0;
static void Check(bool condition, const char* message)
{
    ++checks;
    if (!condition) throw std::runtime_error(message);
}

static void TestMap(const std::vector<Key>& keys)
{
    std::unordered_map<Key, size_t, KeyHash> cache;
    cache.reserve(keys.size());
    for (size_t i = 0; i < keys.size(); ++i)
        Check(cache.emplace(keys[i], i).second, "distinct key insertion");

    // Rehashing, duplicate inserts and retirement/reinsertion must keep the
    // association with the complete key, including heavily shared dimensions.
    cache.rehash(cache.bucket_count() * 2 + 1);
    for (size_t i = 0; i < keys.size(); ++i)
    {
        auto found = cache.find(keys[i]);
        Check(found != cache.end() && found->second == i, "lookup after rehash");
        const auto duplicate = cache.emplace(keys[i], keys.size());
        Check(!duplicate.second && duplicate.first->second == i, "duplicate preserves value");
    }
    for (size_t i = 0; i < keys.size(); i += 3)
        Check(cache.erase(keys[i]) == 1, "erase only requested key");
    for (size_t i = 0; i < keys.size(); ++i)
    {
        auto found = cache.find(keys[i]);
        Check(i % 3 == 0 ? found == cache.end() :
            found != cache.end() && found->second == i, "remaining entries after erase");
    }
    for (size_t i = 0; i < keys.size(); i += 3)
        Check(cache.emplace(keys[i], i + keys.size()).second, "reinsert retired key");
    for (size_t i = 0; i < keys.size(); ++i)
        Check(cache.at(keys[i]) == i + (i % 3 == 0 ? keys.size() : 0), "replacement lookup");
}

static void TestDistribution(const char* name, const std::vector<Key>& keys, size_t bucketCount)
{
    Check(!keys.empty() && keys.size() <= bucketCount, "fixture load at most one");
    Check((bucketCount & (bucketCount - 1)) == 0, "power-of-two bucket fixture");
    std::vector<size_t> lengths(bucketCount);
    for (const auto& key : keys) ++lengths[KeyHash{}(key) & (bucketCount - 1)];
    const auto occupied = std::count_if(lengths.begin(), lengths.end(), [](size_t n) { return n != 0; });
    const size_t longest = *std::max_element(lengths.begin(), lengths.end());
    // Loose bounds for ordinary hashing at <= 1 load, independent of the
    // mixer's constants. The original aligned-address hash fails both bounds.
    Check(size_t(occupied) >= keys.size() / 2, "aligned addresses use enough buckets");
    Check(longest <= 12, "aligned addresses avoid long collision chains");
    TestMap(keys);
    std::printf("%s: %zu keys, %zu buckets, %zu occupied, longest chain %zu\n",
        name, keys.size(), bucketCount, size_t(occupied), longest);
}

static void TestAllFields()
{
    const Key original{ 0x82000000u, 18, 128, 128, 20515 };
    std::array<Key, 7> keys;
    keys.fill(original);
    keys[1].address += 4096;
    keys[2].format += 1;
    keys[3].width += 1;
    keys[4].height += 1;
    keys[5].flags ^= 1;
    keys[6].mipAddress = 0x83000000u;
    TestMap({keys.begin(), keys.end()});

    struct CollidingHash { size_t operator()(const Key&) const { return 0; } };
    std::unordered_map<Key, size_t, CollidingHash> cache;
    for (size_t i = 0; i < keys.size(); ++i)
        Check(cache.emplace(keys[i], i).second, "equality distinguishes every key field");
    for (size_t i = 0; i < keys.size(); ++i)
    {
        const Key copy = keys[i];
        Check(cache.at(copy) == i, "full-hash collision lookup");
        Check(KeyHash{}(copy) == KeyHash{}(keys[i]), "equal keys hash equally");
    }
}

static std::vector<Key> ReadCapturedKeys(std::istream& input)
{
    std::vector<Key> keys;
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream tuple(line);
        tuple >> std::ws;
        if (tuple.eof()) continue;
        Key key{};
        Check(bool(tuple >> key.address >> key.format >> key.width >> key.height >> key.flags), "complete captured key");
        tuple >> std::ws;
        if (!tuple.eof())
        {
            Check(bool(tuple >> key.mipAddress), "valid captured mip address");
            tuple >> std::ws;
            Check(tuple.eof(), "no extra captured key fields");
        }
        keys.push_back(key);
    }
    Check(input.eof(), "valid captured keys");
    return keys;
}

static void TestCapturedKeys()
{
    // Preserve old five-field recordings, and permit a sixth field for mips.
    std::istringstream input("2181038080 18 128 128 20515\n"
        "2181038080 18 128 128 20515 2197815296\n"
        "2181038080 18 128 128 20515 2197819392\n");
    const auto keys = ReadCapturedKeys(input);
    Check(keys.size() == 3, "read legacy and extended captured fixtures");
    Check(keys[0] == Key{0x82000000u, 18, 128, 128, 20515}, "legacy fixture defaults mip source to zero");
    Check(keys[1].mipAddress == 0x83000000u && keys[2].mipAddress == 0x83001000u, "captured mip source addresses");
    Check(!(keys[1] == keys[2]), "same base and layout distinguish separate mip sources");
    Check(KeyHash{}(keys[1]) != KeyHash{}(keys[2]), "mip source participates in hash");
    TestMap(keys);
}

static std::vector<Key> AlignedKeys(size_t count, uint32_t base, uint32_t stride)
{
    std::vector<Key> keys;
    keys.reserve(count);
    for (size_t i = 0; i < count; ++i)
        keys.push_back({base + uint32_t(i) * stride, 18, 128, 128, 20515});
    return keys;
}

int main(int argc, char** argv)
{
    try
    {
        Check(argc <= 2, "usage: texture_key_test [captured-keys.txt]");
        TestAllFields();
        TestCapturedKeys();
        TestDistribution("4KiB-low", AlignedKeys(512, 0, 4096), 512);
        TestDistribution("4KiB-high", AlignedKeys(512, 0x82000000u, 4096), 512);
        TestDistribution("4KiB-large", AlignedKeys(4096, 0x80000000u, 4096), 4096);
        TestDistribution("64KiB-large", AlignedKeys(4096, 0x80000000u, 65536), 4096);
        if (argc == 2)
        {
            // Optional read-only replay: one decimal address/format/width/
            // height/flags[/mipAddress] tuple per line; no live-game dependency.
            std::ifstream input(argv[1]);
            Check(bool(input), "open captured keys");
            const auto keys = ReadCapturedKeys(input);
            TestDistribution("captured", keys, 512);
        }
        std::printf("texture key: %zu checks passed\n", checks);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "texture key failure: %s\n", error.what());
        return 1;
    }
}
