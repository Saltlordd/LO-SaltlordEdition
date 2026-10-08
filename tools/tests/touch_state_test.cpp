#include "hid/touch_state.h"
#include <cassert>
#include <thread>
#include <atomic>
struct Pad { uint16_t wButtons=0; uint8_t bLeftTrigger=0,bRightTrigger=0; int16_t sThumbLX=0,sThumbLY=0,sThumbRX=0,sThumbRY=0; };
int main() {
    Pad physical{0x2000,100,200,20000,-10000,0,25000};
    hid::touch::Set({0x1001,255,0,-30000,5000,-15000,10000});
    hid::touch::Merge(physical,hid::touch::Read());
    assert(physical.wButtons==0x3001&&physical.bLeftTrigger==255&&physical.bRightTrigger==200);
    assert(physical.sThumbLX==-30000&&physical.sThumbLY==-10000&&physical.sThumbRX==-15000&&physical.sThumbRY==25000);
    hid::touch::Set({}); Pad released{};hid::touch::Merge(released,hid::touch::Read());
    assert(released.wButtons==0&&released.sThumbLX==0&&released.bLeftTrigger==0);
    std::atomic<bool> finished=false;
    std::thread writer([&]{for(int i=0;i<100000;i++) {
        int16_t v=int16_t(i%30000);hid::touch::Set({uint16_t(v),0,0,v,v,v,v});
    }finished=true;});
    do {auto s=hid::touch::Read();assert(s.lx==s.ly&&s.ly==s.rx&&s.rx==s.ry&&s.buttons==uint16_t(s.lx));}while(!finished);
    writer.join();hid::touch::Set({});
}
