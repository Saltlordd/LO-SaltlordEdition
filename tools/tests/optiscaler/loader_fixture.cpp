// Loader fixture only. This DLL implements no OptiScaler/NGX/GPU functionality.
#include <windows.h>
static unsigned attachments = 0;
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) ++attachments;
    return TRUE;
}
extern "C" __declspec(dllexport) unsigned LoOptiscalerLoaderFixtureAttachments() {
    return attachments;
}
