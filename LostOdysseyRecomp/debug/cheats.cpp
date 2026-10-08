#include <stdafx.h>
#include "cheats.h"
#include "fast_forward.h"
#if defined(__ANDROID__)
#include <hid/touch_state.h>
#include <os/logger.h>
#include <hid/android_overlay_state.h>
extern void PersistAndroidFastForward();
#endif
#include "menu_overlay.h"
#include <kernel/memory.h>
#include <kernel/xex_loader.h>
#include <settings/menu.h>
#include <host_ui/host_ui.h>
#include <SDL.h>
#include <optional>

namespace debug_menu::cheats {
namespace {
constexpr uint32_t EngineSlot = 0x83315FB4, WorldSlot = 0x83318744, SceneSlot = 0x832CB6B4;
constexpr uint32_t RetailEditorFlag = 0x831EAD88; // CT v1.4 retail flag readers
std::optional<uint8_t> savedEditorFlag;
uint64_t nextSnapshot = 0;
bool AllocationContains(uint32_t p, uint32_t length) {
    if (p < 0x100000 || (p & 3) || p >= 0x7C000000 || length > 0x7C000000-p) return false;
    uint32_t allocation = 0, size = 0;
    return g_pageAllocator.FindAllocation(g_pageAllocator.virtualRegion, p, allocation, size) &&
        p >= allocation && p-allocation <= size && length <= size-(p-allocation);
}
bool ImageContains(uint32_t p, uint32_t length) {
    return XexLoader::s_imageBase && p >= XexLoader::s_imageBase &&
        uint64_t(p)+length <= uint64_t(XexLoader::s_imageBase)+XexLoader::s_imageSize;
}
uint32_t PlayData(PPCContext& ctx, uint8_t* base, uint32_t engine) {
    if (!AllocationContains(engine, 4)) return 0;
    const auto vtable = PPC_LOAD_U32(engine);
    if (!ImageContains(vtable, 0x164)) return 0;
    const auto getter = PPC_LOAD_U32(vtable+0x160) & ~3u;
    if (getter < PPC_CODE_BASE ||
        uint64_t(getter) >= uint64_t(PPC_CODE_BASE)+PPC_CODE_SIZE || !ImageContains(getter, 4)) return 0;
    auto* function = g_memory.FindFunction(getter);
    if (!function) return 0;
    // The unique CT Gold AOB in the published v0.6.15 executable resolves
    // GEngine -> vtable+0x160 -> returned PlayData -> Gold at +0x4C.
    // Call that guest getter rather than relying on a battle-core pointer.
    struct Restore { PPCContext& target; PPCContext saved; ~Restore() { target = saved; } } restore{ctx, ctx};
    ctx.r3.u64 = engine;
    ctx.ctr.u64 = getter;
    function(ctx, base);
    return ctx.r3.u32;
}
void ApplyEditor(uint8_t* base, bool enabled) {
    if (!ImageContains(RetailEditorFlag, 1)) {
        savedEditorFlag.reset(); session.EditorApplied(false); return;
    }
    auto& flag = base[RetailEditorFlag];
    if (enabled) {
        if (!savedEditorFlag) savedEditorFlag = flag;
        flag = 0;
    } else if (savedEditorFlag) {
        if (flag == 0) flag = *savedEditorFlag; // do not overwrite a new game-authored value
        savedEditorFlag.reset();
    }
    session.EditorApplied(enabled);
}
}
void Tick(PPCContext& ctx, uint8_t* base) {
    const uint64_t now = host_ui::GetActiveGameTimeMs();
    const auto previous = session.Get();
    if (now < nextSnapshot && !session.HasPending() && !previous.editorRequested && !savedEditorFlag) return;
    nextSnapshot = now + 250;
    Context context;
    Memory memory{{}};
    if (!base || !ImageContains(EngineSlot, 4) || !ImageContains(WorldSlot, 4) || !ImageContains(SceneSlot, 4)) {
        session.Tick(context, memory, false);
        if (base) ApplyEditor(base, false);
        return;
    }
    const auto world = PPC_LOAD_U32(WorldSlot), scene = PPC_LOAD_U32(SceneSlot);
    // Same observed free-roaming gate as teleport.cpp. CT HP/MP are explicitly
    // OUT-OF-BATTLE fields; do not advertise them as battle invulnerability.
    const bool field = scene == 0 || scene == 10;
    if (field && AllocationContains(world, 0x54)) {
        const auto level = PPC_LOAD_U32(world+0x50);
        if (AllocationContains(level, 4)) {
            const auto playData = PlayData(ctx, base, PPC_LOAD_U32(EngineSlot));
            // CT LO_BASE = Gold address - 0x44 = returned PlayData + 8.
            if (AllocationContains(playData, data::DataSize+8)) {
                context = {playData+8, world, level};
                memory.bytes = {base+context.root, data::DataSize};
            }
        }
    }
    session.Tick(context, memory, field);
    const auto current = session.Get();
    ApplyEditor(base, current.enabled && current.available && current.editorRequested);
}
void PollHostControls() {
    // Called by debug_menu::Update on the existing SDL window/event thread.
    // Borrow HID's already-open controllers; never open, close or own devices.
    static uint32_t gameWindow = 0;
    auto* focus = SDL_GetKeyboardFocus();
    if (gameWindow && !SDL_GetWindowFromID(gameWindow)) gameWindow = 0;
    if (!gameWindow && focus) gameWindow = SDL_GetWindowID(focus);
    const auto edit = session.Get();
    const bool hostAllowed = focus && SDL_GetWindowID(focus) == gameWindow &&
        !(SDL_GetWindowFlags(focus) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN)) &&
        !std::getenv("LO_BACKGROUND") && !host_ui::IsStopping() &&
        !host_ui::IsGamePaused() && !IsOverlayVisible() && !settings::IsOpen() &&
        !edit.editorRequested && !edit.editorApplied;
#if defined(__ANDROID__)
    hid::android_overlay::hostBlocked.store(!hostAllowed,std::memory_order_release);
    const bool allowed=hostAllowed&&!hid::android_overlay::appMenu.load(std::memory_order_acquire);
#else
    const bool allowed=hostAllowed;
#endif
    uint8_t lt = 0, rt = 0;
    bool connected = false;
    uint64_t devices = 14695981039346656037ull;
    static uint64_t previousDevices = 0;
    if (allowed) {
        for (int i=0; i<SDL_NumJoysticks(); ++i) {
            const auto instance = SDL_JoystickGetDeviceInstanceID(i);
            auto* controller = SDL_GameControllerFromInstanceID(instance);
            if (!controller || !SDL_GameControllerGetAttached(controller)) continue;
            connected = true;
            devices = (devices ^ uint32_t(instance)) * 1099511628211ull;
            auto trigger = [&](SDL_GameControllerAxis axis) {
                return uint8_t(std::max(0, int(SDL_GameControllerGetAxis(controller, axis))) >> 7);
            };
            lt = std::max(lt, trigger(SDL_CONTROLLER_AXIS_TRIGGERLEFT));
            rt = std::max(rt, trigger(SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
        }
    }
    #if defined(__ANDROID__)
    if(allowed){const auto touch=hid::touch::Read();lt=std::max(lt,touch.lt);rt=std::max(rt,touch.rt);
        connected=true; // The touch controller remains present even while triggers are released.
        devices=(devices^0x4c4f544f554348ull)*1099511628211ull;}
#endif
    // A newly connected controller must not inherit a prior device's armed LT.
    if (devices != previousDevices) { fast_forward::Release(); previousDevices = devices; }
    fast_forward::Sample(lt, rt, allowed && connected);
#if defined(__ANDROID__)
    // Log changes, not every input/frame. Heartbeats independently record state.
    static int lastSignature=-1;static unsigned reports=0;
    const auto status=fast_forward::GetStatus();
    // Reuse the existing input observation; no new timer/thread or per-frame disk writes.
    static int savedConfiguration=-1;
    const int configuration=(status.enabled?1:0)|(status.mode==fast_forward::Mode::Toggle?2:0)|(status.multiplier<<2);
    if(configuration!=savedConfiguration&&!host_ui::IsStopping()){
        savedConfiguration=configuration;PersistAndroidFastForward();
    }
    const int signature=(status.enabled?1:0)|(status.active?2:0)|(status.mode==fast_forward::Mode::Toggle?4:0)|(status.multiplier<<3);
    if(signature!=lastSignature){
        lastSignature=signature;
        if(reports++<128)LOG_INFO("Android fast-forward transition: enabled={} active={} multiplier={} mode={} allowed={} lt={} rt={}",status.enabled,status.active,status.multiplier,status.mode==fast_forward::Mode::Hold?"Hold":"Toggle",allowed,lt,rt);
    }
#endif
}
} // namespace debug_menu::cheats
