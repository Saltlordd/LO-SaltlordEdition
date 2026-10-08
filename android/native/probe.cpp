#include <SDL.h>
#include <SDL_vulkan.h>
#include <SDL_system.h>
#include <jni.h>
#include <android/log.h>
#include <kernel/guest_address_space.h>
#include <os/user_paths.h>
#include <plume_vulkan.h>
#include <plume_log.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <exception>
#include "audio_probe.h"
#include "xex_probe.h"

namespace plume {
    std::unique_ptr<RenderInterface> CreateVulkanInterface(RenderWindow window);
}
namespace {
void Log(const char* format, ...) {
    char line[2048];
    va_list args;
    va_start(args, format);
    std::vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    __android_log_write(ANDROID_LOG_INFO, "LostOdysseyRecomp", line);
    std::fprintf(stderr, "%s\n", line);
}
void PlumeLog(const plume::LogRecord& record) {
    Log("plume: backend=%s api=%s domain=%s result=0x%08x detail=%s",
        record.backend, record.api, plume::LogErrorDomainName(record.domain), record.rawCode, record.detail);
}
bool ShowResults(const char* summary) {
    auto* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    if (!env) return false;
    auto activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!activity) return false;
    auto type = env->GetObjectClass(activity);
    auto method = type ? env->GetMethodID(type, "showProbeResultsAndWait", "(Ljava/lang/String;)V") : nullptr;
    auto text = method ? env->NewStringUTF(summary) : nullptr;
    if (text) env->CallVoidMethod(activity, method, text);
    const bool exception = env->ExceptionCheck();
    if (exception) { env->ExceptionDescribe(); env->ExceptionClear(); }
    if (text) env->DeleteLocalRef(text);
    if (type) env->DeleteLocalRef(type);
    env->DeleteLocalRef(activity);
    return text && !exception;
}
bool TestGuestMemory() {
    auto* base = GuestAddressSpace::Allocate();
    if (!base) {
        const auto failure = GuestAddressSpace::GetFailureInfo();
        Log("BLOCKED guest memory: operation=%s api=%s address=0x%llx size=0x%llx offset=0x%llx view=%d errno=%u (%s)",
            GuestAddressSpace::FailureOperationName(failure.operation), GuestAddressSpace::FailureApiName(failure.operation),
            static_cast<unsigned long long>(failure.address), static_cast<unsigned long long>(failure.size),
            static_cast<unsigned long long>(failure.offset), failure.viewIndex, failure.error, std::strerror(failure.error));
        return false;
    }
    // Exercise direct base+guest-address access; no detached replacement mapping.
    auto* physical = reinterpret_cast<volatile uint32_t*>(base + 0xA0001000ull);
    auto* cAlias = reinterpret_cast<volatile uint32_t*>(base + 0xC0001000ull);
    auto* eAlias = reinterpret_cast<volatile uint32_t*>(base + 0xE0000000ull);
    *physical = 0x13579BDF;
    bool coherent = *cAlias == *physical && *eAlias == *physical;
    *eAlias = 0x2468ACE0;
    coherent &= *physical == *eAlias && *cAlias == *eAlias;
    Log("guest address space initialized: base=%p A/C/E alias coherence=%s", base, coherent ? "PASS" : "FAIL");
    GuestAddressSpace::Release(base);
    return coherent;
}
int Probe(int argc, char** argv) {
    Log("native ARM64 platform probe loaded; this is not the game runtime");
    if (argc < 3) { Log("BLOCKED: SDLActivity did not supply files/cache paths"); return 1; }
    os::user_paths::InitializeAndroid(argv[1], argv[2]);
    const auto files = os::user_paths::AndroidFilesDir();
    const auto cache = os::user_paths::AndroidCacheDir();
    for (const auto& path : {os::user_paths::ConfigDir(), os::user_paths::DataDir() / "saves",
        os::user_paths::ProfileDir(), os::user_paths::StateDir() / "logs", cache / "shaders"})
        std::filesystem::create_directories(path);
    const auto log = os::user_paths::StateDir() / "logs" / "android-phase1.log";
    if (!std::freopen(log.c_str(), "w", stderr)) {
        __android_log_write(ANDROID_LOG_ERROR, "LostOdysseyRecomp", "BLOCKED: cannot open diagnostic log file");
        return 1;
    }
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    Log("Android platform initialized: files=%s cache=%s log=%s", files.c_str(), cache.c_str(), log.c_str());
    plume::SetLogCallback(PlumeLog);
    const bool memoryReady = TestGuestMemory();
    // Independent graphics probe remains useful even when memory is blocked by 16 KiB pages.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        Log("BLOCKED SDL_Init: %s", SDL_GetError()); return 1;
    }
    Log("SDL initialized; controller count=%d", SDL_NumJoysticks());
    const auto audio = RunAudioProbe(Log);
    if (SDL_Vulkan_LoadLibrary(nullptr) != 0) {
        Log("BLOCKED Vulkan loader: %s", SDL_GetError()); SDL_Quit(); return 1;
    }
    Log("Vulkan loader initialized");
    SDL_Window* window = SDL_CreateWindow("Lost Odyssey Platform Probe", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN);
    if (!window) { Log("BLOCKED SDL Vulkan window: %s", SDL_GetError()); SDL_Quit(); return 1; }
    bool graphicsReady = false;
    {
        auto interface = plume::CreateVulkanInterface(window);
        if (!interface) Log("BLOCKED plume Vulkan instance; inspect native API errors in log file");
        else {
            Log("Vulkan instance created through plume + SDL");
            auto device = interface->createDevice();
            if (!device) Log("BLOCKED plume physical/logical device creation; inspect API errors in log file");
            else {
                Log("GPU detected; logical device created: %s", device->getDescription().name.c_str());
                const auto* vkDevice = static_cast<const plume::VulkanDevice*>(device.get());
                VkPhysicalDeviceFeatures features{};
                vkGetPhysicalDeviceFeatures(vkDevice->physicalDevice, &features);
                Log("GPU API=%u.%u.%u shaderInt64=%u descriptorIndexing=%u scalarBlockLayout=%u",
                    VK_VERSION_MAJOR(vkDevice->physicalDeviceProperties.apiVersion),
                    VK_VERSION_MINOR(vkDevice->physicalDeviceProperties.apiVersion),
                    VK_VERSION_PATCH(vkDevice->physicalDeviceProperties.apiVersion),
                    features.shaderInt64, vkDevice->capabilities.descriptorIndexing, vkDevice->capabilities.scalarBlockLayout);
                auto queue = device->createCommandQueue(plume::RenderCommandListType::DIRECT);
                if (!queue) Log("BLOCKED graphics queue creation");
                else {
                    auto swapchain = queue->createSwapChain(plume::RenderSwapChainDesc(window, plume::RenderFormat::B8G8R8A8_UNORM, 2));
                    const auto* native = static_cast<const plume::VulkanSwapChain*>(swapchain.get());
                    if (native && native->surface) Log("Android Vulkan surface created through SDL");
                    if (!swapchain || swapchain->isEmpty()) Log("BLOCKED swapchain creation; inspect native API errors in log file");
                    else {
                        graphicsReady = true;
                        Log("swapchain created: %ux%u; frame rendering/presentation not exercised", swapchain->getWidth(), swapchain->getHeight());
                    }
                    // No rendering or game loop: tear down graphics before awaiting exit.
                }
            }
        }
    }
    auto game = files / "game" / "disc1";
    std::ifstream overridePath(files / "game-path.txt");
    std::string line;
    if (std::getline(overridePath, line) && !line.empty()) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto candidate = std::filesystem::path(line);
        if (candidate.is_absolute()) game = candidate;
        else Log("game-path.txt ignored: development path must be absolute");
    }
    Log("game path=%s default.xex=%s", game.c_str(), std::filesystem::is_regular_file(game / "default.xex") ? "located" : "missing");
    const auto xex = TestXexImage(game / "default.xex", Log);
    const char* xexStatus = xex == XexProbeResult::Loaded ? "PASS" : xex == XexProbeResult::Missing ? "NOT TESTED" : "BLOCKED";
    Log("RESULT: guest_memory=%s graphics=%s xma_decoder_smoke=%s pcm_queue=%s xex_loader=%s; STOP: probe does not execute generated PPC game code or link the game renderer; game audio is not decoded",
        memoryReady ? "PASS" : "BLOCKED", graphicsReady ? "PASS" : "BLOCKED",
        audio.decoderReady ? "PASS" : "BLOCKED", audio.pcmReady ? "PASS" : "BLOCKED", xexStatus);
    Log("Read log with: adb shell run-as io.github.freefrank.lostodyssey.probe cat files/state/logs/android-phase1.log");
    char summary[768];
    std::snprintf(summary, sizeof(summary),
        "Guest memory: %s\nVulkan swapchain: %s\nXMA decoder smoke: %s\nPCM queue: %s\nDisc 1 XEX loader: %s\n\n"
        "Did you hear the short tone? Please report that with the log.\n"
        "Game audio decoding has not been tested. This app cannot play Lost Odyssey.\n"
        "Tap Share log to send the diagnostic file.\n\nTap Close to exit.",
        memoryReady ? "PASS" : "BLOCKED (see logs)", graphicsReady ? "PASS" : "BLOCKED (see logs)",
        audio.decoderReady ? "PASS" : "BLOCKED (see logs)", audio.pcmReady ? "PASS" : "BLOCKED (see logs)", xexStatus);
    if (!ShowResults(summary) && SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Lost Odyssey XEX Test results", summary, window) != 0)
        Log("result dialog unavailable: %s", SDL_GetError());
    plume::SetLogCallback(nullptr);
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    return memoryReady && graphicsReady && audio.decoderReady && audio.pcmReady ? 0 : 1;
}
}
extern "C" int SDL_main(int argc, char** argv) {
    try { return Probe(argc, argv); }
    catch (const std::exception& e) { Log("BLOCKED native exception: %s", e.what()); return 1; }
}
