#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif
#include "xess_upscaler.h"

#if defined(LO_GPU_PLUME)
#if defined(_WIN32) && defined(LO_HAS_XESS) && LO_HAS_XESS
#include <os/logger.h>
#include <os/runtime_libraries.h>
#include <os/user_paths.h>
#include <plume_d3d12.h>
#include <xess/xess_d3d12.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <type_traits>
#include <vector>
#endif

namespace gpu::xess {

#if defined(_WIN32) && defined(LO_HAS_XESS) && LO_HAS_XESS
namespace {
// Each persisted quality ID selects the XeSS preset of the same name; XeSS 1.3+
// owns the ratios (Quality 1.7x, Balanced 2.0x, Performance 2.3x, AA 1.0x).
xess_quality_settings_t Preset(upscaling::FsrQuality quality) {
    switch (upscaling::NormalizeFsrQuality(quality)) {
    case upscaling::FsrQuality::Quality: return XESS_QUALITY_SETTING_QUALITY;
    case upscaling::FsrQuality::Balanced: return XESS_QUALITY_SETTING_BALANCED;
    case upscaling::FsrQuality::Performance: return XESS_QUALITY_SETTING_PERFORMANCE;
    case upscaling::FsrQuality::NativeAA: return XESS_QUALITY_SETTING_AA;
    }
    return XESS_QUALITY_SETTING_QUALITY;
}
bool LostDevice(HRESULT result) {
    return result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET || result == DXGI_ERROR_DEVICE_HUNG;
}
Status FromSdk(xess_result_t result) {
    if (result >= XESS_RESULT_SUCCESS) return Status::Ready; // Positive values are warnings.
    switch (result) {
    case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE:
    case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER:
    case XESS_RESULT_ERROR_UNSUPPORTED:
    case XESS_RESULT_ERROR_NOT_IMPLEMENTED:
    case XESS_RESULT_ERROR_CANT_LOAD_LIBRARY: return Status::Unavailable;
    default: return Status::Failed;
    }
}
bool ValidTexture(const plume::D3D12Texture& texture, const plume::D3D12Device& device,
    D3D12_RESOURCE_STATES state, plume::RenderFormat format = plume::RenderFormat::UNKNOWN) {
    return texture.device == &device && texture.d3d && texture.allocation &&
        texture.desc.dimension == plume::RenderTextureDimension::TEXTURE_2D &&
        texture.desc.mipLevels == 1 && texture.desc.arraySize == 1 &&
        texture.desc.multisampling.sampleCount == 1 && texture.desc.width && texture.desc.height &&
        (format == plume::RenderFormat::UNKNOWN || texture.desc.format == format) &&
        texture.resourceStates == state &&
        texture.layout == (state == D3D12_RESOURCE_STATE_UNORDERED_ACCESS ?
            plume::RenderTextureLayout::GENERAL : plume::RenderTextureLayout::SHADER_READ);
}
HRESULT BeginIsolated(plume::D3D12CommandList& list) {
    if (list.open || !list.d3d || !list.commandAllocator || !list.queue ||
        list.queue->type != plume::RenderCommandListType::DIRECT) return E_INVALIDARG;
    list.invalidateCachedNativeState();
    list.resetRootBindingStats();
    HRESULT result = list.commandAllocator->Reset();
    if (SUCCEEDED(result)) result = list.d3d->Reset(list.commandAllocator, nullptr);
    if (SUCCEEDED(result)) list.open = true;
    return result;
}
HRESULT EndIsolated(plume::D3D12CommandList& list) {
    if (!list.open) return E_INVALIDARG;
    list.resetSamplePositions();
    const HRESULT result = list.d3d->Close();
    list.open = false;
    // XeSS binds its own descriptor heap, root signature and pipeline.
    list.invalidateCachedNativeState();
    return result;
}
// On-hardware convention checks, as for MetalFX: LO_XESS_JITTER_SCALE="x,y"
// and LO_XESS_MV_SCALE="x,y" feed the SDK's own scale setters.
struct Scales { float jitterX = 1, jitterY = 1, motionX = 1, motionY = 1; };
const Scales& EnvironmentScales() {
    static const Scales scales = [] {
        Scales value;
        if (const char* text = std::getenv("LO_XESS_JITTER_SCALE")) std::sscanf(text, "%f,%f", &value.jitterX, &value.jitterY);
        if (const char* text = std::getenv("LO_XESS_MV_SCALE")) std::sscanf(text, "%f,%f", &value.motionX, &value.motionY);
        return value;
    }();
    return scales;
}
} // namespace

struct Controller::Impl {
    HMODULE module = nullptr;
    bool loadAttempted = false;
    decltype(&xessD3D12CreateContext) create = nullptr;
    decltype(&xessD3D12Init) init = nullptr;
    decltype(&xessD3D12Execute) execute = nullptr;
    decltype(&xessDestroyContext) destroy = nullptr;
    decltype(&xessGetOptimalInputResolution) optimal = nullptr;
    decltype(&xessIsOptimalDriver) optimalDriver = nullptr;
    decltype(&xessSetJitterScale) jitterScale = nullptr;
    decltype(&xessSetVelocityScale) velocityScale = nullptr;
    decltype(&xessGetVersion) version = nullptr;

