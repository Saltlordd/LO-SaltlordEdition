#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <mutex>
namespace hid::touch {
struct State { uint16_t buttons=0; uint8_t lt=0,rt=0; int16_t lx=0,ly=0,rx=0,ry=0; };
inline std::mutex mutex;
inline State state;
inline void Set(State value) { std::lock_guard lock(mutex); state=value; }
inline State Read() { std::lock_guard lock(mutex); return state; }
inline int16_t Stronger(int16_t a,int16_t b) { return std::abs(int(b))>std::abs(int(a))?b:a; }
template<class Pad> void Merge(Pad& pad, const State& s) {
 pad.wButtons |= s.buttons;
 pad.bLeftTrigger=std::max(uint8_t(pad.bLeftTrigger),s.lt);
 pad.bRightTrigger=std::max(uint8_t(pad.bRightTrigger),s.rt);
 pad.sThumbLX=Stronger(pad.sThumbLX,s.lx); pad.sThumbLY=Stronger(pad.sThumbLY,s.ly);
 pad.sThumbRX=Stronger(pad.sThumbRX,s.rx); pad.sThumbRY=Stronger(pad.sThumbRY,s.ry);
}
}
