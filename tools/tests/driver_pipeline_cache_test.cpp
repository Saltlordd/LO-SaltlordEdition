#include <gpu/driver_pipeline_cache.h>
#include <cstdio>

namespace dpc = gpu::driver_pipeline_cache;

static int Fail(const char* what)
{
    std::printf("FAIL: %s\n", what);
    return 1;
}

int main()
{
    const dpc::VulkanIdentity device{ 0x1002, 0x150E, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } };
    std::vector<uint8_t> blob(64, 0xAB);
    auto put = [&](size_t offset, uint32_t value) { gpu::pipeline_cache::detail::Put32(blob.data() + offset, value); };
    put(0, 32);
    put(4, 1);
    put(8, device.vendorId);
    put(12, device.deviceId);
    std::copy(device.uuid.begin(), device.uuid.end(), blob.begin() + 16);
    if (!dpc::ValidVulkanHeader(blob, device)) return Fail("matching header rejected");

    auto mismatch = [&](size_t offset, uint32_t value) {
        auto copy = blob;
        gpu::pipeline_cache::detail::Put32(copy.data() + offset, value);
        return !dpc::ValidVulkanHeader(copy, device);
    };
    if (!mismatch(0, 16) || !mismatch(0, 65)) return Fail("bad header size accepted");
    if (!mismatch(4, 2)) return Fail("unknown header version accepted");
    if (!mismatch(8, 0x10DE)) return Fail("other vendor accepted");
    if (!mismatch(12, 0x1234)) return Fail("other device accepted");
    if (!mismatch(28, 0)) return Fail("other pipeline cache UUID accepted");
    if (dpc::ValidVulkanHeader(std::span(blob).first(31), device)) return Fail("truncated header accepted");
    std::puts("PASS: Vulkan pipeline cache header (size, version, vendor, device, UUID)");

    if (!dpc::ShouldWrite(2, 1) || dpc::ShouldWrite(1, 1) || dpc::ShouldWrite(1, 2))
        return Fail("write only when the cache grew");
    if (!dpc::ShouldWrite(dpc::kMaxBytes, 0) || dpc::ShouldWrite(dpc::kMaxBytes + 1, 0))
        return Fail("size cap");
    std::string error;
    std::puts("PASS: driver pipeline cache size cap and growth check");

    const auto path = std::filesystem::temp_directory_path() / "lo-dpc-test.bin";
    std::vector<uint8_t> read;
    if (!dpc::Write(path, blob, error)) return Fail(error.c_str());
    if (dpc::Read(path, read, error) != dpc::ReadStatus::Loaded || read != blob) return Fail("round trip");
    std::filesystem::remove(path);
    if (dpc::Read(path, read, error) != dpc::ReadStatus::Missing || !read.empty()) return Fail("missing file");
    std::puts("PASS: driver pipeline cache file round trip");
    return 0;
}
