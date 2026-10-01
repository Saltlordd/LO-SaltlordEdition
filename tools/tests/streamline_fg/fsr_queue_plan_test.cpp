#include "gpu/fsr_fg_queue_plan.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace gpu::fsr_fg;
namespace {
unsigned checks = 0;
void Check(bool okay, const char* name) {
    ++checks;
    if (!okay) { std::fprintf(stderr, "FAIL: %s\n", name); std::exit(1); }
}
void Valid(const QueuePlan& plan, const std::vector<QueueCandidate>& queues) {
    Check(plan[0].compute && plan[1].present && plan[1].transfer, "required capabilities");
    for (size_t i = 0; i < plan.size(); ++i) {
        Check(plan[i].available, "SDK never takes an allocated host queue");
        for (size_t j = 0; j < i; ++j) Check(!SameQueue(plan[i], plan[j]), "SDK queues are distinct");
        bool host = false;
        for (const auto& q : queues) if (q.family == plan[i].family &&
            !SameQueue(q, plan[0]) && !SameQueue(q, plan[1]) && !SameQueue(q, plan[2])) host = true;
        Check(host, "future host allocations retain capacity in every family");
    }
}
}
int main() {
    std::vector<QueueCandidate> queues;
    Check(!PlanQueues(queues), "empty device");
    for (uint32_t i = 0; i < 4; ++i) queues.push_back({0,i,true,true,true,i!=0});
    auto plan = PlanQueues(queues);
    Check(bool(plan), "four universal queues including one game queue"); Valid(*plan, queues);
    queues.pop_back(); Check(!PlanQueues(queues), "three native queues cannot be aliased to make four");
    queues.push_back({0,3,true,true,true,true});
    queues[2].available = false; Check(!PlanQueues(queues), "busy queue cannot be reserved");
    queues[2].available = true;
    for (auto& q : queues) q.present = false;
    Check(!PlanQueues(queues), "no present-capable SDK queue");
    for (auto& q : queues) q.present = true, q.compute = false;
    Check(!PlanQueues(queues), "no compute-capable SDK queue");
    queues = {{0,0,true,true,true,false}, {0,1,true,true,true,true},
        {1,0,true,true,false,true}, {1,1,true,true,false,true},
        {2,0,false,true,false,true}, {2,1,false,true,false,true}};
    plan = PlanQueues(queues); Check(bool(plan), "split graphics/compute/transfer families"); Valid(*plan, queues);
    Check(plan->at(1).family==0 && plan->at(1).index==1, "present support belongs to the actual surface");
    queues = {{0,0,true,true,true,false}, {0,1,true,true,true,true},
        {1,0,true,true,false,true}, {2,0,false,true,false,true}};
    Check(!PlanQueues(queues), "exclusive family reservation cannot starve future renderer queues");
    // Exhaust all availability/capability combinations on a small device. A
    // returned plan must preserve the ownership invariant, independent of order.
    for (unsigned mask=0; mask<256; ++mask) {
        queues.clear();
        for (uint32_t i=0; i<4; ++i) queues.push_back({0,i,bool(mask&(1u<<i)),true,bool(mask&(16u<<i)),i!=0});
        if (auto p=PlanQueues(queues)) Valid(*p, queues);
    }
    std::printf("PASS FSR queue reservation: %u checks\n", checks);
}