    xess_context_handle_t context = nullptr;
    plume::D3D12Device* device = nullptr;
    Config config{};
    bool poisoned = false;
    struct Use { uint64_t id = 0, serial = 0; };
    std::vector<Use> uses;
    uint64_t nextUse = 1, completed = 0;
    std::optional<uint64_t> lastRecordedRenderFrameId;

    bool Load() {
        if (loadAttempted) return module != nullptr;
        loadAttempted = true;
        std::filesystem::path path;
        if (const char* override = std::getenv("LO_XESS_RUNTIME_PATH"); override && *override) path = override;
        else {
            std::error_code error;
            path = os::runtime_libraries::Find("libxess.dll", os::user_paths::ExecutableDir(),
                std::filesystem::current_path(error)) / "libxess.dll";
        }
        module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!module) { LOG_WARNING("XeSS: libxess.dll unavailable at {}", path.string()); return false; }
        const auto load = [&](auto& target, const char* name) {
            target = reinterpret_cast<std::remove_reference_t<decltype(target)>>(GetProcAddress(module, name));
            return target != nullptr;
        };
        const bool complete = load(create, "xessD3D12CreateContext") && load(init, "xessD3D12Init") &&
            load(execute, "xessD3D12Execute") && load(destroy, "xessDestroyContext") &&
            load(optimal, "xessGetOptimalInputResolution") && load(optimalDriver, "xessIsOptimalDriver") &&
            load(jitterScale, "xessSetJitterScale") && load(velocityScale, "xessSetVelocityScale") &&
            load(version, "xessGetVersion");
        if (!complete) {
            LOG_WARNING("XeSS: libxess.dll lacks a required export");
            FreeLibrary(module); module = nullptr; return false;
        }
        xess_version_t v{};
        if (version(&v) == XESS_RESULT_SUCCESS)
            LOG_INFO("XeSS: loaded libxess {}.{}.{} from {}", v.major, v.minor, v.patch, path.string());
        return true;
    }
    void Destroy() {
        if (context) {
            const auto result = destroy(context);
            if (result != XESS_RESULT_SUCCESS) LOG_WARNING("XeSS: xessDestroyContext result={}", int(result));
        }
        context = nullptr; device = nullptr; poisoned = false;
        lastRecordedRenderFrameId.reset();
    }
};

Controller::Controller() : impl_(std::make_unique<Impl>()) {}
Controller::~Controller() {
    // Owners call ShutdownAfterGpuDrain first; this only covers an idle context.
    if (impl_->uses.empty()) impl_->Destroy();
    if (impl_->module && !impl_->context) FreeLibrary(impl_->module);
}

