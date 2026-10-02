#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

// Guest occlusion queries (PM4 EVENT_WRITE_ZPD) measured with host GPU queries.
//
// D3D on the console issues one ZPD event for a BEGIN record, the query's draws,
// then one event for the END record, and reads END - BEGIN of the ZPass counts.
// Its GetData (sub_823CF3F0) waits until the GPU fence has passed the query and
// END ZPass A and B no longer both hold the sentinel it stamped. Lost Odyssey
// reads its queries in the same frame, right after issuing them, so an exact
// result costs a GPU sync in the middle of every frame (Mode::Strict). The
// default writes the last measured count of the same record instead
// (Mode::Fast), as Xenia's default "fast" mode does.
//
// This file holds the bookkeeping only; the renderer owns the host queries and
// writes the records this class returns.
namespace gpu::occlusion
{
    // xe_gpu_depth_sample_counts: eight dwords, each count split into A and B.
    struct Record
    {
        uint32_t totalA = 0, totalB = 0;
        uint32_t zfailA = 0, zfailB = 0;
        uint32_t zpassA = 0, zpassB = 0;
        uint32_t stencilFailA = 0, stencilFailB = 0;
    };
    static_assert(sizeof(Record) == 32);

    // The END record sits at a 64-byte slot and the BEGIN record 32 bytes later.
    inline constexpr uint32_t kSlotBytes = 64;
    inline constexpr uint32_t kBeginOffset = 32;
    inline constexpr bool IsBeginRecord(uint32_t address) { return (address & (kSlotBytes - 1)) == kBeginOffset; }
    inline constexpr uint32_t SlotOf(uint32_t address) { return address & ~(kSlotBytes - 1); }

    // Samples reported for a query that was not measured: clearly visible.
    inline constexpr uint32_t kUnmeasuredSamples = 0x10000;

    enum class Mode
    {
        // The END record is written at once with the last measured count of
        // the same record, or 1 when that was zero or is not known yet. The
        // guest never waits and never culls on a stale count, while counts
        // used as magnitudes, such as lens flare visibility, follow the real
        // ones a frame or two late.
        Fast,
        // The END record is written once the host GPU has counted it. The
        // guest waits for exact results and culls with them, at the cost of a
        // GPU sync in the middle of every frame that issues queries.
        Strict,
    };

    // LO_ZPD_MODE: unset or "host" selects Mode::Fast, "strict" Mode::Strict.
    // The command processor's fake modes (grow, xenia, begin0, none) turn
    // host queries off.
    inline std::optional<Mode> HostMode(const char* mode)
    {
        if (!mode || !*mode || std::strcmp(mode, "host") == 0) return Mode::Fast;
        if (std::strcmp(mode, "strict") == 0) return Mode::Strict;
        return std::nullopt;
    }

    // One host query around one guest draw. `scale` converts host samples to
    // guest samples (resolution scale and guest MSAA).
    struct Part
    {
        uint64_t batch = 0;
        uint32_t index = 0;
        double scale = 1.0;
    };

    // Rounds scaled host samples to a guest count. Any passing host sample
    // keeps the query visible; the cap only keeps END - BEGIN well defined.
    inline uint32_t GuestSamples(double samples)
    {
        if (!(samples > 0.0)) return 0;
        if (samples < 1.0) return 1;
        return samples >= double(0x40000000u) ? 0x40000000u : uint32_t(std::llround(samples));
    }

    // Guest samples per host sample for a target `hostWidth` x `hostHeight`
    // that stands for `guestWidth` x `guestHeight` guest pixels with
    // `guestMsaa` (RB_SURFACE_INFO MSAA field: 0, 1, 2 for 1, 2, 4 samples).
    inline double SampleScale(uint32_t guestWidth, uint32_t guestHeight, uint32_t hostWidth, uint32_t hostHeight, uint32_t guestMsaa)
    {
        if (!guestWidth || !guestHeight || !hostWidth || !hostHeight) return 1.0;
        const double samples = double(1u << (guestMsaa > 2 ? 2 : guestMsaa));
        return double(guestWidth) * double(guestHeight) * samples / (double(hostWidth) * double(hostHeight));
    }

    struct Write
    {
        uint32_t address = 0;
        Record record;
        uint32_t samples = 0;  // END - BEGIN this write completes
        bool measured = false; // counted by host queries
        bool apply = true;     // false: written at its END event already; reported for statistics
    };

    class Tracker
    {
    public:
        explicit Tracker(Mode mode = Mode::Fast) : mode_(mode) {}
        void SetMode(Mode mode) { mode_ = mode; }

        // BEGIN event: the record to write now.
        Write Begin(uint32_t address)
        {
            const uint32_t slot = SlotOf(address);
            active_ = Query{slot, ++generations_[slot], counter_};
            return {address, Counts(counter_)};
        }

        bool Active() const { return active_.has_value(); }

