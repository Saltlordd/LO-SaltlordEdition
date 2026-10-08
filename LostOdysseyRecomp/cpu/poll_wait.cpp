#include <stdafx.h>
#include "poll_wait.h"
#include "android_wait_timing.h"

extern "C" PPC_FUNC(__imp__sub_823CF390);
extern "C" PPC_FUNC(__imp__sub_82322478);

// Retain the generated function, including register saves, return values and
// query ordering. Only its repeated not-ready probes can pause on the host.
PPC_FUNC(sub_823CF390)
{
    const auto started=android_wait_timing::Clock::now();const auto caller=uint32_t(ctx.lr);
    poll_wait::RunScoped(poll_wait::Kind::Query, [&] { __imp__sub_823CF390(ctx, base); });
    android_wait_timing::Report("Query",started,caller);
}

// This guest loop repeatedly invokes Sleep(0) while a shared value exceeds its
// threshold. Keep its original reads and comparison; opt in only this call tree.
PPC_FUNC(sub_82322478)
{
    const auto started=android_wait_timing::Clock::now();const auto caller=uint32_t(ctx.lr);
    poll_wait::RunScoped(poll_wait::Kind::SharedValue, [&] { __imp__sub_82322478(ctx, base); });
    android_wait_timing::Report("SharedValue",started,caller);
}

// GpuPoll for the title GPU timestamp wait lives in debug/gpu_wait_trace.cpp.
// That file already owns PPC_FUNC(sub_823B62A0) and PPC_FUNC(sub_827B6278).
