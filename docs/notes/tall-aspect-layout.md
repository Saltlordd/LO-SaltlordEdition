# Tall and non-standard aspect layout — 2026-10-01

> **Development checkpoint.** The implementation is part of v0.7.35 (tag
> `v0.7.35`, 2026-10-02). This note records bounded validation and does not
> claim player acceptance.

## Intended behavior

For displays taller than 16:9, including 4:3 and 3:2, the game renders the 3D
scene across the full drawable area and expands the vertical field of view. The
interface keeps the game's 16:9 layout inside a centered, uniformly scaled
canvas, with native menu quads providing bars above and below. The minimap alone
uses a top anchor on taller outputs; the other HUD elements remain in the
centered 16:9 layout. On ultrawide outputs, the same menu canvas produces bars
at the sides.

Movies use a two-axis fit and retain bars where the source aspect requires them.
Scene projection uses the `hor_plus` / `aspect_layout` path: the horizontal field
of view remains the 16:9 reference while the taller drawable expands the vertical
view.

## Implementation checkpoint

The render-resolution and upscaling paths now use the full drawable dimensions.
The scene projection applies the aspect-layout policy on both axes. Canvas
composition preserves a centered 16:9 interface, and movie composition uses the
two-axis fit policy with bars.

The minimap uses the dedicated tall-output top anchor, while the other HUD
elements remain centered with the 16:9 canvas. The full field menu uses native
quads for aspect-specific bars. The behavior is part of v0.7.35 and is not
user accepted.

## Validation recorded so far

- The final local build passed (`exit 0`); receipt:
  `out/build-tall-aspect-final-retry.log`. The checked executable SHA-256 is
  `A57C952F9CB12AE7DB9FBA81701E51169BF0A1CF3A2906E888AE61B8D7ACB4E4`, built
  from base `95854463134ee13b6c3aa1dbf10c651d669c765e`.
- Final CPU checks passed for aspect layout and menu bars (143), render dimensions (44), frame
  planning (105), and DLAA (1446).
- With the same Vulkan executable, AA/upscaling/frame generation off and
  on-demand shaders, the Uhra runtime checks passed for both taller ratios:
  - 4:3 at 1280×960: the scene fills the drawable and the minimap sits at
    top offset 40 (`out/tall-aspect-final-20261001/4x3/shots/shot_2846.png`).
    The menu and Status captures show complete content with 120-pixel pure-black
    bars above and below (`shot_3942.png`, `shot_5707.png`).
  - 3:2 at 1440×960: the scene fills the drawable and the minimap sits at
    top offset 45 (`out/tall-aspect-final-20261001/3x2/shots/shot_2548.png`).
    The menu and Status captures show complete content with 75-pixel pure-black
    bars above and below (`shot_3501.png`, `shot_3745.png`). The exit-menu scene
    has no bars on any side and the minimap remains in place (`shot_4970.png`).
- 21:9 at 1720×720: the scene has no bars on any side
  (`out/tall-aspect-final-20261001/21x9/shots/shot_2540.png`). The menu and
  Status captures show complete 1280×720 content with 220-pixel pure-black bars
  on each side and no top or bottom bars (`shot_3483.png`, `shot_3726.png`).
- All three ratios' pixel measurements are recorded in each case's
  `evidence.json` and in `out/tall-aspect-final-20261001/receipt.json`.
  Original save, profile and settings hashes were unchanged. Each owned
  process was stopped deliberately after capture; exit code -1 records that
  controlled stop rather than a crash.

## Review changes — 2026-10-02

The review of PR #146 changed four things before merging.

- **Render size on taller outputs.** The first version fixed the total height,
  so a taller output rendered fewer pixels for the 16:9 area than before (a
  16:10 MacBook at its default went from 1280x720 to about 1107x720). A
  render-resolution mode now sets the 16:9 area instead: wider outputs keep its
  height and widen, taller ones keep its width and grow taller. 720 renders
  1280x800 on 16:10 and 1280x960 on 4:3. Auto follows the output with the 16:9
  area capped at 2160 rows (3840x2400 stays 3840x2400, 5120x3200 renders
  3840x2400), and portrait rasters stop at 4320 rows. Targets whose role is not
  catalogued scale by the plan's 16:9 area. Every exact 16:9 output keeps its
  previous size; a test sweeps them.
- **Menu bars.** The bars call the game's tile drawing. That call ran on a
  2 KiB private stack, which a deeper call chain could overrun; it now uses a
  frame below the live guest stack, after the menu dispatcher has returned.
- **Scissor.** A one-pixel clip could round to an empty rectangle, which the
  game reads as clipping off. A fitted axis now keeps at least one pixel.
- **Resolve readback.** `LO_RESOLVE_READBACK` renders 1280x720 on any output;
  the layout now uses 16:9 for it instead of the drawable's aspect.

Scope: every output taller than 16:9 takes this layout, including 16:10
(Steam Deck, MacBooks), 5:4 and portrait. Frame generation, which stayed off on
these outputs while they had bars, now runs on them; that has not been tested.

Checks after these changes: the aspect-layout, render-resolution, frame-plan,
DLAA and DLSS planner tests pass with GCC and clang under ASan and UBSan. On
Windows Vulkan with the Uhra save and render resolution 720, a 1280x960 window
rendered 1280x960 and a 1280x800 window 1280x800; in both the scene filled the
window and the menu and Status pages had bars of 120 and 40 pixels above and
below.

## Pending validation

- The recorded runtime scope is Uhra ordinary menu and Status on Vulkan. Movies,
  battle UI, title screen, Direct3D 12, Metal, frame generation on taller outputs
  and broader hardware remain untested.
- Draws the camera and Canvas classifiers miss are stretched on taller outputs,
  as they already were horizontally on ultrawide ones; battle overlays placed by
  projecting world points are the likeliest case and have not been checked.
- Windows within about a pixel of 16:9 (for example 1360x768) take the new
  layout and resample the HUD by a fraction of a percent.
- Record player/maintainer acceptance separately; local builds and runtime
  captures do not establish that acceptance.
