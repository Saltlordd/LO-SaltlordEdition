#pragma once
#include <cmath>
#include <cstdint>
#include <optional>
namespace gpu::metalfx_fg {
struct Parameters {
    float deltaSeconds, verticalFovDegrees, motionScaleX, motionScaleY;
};
// Apple takes seconds and degrees, and vectors pointing into prevColorTexture
// in output pixels. The renderer supplies milliseconds, radians, input pixels.
inline std::optional<Parameters> ConvertParameters(float milliseconds, float radians,
    uint32_t inputWidth, uint32_t inputHeight, uint32_t outputWidth, uint32_t outputHeight) {
    if (!std::isfinite(milliseconds) || milliseconds <= 0 || !std::isfinite(radians) ||
        radians <= 0 || radians >= 3.14159265358979323846f ||
        !inputWidth || !inputHeight || !outputWidth || !outputHeight) return {};
    return Parameters{milliseconds * 0.001f, radians * (180.0f / 3.14159265358979323846f),
        float(outputWidth)/inputWidth, float(outputHeight)/inputHeight};
}
} // namespace gpu::metalfx_fg