upscaling::OutputSizing Controller::QuerySizing(const plume::D3D12Device& device, const upscaling::SizingKey& key) {
    upscaling::OutputSizing sizing{};
    sizing.key = key;
    for (auto& mode : sizing.modes) { mode.state = upscaling::SizingState::Unavailable; mode.issue = upscaling::SizingIssue::Prerequisite; }
    if (key.provider != upscaling::Upscaler::Xess || !key.outputWidth || !key.outputHeight ||
        !device.d3d || !impl_->Load()) return sizing;
    // Optimal input sizes need only a context, not xessD3D12Init. A separate
    // short-lived context keeps sizing independent of the execution context.
    xess_context_handle_t context = nullptr;
    const auto created = impl_->create(device.d3d, &context);
    if (created < XESS_RESULT_SUCCESS || !context) {
        for (auto& mode : sizing.modes) {
            mode.ngxResult = int32_t(created);
            if (FromSdk(created) != Status::Unavailable) { mode.state = upscaling::SizingState::Error; mode.issue = upscaling::SizingIssue::CapabilityParameters; }
        }
        LOG_INFO("XeSS: unavailable on this adapter result={}", int(created));
        return sizing;
    }
    const xess_2d_t output{key.outputWidth, key.outputHeight};
    for (uint32_t i = 0; i < sizing.modes.size(); ++i) {
        auto& mode = sizing.modes[i];
        xess_2d_t optimal{}, minimum{}, maximum{};
        const auto result = impl_->optimal(context, &output, Preset(upscaling::FsrQuality(i)), &optimal, &minimum, &maximum);
        mode.ngxResult = int32_t(result);
        if (result < XESS_RESULT_SUCCESS) {
            mode.state = upscaling::SizingState::Error; mode.issue = upscaling::SizingIssue::OptimalQuery; continue;
        }
        mode.optimal = {optimal.x, optimal.y}; mode.minimum = {minimum.x, minimum.y}; mode.maximum = {maximum.x, maximum.y};
        // Native AA must render at the output extent; the planner checks equality.
        if (upscaling::FsrQuality(i) == upscaling::FsrQuality::NativeAA) mode.optimal = {key.outputWidth, key.outputHeight};
        const bool valid = mode.optimal.width && mode.optimal.height &&
            mode.optimal.width <= key.outputWidth && mode.optimal.height <= key.outputHeight;
        mode.state = valid ? upscaling::SizingState::Ready : upscaling::SizingState::Error;
        mode.issue = valid ? upscaling::SizingIssue::None : upscaling::SizingIssue::InvalidRange;
    }
    const auto destroyed = impl_->destroy(context);
    if (destroyed != XESS_RESULT_SUCCESS)
        for (auto& mode : sizing.modes) { mode.cleanupResult = int32_t(destroyed); }
    return sizing;
}

Status Controller::EnsureSession(plume::D3D12Device& device, const Config& config) {
    if (!device.d3d || !config.renderWidth || !config.renderHeight || !config.outputWidth ||
        !config.outputHeight || !config.deviceEpoch || !upscaling::KnownFsrQuality(config.quality)) return Status::Unavailable;
    if (impl_->context) return !impl_->poisoned && impl_->device == &device && impl_->config == config ?
        Status::Ready : Status::NeedsReconfigure;
    // A stale device or in-flight uses require the renderer's drained reconfigure.
    if (impl_->device || !impl_->uses.empty()) return Status::NeedsReconfigure;
    if (!impl_->Load()) return Status::Unavailable;
    auto result = impl_->create(device.d3d, &impl_->context);
    if (result < XESS_RESULT_SUCCESS || !impl_->context) {
        LOG_WARNING("XeSS: xessD3D12CreateContext result={}", int(result));
        impl_->context = nullptr;
        return FromSdk(result) == Status::Ready ? Status::Failed : FromSdk(result);
    }
    impl_->device = &device; impl_->config = config;
    if (impl_->optimalDriver(impl_->context) != XESS_RESULT_SUCCESS)
        LOG_WARNING("XeSS: the installed graphics driver is older than XeSS recommends");
    xess_d3d12_init_params_t params{};
    params.outputResolution = {config.outputWidth, config.outputHeight};
    params.qualitySetting = Preset(config.quality);
    // Scene color is display-encoded RGBA8 (ColorEncoding::Sdr), motion is
    // low-res, unjittered, previous-minus-current in pixels.
    params.initFlags = XESS_INIT_FLAG_LDR_INPUT_COLOR | (config.depthInverted ? XESS_INIT_FLAG_INVERTED_DEPTH : 0u);
    // Blocking: first use may compile pipelines. The renderer falls back meanwhile.
    result = impl_->init(impl_->context, &params);
    if (result < XESS_RESULT_SUCCESS) {
        LOG_WARNING("XeSS: xessD3D12Init {}x{} quality={} result={}", config.outputWidth, config.outputHeight,
            uint32_t(config.quality), int(result));
        const auto status = FromSdk(result);
        impl_->Destroy();
        return LostDevice(device.d3d->GetDeviceRemovedReason()) ? Status::DeviceLost : status;
    }
    const auto& scales = EnvironmentScales();
    impl_->jitterScale(impl_->context, scales.jitterX, scales.jitterY);
    impl_->velocityScale(impl_->context, scales.motionX, scales.motionY);
    LOG_INFO("XeSS: ready {}x{} -> {}x{} quality={}", config.renderWidth, config.renderHeight,
        config.outputWidth, config.outputHeight, uint32_t(config.quality));
    return Status::Ready;
}

