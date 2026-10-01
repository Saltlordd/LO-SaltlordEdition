#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace gpu::fsr_fg {
struct QueueCandidate {
    uint32_t family = 0, index = 0;
    bool compute = false, transfer = false, present = false, available = false;
};
using QueuePlan = std::array<QueueCandidate, 3>; // async compute, present, acquire
inline bool SameQueue(const QueueCandidate& a, const QueueCandidate& b) {
    return a.family == b.family && a.index == b.index;
}
// Input lists only native queues actually created by the device. Keep one host
// queue per family: later virtual queue allocation must never borrow SDK queues.
inline std::optional<QueuePlan> PlanQueues(std::span<const QueueCandidate> queues) {
    for (const auto& present : queues) {
        if (!present.available || !present.transfer || !present.present) continue;
        for (const auto& compute : queues) {
            if (!compute.available || !compute.compute || SameQueue(compute, present)) continue;
            for (const auto& acquire : queues) {
                if (!acquire.available || SameQueue(acquire, compute) || SameQueue(acquire, present)) continue;
                const QueuePlan plan{compute, present, acquire};
                bool leavesHostQueues = true;
                for (const auto& reserved : plan) {
                    bool hasHostQueue = false;
                    for (const auto& q : queues)
                        if (q.family == reserved.family && !SameQueue(q, compute) &&
                            !SameQueue(q, present) && !SameQueue(q, acquire)) hasHostQueue = true;
                    leavesHostQueues &= hasHostQueue;
                }
                if (leavesHostQueues) return plan;
            }
        }
    }
    return {};
}
} // namespace gpu::fsr_fg
