#pragma once
#include <host_ui/host_ui.h>
#include <mutex>
namespace android_menu {
// Own only pauses introduced by Android Options; never toggle the guest's Start input.
class PauseLease {
    std::mutex mutex;
    bool open=false,owned=false;
public:
    void Set(bool next,bool gameplayReady,bool otherHostOverlay=false){
        std::lock_guard lock(mutex);
        if(next==open)return;
        open=next;
        if(next){owned=gameplayReady&&!host_ui::IsStopping()&&!host_ui::IsGamePaused();if(owned)host_ui::SetGamePaused(true);}
        else {if(owned&&!otherHostOverlay)host_ui::SetGamePaused(false);owned=false;}
    }
};
}
