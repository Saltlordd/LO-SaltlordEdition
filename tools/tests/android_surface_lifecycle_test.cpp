#include <gpu/android_surface.h>
#include <cassert>
#include <thread>
#include <vector>
int main(){using namespace gpu::android_surface;assert(Read()==0);assert(Signal(false)==0);assert(Signal(true)==3);assert(Signal(true)==3);assert(Signal(false)==2);assert(Signal(true)==5);std::vector<std::thread> threads;for(int i=0;i<8;i++)threads.emplace_back([]{for(int n=0;n<10000;n++)Signal(true);});for(auto& t:threads)t.join();assert(Read()==5);assert(Signal(false)==4);assert(Signal(true)==7);}
