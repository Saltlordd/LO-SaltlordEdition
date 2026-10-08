#include <gpu/upscaling_plan.h>
#include <gpu/aspect_layout.h>
#include <cassert>
#include <cstdio>
int main(){
 using gpu::upscaling::ResolveOutputRegion;
 auto r=ResolveOutputRegion({2504,2256});
#ifdef __ANDROID__
 assert(r.width==2504&&r.height==1878&&r.y==189);
#else
 assert(r.width==2504&&r.height==1408&&r.y==424);
#endif
 auto scale=gpu::aspect_layout::ForAspect(2504.0f/2256.0f);
 assert(scale.x==1.0f&&scale.y<1.0f);
 r=ResolveOutputRegion({2520,1080});assert(r.width==2520&&r.height==1080&&r.x==0&&r.y==0);
 r=ResolveOutputRegion({0,1080});assert(r.width==0&&r.height==0);
 std::puts("PASS: tall, wide and empty output region");
}
