# Experimental HDR output

This note documents the unreleased HDR path for the cross-platform renderer. It describes the current implementation and validation boundary; it does not represent a published feature or complete or cross-platform physical-display coverage. The maintainer confirmed on-device HDR validation on 2026-10-02, but did not specify the platform, backend or display.

## Scope and use

HDR is opt-in in the Graphics menu and takes effect after a restart. The current implemented configuration is:

- Windows with Direct3D 12 or Vulkan, Linux with Vulkan, or macOS with Metal. Vulkan selects an exact HDR surface format/color-space pair; an unsupported pair uses SDR.
- Anti-aliasing Off, upscaling Off, frame generation Off and a non-MetalFX spatial filter.
- Paper-white 80–400 nits and manually entered peak from paper-white through 10,000 nits.

Auto peak is the default. It follows the current display's reported peak when available, otherwise uses a 1000-nit content reference. The Linux Vulkan path does not yet obtain a display peak, so Auto currently uses that fallback there. Existing `hdr_peak_nits` profiles without `hdr_peak_auto` retain their manual choice. The **HDR peak brightness** page freezes a deterministic comparison frame and renders it through the actual output transfer: the left view is an SDR brightness preview clipped at reference white, while the right view uses normal HDR tone mapping and updates the peak live. Mouse or controller **X** switches between the Scene and Test pattern; if no valid scene is available, the standard pattern is used. The menu copies its source once when opened rather than every frame. Use the slider, type a numeric value, or restore Auto. Changes preview while the page is open and apply after saving Graphics settings; only the HDR output toggle needs a restart. Unsupported display state or scene input uses the existing SDR path. The pattern cannot demonstrate HDR while output is inactive. See the [Linux/Vulkan implementation note](linux-vulkan-hdr.md) for WSI conditions and pending hardware acceptance. Savestate work is unrelated to this change.

## Output semantics

Windows Direct3D 12 requests an FP16 scRGB swap chain. The renderer treats scRGB 1.0 as the Windows reference of 80 nits and scales the requested content reference white from that value. Peak highlights are compressed to the requested ratio before output.

Metal requests an FP16 Extended Linear sRGB EDR surface. macOS does not expose the system SDR white as an absolute nit value through this path. Auto multiplies the content paper-white reference by available EDR headroom and labels that value an estimate; manual values set a content peak ratio. Neither is a measured absolute panel luminance claim, and actual output depends on system EDR headroom.

Vulkan requests FP16 extended-linear or packed ten-bit HDR10/PQ only when the active surface advertises that exact format and color-space pair. The final PQ pass converts the game's BT.709 working gamut to BT.2020 and encodes ST2084 after scene composition. The swap chain's HDR content transport can be known while the physical monitor state remains unknown; transport alone does not prove that the monitor is in HDR mode. An SDR-only pair stays SDR even when a device exposes `VK_EXT_hdr_metadata` or a ten-bit SDR format.

The SDR screenshot/preview path remains an explicitly gamma-encoded preview of the linear output. It is useful for checking fallback and tone-map continuity, but it is not an HDR display capture.