Attempt Controller::RecordIsolated(plume::D3D12CommandList& commands, const Config& config,
    const temporal::TemporalFrameInputs& inputs, plume::D3D12Texture& output) {
    Attempt attempt{};
    auto& impl = *impl_;
    if (!impl.context || impl.poisoned || !(impl.config == config)) { attempt.status = Status::NeedsReconfigure; return attempt; }
    const auto& device = *impl.device;
    const auto validRegion = [&](const temporal::TextureRegion& region, plume::RenderFormat format) {
        if (!region.Complete() || region.width != config.renderWidth || region.height != config.renderHeight) return false;
        const auto& native = *static_cast<const plume::D3D12Texture*>(region.texture);
        return ValidTexture(native, device, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, format) &&
            region.allocation.width == native.desc.width && region.allocation.height == native.desc.height;
    };
    if (!inputs.CompleteForConsumer() || inputs.plan.consumer != upscaling::TemporalConsumer::XessSr ||
        inputs.plan.deviceEpoch != config.deviceEpoch) {
        attempt.status = Status::InputUnavailable; return attempt;
    }
    // XeSS writes the output in the input color format; alpha becomes 1 and the
    // renderer's composite never reads it.
    if (inputs.colorEncoding != temporal::ColorEncoding::Sdr ||
        !temporal::MatchesDepthConvention(inputs.depthConvention, config.depthInverted) ||
        !validRegion(inputs.color, plume::RenderFormat::R8G8B8A8_UNORM) ||
        !validRegion(inputs.depth, plume::RenderFormat::R32_FLOAT) ||
        !validRegion(inputs.motion, plume::RenderFormat::R16G16_FLOAT) ||
        !ValidTexture(output, device, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, plume::RenderFormat::R8G8B8A8_UNORM) ||
        !(output.desc.flags & plume::RenderTextureFlag::UNORDERED_ACCESS) ||
        output.desc.width != config.outputWidth || output.desc.height != config.outputHeight ||
        !std::isfinite(inputs.jitter.pixelX) || !std::isfinite(inputs.jitter.pixelY)) {
        attempt.status = Status::Unavailable; return attempt;
    }
    if (!impl.nextUse) { attempt.status = Status::Failed; return attempt; }
    const bool frameGap = impl.lastRecordedRenderFrameId && inputs.renderFrameId != *impl.lastRecordedRenderFrameId + 1;
    const bool reset = inputs.resetHistory || !impl.lastRecordedRenderFrameId || frameGap;
    const HRESULT begin = BeginIsolated(commands);
    if (FAILED(begin)) {
        attempt.hrResult = int32_t(begin);
        attempt.status = LostDevice(begin) ? Status::DeviceLost : Status::Failed;
        return attempt;
    }
    attempt.useId = impl.nextUse++;
    impl.uses.push_back({attempt.useId, 0});
    xess_d3d12_execute_params_t params{};
    params.pColorTexture = static_cast<plume::D3D12Texture*>(inputs.color.texture)->d3d;
    params.pVelocityTexture = static_cast<plume::D3D12Texture*>(inputs.motion.texture)->d3d;
    params.pDepthTexture = static_cast<plume::D3D12Texture*>(inputs.depth.texture)->d3d;
    params.pOutputTexture = output.d3d;
    params.jitterOffsetX = float(inputs.jitter.pixelX);
    params.jitterOffsetY = float(inputs.jitter.pixelY);
    // LDR input: XeSS recommends exposure 1.0 without auto-exposure.
    params.exposureScale = 1.0f;
    params.resetHistory = reset ? 1u : 0u;
    params.inputWidth = config.renderWidth;
    params.inputHeight = config.renderHeight;
    params.inputColorBase = {inputs.color.x, inputs.color.y};
    params.inputMotionVectorBase = {inputs.motion.x, inputs.motion.y};
    params.inputDepthBase = {inputs.depth.x, inputs.depth.y};
    params.outputColorBase = {0, 0};
    const auto result = impl.execute(impl.context, commands.d3d, &params);
    attempt.sdkResult = int32_t(result);
    const HRESULT close = EndIsolated(commands);
    attempt.hrResult = int32_t(close);
    const HRESULT removed = device.d3d->GetDeviceRemovedReason();
    if (LostDevice(close) || LostDevice(removed)) attempt.status = Status::DeviceLost;
    else if (FAILED(close) || result < XESS_RESULT_SUCCESS) attempt.status = Status::Failed;
    else attempt.status = Status::Ready;
    if (attempt.status == Status::Ready) impl.lastRecordedRenderFrameId = inputs.renderFrameId;
    else {
        // A failed record may still be submitted; keep its use until retired.
        impl.poisoned = true;
        LOG_WARNING("XeSS: execute frame={} result={} close={}", inputs.renderFrameId, int(result), int32_t(close));
    }
    return attempt;
}

