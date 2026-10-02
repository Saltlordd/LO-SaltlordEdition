<div align="center">

<img src="assets/lost-odyssey-recomp.png" alt="Lost Odyssey Recomp logo" width="112">

# Lost Odyssey Recomp

**An experimental native PC port of Lost Odyssey for Xbox 360.**

Windows x64 · Linux x64 · macOS arm64 (experimental) · Direct3D 12 · Vulkan · Metal

### [Download](https://github.com/freefrank/LostOdysseyRecomp/releases/latest) · [Installation guide](docs/INSTALLING.md) · [简体中文](README.zh-CN.md)

[Features](#current-features) · [Controls](#controls) · [Debug Menu](#debug-menu)

[Changelog](CHANGELOG.md) · [Report an issue](https://github.com/freefrank/LostOdysseyRecomp/issues) · [Project board](https://github.com/users/freefrank/projects/3) · [Build from source](docs/BUILDING.md)

</div>

> [!IMPORTANT]
> **The port is still in early testing.** Opening areas and selected scenes have been tested; a complete playthrough has not. Rendering and stability issues remain. Supply your own supported game files.

## Start playing

Choose a package from the [latest release](https://github.com/freefrank/LostOdysseyRecomp/releases/latest). The current published version is **v0.7.35**.

| Platform | Package | First launch |
| :--- | :--- | :--- |
| Windows x64 | `LostOdysseyRecomp-windows-x64-v0.7.35.zip` | Extract the complete ZIP to a writable folder, then run `LostOdysseyRecomp.exe`. Requires an AVX-capable CPU; Direct3D 12 is the default backend, with Vulkan available. |
| Linux x64 | `LostOdysseyRecomp-linux-x64-v0.7.35.AppImage` | Make the file executable with `chmod +x`, then run it. Uses Vulkan. |
| Linux x64 | `LostOdysseyRecomp-linux-x64-v0.7.35.flatpak` | Install the Freedesktop 26.08 runtime, then the downloaded bundle. See the [Flatpak commands](docs/INSTALLING.md#flatpak). Uses Vulkan. |
| macOS arm64 (experimental) | `LostOdysseyRecomp-macos-arm64-v0.7.35.dmg` | Open the disk image and drag `LostOdysseyRecomp.app` to the Applications link. The app is not notarized, so macOS blocks the first launch: try to open it, then open System Settings → Privacy & Security and choose **Open Anyway**. Requires an Apple Silicon Mac with macOS 15 or later; the game has only run on macOS 26.6.2. Uses Metal. See the [macOS steps](docs/INSTALLING.md#macos). |

1. **Import your game data.** The built-in importer opens when no usable game installation is found. Use **Files** or **Folder** to select an extracted game folder, `default.xex`, an XDVDFS ISO or GOD data.
2. **Choose the interface language, game language and graphics options.** The game starts after setup and shader preparation. If no precompiled shaders for the selected renderer are installed, it first offers to download them; skipping compiles them on your PC. Later launches reuse the shader cache.
3. **Add the remaining discs and DLC when needed.** Open **Gameplay → Import discs & DLC** in Settings. With all four matching discs imported, the game selects the requested disc automatically.

Disc 1 is required to start. Use one of the audited Asian multilingual or USA/Europe four-disc sets; do not mix editions. The [installation guide](docs/INSTALLING.md) covers disc identification, Linux setup, file locations and updates. Release packages need neither Python nor Visual Studio. Keep your saves and profiles when updating.

### macOS Apple Silicon (experimental)

v0.7.35 is the first release with a macOS package: an experimental arm64 path that renders with Metal. Open `LostOdysseyRecomp-macos-arm64-v0.7.35.dmg`, drag `LostOdysseyRecomp.app` to the Applications link and start it. The app is ad-hoc signed and not notarized, so macOS blocks the first launch. Try to open the app once, then open System Settings → Privacy & Security, choose **Open Anyway** for it and confirm; or run `xattr -dr com.apple.quarantine /Applications/LostOdysseyRecomp.app`. The [installation guide](docs/INSTALLING.md#macos) has the steps and file locations. The game's update check only offers to open the release page, so you replace the app yourself. Validation so far is one Mac: on the maintainer's M1 Max (macOS 26.6.2) the opening new-game battle ran with Metal, and the Metal shader pack was downloaded and used. Long play, broader scenes, other Macs, image quality and performance have not been tested. To build it yourself, use the [macOS build instructions](docs/BUILDING.md#building-on-macos).

### Experimental HDR

v0.7.35 includes experimental HDR output paths for Windows D3D12/Vulkan, Linux Vulkan and macOS Metal. Enable **HDR** in Graphics, save and restart. **HDR peak brightness** opens a frozen frame of the current game scene: the left view is an SDR brightness preview clipped at reference white, while the right view uses normal HDR tone mapping and updates the peak live. Switch between the Scene and Test pattern with the mouse or controller **X** button; if no valid scene is available, the standard pattern is used. Auto follows the active display's reported peak when available; if none is available, it uses a 1000-nit content reference. The current Linux Vulkan path has no display-peak report, so Auto uses that fallback. On macOS, Auto derives a relative EDR headroom estimate, not a measured panel brightness. Peak adjustments preview on the calibration page; peak and paper-white changes apply after saving without a restart. The initial path requires anti-aliasing Off, upscaling Off, frame generation Off and a non-MetalFX spatial filter; unsupported surface format/color-space pairs fall back to SDR. The maintainer confirmed on-device HDR validation on 2026-10-02; exact platform, backend and display coverage was not recorded, so this does not establish cross-platform coverage. On a Mac whose external display had no EDR headroom, HDR requested in the game stayed in SDR, and Metal HDR output itself has not been seen. See the [HDR technical note](docs/notes/hdr-output.md) for output semantics and validation limits.

### Android ARM64 runtime (experimental, build from source)

The branch now includes an experimental arm64 Android runtime target alongside the diagnostic APK. The runtime uses an SDL Android activity, app-owned external storage, the Android ARM64 FFmpeg configuration and an Android DXC build. It is a development build, not a published or generally supported Android release. See the [Android build instructions](packaging/android/README.md) and the [Android port research note](docs/notes/android-port-research-2026-10-02.md) for the pinned toolchain and current validation limits.

The Android runtime now loads the development resources, plays the opening video and reaches the first battle on the test tablet. Touch input passed title/menu navigation and two opening-battle attacks with visible damage; one initial-battle shader preparation observed a pause of about 40 seconds. Longer play, audio beyond native queue evidence, other GPUs, 16 KB devices and physical-controller validation remain pending. See the [Android port research note](docs/notes/android-port-research-2026-10-02.md) and [Android DXC build note](docs/notes/android-dxc-build-2026-10-02.md).

The runtime keeps SDL physical-controller support and adds an Android on-screen touch controller. Its source implementation now supports the `Controller settings` size/opacity controls and an `Edit layout` screen for dragging, saving, resetting and showing or hiding individual controls; the layout model checks and device UI flow are verified on the development tablet. This source path has not yet been accepted through a complete game session. The generated PPC code also remains available as a separate Android ARM64/PIC static-library target.

Connecting a USB or Bluetooth controller automatically hides the touch controls while keeping `CTRL` available. Enable **Show touch controls** to use both together; disconnecting the last controller restores the saved touch preference. Android also hides unavailable desktop graphics options and rebuilds its Vulkan surface when returning from the background. Its Vulkan shaders can be prebuilt on a host with the Android-specific shader contract; desktop Vulkan bundles are incompatible. See the [Android instructions](packaging/android/README.md) for generation and installation.

### Latest changes

[v0.7.35](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.35) is the first release with a macOS package, an experimental Apple Silicon disk image (see above). Packages no longer bundle a shader pack, which makes them much smaller: the Windows ZIP is 77.1 MB (v0.7.25: 254.5 MB), the AppImage 76.3 MB (253.8 MB) and the Flatpak 54.1 MB (231.4 MB). Instead the game offers to download the pack for its renderer at startup, and shaders compiled on your PC are now kept in one file per renderer. It adds experimental HDR output with brightness calibration, a layout that fills screens taller than 16:9 with the 3D scene, Vulkan DLSS frame generation at 2× to 6×, and GPU occlusion queries on Direct3D 12 and Vulkan, so the sun and its lens flare no longer show through terrain (#118; checked on an NVIDIA GPU only). It fixes the battle camera jumping at 90 and 120 FPS (#117), DLSS being unavailable on NVIDIA GPUs in the Linux packages (#116), and TAA, FSR and DLSS flicker in several skies, caves and cutscenes (#121 and places found by a map tour). The flicker fixes are checked by tests and have not been replayed in the game, and reporters have not yet confirmed #116, #118 or #121. See the [changelog](CHANGELOG.md) for the full list and earlier releases and the [development status](docs/STATUS.md) for validation coverage.

## Current features

| Feature | Available controls and behavior |
| :--- | :--- |
| Import and setup | Folder, XEX, ISO and GOD sources; supported DLC; selective disc replacement; language and graphics setup before the first game launch. Original source files remain untouched. |
| Languages | English, Japanese, Korean, Traditional Chinese and Simplified Chinese interface options. Game languages depend on the installed edition. |
| Display and image quality | 16:9 and 21:9 resolution presets, a Widescreen toggle, Off/FXAA/SMAA/experimental TAA, DLSS or FSR 3.1 upscaling, filtering and RGB Range options. Since v0.7.35, screens taller than 16:9 (such as 16:10, 3:2 and 4:3) are filled with the 3D scene, menus and movies keep a 16:9 layout, the minimap moves toward the top on taller screens, and menus on ultrawide screens get side bars ([technical note](docs/notes/tall-aspect-layout.md)). Experimental HDR is also available for Windows D3D12/Vulkan, Linux Vulkan and macOS Metal, with automatic or manual peak calibration; see the [HDR technical note](docs/notes/hdr-output.md). Linux HDR hardware validation is pending. On Linux, DLSS was reported unavailable on an NVIDIA GPU with the v0.7.25 packages ([#116](https://github.com/freefrank/LostOdysseyRecomp/issues/116)); v0.7.35 contains the fix, which has not yet run on an NVIDIA GPU under Linux. |
| Frame rate | 30/60/90/120 FPS targets and FreeSync / G-SYNC Compatible VRR controls. Actual performance depends on the scene and hardware. |
| Frame generation | Windows D3D12 offers Off/DLSS/FSR, supported DLSS multipliers and fixed 2× FSR. Save applies supported changes; switching from DLSS FG to FSR FG requires a restart. Since v0.7.35, Windows Vulkan also offers Off/DLSS with fixed 2×–6× (enabling it after starting with it off, or switching providers, needs a restart), plus experimental Vulkan FSR 2× (source builds only) and experimental macOS MetalFX 2× (not run on Mac hardware); see the [technical note](docs/notes/vulkan-fg-fsr4-metalfx.md). |
| Settings | Original game fonts, scrollable lists, and Save/apply controls. On the Graphics page, **Start/Enter** moves focus to **Save**; confirm that item to save. Options that require a restart offer **Now/Later**. |
| Shader preparation | Parallel compilation, a skip option and cache reuse. The v0.7.25 packages bundled a Vulkan pack that did not match their runtime, so their first launch compiled every shader (about 3 minutes on a 16-thread CPU). The v0.7.35 packages carry no shader pack: when none matching the selected renderer is installed, the game offers to download it at startup, and skipping compiles the shaders on your PC instead ([details](docs/PORTABLE_SHADER_PACK.md#startup-download)). |
| Mods | Mod API v1, LOTEX1/PNG tools, native-menu atlas and font-page replacements, and PlayStation button prompts. See the [modding guide](docs/wiki/Modding.md) for supported replacements and installation, and the [asset inventory](docs/wiki/Asset-Inventory.md) for the read-only resource catalog. |
| Input and tools | SDL-mapped controllers, keyboard input and rumble; an English/Simplified Chinese [Debug Menu](#debug-menu) for captures, same-map teleport, speed controls and game-data editing. |

Fullscreen, mixed-DPI displays, broader upscaler coverage, Linux hardware, the tall/ultrawide layout across more hardware, and later-disc progression still need more testing. Current work is tracked in the [roadmap](docs/ROADMAP.md) and [Project board](https://github.com/users/freefrank/projects/3).

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
| Shader packs | install folder: `shaders/` beside the program when portable, otherwise `shaders/` in the data folder | The game downloads the pack for the selected renderer here (`portable_vk.lospv` for Vulkan, Metal and Android, or `portable_dx12.lospd`); skipping the offer is remembered in `declined-downloads.txt` in the same folder. v0.7.25 and earlier bundled `shaders/portable_vk.lospv` beside the program. |
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
./LostOdysseyRecomp-linux-x64-v0.7.35.AppImage --game ~/Games/LostOdyssey
flatpak run io.github.freefrank.LostOdysseyRecomp --game ~/Games/LostOdyssey
LostOdysseyRecomp.app/Contents/MacOS/LostOdysseyRecomp --game ~/Games/LostOdyssey
```

Environment variables give more launch options. Each one overrides the saved setting for that run.

| Variable | Effect |
| :--- | :--- |
| `LO_GRAPHICS_API` | `d3d12` or `vulkan` on Windows. Linux always uses Vulkan and macOS always uses Metal. |
| `LO_FPS` | Frame-rate cap from 0 to 1000; `0` means uncapped. |
| `LO_FG_PROVIDER`, `LO_FG_MODE`, `LO_FG_MULTIPLIER`, `LO_FG_TARGET_FPS` | Frame generation on Windows: `off`/`dlss`/`fsr`; `off`/`fixed`/`dynamic`; 2–6; target FPS. Since v0.7.35, Vulkan accepts DLSS fixed 2–6 and, in builds with `LO_ENABLE_VULKAN_FSR_FG`, FSR fixed 2; macOS accepts `metalfx` (fixed 2, experimental). Dynamic mode is D3D12 DLSS only. [Details](docs/notes/vulkan-fg-fsr4-metalfx.md). |
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
