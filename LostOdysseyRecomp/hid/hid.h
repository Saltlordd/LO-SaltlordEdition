#pragma once

namespace hid
{
    void Init();
    // All input callers must have stopped. Close handles before SDL_Quit so
    // a later Android activity launch can initialize controllers again.
    void Shutdown();
    void Poll();
    // When another thread owns the SDL event loop (the video thread), it
    // forwards controller hot-plug events here and Poll() stops pumping.
    void SetExternalEventPump(bool external);
    void HandleControllerEvent(uint32_t eventType, int32_t deviceIndexOrInstance);
    void HandleKeyboardEvent(int32_t scancode, bool pressed);
    void ClearKeyboardState();
    void PumpHostInput();
    // Atomic presentation hint for host UI; does not change the guest's buttons.
    bool UsesPlayStationPrompts();

    uint32_t GetState(uint32_t dwUserIndex, XAMINPUT_STATE* pState);
    uint32_t SetState(uint32_t dwUserIndex, XAMINPUT_VIBRATION* pVibration);
    uint32_t GetCapabilities(uint32_t dwUserIndex, XAMINPUT_CAPABILITIES* pCaps);
}
