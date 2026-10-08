#pragma once
#include <atomic>
#include <cstdint>
namespace gpu::android_surface {
inline std::atomic<uint64_t> state{0};
inline uint64_t Signal(bool ready){
    auto previous=state.load(std::memory_order_acquire);
    for(;;){auto next=ready ? ((previous&1)?previous:((previous+2)|1)) : (previous&~uint64_t(1));
        if(state.compare_exchange_weak(previous,next,std::memory_order_acq_rel))return next;}
}
inline uint64_t Read(){return state.load(std::memory_order_acquire);}
}
