#pragma once

// Files for the persistent driver pipeline caches (VkPipelineCache,
// ID3D12PipelineLibrary, MTLBinaryArchive): size cap, atomic replacement and
// the Vulkan cache header check. The blobs themselves are opaque driver data.
#include "pipeline_cache.h"

namespace gpu::driver_pipeline_cache
{
    // About 18 KB per pipeline under vkd3d-proton (1950 recipes: 35 MB); the
    // recipe limit of 16384 would need about 300 MB.
    inline constexpr size_t kMaxBytes = size_t(256) << 20;
    inline constexpr size_t kVulkanHeaderBytes = 32;

    struct VulkanIdentity
    {
        uint32_t vendorId = 0, deviceId = 0;
        std::array<uint8_t, 16> uuid{};
    };

    // VkPipelineCacheHeaderVersionOne: headerSize, headerVersion, vendorID,
    // deviceID, pipelineCacheUUID[16]. The fields are little-endian on every host.
    inline bool ValidVulkanHeader(std::span<const uint8_t> data, const VulkanIdentity& device) noexcept
    {
        using pipeline_cache::detail::Read32;
        if (data.size() < kVulkanHeaderBytes) return false;
        const uint32_t headerSize = Read32(data.data());
        return headerSize >= kVulkanHeaderBytes && headerSize <= data.size() &&
            Read32(data.data() + 4) == 1 && // VK_PIPELINE_CACHE_HEADER_VERSION_ONE
            Read32(data.data() + 8) == device.vendorId && Read32(data.data() + 12) == device.deviceId &&
            std::equal(device.uuid.begin(), device.uuid.end(), data.begin() + 16);
    }

    // A cache is written only when it grew since the last load or write and
    // fits the cap; a larger one stays as it was on disk.
    inline bool ShouldWrite(size_t size, size_t previous) noexcept
    {
        return size > previous && size <= kMaxBytes;
    }

    enum class ReadStatus { Missing, Loaded, TooLarge, IoError };

    inline ReadStatus Read(const std::filesystem::path& path, std::vector<uint8_t>& data, std::string& error)
    {
        data.clear();
        try
        {
            std::error_code ec;
            if (!std::filesystem::exists(path, ec))
            {
                if (!ec) return ReadStatus::Missing;
                error = ec.message();
                return ReadStatus::IoError;
            }
            const auto size = std::filesystem::file_size(path, ec);
            if (ec)
            {
                error = ec.message();
                return ReadStatus::IoError;
            }
            if (size > kMaxBytes) return ReadStatus::TooLarge;
            data.resize(static_cast<size_t>(size));
            std::ifstream input(path, std::ios::binary);
            if (!input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size())))
            {
                data.clear();
                error = "could not read complete file";
                return ReadStatus::IoError;
            }
            return ReadStatus::Loaded;
        }
        catch (const std::exception& e)
        {
            data.clear();
            error = e.what();
            return ReadStatus::IoError;
        }
    }

    inline bool Write(const std::filesystem::path& path, std::span<const uint8_t> data, std::string& error)
    {
        if (data.size() > kMaxBytes)
        {
            error = "driver pipeline cache exceeds the size cap";
            return false;
        }
        try { return pipeline_cache::detail::WriteAtomically(path, data, error); }
        catch (const std::exception& e)
        {
            error = e.what();
            return false;
        }
    }
}
