#include "android_touch.h"

#include <algorithm>
#include <mutex>

#if defined(__ANDROID__)
#include <stdafx.h>
#include "hid.h"
#include <jni.h>
#endif

namespace hid::android_touch
{
namespace
{
std::mutex g_mutex;
State g_state;
}

void Update(int32_t buttons, int32_t leftTrigger, int32_t rightTrigger,
            int32_t leftX, int32_t leftY, int32_t rightX, int32_t rightY)
{
    const State next{
        static_cast<uint16_t>(buttons & 0xF3FF),
        static_cast<uint8_t>(std::clamp(leftTrigger, 0, 255)),
        static_cast<uint8_t>(std::clamp(rightTrigger, 0, 255)),
        static_cast<int16_t>(std::clamp(leftX, -32768, 32767)),
        static_cast<int16_t>(std::clamp(leftY, -32768, 32767)),
        static_cast<int16_t>(std::clamp(rightX, -32768, 32767)),
        static_cast<int16_t>(std::clamp(rightY, -32768, 32767)),
    };
    std::lock_guard lock(g_mutex);
    g_state = next;
}

State Snapshot()
{
    std::lock_guard lock(g_mutex);
    return g_state;
}

void Clear()
{
    std::lock_guard lock(g_mutex);
    g_state = {};
}
}

#if defined(__ANDROID__)
extern "C" JNIEXPORT void JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeActivity_nativeSetTouchInput(
    JNIEnv*, jclass, jint buttons, jint leftTrigger, jint rightTrigger,
    jint leftX, jint leftY, jint rightX, jint rightY)
{
    hid::android_touch::Update(buttons, leftTrigger, rightTrigger,
                               leftX, leftY, rightX, rightY);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeActivity_nativeHasConnectedController(
    JNIEnv*, jclass)
{
    return hid::HasConnectedController() ? JNI_TRUE : JNI_FALSE;
}
#endif
