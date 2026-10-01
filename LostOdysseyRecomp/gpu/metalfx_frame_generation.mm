#include "metalfx_frame_generation.h"
#if defined(__APPLE__) && defined(LO_ENABLE_METALFX_FG)
#import <MetalFX/MetalFX.h>
#include <plume_metal.h>
#include "dlss_fg_depth.h"
#include "dlss_fg_host_use.h"
#include "metalfx_fg_policy.h"
#include <os/logger.h>
#include <cstdio>
#include <cstdlib>

namespace gpu::metalfx_fg {
namespace {
void Require(bool okay, const char* operation) {
    if (okay) return;
    LOG_ERROR("MetalFX FG: {} failed; retaining unresolved GPU ownership by terminating", operation);
    std::fflush(nullptr); std::_Exit(EXIT_FAILURE);
}
id<MTLTexture> Texture(plume::RenderTexture* texture) {
    if (!texture) return nil;
    return (__bridge id<MTLTexture>)static_cast<void*>(static_cast<plume::ExtendedRenderTexture*>(texture)->getTexture());
}
void Copy(plume::RenderCommandList* list, plume::RenderTexture* destination, plume::RenderTexture* source) {
    list->barriers(plume::RenderBarrierStage::COPY, plume::RenderTextureBarrier(source, plume::RenderTextureLayout::COPY_SOURCE));
    list->barriers(plume::RenderBarrierStage::COPY, plume::RenderTextureBarrier(destination, plume::RenderTextureLayout::COPY_DEST));
    list->copyTexture(destination, source);
}
}
struct Session::Impl {
    plume::RenderDevice* device = nullptr;
    dlss_fg::DepthRemapper depth;
    dlss_fg::HostInputUse use;
    std::shared_ptr<frame_generation::ProducerSnapshot> retained;
    std::unique_ptr<plume::RenderTexture> current, previous, generated;
    id<MTLCommandBuffer> recorded = nil;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
    id<MTLFXFrameInterpolator> interpolator API_AVAILABLE(macos(26.0)) = nil;
#endif
    id<MTLFence> fence = nil;
    framegen::Config config{};
    framegen::History history;
    framegen::HistoryKey pendingKey{};
    frame_plan::FramePlan previousPlan{}, pendingPlan{};
    uint64_t pendingFrame = 0, pairs = 0;
    uint32_t inputWidth = 0, inputHeight = 0, width = 0, height = 0;
    plume::RenderFormat format = plume::RenderFormat::UNKNOWN;
    bool supported = false, failed = false, active = false, prepared = false;

