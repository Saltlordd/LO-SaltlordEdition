# Tall and non-standard aspect layout — 2026-10-01

> **Development checkpoint.** The implementation is present in the unreleased
> source tree. This note records bounded validation and does not claim player
> acceptance or release availability.

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
quads for aspect-specific bars. The source behavior remains unreleased and is
not user accepted.

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

## Pending validation

- The recorded runtime scope is Uhra ordinary menu and Status on Vulkan. Movies,
  battle UI, title screen, Direct3D 12 and broader hardware remain untested.
- Record player/maintainer acceptance separately; local builds and runtime
  captures do not establish that acceptance.
