#include <gpu/android_border.h>
#include "android_menu_pause.h"
#include <stdafx.h>
#include <kernel/memory.h>
#include <kernel/guest_address_space.h>
#include <hid/hid.h>
#include <gpu/shader/dxc_compiler.h>
#include <os/logger.h>
#include <os/log_file.h>
#include <os/user_paths.h>
#include <SDL.h>
#include <SDL_system.h>
#include <jni.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <install/import_image.h>
#include "content_bridge.h"
#include "shader_preparation_policy.h"
#include <hid/touch_state.h>
#include <hid/android_overlay_state.h>
#include <debug/fast_forward.h>
#include <debug/save_anywhere.h>
#include <gpu/video.h>
#include <gpu/android_surface.h>
#include <kernel/heap.h>
#include <kernel/xex_loader.h>
#include <kernel/io/file_system.h>
#include <kernel/io/disc_set.h>
#include <kernel/xam.h>
#include <settings/config.h>
#include <settings/android_test_baseline.h>
#include <gpu/taa_collection.h>
#include <host_ui/host_ui.h>
#include <debug/menu_overlay.h>
#include <settings/menu.h>
#include <os/shader_log.h>
extern "C" {
#include <libavutil/sha.h>
#include <libavutil/mem.h>
}
extern "C" JNIEXPORT jint JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeSaveAnywhere(JNIEnv*,jclass,jint choice){
    if(choice==0||choice==1){if(!debug_menu::SetSaveAnywhereEnabled(choice==1))return -2;}
    return debug_menu::SaveAnywhereEnabled()?1:0;
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeTouch(
    JNIEnv*, jclass, jint buttons, jint lt, jint rt, jint lx, jint ly, jint rx, jint ry) {
    hid::touch::State state;
    state.buttons=uint16_t(buttons & 0xf3ff);
    state.lt=uint8_t(std::clamp(int(lt),0,255)); state.rt=uint8_t(std::clamp(int(rt),0,255));
    state.lx=int16_t(std::clamp(int(lx),-32767,32767)); state.ly=int16_t(std::clamp(int(ly),-32767,32767));
    state.rx=int16_t(std::clamp(int(rx),-32767,32767)); state.ry=int16_t(std::clamp(int(ry),-32767,32767));
    hid::touch::Set(state);
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeSurfaceReady(JNIEnv*,jclass,jboolean ready){
    const auto before=gpu::android_surface::Read();const auto after=gpu::android_surface::Signal(ready);
    if(before!=after)LOG_INFO("Android surface lifecycle: ready={} generation={}",bool(after&1),after>>1);
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeDebugMenu(JNIEnv*,jclass){
    SDL_Event event{};event.type=SDL_KEYDOWN;event.key.state=SDL_PRESSED;event.key.keysym.sym=SDLK_F1;event.key.keysym.scancode=SDL_SCANCODE_F1;
    SDL_PushEvent(&event);event.type=SDL_KEYUP;event.key.state=SDL_RELEASED;SDL_PushEvent(&event);
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeAppMenu(JNIEnv*,jclass,jboolean open){
    static android_menu::PauseLease pause;
    hid::android_overlay::appMenu.store(open,std::memory_order_release);debug_menu::fast_forward::Release();
    pause.Set(open,gpu::video::AndroidGuestPresentCount()>0,debug_menu::IsOverlayVisible()||settings::IsOpen());
}
extern "C" JNIEXPORT jint JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeTouchContext(JNIEnv*,jclass){return hid::android_overlay::Context(hid::android_overlay::NowMs());}
extern "C" JNIEXPORT jint JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeRenderResolution(JNIEnv*,jclass,jint height){
    auto config=settings::GetConfig();
    if(height==720||height==1080){config.internalResolution=height;settings::PreviewConfig(config);if(!settings::SaveConfig(config))LOG_ERROR("Could not save wizard resolution choice; current session updated");}
    return config.internalResolution;
}
extern "C" JNIEXPORT jint JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeAntiAliasing(JNIEnv*,jclass,jint choice){
    auto config=settings::GetConfig();
    if(choice>=0&&choice<=2){config.antialiasing=uint32_t(choice);config.fxaa=choice==1;settings::PreviewConfig(config);if(!settings::SaveConfig(config))LOG_ERROR("Could not save anti-aliasing choice; current session updated");}
    return jint(config.antialiasing);
}
extern "C" JNIEXPORT jlong JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativePreparationProgress(JNIEnv*,jclass){return jlong(gpu::video::AndroidShaderPreparationProgress());}
extern "C" JNIEXPORT jintArray JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeGraphicsPreset(JNIEnv* env,jclass,jint height,jint aa){
    auto config=settings::GetConfig();
    if(height==720||height==1080){
        config.width=height==720?1280:1920;config.height=height;config.internalResolution=height;
        if(height==1080&&config.frameRate==60)config.frameRate=30;config.antialiasing=aa>=0&&aa<=2?uint32_t(aa):0;config.fxaa=config.antialiasing==1;
        config.upscaler=gpu::upscaling::Upscaler::Off;config.anisotropicFiltering=0;config.scalingQuality=0;config.expandRgbRange=true;
        config.frameGenerationProvider=framegen::Provider::Off;config.variableRefreshRate=false;
        settings::PreviewConfig(config);if(!settings::SaveConfig(config))LOG_ERROR("Could not save wizard graphics preset");
    }
    jint data[]={jint(config.internalResolution),jint(config.antialiasing)};auto out=env->NewIntArray(2);if(out)env->SetIntArrayRegion(out,0,2,data);return out;
}
// Persist only the shared configuration; held/active trigger state is session-only.
void PersistAndroidFastForward(){
    auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    auto activity=env?static_cast<jobject>(SDL_AndroidGetActivity()):nullptr;
    if(!activity)return;
    const auto status=debug_menu::fast_forward::GetStatus();
    auto type=env->GetObjectClass(activity);
    auto method=type?env->GetMethodID(type,"persistFastForward","(III)V"):nullptr;
    if(method)env->CallVoidMethod(activity,method,status.enabled?1:0,status.mode==debug_menu::fast_forward::Mode::Toggle?1:0,jint(status.multiplier));
    if(env->ExceptionCheck()){env->ExceptionDescribe();env->ExceptionClear();}
    if(type)env->DeleteLocalRef(type);env->DeleteLocalRef(activity);
}
extern "C" JNIEXPORT jintArray JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeFastForward(JNIEnv* env,jclass,jint command,jint value){
    using namespace debug_menu::fast_forward;
    if(command==1)Enable(value!=0);else if(command==2)SetMode(value==1?Mode::Toggle:Mode::Hold);else if(command==3)SetRate(unsigned(value));
    const auto status=GetStatus();jint data[]={status.enabled?1:0,status.mode==Mode::Toggle?1:0,jint(status.multiplier),status.active?1:0,host_ui::IsStopping()?1:0};
    if(command&&!host_ui::IsStopping())PersistAndroidFastForward();
    if(command)LOG_INFO("Android Fast Forward options: enabled={} mode={} multiplier={} (shared debug state)",status.enabled,status.mode==Mode::Toggle?"toggle":"hold",status.multiplier);
    auto result=env->NewIntArray(5);if(result)env->SetIntArrayRegion(result,0,5,data);return result;
}
extern "C" JNIEXPORT jint JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeFrameRate(JNIEnv*, jclass, jint fps) {
    if (fps == 30 || fps == 60) {
        settings::PreviewFrameRate(uint32_t(fps));
        LOG_INFO("Android quick options: frame-rate target={} (applies now and persists)", fps);
    }
    return jint(settings::GetConfig().frameRate);
}

namespace {
std::atomic<bool> importCancelled{false},bootActive{false},guestStarted{false},reportRequested{false},preparationFinished{false};
 }
extern "C" JNIEXPORT jlongArray JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativePerformanceSnapshot(JNIEnv* env,jclass){
    const auto config=settings::GetConfig();
    jlong values[]={jlong(gpu::video::AndroidGuestPresentCount()),jlong(config.frameRate),jlong(config.internalResolution),host_ui::IsGamePaused()?1L:0L,guestStarted.load()?1L:0L};
    auto result=env->NewLongArray(5);if(result)env->SetLongArrayRegion(result,0,5,values);return result;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeGuestStarted(JNIEnv*,jclass){return guestStarted.load()?JNI_TRUE:JNI_FALSE;}
extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativePreparationFinished(JNIEnv*,jclass){return preparationFinished.load()?JNI_TRUE:JNI_FALSE;}
extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativePreparationStopped(JNIEnv*,jclass){return importCancelled.load()||gpu::video::ExitRequested()?JNI_TRUE:JNI_FALSE;}
extern "C" JNIEXPORT void JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativePreparationServiceEvent(JNIEnv*,jclass,jint event){LOG_INFO("Android shader preparation service: {}",event==1?"foreground worker started":event==2?"preparation finished; foreground worker stopping":event==3?"background time limit reached; cached progress retained":"worker event");}
extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeDriverReady(JNIEnv*,jclass){return guestStarted.load()&&gpu::video::AndroidGuestPresentCount()>0?JNI_TRUE:JNI_FALSE;}
extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeFinishTest(JNIEnv*,jclass) {
    if(!bootActive.load()||!guestStarted.load())return JNI_FALSE;
    reportRequested.store(true,std::memory_order_release);return JNI_TRUE;
}
namespace {
bool executableLoaded=false;
uintptr_t runtimeBase=0, compilerBase=0;
constexpr int crashSignals[]={SIGSEGV,SIGBUS,SIGILL,SIGABRT};
struct sigaction previousSignals[4]{};
void CrashSignal(int signal, siginfo_t* info, void* context) {
    char out[256];size_t n=0;
    auto text=[&](const char* p){while(*p&&n<sizeof(out))out[n++]=*p++;};
    auto hex=[&](uintptr_t value){text("0x");for(int shift=60;shift>=0;shift-=4)if(n<sizeof(out))out[n++]="0123456789abcdef"[(value>>shift)&15];};
    text("Android native crash: signal=");hex(signal);
    auto* regs=static_cast<ucontext_t*>(context);
    text(" pc=");hex(regs?regs->uc_mcontext.pc:0);
    text(" fault=");hex(info?reinterpret_cast<uintptr_t>(info->si_addr):0);
    text(" runtimeBase=");hex(runtimeBase);text(" compilerBase=");hex(compilerBase);text("\n");
    os::logger::EmergencyWrite(out,n);
    for(size_t i=0;i<4;++i)if(crashSignals[i]==signal) {
        sigaction(signal,&previousSignals[i],nullptr);break;
    }
    raise(signal); // Chain the original Android handler after writing the app log.
}

void InstallDiagnosticCrashHandler() {
    Dl_info module{};
    if(dladdr(reinterpret_cast<void*>(&CrashSignal),&module))runtimeBase=reinterpret_cast<uintptr_t>(module.dli_fbase);
    void* compiler=dlopen("libdxcompiler.so",RTLD_NOW|RTLD_NOLOAD);
    if(compiler) {
        void* factory=dlsym(compiler,"DxcCreateInstance");
        if(factory&&dladdr(factory,&module))compilerBase=reinterpret_cast<uintptr_t>(module.dli_fbase);
        dlclose(compiler);
    }
    struct sigaction action{};action.sa_sigaction=CrashSignal;
    sigemptyset(&action.sa_mask);action.sa_flags=SA_SIGINFO|SA_RESETHAND;
    for(size_t i=0;i<4;++i)sigaction(crashSignals[i],&action,&previousSignals[i]);
}

std::string PerformanceEnvironment() {
    auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    auto activity=env?static_cast<jobject>(SDL_AndroidGetActivity()):nullptr;
    if(!activity)return "unavailable";
    auto type=env->GetObjectClass(activity);
    auto method=type?env->GetMethodID(type,"performanceEnvironment","()Ljava/lang/String;"):nullptr;
    auto value=method?static_cast<jstring>(env->CallObjectMethod(activity,method)):nullptr;
    std::string result="unavailable";
    if(env->ExceptionCheck()){env->ExceptionClear();value=nullptr;}
    if(value){const char* text=env->GetStringUTFChars(value,nullptr);if(text){result=text;env->ReleaseStringUTFChars(value,text);}env->DeleteLocalRef(value);}
    if(type)env->DeleteLocalRef(type);env->DeleteLocalRef(activity);return result;
}
int ChooseIso() {
    auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    auto activity=env?static_cast<jobject>(SDL_AndroidGetActivity()):nullptr;
    if(!activity)return -1;
    auto type=env->GetObjectClass(activity);
    auto method=type?env->GetMethodID(type,"chooseGameIsoAndWait","()I"):nullptr;
    int fd=method?env->CallIntMethod(activity,method):-1;
    if(env->ExceptionCheck()){env->ExceptionDescribe();env->ExceptionClear();fd=-1;}
    if(type)env->DeleteLocalRef(type);env->DeleteLocalRef(activity);return fd;
}
std::string AssetCheck() {
    const int fd=ChooseIso();
    if(fd<0){LOG_INFO("Disc 1 asset check: SKIPPED (no file selected)");return "Skipped";}
    struct Owned { int fd; ~Owned(){close(fd);} } owned{fd};
    try {
        install::IsoImageReader image(fd);
        const auto entries=image.GetEntries();
        auto executable=std::find_if(entries.begin(),entries.end(),[](const install::Entry& entry){
            std::string name=entry.name;
            std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
            return name=="default.xex";
        });
        if(executable==entries.end()||executable->size!=6623232)
            throw install::Error("Unsupported disc: expected the supplied Disc 1 default.xex");
        std::vector<uint8_t> bytes(size_t(executable->size));image.Read(executable->offset,bytes.data(),bytes.size());
        auto* sha=av_sha_alloc();if(!sha)throw install::Error("SHA-256 allocation failed");
        uint8_t digest[32]{};
        if(av_sha_init(sha,256)<0){av_free(sha);throw install::Error("SHA-256 initialization failed");}
        av_sha_update(sha,bytes.data(),bytes.size());av_sha_final(sha,digest);av_free(sha);
        const auto hash=install::crypto::HexString(digest,sizeof(digest));
        if(hash!="175ae53d109d480a83bebbd186e7b6871f7b03ce80af69ab388db2f747640de3")
            throw install::Error("Unsupported Disc 1 executable fingerprint");
        uint64_t total=0;uint32_t samples=0;
        for(const auto& entry:entries) {
            if(entry.offset>image.GetLimit()||entry.size>image.GetLimit()-entry.offset)
                throw install::Error("Asset extends beyond the ISO partition");
            total+=entry.size;
            if(entry.size&&entry.name!="default.xex"&&samples<8) {
                uint8_t sample[32];image.Read(entry.offset,sample,size_t(std::min<uint64_t>(entry.size,sizeof(sample))));++samples;
            }
        }
        LOG_INFO("Disc 1 asset access: PASS entries={} totalBytes={} sampledFiles={} XEX_SHA256={}",entries.size(),total,samples,hash);
        // This is a bounded startup test: retain only the verified executable.
        // Full disc installation and guest execution are separate stages.
        const auto root=os::user_paths::AndroidCacheDir()/"runtime-check-game";
        std::filesystem::create_directories(root);
        const auto path=root/"default.xex";
        {
            std::ofstream output(path,std::ios::binary|std::ios::trunc);
            output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
            output.close();
            if(!output)throw install::Error("Could not write the verified executable");
        }
        LOG_INFO("Production executable setup: BEGIN (no guest threads)");
        g_userHeap.Init();
        g_pageAllocator.Init();
        FileSystem::Init(root);
        const auto entry=XexLoader::Load(path);
        executableLoaded=entry==0x827ca440 && XexLoader::s_imageBase==0x82000000 &&
            XexLoader::s_imageSize==20709376;
        if(!executableLoaded)throw install::Error("Unexpected loaded executable layout");
        LOG_INFO("Production executable setup: PASS base={:#x} size={} entry={:#x}",XexLoader::s_imageBase,XexLoader::s_imageSize,entry);
        return fmt::format("PASS: {} entries, {} file samples; executable loaded",entries.size(),samples);
    }catch(const std::exception& error){LOG_ERROR("Disc 1 asset access: FAIL {}",error.what());return std::string("FAIL: ")+error.what();}
}

bool Dialog(const std::string& text) {
    auto* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    auto activity = env ? static_cast<jobject>(SDL_AndroidGetActivity()) : nullptr;
    if (!activity) return false;
    auto type = env->GetObjectClass(activity);
    auto method = type ? env->GetMethodID(type, "showProbeResultsAndWait", "(Ljava/lang/String;)V") : nullptr;
    auto message = method ? env->NewStringUTF(text.c_str()) : nullptr;
    if (message) env->CallVoidMethod(activity, method, message);
    const bool failed = env->ExceptionCheck();
    if (failed) { env->ExceptionDescribe(); env->ExceptionClear(); }
    const bool shown = message && !failed;
    if (message) env->DeleteLocalRef(message);
    if (type) env->DeleteLocalRef(type);
    env->DeleteLocalRef(activity);
    return shown;
}
bool Compile(const char* source, const char* profile) {
    const auto result = xenos::CompileHlsl(source, "main", profile, xenos::ShaderBinaryFormat::Spirv);
    uint32_t magic = 0;
    if (result.bytecode.size() >= sizeof(magic)) std::memcpy(&magic, result.bytecode.data(), sizeof(magic));
    const bool passed = result.ok && result.bytecode.size() >= 20 &&
                        result.bytecode.size() % 4 == 0 && magic == 0x07230203;
    LOG_INFO("Android DXC {}: {} bytes={} errors={}", profile, passed ? "PASS" : "FAIL", result.bytecode.size(), result.errors);
    return passed;
}
}

// Bounded check linked into the real runtime. It starts no guest threads and
// loads the verified executable and initializes the renderer, but runs no game code.
int RunAndroidRuntimeReadiness() {
    const auto log = os::user_paths::StateDir() / "logs/android-phase1.log";
    std::error_code ec;
    std::filesystem::create_directories(log.parent_path(), ec);
    if (ec || !os::logger::OpenFile(log)) return 1;
    LOG_INFO("Android Phase 9 executable setup, renderer startup and visible touch controls");
    const bool memory = g_memory.base != nullptr;
    LOG_INFO("production guest address space allocation: {}", memory ? "PASS" : "FAIL");
    if (!memory) {
        const auto failure = GuestAddressSpace::GetFailureInfo();
        LOG_ERROR("guest allocation failure: operation={} view={} address={:#x} size={:#x} offset={:#x} errno={}",
            GuestAddressSpace::FailureOperationName(failure.operation), failure.viewIndex,
            failure.address, failure.size, failure.offset, failure.error);
    }
    const bool dxc = xenos::DxcAvailable();
    LOG_INFO("Android DXC load: {} identity={}", dxc ? "PASS" : "FAIL", xenos::DxcIdentity());
    InstallDiagnosticCrashHandler();
    setenv("LO_ANDROID_SHADER_TRACE","1",1);
    const bool vertex = dxc && Compile("float4 main(uint id : SV_VertexID) : SV_Position { return float4(float(id & 1), float(id >> 1), 0, 1); }", "vs_6_0");
    const bool pixel = dxc && Compile("float4 main() : SV_Target { return float4(1, 0, 0, 1); }", "ps_6_0");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        LOG_ERROR("Android SDL initialization failed: {}", SDL_GetError());
        Dialog("SDL initialization failed. Share the log for diagnosis.");
        return 1;
    }
    auto* window = SDL_CreateWindow("Controller input test", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_FULLSCREEN);
    if (!window) {
        LOG_ERROR("Android SDL window failed: {}", SDL_GetError());
        Dialog("SDL window creation failed. Share the log for diagnosis.");
        SDL_Quit(); return 1;
    }
    hid::Init();
    hid::SetExternalEventPump(true);
    Dialog("Runtime loaded. Controller test starts when you tap Continue.\n\nFor 20 seconds, drag both stick knobs and press each D-pad arrow and button. You can also use your controller. A black background is expected. After input testing, choose your Disc 1 ISO and the renderer startup check will run.\n\nThis test does not run the game.");
    uint16_t buttons = 0;
    bool connected = false, leftStick = false, rightStick = false, leftTrigger = false, rightTrigger = false;
    const auto end = SDL_GetTicks64() + 20000;
    bool quit = false;
    uint16_t touchButtons=0;bool touchLeft=false,touchRight=false,touchLT=false,touchRT=false;
    while (!quit && SDL_GetTicks64() < end) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = true;
            else if (event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED)
                hid::HandleControllerEvent(event.type, event.cdevice.which);
        }
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            auto* controller = SDL_GameControllerFromInstanceID(SDL_JoystickGetDeviceInstanceID(i));
            connected |= controller && SDL_GameControllerGetAttached(controller);
        }
        const auto touch=hid::touch::Read();
        touchButtons|=touch.buttons;touchLeft|=std::abs(int(touch.lx))>12000||std::abs(int(touch.ly))>12000;
        touchRight|=std::abs(int(touch.rx))>12000||std::abs(int(touch.ry))>12000;
        touchLT|=touch.lt>32;touchRT|=touch.rt>32;
        XAMINPUT_STATE state{};
        if (hid::GetState(0, &state) == 0) {
            const auto& pad = state.Gamepad;
            buttons |= pad.wButtons;
            leftStick |= std::abs(int(pad.sThumbLX)) > 12000 || std::abs(int(pad.sThumbLY)) > 12000;
            rightStick |= std::abs(int(pad.sThumbRX)) > 12000 || std::abs(int(pad.sThumbRY)) > 12000;
            leftTrigger |= pad.bLeftTrigger > 32;
            rightTrigger |= pad.bRightTrigger > 32;
        }
        SDL_Delay(16);
    }
    LOG_INFO("production HID observations: connected={} Xbox buttons={:#06x} leftStick={} rightStick={} leftTrigger={} rightTrigger={}",
        connected, buttons, leftStick, rightStick, leftTrigger, rightTrigger);
    LOG_INFO("touch source observations: buttons={:#06x} leftStick={} rightStick={} leftTrigger={} rightTrigger={}",touchButtons,touchLeft,touchRight,touchLT,touchRT);
    const auto assets=quit?std::string("Skipped (test closed)"):AssetCheck();
    Dialog("Disc 1 access result: " + assets + "\n\nShare the log now if desired. Tap Continue to start the separate renderer check. This build does not launch the game.");
    hid::touch::Set({});
    SDL_DestroyWindow(window);
    bool renderer=false;
    if(!quit) {
        // The diagnostic stores only default.xex, not the game's shader-bearing
        // assets. Keep built-in renderer shaders enabled, defer game prebuilding.
        setenv("LO_NO_SHADER_PREPARE","1",1);
        setenv("LO_NO_PIPELINE_PREPARE","1",1);
        LOG_INFO("Game shader prebuild deferred: diagnostic has executable only; loaded={}",executableLoaded);
        LOG_INFO("Production Vulkan renderer startup: BEGIN (no guest threads)");
        try {renderer=gpu::video::Init();}
        catch(const std::exception& error){LOG_ERROR("Production renderer startup exception: {}",error.what());}
        LOG_INFO("Production Vulkan renderer startup: {}",renderer?"PASS":"FAIL");
        if(renderer) {
            const auto until=SDL_GetTicks64()+2000;
            while(SDL_GetTicks64()<until&&!gpu::video::ExitRequested()) {
                gpu::video::PumpEvents();SDL_Delay(16);
            }
        }
        gpu::video::Shutdown();
    }
    hid::Shutdown();
    LOG_INFO("No guest instructions, game shader translation, rendered frames or game audio exercised; no disc installation performed");
    Dialog(fmt::format("Guest memory: {}\nDXC vertex/pixel: {} / {}\n\nInput Xbox buttons: {:#06x}\nTouch buttons: {:#06x}\nTouch sticks L/R: {} / {}\nTouch triggers L/R: {} / {}\n\nDisc 1 assets: {}\nProduction renderer startup: {}\n\nShare this log. This build does not run the game.",
        memory?"PASS":"FAIL",vertex?"PASS":"FAIL",pixel?"PASS":"FAIL",buttons,touchButtons,touchLeft,touchRight,touchLT,touchRT,assets,renderer?"PASS":"FAIL / skipped"));
    SDL_Quit();
    return memory && vertex && pixel && renderer ? 0 : 1;
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeCancelImport(JNIEnv*,jclass) {
    importCancelled.store(true);
    if(bootActive.load()) {
        constexpr char line[]="Android boot test: activity destroyed; terminating isolated boot process\n";
        os::logger::EmergencyWrite(line,sizeof(line)-1);
        std::_Exit(0); // Guest threads cannot safely survive SDL/native-library unloading.
    }
}

void AndroidBootGuestStarted() {
    // GPU initialization/preparation has completed; guest input/gameplay has not begun.
    preparationFinished.store(true,std::memory_order_release);
    const bool compiled=xenos::GetDxcStatistics().succeeded>0;
    LOG_INFO("Android preparation completed before guest entry: new successful DXC compilations={} restart prompt eligible={}",xenos::GetDxcStatistics().succeeded,compiled);
    auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());auto activity=env?static_cast<jobject>(SDL_AndroidGetActivity()):nullptr;
    if(activity){auto type=env->GetObjectClass(activity);auto method=type?env->GetMethodID(type,"awaitPreparationRestart","(Z)V"):nullptr;if(method)env->CallVoidMethod(activity,method,compiled?JNI_TRUE:JNI_FALSE);if(env->ExceptionCheck()){env->ExceptionDescribe();env->ExceptionClear();}if(type)env->DeleteLocalRef(type);env->DeleteLocalRef(activity);}
    SDL_SetHint("LO_ANDROID_SHADER_PREPARATION","0");guestStarted.store(true,std::memory_order_release);
}

namespace {
void ImportProgress(uint64_t done,uint64_t total,std::string_view label) {
    auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    auto activity=env?static_cast<jobject>(SDL_AndroidGetActivity()):nullptr;
    if(!activity)return;
    auto type=env->GetObjectClass(activity);
    auto method=type?env->GetMethodID(type,"updateImportProgress","(JJLjava/lang/String;)V"):nullptr;
    auto message=method?env->NewStringUTF(std::string(label).c_str()):nullptr;
    if(message)env->CallVoidMethod(activity,method,jlong(done),jlong(total),message);
    if(env->ExceptionCheck()){env->ExceptionDescribe();env->ExceptionClear();importCancelled.store(true);}
    if(message)env->DeleteLocalRef(message);
    if(type)env->DeleteLocalRef(type);env->DeleteLocalRef(activity);
}
bool VerifiedInstalledDisc(const std::filesystem::path& root) {
    if(!DiscSet::Validate(root,{3,1}))return false;
    std::error_code error;
    if(std::filesystem::file_size(root/"default.xex",error)!=6623232||error)return false;
    std::ifstream input(root/"default.xex",std::ios::binary);
    std::vector<uint8_t> bytes(6623232);
    if(!input.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))return false;
    auto* sha=av_sha_alloc();if(!sha)return false;
    uint8_t digest[32]{};
    if(av_sha_init(sha,256)<0){av_free(sha);return false;}
    av_sha_update(sha,bytes.data(),bytes.size());av_sha_final(sha,digest);av_free(sha);
    return install::crypto::HexString(digest,sizeof(digest))==
        "175ae53d109d480a83bebbd186e7b6871f7b03ce80af69ab388db2f747640de3";
}
}