For the platform API background, see Microsoft's [High Dynamic Range](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range) guidance and Apple's [Performing your own tone mapping](https://developer.apple.com/documentation/metal/performing-your-own-tone-mapping).

## Current validation

The Windows runtime and SDK build links successfully. On an RTX 5080, a hidden-window DXGI test negotiated an active FP16 scRGB HDR swap chain and reported a 1015.27-nit display peak. This confirms HDR capability and color-space negotiation, not visual correctness.

The Windows D3D12 and Vulkan GPU pattern, scene, peak-adjustment and PQ fixtures passed. Windows Vulkan negotiated an HDR10/PQ surface with HDR content transport while the physical monitor state remained unknown; HDR→SDR→HDR swap-chain renegotiation passed. Menu flow, rendering, default SDR SMAA/FXAA presentation and the one-time calibration-page source copy passed targeted checks. These are controlled fixture results, not an HDR image-quality acceptance on a physical display.

An isolated D3D12 Uhra smoke run using the HDR build ran for 120 seconds and exited normally. The run exercised the tone-map sidecar, padded FP16 scene allocation, full-scene copy, HDR resolve and video extended-scene path. At swap 1600, the 1280×720 FP16 swapchain read back a linear maximum of 9.9296875 and reference white of 2.5375 (203/80), with 8350 pixels above reference white. The roughly 794.38-nit scRGB value is a nominal conversion, not a physical measurement. The SDR preview showed the Uhra scene and its UI intact.

The earlier Windows Vulkan Uhra smoke completed 120 seconds with exit 0 and no forced stop. It negotiated HDR10/PQ and reached the extended scene path; swap1600 readback decoded a maximum of 794.5004 content nits with 8618 pixels above the 203-nit reference white. Its SDR preview was intact. This is pipeline evidence, not a panel measurement. The subsequent frozen-scene capture check is recorded separately below.

Offline D3D12 and SPIR-V/Vulkan HDR math, SDR paths and separated-UI lease checks pass. Calibration menu flow, numeric/slider input, configuration migration and software raster controls pass targeted Windows tests. On WSLg 1.0.73 with Mesa Dozen 26.2.2, Plume Vulkan, the surface probe and CPU math compiled. The SDL Wayland surface probe requested HDR, selected BGRA8/sRGB SDR instead, and passed resize. That WSLg surface exposed no HDR presentation transport in this test; these checks do not establish Linux HDR display output.

## Frozen game-scene preview validation — 2026-10-02

The calibration page now prefers an owned FP16 snapshot of a valid HDR game scene at host-menu entry. The left view clips the same source at reference white as an SDR luminance preview; it is not a separate capture of the stock SDR renderer. The right view uses the normal HDR highlight mapping and current peak setting. Scene/Test pattern can be selected with the mouse or controller X. An absent or stale HDR resolve keeps the test pattern available. Gameplay records only source identity and extent; copying happens once at overlay entry, and the owned texture is released after the present fence when gameplay resumes. Independent source storage survives swapchain resizing.

The full Windows runtime build passed. D3D12 and Vulkan fixtures verified test-pattern output, the same frozen scene in both halves, peak adjustment, menu transfer and PQ output. Menu flow/render tests and the default SDR/SMAA/FXAA and production capture regressions passed. The first Windows Vulkan Uhra overlay smoke ran 120 seconds and froze a 1280x720 HDR scene; the follow-up capture ran 95 seconds, exported that scene successfully and exited 0 without a forced stop. Its original extended-gamma maximum was 3.130859375, with 8238 source pixels above 1. The 203/1015-nit preview rendered from that capture had maxima of 202.96875 and 748.75 content nits in the SDR/HDR halves; 1104 paired pixels differed by more than 1 nit. A shared Reinhard curve was used only for the chat's SDR PNG. These are shader/readback values, not measured panel luminance; the maintainer-confirmed on-device validation has no platform/backend/display breakdown yet.

The opt-in `LO_HDR_CALIBRATION_CAPTURE=<path>` diagnostic writes the frozen source when a screenshot is requested. Its local replay format starts with `LOHDR1 <width> <height>\n`, followed by tightly packed RGBA16F extended-gamma pixels. Normal gameplay performs no CPU readback for calibration. Local evidence is under `out/hdr/scene-preview` and `out/hdr/game-smoke-hdr-scene-capture`; captures contain private game imagery and are not packaged or committed.

## Unfinished validation

The Metal HDR source path is present but has not been compiled or run on Mac hardware in this worktree. Linux Wayland/Gamescope HDR and packaged AppImage/Flatpak output have not been run on HDR hardware in the collected evidence. The maintainer confirmed on-device HDR validation on 2026-10-02, but did not specify its platform/backend/display scope. Broader scenes, GPUs, display modes, monitor changes, system HDR toggles, performance and long play remain open validation work.
