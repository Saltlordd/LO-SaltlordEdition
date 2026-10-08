#pragma once
#include "config.h"
namespace settings {
// Test-session policy. Apply once before any renderer/guest threads start;
// live menu edits remain possible and saved preferences are not overwritten.
inline Config AndroidLowGraphicsBaseline(Config value) {
    value.width = 1280; value.height = 720;
    value.internalResolution = 720;
    value.antialiasing = 0; value.fxaa = false;
    value.anisotropicFiltering = 0;
    value.scalingQuality = 0;
    value.upscaler = gpu::upscaling::Upscaler::Off;
    value.fsrSharpnessPercent = 0;
    value.frameGenerationProvider = framegen::Provider::Off;
    value.frameGenerationMode = framegen::Mode::Fixed;
    value.frameGenerationTargetFps = 0;
    value.skipShaderPrebuild = false;
    value.frameRate = 30; value.variableRefreshRate = false;
    return value;
}
}