void Controller::OnBatchSubmitted(uint64_t useId, uint64_t serial) {
    if (!useId || !serial || serial <= impl_->completed) return;
    for (auto& use : impl_->uses) if (use.id == useId && !use.serial) { use.serial = serial; break; }
}
void Controller::OnBatchDiscarded(uint64_t useId) {
    auto& uses = impl_->uses;
    const auto it = std::find_if(uses.begin(), uses.end(), [useId](const Impl::Use& use) { return use.id == useId && !use.serial; });
    if (it == uses.end()) return;
    // History now lacks the discarded frame; the next record resets it.
    impl_->lastRecordedRenderFrameId.reset();
    uses.erase(it);
}
void Controller::ReleaseCompletedThrough(uint64_t serial) {
    impl_->completed = std::max(impl_->completed, serial);
    std::erase_if(impl_->uses, [&](const Impl::Use& use) { return use.serial && use.serial <= impl_->completed; });
}
bool Controller::HasFeatureState() const { return impl_->context || !impl_->uses.empty(); }
void Controller::ReleaseFeatureAfterGpuDrain() { if (impl_->uses.empty()) impl_->Destroy(); }
void Controller::ShutdownAfterGpuDrain() { impl_->uses.clear(); impl_->Destroy(); }
void Controller::AbandonUsesAfterDeviceLoss() { impl_->uses.clear(); impl_->Destroy(); }

#else
struct Controller::Impl {};
Controller::Controller() : impl_(std::make_unique<Impl>()) {}
Controller::~Controller() = default;
upscaling::OutputSizing Controller::QuerySizing(const plume::D3D12Device&, const upscaling::SizingKey& key) {
    upscaling::OutputSizing sizing{};
    sizing.key = key;
    for (auto& mode : sizing.modes) mode.state = upscaling::SizingState::Unavailable;
    return sizing;
}
Status Controller::EnsureSession(plume::D3D12Device&, const Config&) { return Status::Unavailable; }
Attempt Controller::RecordIsolated(plume::D3D12CommandList&, const Config&,
    const temporal::TemporalFrameInputs&, plume::D3D12Texture&) { return {}; }
void Controller::OnBatchSubmitted(uint64_t, uint64_t) {}
void Controller::OnBatchDiscarded(uint64_t) {}
void Controller::ReleaseCompletedThrough(uint64_t) {}
bool Controller::HasFeatureState() const { return false; }
void Controller::ReleaseFeatureAfterGpuDrain() {}
void Controller::ShutdownAfterGpuDrain() {}
void Controller::AbandonUsesAfterDeviceLoss() {}
#endif

} // namespace gpu::xess
#endif
