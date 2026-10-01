#pragma once
#if defined(_WIN32) && defined(LO_ENABLE_VULKAN_FSR_FG)
#include "frame_generation_composite.h"
#include "../../shared/frame_generation/core.h"
#include <plume_vulkan.h>
#include <filesystem>
#include <memory>
#include <string>

namespace gpu::fsr_fg {
// One presentation-thread owner. The SDK gets three exclusive native queues;
// submissions to the shared game queue use Plume's queue mutex. Inputs are read
// only by Prepare in the host presentation batch, so the host's own completion
// fence retires them (AfterHostDrain); SDK presents are drained only at
// configuration, resize and shutdown boundaries.
class Session {
public:
    Session();
    ~Session();
    bool Initialize(plume::VulkanDevice& device, plume::VulkanCommandQueue& queue,
        const std::filesystem::path& runtime, std::string& reason);
    bool Reconfigure(const framegen::Config& config, std::string& reason);
    bool Requested() const;
    bool Available() const;
    bool Failed() const;
    bool UsesProxySwapchain() const;
    void PrepareAfterHostDrain(const frame_generation::CompositeHandoff& inputs,
        plume::VulkanSwapChain& swap, plume::VulkanCommandList& commands);
    void SubmitStart();
    void HostSubmitted(bool success, uint64_t serial);
    void Presented(bool accepted);
    // The host presentation fence has completed every submitted input read.
    void AfterHostDrain();
    void CancelUnsubmitted(plume::RenderCommandList* commands);
    void Quiesce();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace gpu::fsr_fg
#endif
