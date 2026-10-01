#include "gpu/metalfx_fg_policy.h"
#include "gpu/frame_generation_settings.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
namespace {
void Check(bool okay, const char* why) { if (!okay) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); } }
bool Near(float a,float b) { return std::abs(a-b)<1e-5f; }
}
int main() {
    using namespace gpu::metalfx_fg;
    using namespace framegen;
    auto p=ConvertParameters(16,1.57079632679f,1280,720,3840,2160);
    Check(p && Near(p->deltaSeconds,.016f) && Near(p->verticalFovDegrees,90), "SDK seconds/degrees, not milliseconds/radians");
    Check(Near(p->motionScaleX,3) && Near(p->motionScaleY,3), "input-pixel motion addresses output-resolution previous color");
    p=ConvertParameters(33,1,2560,1440,1280,720);
    Check(p && Near(p->motionScaleX,.5f) && Near(p->motionScaleY,.5f), "downscaled presentation motion");
    for (float invalid : {0.0f,-1.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        Check(!ConvertParameters(invalid,1,1,1,1,1), "invalid frame time");
        Check(!ConvertParameters(16,invalid,1,1,1,1), "invalid projection");
    }
    Check(!ConvertParameters(16,4,1,1,1,1) && !ConvertParameters(16,1,0,1,1,1), "invalid FOV/empty input");
    Check(ParseEnvironment("metalfx",nullptr,nullptr,nullptr).Enabled(), "MetalFX fixed 2x default");
    Check(!ParseEnvironment("metalfx","dynamic",nullptr,nullptr).Enabled(), "no dynamic MetalFX");
    Check(!ParseEnvironment("metalfx","fixed","3",nullptr).Enabled(), "no invented MetalFX MFG");
    Check(!ParseEnvironment("off",nullptr,nullptr,nullptr,"1").Enabled(), "explicit Off wins");
    settings::Config saved; saved.graphicsBackend=settings::GraphicsBackend::Metal; saved.frameGenerationProvider=Provider::MetalFx;
    auto selected=gpu::frame_generation::ResolveSelection(saved.graphicsBackend,saved,nullptr,nullptr,nullptr,nullptr,nullptr);
#if defined(__APPLE__) && defined(LO_ENABLE_METALFX_FG)
    Check(selected.Enabled(), "macOS provider exposed with compiled adapter");
#else
    Check(!selected.Enabled(), "other platforms never expose MetalFX FG");
#endif
    Check(selected.config.provider==Provider::MetalFx, "persisted provider survives unsupported builds");
    Check(!gpu::frame_generation::ResolveVulkanSelection(saved,"metalfx",nullptr,nullptr,nullptr,nullptr).Enabled(), "MetalFX is not a Vulkan SDK");
    Check(!gpu::frame_generation::ResolveD3D12Selection(saved,"metalfx",nullptr,nullptr,nullptr,nullptr).Enabled(), "MetalFX is not a D3D12 SDK");
    Check(Select({Provider::MetalFx,Mode::Fixed,1,0},{true,1,false}).Enabled(), "runtime support still required");
    Check(!Select({Provider::MetalFx,Mode::Fixed,1,0},{}).Enabled(), "OS support alone cannot enable GPU feature");
    std::puts("PASS MetalFX parameter/selection contracts");
}