    void Fail(const char* reason) {
        if (!failed) LOG_ERROR("MetalFX FG unavailable: {}", reason);
        failed = true; active = false; history.Reset();
    }
    void Begin(plume::RenderCommandList* commands) {
        Require(use.Begin(commands), "overlapping input recording");
        recorded = (__bridge id<MTLCommandBuffer>)static_cast<void*>(static_cast<plume::MetalCommandList*>(commands)->mtl);
        Require(recorded != nil, "command buffer");
    }
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
    bool Ensure(uint32_t iw, uint32_t ih, uint32_t w, uint32_t h, plume::RenderFormat f) API_AVAILABLE(macos(26.0)) {
        if (interpolator && iw==inputWidth && ih==inputHeight && w==width && h==height && f==format) return true;
        Require(!use.Pending(), "recreate before completion");
        interpolator = nil; current.reset(); previous.reset(); generated.reset(); history.Reset();
        if (!iw || !ih || !w || !h || (f != plume::RenderFormat::R8G8B8A8_UNORM && f != plume::RenderFormat::B8G8R8A8_UNORM)) {
            Fail("unsupported SDR texture geometry/format"); return false;
        }
        const auto desc = plume::RenderTextureDesc::Texture2D(w,h,1,f,
            plume::RenderTextureFlag::RENDER_TARGET | plume::RenderTextureFlag::UNORDERED_ACCESS);
        current = device->createTexture(desc); previous = device->createTexture(desc); generated = device->createTexture(desc);
        if (!Texture(current.get()) || !Texture(previous.get()) || !Texture(generated.get())) {
            Fail("color/history allocation"); return false;
        }
        MTLFXFrameInterpolatorDescriptor* descriptor = [[MTLFXFrameInterpolatorDescriptor alloc] init];
        descriptor.inputWidth = iw; descriptor.inputHeight = ih;
        descriptor.outputWidth = w; descriptor.outputHeight = h;
        descriptor.colorTextureFormat = Texture(current.get()).pixelFormat;
        descriptor.outputTextureFormat = Texture(generated.get()).pixelFormat;
        descriptor.depthTextureFormat = MTLPixelFormatR32Float;
        descriptor.motionTextureFormat = MTLPixelFormatRG16Float;
        // Fully composited game color; no separate UI texture is claimed.
        descriptor.uiTextureFormat = MTLPixelFormatInvalid;
        descriptor.scaler = nil;
        interpolator = [descriptor newFrameInterpolatorWithDevice:Texture(current.get()).device];
        if (!interpolator) { Fail("create frame interpolator"); return false; }
        inputWidth=iw; inputHeight=ih; width=w; height=h; format=f;
        return true;
    }
    // Bridge every prior Plume encoder to the SDK fence, then bridge the SDK's
    // updated fence back into Plume. Plume textures have untracked hazards.
    void Encode(plume::MetalCommandList& list) API_AVAILABLE(macos(26.0)) {
        constexpr uint64_t all = (1u << plume::MetalBarrierStage::COUNT) - 1;
        list.endOtherEncoders(plume::EncoderType::None); list.activeType = plume::EncoderType::None;
        list.handlePendingClears();
        list.endOtherEncoders(plume::EncoderType::None); list.activeType = plume::EncoderType::None;
        list.setBarrier(all,all);
        list.checkActiveBlitEncoder();
        auto* nativeFence = reinterpret_cast<MTL::Fence*>((__bridge void*)fence);
        list.activeBlitEncoder->updateFence(nativeFence);
        list.endOtherEncoders(plume::EncoderType::None); list.activeType = plume::EncoderType::None;
        interpolator.fence = fence;
        [interpolator encodeToCommandBuffer:recorded];
        list.setBarrier(all,all);
        list.checkActiveBlitEncoder();
        list.activeBlitEncoder->waitForFence(nativeFence);
        list.endOtherEncoders(plume::EncoderType::None); list.activeType = plume::EncoderType::None;
        list.setBarrier(all,all);
    }
#endif
    ~Impl() { Require(!use.Pending(), "destroy before input completion"); }
};
Session::Session() : impl_(std::make_unique<Impl>()) {}
Session::~Session() = default;
bool Session::Initialize(plume::RenderDevice* device, std::string& reason) {
    auto& s=*impl_;
    if (!device || device->getCapabilities().shaderFormat != plume::RenderShaderFormat::METAL) {
        reason="Metal device required"; return false;
    }
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
    if (@available(macOS 26.0, *)) {
        auto native=(__bridge id<MTLDevice>)static_cast<void*>(static_cast<plume::MetalDevice*>(device)->mtl);
        if ([MTLFXFrameInterpolatorDescriptor supportsDevice:native]) {
            s.fence=[native newFence]; s.device=device; s.supported=s.fence != nil;
            if (s.supported) return true;
        }
        reason="frame interpolation is not supported by this Metal device"; return false;
    }
#endif
    reason="MetalFX frame generation requires the macOS 26 SDK and runtime"; return false;
}
bool Session::Reconfigure(const framegen::Config& config, std::string& reason) {
    auto& s=*impl_;
    SuspendAfterHostDrain(); s.config={};
    if (config.provider == framegen::Provider::Off || config.mode == framegen::Mode::Off) return true;
    if (config.provider != framegen::Provider::MetalFx || config.mode != framegen::Mode::Fixed || config.generatedFrames != 1) {
        reason="MetalFX FG supports fixed 2x only"; return false;
    }
    if (!s.supported || s.failed || !s.depth.Initialize(s.device)) {
        reason="MetalFX FG device or depth conversion unavailable"; return false;
    }
    s.config=config; return true;
}
bool Session::Requested() const { return impl_->config.provider == framegen::Provider::MetalFx && !impl_->failed; }
bool Session::Available() const { return Requested() && impl_->active; }
bool Session::Failed() const { return impl_->failed; }
bool Session::Record(const frame_generation::CompositeHandoff& handoff,
    plume::RenderCommandList* commands, plume::RenderTexture* backbuffer) {
    auto& s=*impl_;
    Require(!s.use.Pending(), "record before input completion");
    s.prepared=false;
    if (!Requested() || !commands || !backbuffer || !handoff.Ready()) {
        s.history.Reset(); s.active=false; return false;
    }
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
    if (@available(macOS 26.0, *)) {
        const auto& in=handoff.producer->inputs;
        const auto& output=static_cast<plume::ExtendedRenderTexture*>(backbuffer)->desc;
        framegen::Camera camera{}; frame_generation::DepthRemap mapping{};
        if (!dlss_fg::FullFramePresentation(handoff.outputWidth,handoff.outputHeight,output.width,output.height) ||
            !frame_generation::BuildCamera(in,nullptr,camera,&mapping)) { s.history.Reset(); s.active=false; return false; }
        const auto parameters=ConvertParameters(in.frameTimeDeltaMilliseconds,camera.fovRadians,
            in.depth.width,in.depth.height,output.width,output.height);
        if (!parameters || !s.Ensure(in.depth.width,in.depth.height,output.width,output.height,output.format)) return false;
        s.pendingKey={in.plan.deviceEpoch,in.temporalEpoch,output.width,output.height,in.depth.width,in.depth.height,uint32_t(output.format),s.config};
        s.pendingFrame=in.renderFrameId; s.pendingPlan=in.plan;
        const bool reset=s.history.NeedsReset(in.renderFrameId,s.pendingKey,in.resetHistory ||
            in.motionState==temporal::MotionState::ResetInitialization ||
            !dlss_fg::SameHistoryConfiguration(s.previousPlan,in.plan));
        s.Begin(commands); s.retained=handoff.producer;
        auto* depth=s.depth.Record(commands,in.depth,mapping);
        if (!depth) { s.history.Reset(); s.active=false; return false; }
        Copy(commands,s.current.get(),backbuffer);
        auto effect=s.interpolator;
        effect.colorTexture=Texture(s.current.get());
        effect.prevColorTexture=Texture(reset ? s.current.get() : s.previous.get());
        effect.outputTexture=Texture(s.generated.get());
        effect.depthTexture=Texture(depth); effect.motionTexture=Texture(in.motion.texture);
        const auto usable=[](id<MTLTexture> texture, MTLTextureUsage usage) {
            return texture && (texture.usage & usage)==usage;
        };
        if (!usable(effect.colorTexture,effect.colorTextureUsage) || !usable(effect.prevColorTexture,effect.colorTextureUsage) ||
            !usable(effect.outputTexture,effect.outputTextureUsage) || !usable(effect.depthTexture,effect.depthTextureUsage) ||
            !usable(effect.motionTexture,effect.motionTextureUsage)) { s.Fail("SDK texture usage requirements"); return false; }
        effect.deltaTime=parameters->deltaSeconds; effect.fieldOfView=parameters->verticalFovDegrees;
        effect.nearPlane=camera.nearPlane; effect.farPlane=camera.farPlane; effect.aspectRatio=camera.aspect;
        effect.depthReversed=YES; effect.shouldResetHistory=reset;
        effect.motionVectorScaleX=parameters->motionScaleX; effect.motionVectorScaleY=parameters->motionScaleY;
        effect.jitterOffsetX=camera.jitterX; effect.jitterOffsetY=camera.jitterY;
        effect.uiTexture=nil; effect.uiTextureComposited=NO;
        s.Encode(*static_cast<plume::MetalCommandList*>(commands));
        s.prepared=true;
        // Reset frames prime the SDK but present the real frame only.
        if (!reset) Copy(commands,backbuffer,s.generated.get());
        return !reset;
    }
#endif
    return false;
}
void Session::RecordOriginal(plume::RenderCommandList* commands, plume::RenderTexture* backbuffer) {
    auto& s=*impl_;
    Require(s.prepared && s.current && commands && backbuffer, "second present without a real frame");
    s.Begin(commands); Copy(commands,backbuffer,s.current.get());
}
void Session::SubmitStart() { Require(impl_->use.SubmissionStarted(), "duplicate submit"); }
void Session::HostSubmitted(bool success, uint64_t serial) {
    if (impl_->use.Pending()) Require(impl_->use.Submitted(success,serial), "host submission");
}
void Session::AfterHostDrain() {
    auto& s=*impl_;
    if (!s.use.Pending()) return;
    Require(s.use.Serial() && s.recorded.status==MTLCommandBufferStatusCompleted && !s.recorded.error, "Metal command completion");
    Require(s.use.Completed(), "input lease completion");
    s.recorded=nil; s.retained.reset(); s.depth.ReleaseAfterInputDrain();
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
    if (@available(macOS 26.0, *)) {
        s.interpolator.depthTexture=nil; s.interpolator.motionTexture=nil;
        s.interpolator.colorTexture=nil; s.interpolator.prevColorTexture=nil; s.interpolator.outputTexture=nil;
    }
#endif
}
void Session::FinishPresent(bool accepted, bool generated) {
    auto& s=*impl_;
    s.active=accepted && generated && s.prepared && !s.failed;
    if (accepted && s.prepared && !s.failed) {
        s.history.Accepted(s.pendingFrame,s.pendingKey); s.previousPlan=s.pendingPlan; std::swap(s.previous,s.current);
    } else s.history.Reset();
    if (s.active && ++s.pairs % 120 == 0)
        LOG_INFO("MetalFX FG: generated_then_real_pairs={} scope=host_not_display",s.pairs);
    s.prepared=false;
}
bool Session::HasUnsubmitted(plume::RenderCommandList* commands) const { return impl_->use.CanCancel(commands); }
void Session::CancelUnsubmitted(plume::RenderCommandList* commands, bool producerDrained) {
    auto& s=*impl_;
    Require(s.use.CanCancel(commands) && producerDrained, "cancel after submission or before producer drain");
    auto& list=*static_cast<plume::MetalCommandList*>(commands);
    list.end(); // Close encoders before discarding the uncommitted command buffer.
    Require(s.recorded.status==MTLCommandBufferStatusNotEnqueued, "cancel committed command buffer");
    list.mtl->release(); list.mtl=nullptr;
    Require(s.use.Canceled(commands,true,true,producerDrained), "cancel input lease");
    s.recorded=nil; s.retained.reset(); s.depth.DiscardUnsubmitted();
    SuspendAfterHostDrain();
}
void Session::SuspendAfterHostDrain() {
    Require(!impl_->use.Pending(), "suspend before input retirement");
    impl_->history.Reset(); impl_->active=impl_->prepared=false;
}
} // namespace gpu::metalfx_fg
#endif
