#include <gpu/upscaling_plan.h>
#include <gpu/render_resolution.h>
#include <gpu/aspect_layout.h>
#include <cassert>
#include <cstdio>
int main() {
 using gpu::resolution::Size;
 using gpu::resolution::ResolveInternalSize;
 using gpu::upscaling::ResolveOutputRegion;
 for (unsigned mode : {720u,1080u,1440u,2160u}) {
  auto inner=ResolveOutputRegion({2504,2256});
  assert(inner.height==1878 && inner.y==189);
  assert((ResolveInternalSize(mode,inner.width,inner.height)==Size{mode*4/3,mode}));
  assert((ResolveInternalSize(mode,2520,1080)==Size{mode*7/3,mode}));
  assert((ResolveInternalSize(mode,1920,1080)==Size{mode*16/9,mode}));
 }
 assert((ResolveInternalSize(720,0,0)==Size{1280,720}));
 assert((ResolveInternalSize(0,2504,1878)==Size{2504,1878}));
 auto s=ResolveInternalSize(0,3840,2160);assert((s==Size{3840,2160}));
 auto p=ResolveOutputRegion({1001,1000});assert(p.height==750&&p.y==125);
 auto axes=gpu::aspect_layout::ForAspect(4.0f/3.0f);
 assert(axes.x==1.0f && std::abs(axes.y-0.75f)<1e-6f);
 auto edges=gpu::aspect_layout::FitScissor({0,0,1280,720},axes);
 assert((edges==std::array<uint32_t,4>{0,90,1280,630}));
 for(int i=0;i<=720;i++) {
  float y=gpu::aspect_layout::FitBoundary(float(i),720,axes.y);
  assert(std::abs(y-(90.0f+i*0.75f))<1e-4f);
 }
 assert(gpu::aspect_layout::ForAspect(0).IsIdentity());
 assert(gpu::aspect_layout::ForAspect(16.0f/9.0f).IsIdentity());
 puts("PASS: Android 4:3 scene, ultrawide, native, empty and rounding");
}
