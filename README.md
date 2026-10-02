<div align="center">

<img src="assets/lost-odyssey-recomp.png" alt="Lost Odyssey Recomp logo" width="112">

# Lost Odyssey Recomp

**An experimental native PC port of Lost Odyssey for Xbox 360.**

Windows x64 · Linux x64 · Direct3D 12 · Vulkan

### [Download](https://github.com/freefrank/LostOdysseyRecomp/releases/latest) · [Installation guide](docs/INSTALLING.md) · [简体中文](README.zh-CN.md)

[Features](#current-features) · [Controls](#controls) · [Debug Menu](#debug-menu)

[Changelog](CHANGELOG.md) · [Report an issue](https://github.com/freefrank/LostOdysseyRecomp/issues) · [Project board](https://github.com/users/freefrank/projects/3) · [Build from source](docs/BUILDING.md)

</div>

> [!IMPORTANT]
> **The port is still in early testing.** Opening areas and selected scenes have been tested; a complete playthrough has not. Rendering and stability issues remain. Supply your own supported game files.

## Start playing

Choose a package from the [latest release](https://github.com/freefrank/LostOdysseyRecomp/releases/latest). The current published version is **v0.7.25**.

| Platform | Package | First launch |
| :--- | :--- | :--- |
| Windows x64 | `LostOdysseyRecomp-windows-x64-v0.7.25.zip` | Extract the complete ZIP to a writable folder, then run `LostOdysseyRecomp.exe`. Requires an AVX-capable CPU; Direct3D 12 is the default backend, with Vulkan available. |
| Linux x64 | `LostOdysseyRecomp-linux-x64-v0.7.25.AppImage` | Make the file executable with `chmod +x`, then run it. Uses Vulkan. |
| Linux x64 | `LostOdysseyRecomp-linux-x64-v0.7.25.flatpak` | Install the Freedesktop 26.08 runtime, then the downloaded bundle. See the [Flatpak commands](docs/INSTALLING.md#flatpak). Uses Vulkan. |

1. **Import your game data.** The built-in importer opens when no usable game installation is found. Use **Files** or **Folder** to select an extracted game folder, `default.xex`, an XDVDFS ISO or GOD data.
2. **Choose the interface language, game language and graphics options.** The game starts after setup and shader preparation. Later launches reuse the shader cache.
3. **Add the remaining discs and DLC when needed.** Open **Gameplay → Import discs & DLC** in Settings. With all four matching discs imported, the game selects the requested disc automatically.

Disc 1 is required to start. Use one of the audited Asian multilingual or USA/Europe four-disc sets; do not mix editions. The [installation guide](docs/INSTALLING.md) covers disc identification, Linux setup, file locations and updates. Release packages need neither Python nor Visual Studio. Keep your saves and profiles when updating.

### macOS Apple Silicon (experimental, build from source)

This branch includes an experimental arm64 macOS path using Metal. Local Windows and Mac runtime builds now compile and link, and a limited new-game/first-battle test has passed. There is no published macOS package yet. Use the [macOS build instructions](docs/BUILDING.md#building-on-macos) to build it locally. Long play, broader scenes, image quality and performance validation are still pending.

### Latest changes

[v0.7.25](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.25) adds an F1 button that restores RB party switching after a split-party Save Anywhere load (#74), fixes the Legacy of the Eastern Tribe sky flicker with TAA or FSR (#102) and logs similar flicker suspects, hardens the runtime against malformed game requests, writes SHA-256 values into Windows ZIP manifests for older updaters (#105), and explains Flatpak updates in every interface language. The source also gains the experimental Apple Silicon macOS path above; no macOS package is published. See the [changelog](CHANGELOG.md) for earlier releases and the [development status](docs/STATUS.md) for validation coverage.

## Current features

| Feature | Available controls and behavior |
| :--- | :--- |
| Import and setup | Folder, XEX, ISO and GOD sources; supported DLC; selective disc replacement; language and graphics setup before the first game launch. Original source files remain untouched. |
| Languages | English, Japanese, Korean, Traditional Chinese and Simplified Chinese interface options. Game languages depend on the installed edition. |
| Display and image quality | 16:9 and 21:9 resolution presets, a Widescreen toggle, Off/FXAA/SMAA/experimental TAA, DLSS or FSR 3.1 upscaling, filtering and RGB Range options. Unreleased source builds also fill screens taller than 16:9 (such as 16:10, 3:2 and 4:3) with the 3D scene, keep menus and movies in a 16:9 layout, move the minimap toward the top on taller screens, and add side bars to menus on ultrawide screens ([technical note](docs/notes/tall-aspect-layout.md)). On Linux, DLSS was reported unavailable on an NVIDIA GPU with the v0.7.25 packages ([#116](https://github.com/freefrank/LostOdysseyRecomp/issues/116)); the fix is merged and unreleased. |
| Frame rate | 30/60/90/120 FPS targets and FreeSync / G-SYNC Compatible VRR controls. Actual performance depends on the scene and hardware. |
| Frame generation | Windows D3D12 offers Off/DLSS/FSR, supported DLSS multipliers and fixed 2× FSR. Save applies supported changes; switching from DLSS FG to FSR FG requires a restart. Unreleased source also offers Off/DLSS with fixed 2×–6× on Windows Vulkan (enabling it after starting with it off, or switching providers, needs a restart), plus experimental Vulkan FSR 2× (source builds only) and experimental macOS MetalFX 2× (not run on Mac hardware); see the [technical note](docs/notes/vulkan-fg-fsr4-metalfx.md). |
| Settings | Original game fonts, scrollable lists, and Save/apply controls. On the Graphics page, **Start/Enter** moves focus to **Save**; confirm that item to save. Options that require a restart offer **Now/Later**. |
| Shader preparation | Parallel compilation, a skip option and cache reuse. The v0.7.25 packages bundle a Vulkan pack that does not match their runtime, so the first launch compiles every shader (about 3 minutes on a 16-thread CPU). In unreleased source, packages carry no shader pack: when none matching the selected renderer is installed, the game offers to download it at startup, and skipping compiles the shaders on your PC instead ([details](docs/PORTABLE_SHADER_PACK.md#startup-download)). |
| Mods | Mod API v1, LOTEX1/PNG tools, native-menu atlas and font-page replacements, and PlayStation button prompts. See the [modding guide](docs/wiki/Modding.md) for supported replacements and installation. |
| Input and tools | SDL-mapped controllers, keyboard input and rumble; an English/Simplified Chinese [Debug Menu](#debug-menu) for captures, same-map teleport, speed controls and game-data editing. |

Fullscreen, mixed-DPI displays, broader upscaler coverage, Linux hardware, the unreleased tall/ultrawide layout across more hardware, and later-disc progression still need more testing. Current work is tracked in the [roadmap](docs/ROADMAP.md) and [Project board](https://github.com/users/freefrank/projects/3).

## Controls

SDL-mapped controllers and the keyboard can be used together for player 1. Unmapped joysticks need an SDL controller mapping; see the [input reference](docs/notes/controller-input.md).

| Game action | Keyboard |
| :--- | :--- |
| Start / Back | Enter / Backspace |
| A / B / X / Y | Z / X / A / S |
| D-pad / left stick | Arrow keys / I, J, K, L |
| Left / right shoulder | Q / W |
| Left / right trigger | E / R |
| Debug Menu | F1 |

For Ring actions, use the controller's **right trigger** or **R**. Rumble is enabled by default; set `LO_CONTROLLER_RUMBLE=0` to disable it.

## Debug menu

Press **F1**, or **LB+RB** on a controller (**L1+R1** on PlayStation layouts), to open or close the Debug Menu. **The game pauses while it is open.** It has three pages: **Overview**, **Teleport** and **Cheats**. This overlay is separate from the normal Settings menu and uses keyboard or controller input, not the mouse.

| Action | Keyboard | Controller |
| :--- | :--- | :--- |
| Select a row | ↑ / ↓ | D-pad up / down |
| Change a value | ← / → | D-pad left / right |
| Confirm | Enter | A |
| Return or close | Esc | B |
| Previous / next page | Q or Tab / E | LB / RB |
| Change Cheats category | Select the category row, then ← / → | LT / RT |
| Open or close the overlay | F1 | LB+RB |

### Overview: captures and game actions

**Overview** shows the current map name and ID. It also contains the menu language, **Capture render state**, **Save Anywhere**, and actions to request a win for the current battle or cancel a pending win request.

To record a rendering problem, select **Capture render state**, confirm, then **close the menu so rendering can continue**. The capture collects three frames and creates an archive in `captures/` in the background. The status message gives its absolute path: `.zip` on Windows or `.tar.gz` on Linux. If archiving fails, the original capture directory remains available. Captures include screenshots, rendering data, shaders and logs; review the contents before sharing.

**Save Anywhere** enables the original game's **System → Save** action. Close the Debug Menu and reopen the game's System menu to use it. It does not create a separate quicksave.

> [!WARNING]
> **Save Anywhere has a known party-state limitation.** Loading a save made after the party splits can lose RB character switching ([#74](https://github.com/freefrank/LostOdysseyRecomp/issues/74)). Keep a separate normal save. Since v0.7.25, Save Anywhere stays off while the party is split, and the F1 menu's **Force RB Party Switch** button turns switching back on after loading an older save of this kind.

### Teleport: positions within the current map

The **Teleport** page provides a position bookmark, editable X/Y/Z coordinates and step size, and the current map's available points of interest. On the coordinate row, press **Enter** to select X, Y or Z, then use **←/→** to adjust that axis by the selected step. Confirm the teleport action or a point of interest, then close the menu to move. A scene change clears the bookmark and cancels a pending teleport.

### Cheats: speed and game-data tools

The **Cheats** page groups its tools into six categories:

| Category | What it does |
| :--- | :--- |
| **Quick tools** | Fast-forward mode and multiplier, **Allow memory edits**, gold and HP/MP actions. |
| **Characters** | Character HP/MP, EXP values from 0–99, and skills. The EXP field is not a level selector. |
| **Inventory** | Item and material quantities of 1, 10, 50 or 99, plus fill actions for known categories. Use the game's inventory sort to refresh the display when needed. |
| **Equipment** | Experimental weapon, ring and existing accessory-slot changes. |
| **Party** | Experimental five-slot party composition, front/back row and field-character controls. Some changes may need a reload to appear. |
| **Developer** | Experimental access to the original **EDIT MENU**. Enable it, close F1, then press **LT+RT**. Turn the option off after leaving the editor. |

Fast-forward works independently of memory editing and currently requires a controller. Choose **Hold** to accelerate while holding **LT**, or **Toggle** to switch acceleration on and off with each LT press. Select a multiplier of **2×, 3×, 4×, 6× or 8×**, then close the overlay to use it. Acceleration stops while a menu is open, the window is unfocused or the scene editor is active; **LT+RT** does not accelerate. Audio is not time-stretched.

Memory editing is off by default. To change game data, first back up your save and enter a controllable scene outside battle. Enable **Allow memory edits**, choose an action, then confirm **Yes**. When the status shows a pending change, close F1 to let the action run. Reopen the menu to check the result. A scene change cancels a pending edit.

The menu language and Save Anywhere option are written to `settings.ini`. Fast-forward settings and memory-edit permission reset when the program restarts. Changes to game values can become part of a normal game save.

## Files and folders

The Windows ZIP is **portable**: everything stays in the folder you extracted it to. A Linux build whose own folder is writable, such as a source build or an extracted AppImage, is portable too. The AppImage, the Flatpak and a macOS `.app` cannot write next to the program, so they keep your files in per-user folders.

| Package | Config folder | Data folder | Log folder |
| :--- | :--- | :--- | :--- |
| Windows ZIP | folder with `LostOdysseyRecomp.exe` | same | same |
| Linux, writable program folder | program folder | same | same |
| Linux AppImage | `~/.config/lost-odyssey-recomp/` | `~/.local/share/lost-odyssey-recomp/` | `~/.local/state/lost-odyssey-recomp/` |
| Linux Flatpak | `~/.var/app/io.github.freefrank.LostOdysseyRecomp/config/lost-odyssey-recomp/` | `~/.var/app/io.github.freefrank.LostOdysseyRecomp/data/` (`/var/data` inside the sandbox) | `~/.var/app/io.github.freefrank.LostOdysseyRecomp/.local/state/lost-odyssey-recomp/` |
| macOS `.app` (experimental) | `~/Library/Application Support/LostOdysseyRecomp/` | same | `~/Library/Logs/LostOdysseyRecomp/` |

On Linux, set `XDG_CONFIG_HOME`, `XDG_DATA_HOME` or `XDG_STATE_HOME` to move the AppImage folders.

| Content | Location | Notes |
| :--- | :--- | :--- |
| Imported game data | data folder: `game/` with `disc1/`–`disc4/` and `dlc/` | The importer's default destination. You can import elsewhere. |
| Chosen game folder | `game-path.txt`: beside the program when portable, otherwise in the config folder | Written by the importer. |
| Settings | config folder: `settings.ini` and `taa-collection.ini` | Without `settings.ini`, the first-launch setup runs. |
| Saves | data folder: `save/` | Keep when updating. |
| Profiles | data folder: `profile/` | Keep when updating. `LO_PROFILE_DIR` overrides it. |
| Shader and pipeline cache | data folder: `cache/shaders/` | Rebuilt if deleted. `LO_SHADER_CACHE_DIR` overrides it. |
| Logs | log folder: `logs/runtime-*.log` and `logs/shader-*.jsonl` | The current run and the two previous runs are kept. |
| F1 render captures | config folder: `captures/` | `.zip` on Windows, `.tar.gz` on Linux and macOS. |
| Mods | `mods/`: beside the program when portable, otherwise in the data folder | `LO_MODS_DIR` overrides it. |
| Shader packs | install folder: `shaders/` beside the program when portable, otherwise `shaders/` in the data folder | Unreleased source downloads the pack for the selected renderer here (`portable_vk.lospv`, `portable_dx12.lospd` or `portable_metal.lospv`). v0.7.25 and earlier bundle `shaders/portable_vk.lospv` beside the program. |
| Updater work files | Windows: `.update\` beside the program. AppImage: log folder `.update/` | Flatpak and macOS packages are updated manually. |

**How the game is found** when `--game` is not given:

1. The program reads `game-path.txt`.
2. If that file is absent, it looks for `default.xex` in the data folder's `game/` (per-user packages only).
3. Then it checks `game/`, the program folder and `../game` next to the program.
4. If nothing is found, the importer opens.

Launching with `--game` keeps the current working directory. In a portable layout, settings, saves, profiles, cache, logs and captures then follow the folder you start from. In per-user packages only `captures/` does.

## Command-line options

| Option | Effect |
| :--- | :--- |
| `--game <path>` | Uses this game: a folder that contains `default.xex` or `disc1/`, or the `default.xex` file itself. Skips `game-path.txt`, the search and the automatic importer, and exits with an error if no `default.xex` is found. |
| `--install` | Opens the importer even when a game is already set up, then exits: 0 after a successful import, 1 if cancelled or failed. **Gameplay → Import discs & DLC** relaunches with this option. |
| `--setup` | Runs the first-launch setup again, then starts the game. On Windows this is the setup dialog. Linux and macOS have no setup screen yet, so it only saves the current settings. |
| `--setup-only` | Like `--setup`, then exits. |
| `--prepare-shaders-only` | Loads the game data, prepares shaders and pipelines, then exits without starting the game: 0 on success, 1 on failure. Use it to warm the shader cache. |
| `--quiet-kernel` | Leaves kernel trace lines out of the log. |

Options must be spelled exactly. Write `--game <path>` as two arguments; `--game=<path>` is ignored, along with any other unknown argument. There is no `--help` or `--version`. The updater and restart logic use internal arguments (`--apply-plan`, `--wait-process`, `--restart-ready`, `--restart-parent-fd`, `--restart-ready-fd`); do not pass them yourself. `LostOdysseyRecomp.exe` is a GUI program and prints nothing to a console; check the log instead.

```bash
LostOdysseyRecomp.exe --game "D:\Games\Lost Odyssey"
./LostOdysseyRecomp-linux-x64-v0.7.25.AppImage --game ~/Games/LostOdyssey
flatpak run io.github.freefrank.LostOdysseyRecomp --game ~/Games/LostOdyssey
LostOdysseyRecomp.app/Contents/MacOS/LostOdysseyRecomp --game ~/Games/LostOdyssey
```

Environment variables give more launch options. Each one overrides the saved setting for that run.

| Variable | Effect |
| :--- | :--- |
| `LO_GRAPHICS_API` | `d3d12` or `vulkan` on Windows. Linux always uses Vulkan and macOS always uses Metal. |
| `LO_FPS` | Frame-rate cap from 0 to 1000; `0` means uncapped. |
| `LO_FG_PROVIDER`, `LO_FG_MODE`, `LO_FG_MULTIPLIER`, `LO_FG_TARGET_FPS` | Frame generation on Windows: `off`/`dlss`/`fsr`; `off`/`fixed`/`dynamic`; 2–6; target FPS. In unreleased source, Vulkan accepts DLSS fixed 2–6 and, in builds with `LO_ENABLE_VULKAN_FSR_FG`, FSR fixed 2; macOS accepts `metalfx` (fixed 2, experimental). Dynamic mode is D3D12 DLSS only. [Details](docs/notes/vulkan-fg-fsr4-metalfx.md). |
| `LO_OPTISCALER_PATH` | Experimental Windows OptiScaler loading: absolute path to your `OptiScaler.dll`. Requires DLSS/NGX in the build and `LO_FG_PROVIDER=off`; select DLSS in-game. [Setup and limits](docs/notes/vulkan-fg-fsr4-metalfx.md#optional-optiscaler-loading-on-windows). |
| `LO_NO_UPDATE` | Any value other than `0` skips the update check. |
| `LO_PROFILE_DIR`, `LO_SHADER_CACHE_DIR`, `LO_MODS_DIR` | Use another profile, shader cache or mods folder. An empty `LO_SHADER_CACHE_DIR` disables the shader cache. |
| `LO_MODS` | `0` or `false` disables mods. |
| `LO_LOG_FILE` | Write the log to this path, or `0` for no log file. |
| `LO_AUDIO_MUTE`, `LO_CONTROLLER_RUMBLE` | `LO_AUDIO_MUTE=1` mutes audio; `LO_CONTROLLER_RUMBLE=0` turns rumble off. |

## Reporting a problem

Include the exact package or source version, operating system, graphics backend, GPU/driver, game edition and disc, and the steps or scene that reproduce the problem. Attach the complete current `logs/runtime-<timestamp>.log`; it records startup and graphics details. `LO_LOG_FILE=<path>` selects another log path, and `LO_LOG_FILE=0` disables the duplicate file sink.

For a visual defect, use the [capture procedure](#overview-captures-and-game-actions) while the problem is visible. Share the resulting archive only after reviewing it. Do not attach game executables, resource archives, saves or personal data.

Optional diagnostics are off by default and can be disabled in Settings. See [Privacy](PRIVACY.md) for the data collected and sharing controls.

## In-game screenshots

<img src="docs/images/title-screen.png" alt="Lost Odyssey title screen — Press START" width="960">

| Ring combat | City exploration |
| :---: | :---: |
| ![Kaim attacking with the Ring timing interface](docs/images/ring-battle.png) | ![Exploring the industrial city](docs/images/city-exploration.png) |

*Unmodified screenshots from development builds leading up to v0.1.*

## Development

See [Building](docs/BUILDING.md) for dependencies and build commands, [Developer tools](tools/README.md) for the available utilities, and the [documentation index](docs/README.md) for current references and historical notes.

| Directory | Contents |
| :--- | :--- |
| `LostOdysseyRecomp/` | Host kernel, graphics, audio, input and debugging |
| `LostOdysseyRecompLib/` | Configuration; ignored `private/` game data and generated `ppc/` code |
| `tools/` | Recompilers, dependency patches, Ghidra scripts and the optional [assembly profiler](tools/asm-profiler/README.md) |
| `thirdparty/` | Rendering, audio and other dependencies |
| `docs/` | Current status, guides, research and historical archives |

## Sponsors

Thank you to **Cristian** and **Whitesun** for supporting the project on Ko-fi.

## Credits and game data

With research and tools from [UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp), [re:Blue](https://github.com/zolaware/reblue), [XenonRecomp](https://github.com/hedge-dev/XenonRecomp), [XenosRecomp](https://github.com/hedge-dev/XenosRecomp), [plume](https://github.com/renderbag/plume) and [Xenia](https://github.com/xenia-project/xenia). Audio uses the pinned [Xenia FFmpeg fork](https://github.com/xenia-project/FFmpeg), with its [license](thirdparty/ffmpeg-LICENSE.txt).

Lost Odyssey and its assets belong to their respective owners. This is an unofficial project. Supply data extracted from your own discs; do not commit game executables, resource archives, textures, audio, video, generated game code or captures to this repository. Dependencies retain their respective licenses.
