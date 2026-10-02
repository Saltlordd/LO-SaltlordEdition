#include <gpu/render_resolution.h>
#include <gpu/frame_plan.h>
#include <gpu/depth_clear_layout.h>
#include <cstdio>
#include <cstdlib>
static unsigned checks=0;
static void Check(bool condition,const char* message) {
    ++checks;
    if(!condition){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
int main() {
    using namespace gpu::resolution;
    using namespace gpu::frame_plan;
    Check(TableSurfaceRole(0xCC)==SurfaceRole::Shadow&&TableSurfaceRole(0xF0)==SurfaceRole::Shadow,"named guest shadow table resources");
    for(auto slot:{0x3Cu,0x60u,0x84u,0xA8u,0x138u,0x15Cu,0x180u,0x1A4u})
        Check(TableSurfaceRole(slot)==SurfaceRole::Scene,"scene table resources retain their role");
    Check(MergeSurfaceRoles(true,false,true,SurfaceRole::Shadow)==SurfaceRole::Unknown,"scene alias revokes prior shadow classification");
    Check(MergeSurfaceRoles(false,true,true,SurfaceRole::Shadow)==SurfaceRole::Unknown,"fixed alias revokes prior shadow classification");
    Check(MergeSurfaceRoles(false,false,false,SurfaceRole::Shadow)==SurfaceRole::Unknown,"removed resource clears role");
    Check(MergeSurfaceRoles(false,false,true,SurfaceRole::Unknown)==SurfaceRole::Shadow,"unambiguous shadow acquires role");
    Check(TargetGuestHeight(TargetRole::Shadow,422)==896&&TargetGuestHeight(TargetRole::Shadow,864)==896,"first atlas tile preallocates both tiles plus alignment");
    Check(TargetGuestHeight(TargetRole::Fixed,422)==448,"unrelated fixed targets keep their height");
    for(uint32_t scale:{1u,2u,4u}) for(Size scene: {Size{1280,720},Size{3840,2160},Size{3440,1440},Size{1280,960},Size{853,480}}) {
        const auto shadow=TargetSizeForPlan(TargetRole::Shadow,880,896,scene,scene,scale);
        Check(ScaleX(864,shadow.width)==864*scale&&Scale(864,shadow.height)==864*scale,"shadow resolve scale is independent of scene/aspect/SR");
        Check(ScaleX(880,shadow.width)==880*scale&&Scale(896,shadow.height)==896*scale,"backing atlas padding uses the same scale");
        Check(TargetSizeForPlan(TargetRole::Fixed,864,864,scene,scene,scale)==Size{},"fixed luminance surfaces do not acquire shadow scale");
        Check(TargetSizeForPlan(TargetRole::Scene,1280,720,scene,scene,scale)==scene,"scene resolution remains independent");
        auto rectangles=gpu::renderer::MapDepthClear(440,2,{240,0,400,216},880,896,0);
        Check(!rectangles.empty(),"partial clear must produce rectangles, never API full-clear sentinel");
        for(const auto& rect:rectangles) {
            Check(ScaleX(rect.left,shadow.width)>=480*scale&&ScaleX(rect.right,shadow.width)<=800*scale,
                  "cross-pitch right-tile clear never touches left tile");
            Check(Scale(rect.bottom,shadow.height)<=432*scale,"cross-pitch clear does not touch atlas bottom");
        }
    }
    ShadowResolutionState state;
    Check(state.BeginFrame(1,1)==1,"default native shadow resolution");
    Check(state.BeginFrame(2,4)==4,"first resolve in next frame snapshots new scale");
    Check(state.BeginFrame(2,2)==4,"later draw in same frame cannot resize earlier resolve");
    state.AllocationFailed();
    Check(state.BeginFrame(2,4)==4,"allocation failure never resizes a partially drawn atlas");
    Check(state.BeginFrame(3,4)==1&&state.BeginFrame(4,4)==1,"allocation failure latches native scale across frames");
    Check(state.BeginFrame(5,2)==2&&!state.Failed(),"explicit setting change retries supported scale");
    Check(state.BeginFrame(6,3)==1,"invalid multiplier normalizes to native");
    std::printf("PASS: %u shadow role, extent, clear and frame-boundary checks\n",checks);
}
