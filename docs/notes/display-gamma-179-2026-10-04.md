# Display gamma ramp (#78, #179) — 2026-10-04

#78 (v0.7.10) reported a white veil and raised blacks compared with Xenia Canary. The answer then was the Expanded RGB range option (`expand_rgb_range`), which stretches 16–235 to 0–255 at presentation. #179 (v0.8.0, same reporter, same RX 6600) then reported blown-out clouds and lost highlight detail; with that option on, everything above 235 clips to white.

## Cause

- `VdGetCurrentDisplayGamma` returned type 1 (sRGB). Xenia's default is type 2 (BT.709, "looks the closest to the Xbox 360 connected to an HDTV"). D3D's SetGammaRamp adjusts the ramp it writes to the `DC_LUT_*` registers for that type.
- A Mac run with the registers logged: with type 1 the game writes an identity 256-entry ramp once at start; with type 2 it writes a non-identity one (16→6.0, 54→38.1, 128→115.2, 235→232.3, 255→255). That table is the sRGB decode followed by the BT.709 encode.
- The renderer never applied `DC_LUT_*` writes. Xenia does, at every swap.
- The #78 screenshots fit: Xenia's luminance quantiles equal this ramp applied to ours in the darks and midtones (ours 16/21/27/40/54/75/100 → Xenia 5/8/12/22/36/59/87; the ramp gives 6.0/8.5/12.5/24.2/38.1/59.3/85.5). The top 0.1 % differs, from JPEG and few pixels.

## Change

- `VdGetCurrentDisplayGamma` returns type 2.
- `gpu/display_gamma.h` decodes the 256-entry `DC_LUT_*` protocol like Xenia (`DC_LUT_RW_INDEX`, `DC_LUT_30_COLOR` and `DC_LUT_SEQ_COLOR` with the write-enable mask; the piecewise-linear mode is logged and ignored), fed from `CommandProcessor::WriteRegister`. It logs the table once it is complete: `display gamma ramp: identity=... 0->... 255->...`.
- Presentation keeps the table in a 4 KB constant buffer (three, rotated when the guest writes a new one) and applies it at the final pass to guest frames, before Expanded RGB range and before the HDR output conversion. Extended-gamma values above 1 keep their excess. The frozen HDR calibration scene gets the same ramp; host menus and status screens do not.
- Expanded RGB range stays as an option and runs after the ramp.

## Validation

- psvita (D3D12 through vkd3d-proton), Uhra save, AA off: the new presentation shader runs; the log shows the ramp above. A screenshot at the same moment as main `3afdc4e` (no ramp) equals that screenshot passed through the ramp per pixel (median error 0.27 levels, 90th percentile 0.75). Luminance quantiles: 1 % 31→16, median 109→95, 99.9 % 244→246; pixels at 255 0.006 %→0.008 %.
- Not checked: a real Xbox 360; the game's own brightness options, if it changes the ramp at runtime.
