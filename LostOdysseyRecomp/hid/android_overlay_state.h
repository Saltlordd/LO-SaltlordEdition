#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
namespace hid::android_overlay {
inline std::atomic<bool> appMenu{false},hostBlocked{true};
inline std::atomic<uint64_t> titleObservation{0};
inline uint64_t NowMs(){return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
inline void ObserveTitle(uint32_t state,uint64_t now){titleObservation.store((now<<1)|(state==6?1u:0u),std::memory_order_release);}
inline int Context(uint64_t now){const auto observed=titleObservation.load(std::memory_order_acquire),stamp=observed>>1;
 // A freshness lease rejects a missing/blocked hook; it never infers a game state from elapsed time.
 return !appMenu.load()&&!hostBlocked.load()&&(observed&1)&&now>=stamp&&now-stamp<=300?1:0;
}
}
