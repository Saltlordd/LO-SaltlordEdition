#pragma once
#include <chrono>
#include <cstdint>
#include <os/logger.h>
namespace android_wait_timing {
using Clock=std::chrono::steady_clock;
inline void Report(const char* kind,Clock::time_point started,uint32_t caller) {
#if defined(__ANDROID__)
 const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-started).count();
 if(ms>=100)LOG_INFO("Android guest wait: kind={} elapsedMs={} caller={:#x} (original guest wait preserved)",kind,ms,caller);
#endif
}
}
