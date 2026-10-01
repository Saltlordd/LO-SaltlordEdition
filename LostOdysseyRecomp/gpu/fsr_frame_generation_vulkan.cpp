#include "fsr_frame_generation_vulkan.h"
#if defined(_WIN32) && defined(LO_ENABLE_VULKAN_FSR_FG)
#include <ffx_api/ffx_api.h>
#include <ffx_api/ffx_framegeneration.h>
#include <ffx_api/vk/ffx_api_vk.h>
#include "fsr_fg_queue_plan.h"
#include "dlss_fg_depth.h"
#include "dlss_fg_host_use.h"
#include "vulkan_command_recording.h"
#include <os/logger.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace gpu::fsr_fg {
namespace {
// Only for states where GPU completion of retained resources is unknown.
void Require(bool okay, const char* operation) {
    if (okay) return;
    LOG_ERROR("Vulkan FSR FG: {} failed; GPU ownership cannot be retired", operation);
    std::fflush(nullptr); std::_Exit(EXIT_FAILURE);
}
bool Check(ffxReturnCode_t result, const char* operation, std::string& reason) {
    if (result == FFX_API_RETURN_OK) return true;
    reason = std::string(operation) + " result=" + std::to_string(result); return false;
}
// A lost device runs no further work, so its resources are no longer in use.
bool Retired(VkResult result) { return result == VK_SUCCESS || result == VK_ERROR_DEVICE_LOST; }
FfxApiResource Resource(plume::VulkanTexture& texture) {
    VkImageCreateInfo desc{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    desc.imageType = VK_IMAGE_TYPE_2D; desc.format = texture.imageFormat;
    desc.extent = {texture.desc.width, texture.desc.height, 1};
    desc.mipLevels = 1; desc.arrayLayers = 1; desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    return ffxApiGetResourceVK(reinterpret_cast<void*>(texture.vk),
        ffxApiGetImageResourceDescriptionVK(texture.vk, desc, 0), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
}
}
struct Session::Impl {
    static inline Impl* current = nullptr;
    plume::VulkanDevice* device = nullptr;
    plume::VulkanCommandQueue* queue = nullptr;
    HMODULE module = nullptr;
    PfnFfxCreateContext create = nullptr;
    PfnFfxDestroyContext destroy = nullptr;
    PfnFfxConfigure configure = nullptr;
    PfnFfxQuery query = nullptr;
    PfnFfxDispatch dispatch = nullptr;
    PFN_vkCreateSwapchainKHR nativeCreate = nullptr;
    PFN_vkDestroySwapchainKHR nativeDestroy = nullptr;
    PFN_vkGetSwapchainImagesKHR nativeImages = nullptr;
    PFN_vkAcquireNextImageKHR nativeAcquire = nullptr;
    PFN_vkQueuePresentKHR nativePresent = nullptr;
    PFN_vkGetDeviceProcAddr nativeProc = nullptr;
    ffxContext swapContext = nullptr, fgContext = nullptr;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    ffxCreateContextDescFrameGenerationSwapChainVK swapDescription{};
    ffxCreateContextDescFrameGenerationSwapChainModeVK composition{};
    ffxQueryDescSwapchainReplacementFunctionsVK functions{};
    ffxCreateContextDescFrameGeneration fgDescription{};
    ffxCreateBackendVKDesc backend{};
    std::optional<QueuePlan> reserved;
    framegen::Config config{};
    dlss_fg::DepthRemapper depth;
    dlss_fg::HostInputUse hostUse;
    std::shared_ptr<frame_generation::ProducerSnapshot> retained;
    frame_plan::FramePlan previousPlan{};
    temporal::Matrix previousVP{};
    temporal::Viewport previousRaster{};
    uint64_t previousFrame = 0, previousEpoch = 0, frame = 0, presents = 0;
    std::atomic<bool> reset{true};
    std::atomic<uint32_t> callbackError{FFX_API_RETURN_OK};
    std::atomic<uint64_t> generations{0};
    bool installed = false, failed = false, prepared = false, active = false, generating = false;

    static VkResult SubmitGame(uint32_t count, const VkSubmitInfo* submits, VkFence fence) {
        auto& self = *current;
        const std::scoped_lock lock(*self.queue->queue->mutex);
        return vkQueueSubmit(self.queue->queue->vk, count, submits, fence);
    }
    static ffxReturnCode_t Generate(ffxDispatchDescFrameGeneration* params, void* user) {
        auto& self = *static_cast<Impl*>(user);
        if (!params || !self.fgContext) return FFX_API_RETURN_ERROR_PARAMETER;
        params->reset |= self.reset.load(std::memory_order_acquire);
        params->backbufferTransferFunction = FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SRGB;
        params->minMaxLuminance[0] = 0; params->minMaxLuminance[1] = 1;
        const auto result = self.dispatch(&self.fgContext, &params->header);
        if (result != FFX_API_RETURN_OK) self.callbackError.store(result, std::memory_order_release);
        else if (params->numGeneratedFrames) self.generations.fetch_add(1, std::memory_order_relaxed);
        return result;
    }
    // FidelityFX 1.1.4 resolves vkGetBufferMemoryRequirements2KHR, an alias of a
    // Vulkan 1.1 core command. Plume does not enable that promoted extension and
    // strict drivers return NULL for the alias, so fall back to the core name
    // instead of changing device creation for every backend user.
    static PFN_vkVoidFunction VKAPI_PTR DeviceProc(VkDevice d, const char* name) {
        auto& self = *current;
        if (const auto function = self.nativeProc(d, name)) return function;
        constexpr std::string_view suffix = "KHR";
        const std::string_view requested = name ? name : "";
        if (requested.size() <= suffix.size() || !requested.ends_with(suffix)) return nullptr;
        const std::string core(requested.substr(0, requested.size() - suffix.size()));
        const auto function = self.nativeProc(d, core.c_str());
        if (function) LOG_INFO("Vulkan FSR FG: resolved {} through core {}", requested, core);
        return function;
    }
    void Fail(std::string_view reason) {
        if (!failed) LOG_ERROR("Vulkan FSR FG unavailable: {}", reason);
        failed = true; active = false;
    }
    bool Configure(bool enabled, std::string& reason) {
        if (!fgContext) return true;
        ffxConfigureDescFrameGeneration desc{};
        desc.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
        desc.swapChain = reinterpret_cast<void*>(swapchain);
        desc.frameGenerationEnabled = enabled; desc.allowAsyncWorkloads = false;
        desc.frameGenerationCallback = Generate; desc.frameGenerationCallbackUserContext = this;
        desc.generationRect = {0, 0, int32_t(fgDescription.displaySize.width), int32_t(fgDescription.displaySize.height)};
        desc.frameID = frame;
        if (!Check(configure(&fgContext, &desc.header), "configure", reason)) return false;
        generating = enabled;
        return true;
    }
    // Disabling only stops later SDK presents from interpolating. It retires no
    // GPU work; a context that cannot be disabled is never prepared again.
    void Off() {
        prepared = active = false; previousFrame = previousEpoch = 0;
        if (!generating) return;
        std::string reason;
        if (!Configure(false, reason)) Fail(reason);
    }
    // Boundary drain: SDK presentation work, then every native queue. Never a
    // per-frame wait; the SDK's waitForPresents also idles its own queues.
    VkResult WaitSdk() {
        if (swapContext) {
            ffxDispatchDescFrameGenerationSwapChainWaitForPresentsVK wait{};
            wait.header.type = FFX_API_DISPATCH_DESC_TYPE_FGSWAPCHAIN_WAIT_FOR_PRESENTS_VK;
            if (dispatch(&swapContext, &wait.header) != FFX_API_RETURN_OK) return VK_ERROR_UNKNOWN;
        }
        return vkDeviceWaitIdle(device->vk);
    }
    // Producer copies precede the host batch on the shared game queue.
    VkResult WaitGameQueue() {
        const std::scoped_lock lock(*queue->queue->mutex);
        return vkQueueWaitIdle(queue->queue->vk);
    }
    void ReleaseInputs(bool executed) {
        retained.reset();
        // An unexecuted recording changed Plume's cached layout of its image.
        if (executed) depth.ReleaseAfterInputDrain(); else depth.DiscardUnsubmitted();
    }
    void DrainBoundary() {
        Off();
        Require(Retired(WaitSdk()), "SDK and native queue completion");
        if (!hostUse.Pending()) return;
        Require(hostUse.Completed(), "boundary drain before host submission");
        ReleaseInputs(true);
    }
    void DestroyFeature() {
        if (!fgContext) return;
        DrainBoundary();
        Require(destroy(&fgContext, nullptr) == FFX_API_RETURN_OK, "destroy interpolation context");
        fgContext = nullptr; generating = false;
    }
    bool Reserve(VkSurfaceKHR surface, std::string& reason) {
        if (reserved) return true;
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device->physicalDevice, &count, nullptr);
        std::vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device->physicalDevice, &count, properties.data());
        std::vector<QueueCandidate> candidates;
        for (uint32_t family = 0; family < device->queueFamilies.size(); ++family) {
            auto& pool = device->queueFamilies[family];
            if (pool.queues.empty()) continue;
            VkBool32 present = VK_FALSE;
            if (family >= count || vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice, family, surface, &present) != VK_SUCCESS) {
                reason = "surface queue support query failed"; return false;
            }
            const auto flags = properties[family].queueFlags;
            const std::scoped_lock lock(*pool.allocationMutex);
            for (uint32_t index = 0; index < pool.queues.size(); ++index) {
                const auto& q = pool.queues[index];
                if (q.reserved) continue;
                candidates.push_back({family, index, bool(flags & VK_QUEUE_COMPUTE_BIT),
                    bool(flags & (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)),
                    present == VK_TRUE, q.virtualQueues.empty() && q.vk != queue->queue->vk});
            }
        }
        auto plan = PlanQueues(candidates);
        if (!plan) { reason = "four distinct Vulkan queues with compute/present support and host capacity are required"; return false; }
        size_t acquired = 0;
        for (; acquired < plan->size(); ++acquired) {
            const auto& q = (*plan)[acquired];
            if (!device->queueFamilies[q.family].reserve(q.index)) break;
        }
        if (acquired != plan->size()) {
            while (acquired) { const auto& q = (*plan)[--acquired]; device->queueFamilies[q.family].release(q.index); }
            reason = "Vulkan queue reservation raced another allocation"; return false;
        }
        reserved = plan;
        const auto info = [&](size_t i) {
            const auto& q = (*plan)[i];
            return VkQueueInfoFFXAPI{device->queueFamilies[q.family].queues[q.index].vk, q.family, nullptr};
        };
        swapDescription.gameQueue = {queue->queue->vk, queue->familyIndex, SubmitGame};
        swapDescription.asyncComputeQueue = info(0);
        swapDescription.presentQueue = info(1);
        swapDescription.imageAcquireQueue = info(2);
        LOG_INFO("Vulkan FSR FG queues: game={}:{} compute={}:{} present={}:{} acquire={}:{} exclusive=1",
            queue->familyIndex, queue->queueIndex, (*plan)[0].family, (*plan)[0].index,
            (*plan)[1].family, (*plan)[1].index, (*plan)[2].family, (*plan)[2].index);
        return true;
    }
    static VkResult VKAPI_PTR CreateSwapchain(VkDevice d, const VkSwapchainCreateInfoKHR* desc,
        const VkAllocationCallbacks* allocator, VkSwapchainKHR* output) {
        auto& self = *current;
        if (d != self.device->vk || self.failed) return self.nativeCreate(d, desc, allocator, output);
        if (!desc || !output || self.swapContext || desc->oldSwapchain) return VK_ERROR_INITIALIZATION_FAILED;
        std::string reason;
        if (self.Reserve(desc->surface, reason)) {
            auto& create = self.swapDescription;
            create.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FGSWAPCHAIN_VK;
            create.header.pNext = &self.composition.header;
            self.composition.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FGSWAPCHAIN_MODE_VK;
            self.composition.composeOnPresentQueue = false;
            create.physicalDevice = self.device->physicalDevice; create.device = d;
            create.swapchain = &self.swapchain;
            create.allocator = const_cast<VkAllocationCallbacks*>(allocator); create.createInfo = *desc;
            if (Check(self.create(&self.swapContext, &create.header, nullptr), "swapchain context", reason) && self.swapchain) {
                self.functions = {};
                self.functions.header.type = FFX_API_QUERY_DESC_TYPE_FGSWAPCHAIN_FUNCTIONS_VK;
                if (Check(self.query(&self.swapContext, &self.functions.header), "WSI functions", reason)) {
                    if (self.functions.pOutGetSwapchainImagesKHR && self.functions.pOutAcquireNextImageKHR && self.functions.pOutQueuePresentKHR) {
                        *output = self.swapchain; return VK_SUCCESS;
                    }
                    reason = "SDK returned incomplete WSI functions";
                }
            }
            if (self.swapContext) {
                Require(Retired(self.WaitSdk()), "failed swapchain drain");
                Require(self.destroy(&self.swapContext, nullptr) == FFX_API_RETURN_OK, "failed swapchain cleanup");
                self.swapContext = nullptr; self.swapchain = VK_NULL_HANDLE;
            }
        }
        self.Fail(reason + "; creating ordinary swapchain");
        return self.nativeCreate(d, desc, allocator, output);
    }
    static void VKAPI_PTR DestroySwapchain(VkDevice d, VkSwapchainKHR swap, const VkAllocationCallbacks* allocator) {
        auto& self = *current;
        if (self.swapContext && swap == self.swapchain) {
            if (self.fgContext) self.DestroyFeature(); // Drains first.
            else self.DrainBoundary();
            Require(self.destroy(&self.swapContext, nullptr) == FFX_API_RETURN_OK, "destroy SDK swapchain");
            self.swapContext = nullptr; self.swapchain = VK_NULL_HANDLE;
        } else self.nativeDestroy(d, swap, allocator);
    }
    static VkResult VKAPI_PTR Images(VkDevice d, VkSwapchainKHR swap, uint32_t* count, VkImage* images) {
        auto& s = *current;
        return s.swapContext && swap == s.swapchain ? s.functions.pOutGetSwapchainImagesKHR(d, swap, count, images) : s.nativeImages(d, swap, count, images);
    }
    static VkResult VKAPI_PTR Acquire(VkDevice d, VkSwapchainKHR swap, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* index) {
        auto& s = *current;
        return s.swapContext && swap == s.swapchain ? s.functions.pOutAcquireNextImageKHR(d, swap, timeout, semaphore, fence, index) : s.nativeAcquire(d, swap, timeout, semaphore, fence, index);
    }
    static VkResult VKAPI_PTR Present(VkQueue q, const VkPresentInfoKHR* info) {
        auto& s = *current;
        if (s.swapContext && info && info->swapchainCount == 1 && info->pSwapchains[0] == s.swapchain)
            return s.functions.pOutQueuePresentKHR(q, info);
        // Plume delegates synchronization to this hook even after SDK fallback.
        const std::scoped_lock lock(*s.queue->queue->mutex);
        return s.nativePresent(q, info);
    }
    bool EnsureFeature(uint32_t width, uint32_t height, VkFormat format,
        uint32_t inputWidth, uint32_t inputHeight, std::string& reason) {
        if (fgContext && fgDescription.displaySize.width == width && fgDescription.displaySize.height == height &&
            fgDescription.maxRenderSize.width == inputWidth && fgDescription.maxRenderSize.height == inputHeight &&
            fgDescription.backBufferFormat == ffxApiGetSurfaceFormatVK(format)) return true;
        DestroyFeature();
        if (!width || !height || width > INT32_MAX || height > INT32_MAX ||
            (format != VK_FORMAT_R8G8B8A8_UNORM && format != VK_FORMAT_B8G8R8A8_UNORM)) {
            reason = "FSR FG requires an SDR RGBA8/BGRA8 output"; return false;
        }
        fgDescription = {}; backend = {};
        fgDescription.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;
        fgDescription.header.pNext = &backend.header;
        fgDescription.displaySize = {width, height}; fgDescription.maxRenderSize = {inputWidth, inputHeight};
        fgDescription.backBufferFormat = ffxApiGetSurfaceFormatVK(format);
        fgDescription.flags = FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED;
        backend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK;
        backend.vkDevice = device->vk; backend.vkPhysicalDevice = device->physicalDevice; backend.vkDeviceProcAddr = DeviceProc;
        return Check(create(&fgContext, &fgDescription.header, nullptr), "interpolation context", reason);
    }
    ~Impl() {
        if (installed) {
            Require(!swapContext && !fgContext && !hostUse.Pending(), "shutdown before swapchain/input retirement");
            vkCreateSwapchainKHR = nativeCreate; vkDestroySwapchainKHR = nativeDestroy;
            vkGetSwapchainImagesKHR = nativeImages; vkAcquireNextImageKHR = nativeAcquire;
            vkQueuePresentKHR = nativePresent;
            device->externalSwapchainSynchronization = device->destroySwapchainBeforeResize = false;
            current = nullptr;
        }
        if (reserved)
            for (const auto& q : *reserved) device->queueFamilies[q.family].release(q.index);
        if (module && !FreeLibrary(module)) LOG_WARNING("Vulkan FSR FG: runtime unload failed error={}", GetLastError());
    }
};
Session::Session() : impl_(std::make_unique<Impl>()) {}
Session::~Session() = default;
bool Session::Initialize(plume::VulkanDevice& device, plume::VulkanCommandQueue& queue,
    const std::filesystem::path& runtime, std::string& reason) {
    auto& s = *impl_;
    if (s.installed || Impl::current || !device.timelineSemaphoresSupported) {
        reason = "Vulkan FSR FG requires an unused WSI owner and enabled timeline semaphores"; return false;
    }
    s.device = &device; s.queue = &queue;
    std::error_code ec;
    const auto path = std::filesystem::absolute(runtime, ec);
    if (ec || !std::filesystem::is_regular_file(path, ec)) { reason = "FidelityFX Vulkan runtime missing"; return false; }
    s.module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!s.module) { reason = "FidelityFX Vulkan runtime load failed"; return false; }
    s.create = reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(s.module, "ffxCreateContext"));
    s.destroy = reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(s.module, "ffxDestroyContext"));
    s.configure = reinterpret_cast<PfnFfxConfigure>(GetProcAddress(s.module, "ffxConfigure"));
    s.query = reinterpret_cast<PfnFfxQuery>(GetProcAddress(s.module, "ffxQuery"));
    s.dispatch = reinterpret_cast<PfnFfxDispatch>(GetProcAddress(s.module, "ffxDispatch"));
    if (!s.create || !s.destroy || !s.configure || !s.query || !s.dispatch) { reason = "FidelityFX Vulkan API export missing"; return false; }
    for (const auto type : {FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION, FFX_API_CREATE_CONTEXT_DESC_TYPE_FGSWAPCHAIN_VK}) {
        uint64_t count = 0;
        ffxQueryDescGetVersions versions{};
        versions.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS; versions.createDescType = type;
        versions.device = reinterpret_cast<void*>(device.vk); versions.outputCount = &count;
        if (!Check(s.query(nullptr, &versions.header), "provider query", reason) || !count) {
            reason = "FidelityFX runtime has no required Vulkan FG/swapchain provider"; return false;
        }
    }
    if (!s.depth.Initialize(&device)) { reason = "FG depth conversion unavailable"; return false; }
    s.nativeCreate = vkCreateSwapchainKHR; s.nativeDestroy = vkDestroySwapchainKHR;
    s.nativeImages = vkGetSwapchainImagesKHR; s.nativeAcquire = vkAcquireNextImageKHR;
    s.nativePresent = vkQueuePresentKHR; s.nativeProc = vkGetDeviceProcAddr;
    Impl::current = &s;
    // Only WSI entry points are replaced. Every other Vulkan caller, including
    // vkDeviceWaitIdle, keeps native behavior; SDK drains are explicit.
    vkCreateSwapchainKHR = Impl::CreateSwapchain; vkDestroySwapchainKHR = Impl::DestroySwapchain;
    vkGetSwapchainImagesKHR = Impl::Images; vkAcquireNextImageKHR = Impl::Acquire;
    vkQueuePresentKHR = Impl::Present;
    device.externalSwapchainSynchronization = device.destroySwapchainBeforeResize = true;
    s.installed = true;
    return true;
}
bool Session::Requested() const { return impl_->config.provider == framegen::Provider::Fsr && !impl_->failed; }
bool Session::Available() const { return Requested() && impl_->active; }
bool Session::Failed() const { return impl_->failed; }
bool Session::UsesProxySwapchain() const { return impl_->swapContext != nullptr; }
bool Session::Reconfigure(const framegen::Config& config, std::string& reason) {
    auto& s = *impl_;
    Quiesce(); s.config = {};
    if (config.provider == framegen::Provider::Off || config.mode == framegen::Mode::Off) return true;
    if (config.provider != framegen::Provider::Fsr || config.mode != framegen::Mode::Fixed || config.generatedFrames != 1) {
        reason = "Vulkan FSR 3.1.4 FG supports fixed 2x only"; return false;
    }
    if (!s.swapContext || s.failed) { reason = "Vulkan FSR FG session is unavailable"; return false; }
    s.config = config; return true;
}
void Session::PrepareAfterHostDrain(const frame_generation::CompositeHandoff& inputs,
    plume::VulkanSwapChain& swap, plume::VulkanCommandList& commands) {
    auto& s = *impl_;
    // The host waited for its previous presentation batch before recording;
    // AfterHostDrain, a failed submit, or cancellation ended that input lease.
    Require(!s.hostUse.Pending(), "prepare before previous input retirement");
    s.prepared = false;
    if (!Requested() || !inputs.ReadyForOrderedSubmission() || swap.vk != s.swapchain ||
        !dlss_fg::FullFramePresentation(inputs.outputWidth, inputs.outputHeight, swap.getWidth(), swap.getHeight())) { s.Off(); return; }
    const auto& in = inputs.producer->inputs;
    const bool reset = s.previousFrame + 1 != in.renderFrameId || s.previousEpoch != in.temporalEpoch ||
        in.resetHistory || in.motionState == temporal::MotionState::ResetInitialization ||
        !dlss_fg::SameHistoryConfiguration(s.previousPlan, in.plan);
    framegen::Camera camera{};
    frame_generation::DepthRemap mapping{};
    if (!frame_generation::BuildCamera(in, reset ? nullptr : &s.previousVP, camera, &mapping,
        reset ? nullptr : &s.previousRaster)) { s.Off(); return; }
    std::string reason;
    if (!s.EnsureFeature(swap.getWidth(), swap.getHeight(), swap.createInfo.imageFormat, in.depth.width, in.depth.height, reason)) {
        s.Off(); s.Fail(reason); return;
    }
    Require(s.hostUse.Begin(&commands), "overlapping input recording");
    s.retained = inputs.producer;
    auto* converted = s.depth.Record(&commands, in.depth, mapping);
    if (!converted) { s.Off(); return; }
    Require(s.frame != UINT64_MAX, "frame ID overflow"); ++s.frame;
    s.reset.store(reset || !s.previousFrame, std::memory_order_release);
    ffxDispatchDescFrameGenerationPrepare prepare{};
    prepare.header.type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE;
    prepare.frameID = s.frame; prepare.renderSize = {in.depth.width, in.depth.height};
    prepare.jitterOffset = {camera.jitterX, camera.jitterY}; prepare.motionVectorScale = {1, 1};
    prepare.frameTimeDelta = in.frameTimeDeltaMilliseconds;
    prepare.cameraNear = camera.nearPlane; prepare.cameraFar = camera.farPlane;
    prepare.cameraFovAngleVertical = camera.fovRadians; prepare.viewSpaceToMetersFactor = 1;
    prepare.depth = Resource(*static_cast<plume::VulkanTexture*>(converted));
    prepare.motionVectors = Resource(*static_cast<plume::VulkanTexture*>(in.motion.texture));
    ffxDispatchDescFrameGenerationPrepareCameraInfo cameraInfo{};
    cameraInfo.header.type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_CAMERAINFO;
    std::copy(camera.position.begin(), camera.position.end(), cameraInfo.cameraPosition);
    std::copy(camera.right.begin(), camera.right.end(), cameraInfo.cameraRight);
    std::copy(camera.up.begin(), camera.up.end(), cameraInfo.cameraUp);
    std::copy(camera.forward.begin(), camera.forward.end(), cameraInfo.cameraForward);
    prepare.header.pNext = &cameraInfo.header;
    if (!s.Configure(true, reason)) { s.Off(); s.Fail(reason); return; }
    prepare.commandList = reinterpret_cast<void*>(commands.beginExternalCommands());
    Require(prepare.commandList != nullptr, "native command scope");
    const auto result = s.dispatch(&s.fgContext, &prepare.header);
    commands.endExternalCommands();
    if (!Check(result, "prepare", reason)) { s.Off(); s.Fail(reason); return; }
    s.prepared = true;
    s.previousFrame = in.renderFrameId; s.previousEpoch = in.temporalEpoch;
    s.previousVP = in.cameraViewProjection; s.previousRaster = in.cameraRaster; s.previousPlan = in.plan;
}
void Session::SubmitStart() { Require(impl_->hostUse.SubmissionStarted(), "duplicate submit"); }
void Session::HostSubmitted(bool success, uint64_t serial) {
    auto& s = *impl_;
    if (!s.hostUse.Pending()) return;
    if (success) { Require(s.hostUse.Submitted(true, serial), "host submission serial"); return; }
    // vkQueueSubmit failure leaves recorded resources unaffected (or the device
    // is lost); only the producer copies before it may still be running.
    s.Off();
    Require(Retired(s.WaitGameQueue()), "failed submission producer completion");
    Require(s.hostUse.SubmitFailed(), "failed submission lease");
    s.ReleaseInputs(false);
}
void Session::Presented(bool accepted) {
    auto& s = *impl_;
    const bool callbackFailed = s.callbackError.load(std::memory_order_acquire) != FFX_API_RETURN_OK;
    if (++s.presents % 300 == 0)
        LOG_INFO("Vulkan FSR FG: presents={} prepared_frames={} generation_dispatches={} active={} scope=sdk_not_display",
            s.presents, s.frame, s.generations.load(std::memory_order_relaxed), s.prepared && accepted);
    s.active = accepted && s.prepared && !callbackFailed;
    // A rejected present still leaves Prepare in the submitted host batch; the
    // host fence retires it. Only the interpolation history is lost.
    if (!accepted || callbackFailed) s.Off();
    if (callbackFailed) s.Fail("generation callback failed");
    s.prepared = false;
}
void Session::AfterHostDrain() {
    auto& s = *impl_;
    // Unsubmitted recordings belong to the current batch, not to this fence.
    if (!s.hostUse.Pending() || !s.hostUse.Serial()) return;
    // Prepare in that batch was the only reader; SDK presents use SDK images.
    Require(s.hostUse.Completed(), "input completion");
    s.ReleaseInputs(true);
}
void Session::CancelUnsubmitted(plume::RenderCommandList* list) {
    auto& s = *impl_;
    if (!s.hostUse.Matches(list) || s.hostUse.Serial()) return;
    Require(s.hostUse.CanCancel(list), "cancellation after submit attempt");
    auto& commands = *static_cast<plume::VulkanCommandList*>(list);
    if (commands.recording) Require(submission::EndCommands(commands) == VK_SUCCESS, "cancel command end");
    Require(vkResetCommandBuffer(commands.vk, 0) == VK_SUCCESS, "cancel command reset");
    submission::ClearBindings(commands);
    s.Off();
    Require(Retired(s.WaitGameQueue()), "cancel producer completion");
    Require(s.hostUse.Canceled(list, true, true, true), "cancel input lease");
    s.ReleaseInputs(false);
}
void Session::Quiesce() {
    auto& s = *impl_;
    if (!s.installed) return;
    s.DrainBoundary();
}
} // namespace gpu::fsr_fg
#endif
