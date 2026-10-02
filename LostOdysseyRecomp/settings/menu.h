#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace settings
{
struct HdrDisplayInfo
{
    bool active = false;
    uint32_t peakNits = 0; // Zero means the platform has no usable peak report.
    bool relative = false; // EDR headroom estimate, not measured panel nits.
};
struct HdrCalibration
{
    bool open = false;
    bool automatic = true;
    bool detectedValid = false;
    bool relative = false;
    bool hdrActive = false;
    bool sceneAvailable = false;
    bool scenePreview = true;
    bool numericEditing = false;
    uint32_t manualNits = 1000;
    uint32_t detectedNits = 0;
    uint32_t effectiveNits = 1000;
    uint32_t paperWhiteNits = 203;
    int focus = 0;
    std::wstring numericText;
    bool operator==(const HdrCalibration &) const = default;
};
void SetHdrDisplayInfo(HdrDisplayInfo info);
// Called by presentation when its owned, frozen HDR scene becomes available.
void SetHdrCalibrationSceneAvailable(bool available);
HdrCalibration GetHdrCalibration();
// ASCII digits, Backspace (8), Enter (13), and Escape (27). Returns true when
// the calibration page consumes this host key before game input mapping.
bool CalibrationKey(uint32_t key);
void PointerDrag(float x, float y, bool held);
// Game tab actions follow the seven adjustable retail settings.
inline constexpr int GameRestoreRow = 7;
inline constexpr int GameMainMenuRow = 8;
inline constexpr int GameImportRow = 9;
// Logical ids for the graphics tab. MenuSnapshot::row stores these as int.
// Count is the tab length, not the on-screen viewport.
enum class GraphicsRow : int
{
    Backend = 0,
    DisplayMode = 1,
    Widescreen = 2,
    OutputResolution = 3,
    RenderResolution = 4,
    AntiAliasing = 5,
    DlssQuality = 6,
    FsrSharpness = 7,
    AnisotropicFiltering = 8,
    ScalingQuality = 9,
    RgbRange = 10,
    FrameRate = 11,
    FrameGeneration = 12,
    FrameGenerationMultiplier = 13,
    VariableRefreshRate = 14,
    Hdr = 15,
    HdrPaperWhite = 16,
    HdrPeak = 17,
    Brightness = 18,
    Save = 19,
    Count = 20,
};
inline constexpr int MenuTabCount = 4;
inline constexpr int MenuTabWidth = 640 / MenuTabCount;
// Called by input polling before returning the guest-facing controller state.
bool FilterInput(uint16_t &buttons, int16_t leftX, int16_t leftY);
// Snapshot rendered on the presentation thread, never accessing guest memory.
bool DrawMenu(std::vector<uint32_t> &pixels, uint64_t &revision, uint32_t width = 1280, uint32_t height = 720);
void PointerClick(float x, float y, bool reverse);
bool IsOpen();
} // namespace settings