        // A guest draw inside the active query, measured or not.
        void DrawSeen() { if (active_) ++active_->drawsSeen; }
        void DrawMeasured(const Part& part) { if (active_) active_->parts.push_back(part); }

        // END event. Returns the record to write now, or nothing when it waits
        // for host results (Mode::Strict).
        std::optional<Write> End(uint32_t address)
        {
            const uint32_t slot = SlotOf(address);
            if (!active_ || active_->slot != slot) {
                // An END without its BEGIN cannot be measured.
                if (active_ && active_->slot != slot) active_.reset();
                return Unmeasured(address, counter_);
            }
            Query query = std::move(*active_);
            active_.reset();
            if (query.parts.empty() || query.parts.size() != query.drawsSeen)
                return Unmeasured(address, query.begin);
            query.end = address;
            std::optional<Write> write;
            if (mode_ == Mode::Fast) {
                const auto known = last_.find(slot);
                const uint32_t samples = known != last_.end() && known->second ? known->second : 1;
                write = Write{address, Counts(query.begin + samples), samples, false};
                counter_ = query.begin + samples;
                query.written = true;
            }
            pending_.push_back(std::move(query));
            return write;
        }

        // A submitted batch completed; `results` holds its host query results
        // (UINT64_MAX when unavailable). Returns every query it finished.
        std::vector<Write> Complete(uint64_t batch, std::span<const uint64_t> results)
        {
            for (auto& query : pending_)
                for (auto& part : query.parts)
                    if (part.batch == batch && !query.failed) {
                        if (part.index >= results.size() || results[part.index] == ~0ull) query.failed = true;
                        else query.samples += double(results[part.index]) * part.scale;
                        part.batch = 0;
                    }
            return Collect();
        }

        // A batch that will not run: its queries read as visible.
        std::vector<Write> Abandon(uint64_t batch)
        {
            for (auto& query : pending_)
                for (auto& part : query.parts)
                    if (part.batch == batch) {
                        query.failed = true;
                        part.batch = 0;
                    }
            return Collect();
        }

        // A guest may be waiting on an END record that is not written yet.
        bool HasAwaited() const
        {
            for (const auto& query : pending_)
                if (!query.written) return true;
            return false;
        }

        // An unwritten END record waits on host queries in `batch`.
        bool Waiting(uint64_t batch) const
        {
            for (const auto& query : pending_)
                if (!query.written)
                    for (const auto& part : query.parts)
                        if (part.batch == batch) return true;
            return false;
        }

        bool HasPending() const { return !pending_.empty(); }

        // Device loss or shutdown: every unwritten END record reads as visible.
        std::vector<Write> AbandonAll()
        {
            for (auto& query : pending_) {
                query.failed = true;
                for (auto& part : query.parts) part.batch = 0;
            }
            active_.reset();
            return Collect();
        }

    private:
        struct Query
        {
            uint32_t slot = 0;
            uint32_t generation = 0;
            uint32_t begin = 0;
            uint32_t end = 0;
            uint32_t drawsSeen = 0;
            std::vector<Part> parts;
            double samples = 0.0; // guest samples measured so far
            bool failed = false;
            bool written = false; // END record written at its event (Mode::Fast)
        };

        static Record Counts(uint32_t zpass)
        {
            Record record;
            record.totalA = record.zpassA = zpass;
            return record;
        }

        Write Unmeasured(uint32_t address, uint32_t begin)
        {
            const uint32_t end = begin + kUnmeasuredSamples;
            counter_ = end;
            return {address, Counts(end), kUnmeasuredSamples, false};
        }

        // Finished queries leave in issue order. Each measured count becomes
        // the next Fast answer for its record. An unwritten record whose slot
        // the guest has reissued since is dropped: it belongs to the new query.
        std::vector<Write> Collect()
        {
            std::vector<Write> writes;
            for (auto it = pending_.begin(); it != pending_.end();) {
                bool done = it->failed;
                if (!done) {
                    done = true;
                    for (const auto& part : it->parts) done &= part.batch == 0;
                }
                if (!done) { ++it; continue; }
                const uint32_t samples = it->failed ? kUnmeasuredSamples : GuestSamples(it->samples);
                if (!it->failed) last_[it->slot] = samples;
                const bool current = generations_[it->slot] == it->generation;
                if (it->written || current) {
                    writes.push_back({it->end, Counts(it->begin + samples), samples, !it->failed, !it->written});
                    if (!it->written && it->begin + samples - counter_ < 0x80000000u) counter_ = it->begin + samples;
                }
                it = pending_.erase(it);
            }
            return writes;
        }

        Mode mode_;
        uint32_t counter_ = 0;
        std::optional<Query> active_;
        std::deque<Query> pending_;
        std::unordered_map<uint32_t, uint32_t> generations_;
        std::unordered_map<uint32_t, uint32_t> last_; // last measured count per slot
    };
}
