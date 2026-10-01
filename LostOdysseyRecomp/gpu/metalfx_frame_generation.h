#pragma once
#if defined(__APPLE__) && defined(LO_ENABLE_METALFX_FG)
#include "frame_generation_composite.h"
#include "../../shared/frame_generation/core.h"
#include <memory>
#include <string>

namespace gpu::metalfx_fg {
// Presentation-thread owner. The caller drains its checked Metal submission
// before reconfiguration, resource reuse, or destruction.
class Session {
public:
    Session();
    ~Session();
    bool Initialize(plume::RenderDevice* device, std::string& reason);
    bool Reconfigure(const framegen::Config& config, std::string& reason);
    bool Requested() const;
    bool Available() const;
    bool Failed() const;
    // Always primes SDK history on a reset; only a continuous pair replaces the
    // first drawable with an interpolated frame and asks for a second present.
    bool Record(const frame_generation::CompositeHandoff& inputs,
        plume::RenderCommandList* commands, plume::RenderTexture* backbuffer);
    void RecordOriginal(plume::RenderCommandList* commands, plume::RenderTexture* backbuffer);
    void SubmitStart();
    void HostSubmitted(bool success, uint64_t serial);
    // The host's presentation wait finished; a command buffer error disables
    // FG instead of terminating, since an errored buffer no longer runs.
    void AfterHostDrain();
    // Teardown: waits for the session's own submitted command buffer, so it
    // does not depend on the host wait succeeding after a GPU error.
    void Retire();
    void FinishPresent(bool accepted, bool generated);
    bool HasUnsubmitted(plume::RenderCommandList* commands) const;
    void CancelUnsubmitted(plume::RenderCommandList* commands, bool producerDrained);
    void SuspendAfterHostDrain();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace gpu::metalfx_fg
#endif
