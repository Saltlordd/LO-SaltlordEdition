#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace gpu::temporal
{
// Runtime locator for the #67/#102 flicker class without an F1 capture: an
// unmapped VS draws the scene camera into the main scene depth while jitter is
// active, and either writes depth (whole-frame motion fallback) or repeats the
// geometry of an earlier jittered depth writer (the sky/material pair case).
// The renderer logs each VS/PS pair once, with the banks a CPU fixture needs.
struct SuspectGeometry
{
    uint32_t indexBase = 0, indexCount = 0, baseVertex = 0, fetch95 = 0;
    bool operator==(const SuspectGeometry&) const = default;
};
struct SuspectCompanion
{
    SuspectGeometry geometry;
    uint64_t vs = 0;
    int slot = -1;
    uint32_t draw = 0;
    std::array<uint32_t, 16> world{}, vp{};
};

inline bool SuspectWindowIs(const uint32_t* constants, int slot, const std::array<uint32_t, 16>& camera)
{
    return slot >= 0 && slot <= 252 && std::memcmp(constants + slot * 4, camera.data(), sizeof(camera)) == 0;
}
// First VP window equal to the scene camera, in the order the reviewed maps use.
inline int SceneCameraSlot(const uint32_t* constants, const std::array<uint32_t, 16>& camera)
{
    for (const int slot : {7, 8, 4, 0, 233, 230})
        if (SuspectWindowIs(constants, slot, camera)) return slot;
    return -1;
}

// Off-thread position evidence decides which window actually feeds oPos.
struct SuspectEvidence
{
    bool available = false, ready = false;  // collection exists / result arrived
    uint32_t kind = 0, issues = 0;          // position_evidence::Summary
    int slot = -1;
};
struct SuspectDecision
{
    enum class Action { Report, Dismiss, Wait } action = Action::Wait;
    int cameraSlot = -1, positionKind = -1, evidenceSlot = -1;
};
inline constexpr uint64_t SuspectEvidenceFrames = 120;
// Constants persist across draws, so a scene camera left in another window is
// not proof. Only a direct-position VS is dismissed for good; a proven window
// that differs on this draw just waits for a draw where it matches.
inline SuspectDecision DecideSuspect(const SuspectEvidence& evidence, const uint32_t* constants,
    const std::array<uint32_t, 16>& camera, int fallbackSlot, uint64_t framesWaited)
{
    using Action = SuspectDecision::Action;
    const bool proven = evidence.ready && evidence.issues == 0 && (evidence.kind == 1 || evidence.kind == 2);
    if (!proven && evidence.available && !evidence.ready && framesWaited < SuspectEvidenceFrames)
        return {Action::Wait};
    if (!proven) return {Action::Report, fallbackSlot, evidence.ready ? int(evidence.kind) : -1, -1};
    if (evidence.kind == 2) return {Action::Dismiss, -1, 2, -1};
    if (SuspectWindowIs(constants, evidence.slot, camera)) return {Action::Report, evidence.slot, 1, evidence.slot};
    return {Action::Wait, -1, 1, evidence.slot};
}

class SuspectTracker
{
public:
    static constexpr size_t MaxReports = 32, MaxSettled = 256, MaxCompanions = 4096, TableSize = 8192;
    bool Done() const { return logged_ >= MaxReports; }
    void BeginFrame(uint64_t frame)
    {
        if (frame == frame_) return;
        frame_ = frame;
        companions_.clear();
        // Generation stamps retire the previous frame's table without clearing it.
        if (++generation_ == 0) { slots_.fill({}); generation_ = 1; }
    }
    // Mapped, jittered main-scene depth writer; constants are the pre-jitter upload.
    void ObserveJitteredDepth(const SuspectGeometry& geometry, uint64_t vs, int slot, uint32_t draw,
        const std::array<uint32_t, 16>& world, const std::array<uint32_t, 16>& vp)
    {
        if (companions_.size() >= MaxCompanions) return;
        if (companions_.empty()) companions_.reserve(MaxCompanions);
        const auto index = uint32_t(companions_.size());
        companions_.push_back({geometry, vs, slot, draw, world, vp});
        for (size_t i = Hash(geometry);; i = (i + 1) & (TableSize - 1)) {
            auto& entry = slots_[i];
            // The latest writer of the same geometry is the one a later pass tests against.
            if (entry.generation != generation_ || companions_[entry.index].geometry == geometry) {
                entry = {generation_, index};
                return;
            }
        }
    }
    const SuspectCompanion* FindCompanion(const SuspectGeometry& geometry) const
    {
        for (size_t i = Hash(geometry); slots_[i].generation == generation_; i = (i + 1) & (TableSize - 1))
            if (companions_[slots_[i].index].geometry == geometry) return &companions_[slots_[i].index];
        return nullptr;
    }
    // A settled pair was either logged or dismissed by its position evidence.
    bool Settled(uint64_t vs, uint64_t ps) const
    {
        if (Done() || settled_.size() >= MaxSettled) return true;
        for (const auto& pair : settled_) if (pair[0] == vs && pair[1] == ps) return true;
        return false;
    }
    void Settle(uint64_t vs, uint64_t ps, bool logged)
    {
        if (settled_.size() < MaxSettled) settled_.push_back({vs, ps});
        logged_ += logged;
    }
    // Frames since the pair first waited for position evidence.
    uint64_t Waited(uint64_t vs, uint64_t ps, uint64_t frame)
    {
        for (const auto& pair : waiting_) if (pair[0] == vs && pair[1] == ps) return frame - pair[2];
        if (waiting_.size() < MaxSettled) waiting_.push_back({vs, ps, frame});
        return waiting_.size() < MaxSettled ? 0 : SuspectEvidenceFrames;
    }
private:
    struct Entry { uint32_t generation = 0, index = 0; };
    static size_t Hash(const SuspectGeometry& g)
    {
        uint64_t h = (uint64_t(g.indexBase) << 32 | g.indexCount) * 0x9e3779b97f4a7c15ull;
        h ^= (uint64_t(g.baseVertex) << 32 | g.fetch95) * 0xc2b2ae3d27d4eb4full;
        return size_t(h ^ (h >> 29)) & (TableSize - 1);
    }
    uint64_t frame_ = ~0ull;
    uint32_t generation_ = 1;
    std::vector<SuspectCompanion> companions_;
    std::array<Entry, TableSize> slots_{};
    std::vector<std::array<uint64_t, 2>> settled_;
    std::vector<std::array<uint64_t, 3>> waiting_;
    size_t logged_ = 0;
};

inline std::string SuspectHex(const uint32_t* words, size_t count)
{
    static constexpr char digits[] = "0123456789abcdef";
    std::string text;
    text.reserve(count * 9);
    for (size_t i = 0; i < count; ++i) {
        if (i) text += ',';
        for (int shift = 28; shift >= 0; shift -= 4) text += digits[(words[i] >> shift) & 15];
    }
    return text;
}
struct SuspectReport
{
    uint64_t vs = 0, ps = 0, frame = 0;
    uint32_t draw = 0, depthControl = 0;
    int cameraSlot = -1, positionKind = -1, evidenceSlot = -1;
    SuspectGeometry geometry;
    const SuspectCompanion* companion = nullptr;
    const uint32_t* vsConstants = nullptr;  // 256 float4
    const uint32_t* psConstants = nullptr;  // 256 float4
};
// Two log lines: identity/state, then the banks a CPU fixture needs. The
// preceding "current map" line in the same log places the pair.
inline std::array<std::string, 2> FormatSuspect(const SuspectReport& r)
{
    const auto hex64 = [](uint64_t value) {
        const uint32_t words[2]{uint32_t(value >> 32), uint32_t(value)};
        return SuspectHex(words, 1) + SuspectHex(words + 1, 1);
    };
    const auto hex32 = [](uint32_t value) { return "0x" + SuspectHex(&value, 1); };
    const auto* c = r.companion;
    const bool zwrite = (r.depthControl & 4) != 0;
    const bool sameWorld = c && std::memcmp(c->world.data(), r.vsConstants, sizeof(c->world)) == 0;
    const bool sameCamera = c && r.cameraSlot >= 0 &&
        std::memcmp(c->vp.data(), r.vsConstants + r.cameraSlot * 4, sizeof(c->vp)) == 0;
    const auto flag = [](bool value) { return value ? "true" : "false"; };
    std::string id = "vs=" + hex64(r.vs) + " ps=" + hex64(r.ps);
    std::string first = std::string("temporal suspect: kind=") +
        (c ? (zwrite ? "depth_writer_after_jittered_geometry" : "after_jittered_geometry") : "depth_writer") +
        " " + id + " frame=" + std::to_string(r.frame) + " draw=" + std::to_string(r.draw) +
        " camera_slot=" + std::to_string(r.cameraSlot) + " position_kind=" + std::to_string(r.positionKind) +
        " evidence_slot=" + std::to_string(r.evidenceSlot) + " depth_control=" + hex32(r.depthControl) +
        " zfunc=" + std::to_string((r.depthControl >> 4) & 7) + " zwrite=" + flag(zwrite) +
        " index_base=" + hex32(r.geometry.indexBase) + " index_count=" + std::to_string(r.geometry.indexCount) +
        " base_vertex=" + std::to_string(r.geometry.baseVertex) + " fetch95=" + hex32(r.geometry.fetch95) +
        " companion_vs=" + hex64(c ? c->vs : 0) + " companion_slot=" + std::to_string(c ? c->slot : -1) +
        " companion_draw=" + std::to_string(c ? c->draw : 0) + " same_world=" + flag(sameWorld) +
        " same_camera=" + flag(sameCamera);
    std::string second = "temporal suspect banks: " + id + " material=" + SuspectHex(r.vsConstants, 64) +
        " late=" + SuspectHex(r.vsConstants + 254 * 4, 8) +
        " camera=" + (r.cameraSlot >= 0 && r.cameraSlot <= 252 ? SuspectHex(r.vsConstants + r.cameraSlot * 4, 16) : "-") +
        " pixel=" + SuspectHex(r.psConstants, 64) +
        " companion_world=" + (c ? SuspectHex(c->world.data(), 16) : "-") +
        " companion_vp=" + (c ? SuspectHex(c->vp.data(), 16) : "-");
    return {std::move(first), std::move(second)};
}
}
