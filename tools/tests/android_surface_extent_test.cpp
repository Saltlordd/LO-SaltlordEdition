#include "plume_surface_extent.h"
#include <cassert>
#include <cstdio>
int main() {
 VkSurfaceCapabilitiesKHR c{};c.currentExtent={2504,2256};c.minImageExtent={1,1};c.maxImageExtent={4096,4096};
 auto e=plume::ResolveSurfaceExtent({1536,1384},c);assert(e.width==2504&&e.height==2256);
 e=plume::ResolveSurfaceExtent({2504,2256},c);assert(e.width==2504&&e.height==2256);
 c.currentExtent={2256,2504};e=plume::ResolveSurfaceExtent({2504,2256},c);assert(e.width==2256&&e.height==2504);
 c.currentExtent={UINT32_MAX,UINT32_MAX};c.minImageExtent={100,200};c.maxImageExtent={2000,3000};
 e=plume::ResolveSurfaceExtent({50,4000},c);assert(e.width==100&&e.height==3000);
 e=plume::ResolveSurfaceExtent({1500,2000},c);assert(e.width==1500&&e.height==2000);
 c.currentExtent={0,0};e=plume::ResolveSurfaceExtent({2504,2256},c);assert(e.width==0&&e.height==0);
 std::puts("PASS: fixed surface extent, orientation change, variable clamp, stable extent, zero surface");
}