int RunAndroidGuestBoot(uint32_t entry);
static bool ChooseStartupPreparation() {
    auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    auto activity=env?static_cast<jobject>(SDL_AndroidGetActivity()):nullptr;
    if(!activity)return true;
    auto type=env->GetObjectClass(activity);
    auto method=type?env->GetMethodID(type,"chooseStartupAndWait","()Z"):nullptr;
    bool prepare=true;
    if(method)prepare=env->CallBooleanMethod(activity,method)==JNI_TRUE;
    if(env->ExceptionCheck()){env->ExceptionClear();prepare=false;}
    if(type)env->DeleteLocalRef(type);env->DeleteLocalRef(activity);
    return prepare;
}
int RunAndroidFirstBootCheck() {
    importCancelled.store(false);
    const auto log=os::user_paths::StateDir()/"logs/android-phase1.log";
    std::filesystem::create_directories(log.parent_path());
    if(!os::logger::OpenFile(log))return 1;
    LOG_INFO("Android Phase 33 streamlined setup; mandatory shader preparation; stable rendering retained");
    // Saved graphics preferences are retained across launches.
    os::logger::g_kernelTrace=false;
    LOG_INFO("Android gameplay tracing: routine kernel trace disabled; warnings, errors and stall timings retained");
    const auto baseline=settings::GetConfig();
    LOG_INFO("Android retained graphics settings: sceneHeight={} AA={} AF={} scaling={} upscaler={} frameGeneration={} frameRate={} VRR={} (saved graphics settings retained)",
        baseline.internalResolution,baseline.antialiasing,baseline.anisotropicFiltering,baseline.scalingQuality,
        uint32_t(baseline.upscaler),uint32_t(baseline.frameGenerationProvider),baseline.frameRate,baseline.variableRefreshRate);
    LOG_INFO("Android storage: files={} cache={}",os::user_paths::AndroidFilesDir().string(),os::user_paths::AndroidCacheDir().string());
    const auto driverCache=(os::user_paths::AndroidCacheDir()/"vulkan-pipelines.bin").string();
    setenv("LO_ANDROID_VK_PIPELINE_CACHE",driverCache.c_str(),1);
    if(!g_memory.base){LOG_ERROR("Guest address space unavailable");Dialog("Guest memory allocation failed. Share log.");return 1;}
    const bool dxc=xenos::DxcAvailable();
    InstallDiagnosticCrashHandler();
    if(!dxc){LOG_ERROR("DXC unavailable");Dialog("Shader compiler unavailable. Share log.");return 1;}
    SDL_SetHint("LO_ANDROID_SHADER_PREPARATION","1");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0)return 1;
    auto* window=SDL_CreateWindow("Lost Odyssey first boot test",SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,1280,720,SDL_WINDOW_FULLSCREEN);
    if(!window){SDL_Quit();return 1;}
    if(!ChooseStartupPreparation()){SDL_DestroyWindow(window);SDL_Quit();return 0;}
    const bool prepareBeforeGameplay=true;
    LOG_INFO("Android shader preparation selected: {} (required runtime compilation remains enabled)",prepareBeforeGameplay?"recommended startup preparation":"compile while playing");
    const auto destination=os::user_paths::AndroidFilesDir()/"game";
    const auto root=destination/"disc1";
    try {
        bool importedContent=false;
        const auto pendingDirectory=os::user_paths::AndroidFilesDir()/"pending-content";
        if(std::filesystem::exists(pendingDirectory))for(const auto& item:std::filesystem::directory_iterator(pendingDirectory))if(item.path().extension()==".ready")importedContent=true;
        std::filesystem::create_directories(destination);
        auto contentLast=std::chrono::steady_clock::time_point{};
        const AndroidContentProgress contentProgress=[&](uint64_t done,uint64_t total,std::string_view label){
            const auto now=std::chrono::steady_clock::now();
            if(done==total||(done==0&&total==1)||now-contentLast>=std::chrono::milliseconds(250)){ImportProgress(done,total,label);contentLast=now;}
        };
        const auto initialContentWarning=ApplyPendingAndroidContent(contentProgress,[]{return importCancelled.load();});
        if(importCancelled.load())throw install::Error("Installation cancelled; added files kept",true);
        if(!initialContentWarning.empty())Dialog("Some added files could not be installed. They are kept for retry.\n\n"+initialContentWarning);
        if(VerifiedInstalledDisc(root))LOG_INFO("Disc 1 full installation: REUSED (executable fingerprint and disc index/files checked)");
        else {
            if(std::filesystem::exists(root))throw install::Error("Existing Disc 1 installation failed validation; existing files were kept");
            const int fd=ChooseIso();
            if(fd<0)throw install::Error("No Disc 1 ISO selected",true);
            struct Owned {int fd;~Owned(){close(fd);}} owned{fd};
            auto last=std::chrono::steady_clock::time_point{};
            uint64_t logged=0;
            const auto result=install::InstallAndroidDisc1(fd,destination,[&](uint64_t done,uint64_t total,std::string_view label){
                const auto now=std::chrono::steady_clock::now();
                if(done==total||now-last>=std::chrono::milliseconds(250)) {
                    ImportProgress(done,total,label);last=now;
                }
                if(total&&done>=logged+256ULL*1024*1024) {
                    LOG_INFO("Disc 1 import progress: {} / {} bytes file={}",done,total,label);logged=done;
                }
            },[]{return importCancelled.load();});
            ImportProgress(0,0,"");
            if(result.cancelled)throw install::Error("Import cancelled; source ISO kept",true);
            if(!result.error.empty())throw install::Error(result.error);
            if(!VerifiedInstalledDisc(root))throw install::Error("Imported disc failed executable/index validation");
            importedContent=true;
            LOG_INFO("Disc 1 full installation: PASS (all files copied; executable and index validated)");
        }
        const auto contentWarning=ApplyPendingAndroidContent(contentProgress,[]{return importCancelled.load();});
        if(importCancelled.load())throw install::Error("Installation cancelled; added files kept",true);
        if(!contentWarning.empty())Dialog("A queued content import failed and was retained for retry. Existing content was kept. You can discard pending imports in Options > Discs & DLC.\n\n"+contentWarning);
        if(!VerifiedInstalledDisc(root))throw install::Error("Installed Disc 1 failed validation after pending import");
        if(importedContent&&initialContentWarning.empty()&&contentWarning.empty()){
            auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());auto activity=env?static_cast<jobject>(SDL_AndroidGetActivity()):nullptr;
            if(activity){auto type=env->GetObjectClass(activity);auto method=type?env->GetMethodID(type,"importCompleted","()V"):nullptr;if(method)env->CallVoidMethod(activity,method);if(env->ExceptionCheck()){env->ExceptionDescribe();env->ExceptionClear();}if(type)env->DeleteLocalRef(type);env->DeleteLocalRef(activity);}
        }
        ImportProgress(0,0,"");
        settings::ConfigureGameLanguages(root/"default.xex");
        if(importCancelled.load())throw install::Error("Test closed before boot",true);
        gpu::taa_collection::Initialize();
        g_userHeap.Init();g_pageAllocator.Init();FileSystem::Init(root);XamInit();
        const auto entry=XexLoader::Load(root/"default.xex");
        if(entry!=0x827ca440||XexLoader::s_imageBase!=0x82000000||XexLoader::s_imageSize!=20709376)
            throw install::Error("Unexpected loaded executable layout");
        LOG_INFO("First boot executable setup: PASS entry={:#x}",entry);
        LOG_INFO("Disc installation and executable setup passed; proceeding with chosen preparation policy");
        if(importCancelled.load())throw install::Error("Test closed before boot",true);
        hid::touch::Set({});SDL_DestroyWindow(window);window=nullptr;
        ApplyAndroidShaderPreparation(true);
        setenv("LO_ANDROID_REQUIRE_SHADER_PREPARE","1",1);
        auto preparedConfig=settings::GetConfig();preparedConfig.skipShaderPrebuild=false;settings::PreviewConfig(preparedConfig);
        setenv("LO_SHADER_WORKERS","2",1);setenv("LO_PIPELINE_WORKERS","2",1);
        setenv("LO_ANDROID_STALL_TRACE","1",1);
        LOG_INFO("Android prelaunch policy: resource shaders and learned pipelines enabled; 2 compilation workers; gameplay timer waits for guest startup");
        bootActive.store(true);
        std::thread([]{
            uint64_t preparationSeconds=0;
            while(!guestStarted.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if(++preparationSeconds%10==0) {
                    const auto shaders=xenos::GetDxcStatistics();
                    LOG_INFO("Android preparation heartbeat: elapsed={}s DXC calls={} succeeded={} rejected={} infrastructureFailed={} lastFile={}",
                        preparationSeconds,shaders.calls,shaders.succeeded,shaders.rejected,shaders.infrastructureFailed,FileSystem::LastOpenedFile());
                }
            }
            LOG_INFO("Android preparation finished: guest startup reached after approximately {}s; starting gameplay observation",preparationSeconds);
            auto previous=gpu::video::AndroidPerformanceSnapshot();
            uint64_t previousPresents=gpu::video::AndroidGuestPresentCount();
            auto previousTime=std::chrono::steady_clock::now();
            const auto sessionStart=std::chrono::steady_clock::now();
            while(!reportRequested.load(std::memory_order_acquire)) {
                for(int tick=0;tick<10&&!reportRequested.load(std::memory_order_acquire);++tick)
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                const int elapsed=int(std::chrono::duration<double>(std::chrono::steady_clock::now()-sessionStart).count());
                LOG_INFO("Android boot heartbeat: elapsed={}s guestEntryStarted={} guestPresents={}",elapsed,guestStarted.load(),gpu::video::AndroidGuestPresentCount());
                const auto now=std::chrono::steady_clock::now();
                const auto metrics=gpu::video::AndroidPerformanceSnapshot();
                const auto presents=gpu::video::AndroidGuestPresentCount();
                const double seconds=std::chrono::duration<double>(now-previousTime).count();
                const auto calls=metrics.frontbufferCalls-previous.frontbufferCalls;
                LOG_INFO("Android interval performance: presentsPerSecond={:.2f} frontbufferCalls={} meanHostPresentMs={:.2f} swapchainResizes={} (host path including waits; not GPU timing)",
                    double(presents-previousPresents)/seconds,calls,calls?double(metrics.hostMicroseconds-previous.hostMicroseconds)/calls/1000.0:0.0,
                    metrics.swapchainResizes-previous.swapchainResizes);
                LOG_INFO("Android interval frame gaps: atLeast50Ms={} atLeast100Ms={} atLeast250Ms={} (overlapping counts between frontbuffer calls; not display/GPU frame timing)",
                    metrics.gaps50-previous.gaps50, metrics.gaps100-previous.gaps100, metrics.gaps250-previous.gaps250);
                previous=metrics;previousPresents=presents;previousTime=now;
                LOG_INFO("Android interval environment: {}",PerformanceEnvironment());
                const auto ff=debug_menu::fast_forward::GetStatus();
                LOG_INFO("Android interval fast-forward: enabled={} active={} multiplier={} mode={} (shared clock state; no pacing policy change)",ff.enabled,ff.active,ff.multiplier,ff.mode==debug_menu::fast_forward::Mode::Hold?"Hold":"Toggle");
                const auto shaders=xenos::GetDxcStatistics();
                LOG_INFO("Android boot work: DXC calls={} succeeded={} rejected={} infrastructureFailed={} lastFile={}",
                    shaders.calls,shaders.succeeded,shaders.rejected,shaders.infrastructureFailed,FileSystem::LastOpenedFile());
            }
            const auto presents=gpu::video::AndroidGuestPresentCount();
            LOG_INFO("Android user-finished observation: elapsedSeconds={} guestEntryStarted={} successfulGuestPresents={} (gameplay and frame content require visual confirmation)",int(std::chrono::duration<double>(std::chrono::steady_clock::now()-sessionStart).count()),guestStarted.load(),presents);
            host_ui::SetGamePaused(true);
            std::fflush(nullptr);
            Dialog(fmt::format("Test finished at your request.\n\nGuest entry started: {}\nSuccessful guest presentations: {}\n\nShare log and describe what appeared on screen. Presentation counts alone do not confirm useful graphics or gameplay. Continue closes this test.",guestStarted.load(),presents));
            os::shaderlog::CloseForExit();std::fflush(nullptr);std::_Exit(0);
        }).detach();
        return RunAndroidGuestBoot(entry);
    }catch(const std::exception& error) {
        ImportProgress(0,0,"");LOG_ERROR("Android first boot preparation failed: {}",error.what());
        Dialog(std::string("Boot preparation stopped: ")+error.what()+"\n\nShare log. Your source ISO was kept.");
        // The Activity has its own :bootcheck process. Finish it after the
        // error dialog closes rather than reusing partially initialized SDL
        // and guest state on the next launch. The log provider stays separate.
        os::shaderlog::CloseForExit();std::fflush(nullptr);std::_Exit(1);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_freefrank_lostodyssey_RuntimeReadinessActivity_nativeBorderColour(JNIEnv*,jclass,jint value){gpu::android_border::colour.store(std::clamp(int(value),0,2),std::memory_order_relaxed);}
