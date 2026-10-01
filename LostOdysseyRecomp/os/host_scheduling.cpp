#include "host_scheduling.h"
#include <os/platform.h>

#if LO_PLATFORM_MACOS
#include <cstdint>
#include <objc/message.h>
#include <objc/runtime.h>
#endif

namespace os::scheduling
{
#if LO_PLATFORM_MACOS
void BeginLatencyCriticalActivity()
{
    // [[NSProcessInfo processInfo] beginActivityWithOptions:reason:] through the
    // Objective-C runtime, so the runtime needs no Objective-C++ source. The
    // returned token is intentionally retained for the process lifetime.
    // NSActivityUserInitiated (0x00FFFFFF) | NSActivityLatencyCritical (0xFF00000000)
    constexpr uint64_t kOptions = 0x00FFFFFFull | 0xFF00000000ull;
    using ProcessInfoFn = id (*)(Class, SEL);
    using StringFn = id (*)(Class, SEL, const char*);
    using BeginFn = id (*)(id, SEL, uint64_t, id);
    using RetainFn = id (*)(id, SEL);

    Class processInfoClass = objc_getClass("NSProcessInfo");
    Class stringClass = objc_getClass("NSString");
    if (!processInfoClass || !stringClass)
        return;
    id processInfo = reinterpret_cast<ProcessInfoFn>(objc_msgSend)(processInfoClass, sel_registerName("processInfo"));
    id reason = reinterpret_cast<StringFn>(objc_msgSend)(stringClass, sel_registerName("stringWithUTF8String:"),
        "Real-time game rendering and audio");
    if (!processInfo || !reason)
        return;
    id activity = reinterpret_cast<BeginFn>(objc_msgSend)(processInfo,
        sel_registerName("beginActivityWithOptions:reason:"), kOptions, reason);
    if (activity)
        reinterpret_cast<RetainFn>(objc_msgSend)(activity, sel_registerName("retain"));
}
#else
void BeginLatencyCriticalActivity()
{
}
#endif
}
