#include <gpu/aspect_layout.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace
{
int checks = 0;
void Require(bool ok, const char* message)
{
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
bool Near(double a, double b, double tolerance = 0.0001)
{
    return std::abs(a - b) <= tolerance;
}
}

int main()
{
    using namespace gpu::aspect_layout;
    for (float aspect : {0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(),
                         std::numeric_limits<float>::infinity()})
        Require(ForAspect(aspect).IsIdentity(), "invalid aspect preserves native layout");
    Require(ForAspect(NativeAspect).IsIdentity(), "16:9 is an identity transform");

    struct Output { float width, height, x, y; };
    for (const auto output : std::array<Output, 5>{{
        {1600, 1200, 1, 0.75f}, {1800, 1200, 1, 0.84375f},
        {1920, 1200, 1, 0.9f}, {1920, 1080, 1, 1}, {2560, 1080, 0.75f, 1}
    }})
    {
        const auto scale = ForAspect(output.width / output.height);
        Require(Near(scale.x, output.x) && Near(scale.y, output.y), "projection expansion matches each output aspect");
        // The native perspective uses fx=fy/(16:9). Expanded X/Y columns must
        // describe the actual raster aspect and never crop either native FOV.
        const double fx = scale.x / NativeAspect, fy = scale.y;
        Require(Near(fy / fx, output.width / output.height), "expanded perspective matches output aspect");
        Require(scale.x <= 1 && scale.y <= 1, "scene view expands without cropping native framing");

        const auto left = FitBoundary(0, NativeWidth, scale.x);
        const auto right = FitBoundary(NativeWidth, NativeWidth, scale.x);
        const auto top = FitBoundary(0, NativeHeight, scale.y);
        const auto bottom = FitBoundary(NativeHeight, NativeHeight, scale.y);
        Require(Near(left + right, NativeWidth) && Near(top + bottom, NativeHeight), "UI and movie safe area is centred");
        Require(Near((right-left)*output.width/NativeWidth / ((bottom-top)*output.height/NativeHeight), NativeAspect),
                "UI and movie retain 16:9 on the physical raster");

        double coveredArea = 0;
        unsigned bars = 0;
        for (const auto& rect : MenuBars(scale))
        {
            if (rect.width <= 0 || rect.height <= 0) continue;
            ++bars;
            const double l = FitBoundary(rect.x, NativeWidth, scale.x);
            const double r = FitBoundary(rect.x + rect.width, NativeWidth, scale.x);
            const double t = FitBoundary(rect.y, NativeHeight, scale.y);
            const double b = FitBoundary(rect.y + rect.height, NativeHeight, scale.y);
            Require(l >= -0.0001 && t >= -0.0001 && r <= NativeWidth + 0.0001 && b <= NativeHeight + 0.0001,
                    "native menu quads fit inside the physical output");
            Require(r <= left + 0.0001 || l >= right - 0.0001 || b <= top + 0.0001 || t >= bottom - 0.0001,
                    "menu bars do not overlap 16:9 menu content");
            coveredArea += (r-l) * (b-t);
        }
        Require(bars == (scale.IsIdentity() ? 0u : 2u), "only the expanded output axis receives menu bars");
        Require(Near(coveredArea, NativeWidth*NativeHeight - (right-left)*(bottom-top), 0.15),
                "menu quads cover the complete output margin on tall and wide screens");

        const double physicalX = output.width / NativeWidth;
        const double physicalY = output.height / NativeHeight;
        Require(Near(40 * scale.x * physicalX, 40 * scale.y * physicalY), "square HUD geometry remains square");
        for (const auto disabled : std::array<std::array<uint32_t, 4>, 3>{{
            {0, 0, 0, 0}, {17, 20, 17, 300}, {12, 42, 800, 42}
        }})
        {
            Require(FitScissor(disabled, scale) == disabled, "centred fit preserves disabled guest scissor");
            Require(FitScissor(disabled, scale, 0) == disabled, "top anchor preserves disabled guest scissor");
        }
        if (scale.y < 1)
        {
            const auto clip = FitScissor({972, 40, 1192, 205}, scale, 0);
            Require(Near(FitBoundary(40, NativeHeight, scale.y, 0) * physicalY, 40 * physicalX),
                    "minimap keeps its native top margin at physical UI scale");
            Require(clip[0] == 972 && clip[2] == 1192, "tall minimap keeps horizontal anchoring");
            Require(std::abs(double(clip[1]) - 40 * scale.y) <= 0.5 &&
                    std::abs(double(clip[3]) - 205 * scale.y) <= 0.5, "minimap clip follows top-anchored geometry");
            for (const float pixel : {0.0f, 40.0f, 205.0f, 720.0f})
            {
                const float halfPixel = -1.0f / NativeHeight;
                const float ndc = 1.0f - 2.0f * pixel / NativeHeight;
                const float transformed = CanvasComponent(ndc - halfPixel, 1, scale.y, halfPixel, 1 - scale.y);
                const float screenPixel = (1.0f - (transformed + halfPixel)) * NativeHeight * 0.5f;
                Require(Near(screenPixel, FitBoundary(pixel, NativeHeight, scale.y, 0)),
                        "top-anchored matrix and clipping use the same pixel mapping");
            }
        }
        // These native minimap boundaries come from the Uhra capture. The same
        // fit applied to clip edges and transformed corners keeps them aligned.
        for (const auto axis : std::array<std::array<float, 4>, 2>{{
            {972, 1192, NativeWidth, scale.x}, {40, 205, NativeHeight, scale.y}
        }})
        {
            const double start = FitBoundary(axis[0], axis[2], axis[3]);
            const double end = FitBoundary(axis[1], axis[2], axis[3]);
            Require(Near(end-start, (axis[1]-axis[0])*axis[3]), "minimap clip retains the transformed content extent");
            Require(std::abs(std::lround(start)-start) <= 0.5 && std::abs(std::lround(end)-end) <= 0.5,
                    "integer scissor boundaries round within half a guest pixel");
        }

        // A composed canvas may include rotation, translation and projective W.
        // Test a mixed column against screen-space scaling after the half-pixel
        // correction, independently of any particular matrix storage spelling.
        const std::array<float, 4> value{0.003f, -0.007f, 0.002f, 0.87f};
        const std::array<float, 4> homogeneous{0.004f, 0.001f, -0.003f, 1.0f};
        const std::array<float, 4> vertex{17, -31, 9, 1};
        for (unsigned axis = 0; axis < 2; ++axis)
        {
            const float k = axis ? scale.y : scale.x;
            const float halfPixel = axis ? -1.0f/NativeHeight : 1.0f/NativeWidth;
            double before = 0, after = 0, w = 0;
            for (unsigned row = 0; row < 4; ++row)
            {
                before += value[row] * vertex[row];
                after += CanvasComponent(value[row], homogeneous[row], k, halfPixel) * vertex[row];
                w += homogeneous[row] * vertex[row];
            }
            Require(Near(after + w * halfPixel, k * (before + w * halfPixel), 0.000001),
                    "canvas fit preserves the guest half-pixel centre through composed transforms");
        }
    }
    const auto tall = ForAspect(4.0f/3.0f);
    Require(Near(FitBoundary(0, NativeHeight, tall.y), 90) &&
            Near(FitBoundary(NativeHeight, NativeHeight, tall.y), 630), "4:3 movie bars cover the expected 90 guest pixels on each side");
    std::printf("aspect layout: %d checks passed\n", checks);
}
