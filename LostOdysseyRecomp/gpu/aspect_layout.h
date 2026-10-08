#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace gpu::aspect_layout {
struct Scale {
 float x=1.0f,y=1.0f;
 bool IsIdentity() const { return x==1.0f && y==1.0f; }
};
inline Scale ForAspect(float aspect) {
 constexpr float native=16.0f/9.0f;
 if (!std::isfinite(aspect) || aspect<=0) return {};
 return aspect>=native ? Scale{native/aspect,1} : Scale{1,aspect/native};
}
inline float FitBoundary(float value,float extent,float scale) {
 return (1.0f-scale)*extent*0.5f+value*scale;
}
inline float CanvasComponent(float value,float homogeneous,float scale,float halfPixel) {
 return (value-halfPixel*homogeneous)*scale+halfPixel*homogeneous;
}
inline std::array<uint32_t,4> FitScissor(std::array<uint32_t,4> edges,Scale scale) {
 return {uint32_t(std::clamp(std::floor(FitBoundary(float(edges[0]),1280,scale.x)),0.0f,1280.0f)),
         uint32_t(std::clamp(std::floor(FitBoundary(float(edges[1]),720,scale.y)),0.0f,720.0f)),
         uint32_t(std::clamp(std::ceil(FitBoundary(float(edges[2]),1280,scale.x)),0.0f,1280.0f)),
         uint32_t(std::clamp(std::ceil(FitBoundary(float(edges[3]),720,scale.y)),0.0f,720.0f))};
}
}
