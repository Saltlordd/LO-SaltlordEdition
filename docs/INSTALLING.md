# Installing Lost Odyssey Recomp

[简体中文](INSTALLING.zh-CN.md)

This guide covers the published v0.7.35 packages and the current source path. Start with the package for your platform, import your own game data, then keep the save and profile folders when you update.

## Windows quick start

Windows x64 and an AVX-capable CPU are required. Direct3D 12 is the default graphics backend. The release package already contains the importer, updater, DXC v1.8.2407 DLL pair and dependency licenses; Python and Visual Studio are not required to play.

1. Download `LostOdysseyRecomp-windows-x64-v0.7.35.zip` from the [v0.7.35 release](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.35).
2. Extract the complete ZIP to a writable folder outside `Program Files`.
3. Run `LostOdysseyRecomp.exe`. If no usable game installation is found, the built-in importer opens.
4. Choose the interface and game language, then set the graphics options. The game continues after the first-launch setup and shader preparation. It may first offer to download precompiled shaders for the selected renderer; see [Shader preparation](#shader-preparation).

The download does not include game files. Disc 1 is required to start.

<a id="automatic-content-import"></a>

## Importing game data

On the importer’s source page, choose **Files** for selected files or **Folder** for a directory. The importer recognizes the source from its disc metadata; you do not need to choose a separate disc or DLC mode. Review the detected content before confirming the import.

<a id="supported-sources"></a>

Supported sources are:

- an extracted game folder or its `default.xex`;
- an XDVDFS ISO, including a padded image with its descriptor within the first 512 MiB;
- a GOD/SVOD header, its `.data` folder, or a folder containing multiple GOD discs.

The audited sets use Title ID `4D5307FA`:

| Edition | Version | Media IDs for discs 1–4 |
|---|---:|---|
| Asian multilingual | 4 | `39F7D748`, `0EF8CEA8`, `309E3386`, `7B21A91D` |
| USA/Europe | 3 | `368DE6DD`, `1888BE4E`, `6DD59D08`, `0C0E80B5` |

Do not mix discs from the two editions. Other regional builds, title updates and modified XEX files are outside the audited set.

After importing all four discs, the game selects the requested disc automatically. You do not need to swap discs manually. Later-disc progression is not fully verified.

## Adding or replacing discs and DLC

Open the importer again from **Settings → Gameplay → Import discs & DLC** when you need another disc or DLC package. Select the content you want to add or replace, review the combined result, and confirm the import. Unrelated installed discs, saves, profiles and settings stay available.

DLC can be selected directly or included in a scanned folder. Supported packages have Lost Odyssey Title ID `4D5307FA`, Marketplace content type `2`, and an STFS volume. DLC files are not included in the program download.

The importer stages the selected content before it becomes part of the installation. If an import is cancelled or fails, retry the remaining content from the importer. Keep the original source files until you have confirmed that the imported installation works; the importer copies source data and does not move your dump.

On the destination page, choose **New folder**, press **F2**, or use controller **Y** to create a folder. You can edit its name before continuing.

Game data normally goes under `game/disc1` through `game/disc4`, with DLC under `game/dlc/<content-id>`. You can choose another destination. In portable mode, `game-path.txt` beside the executable selects the location; an empty or missing file falls back to `game` beside the executable, then the older `../game` location. An explicit `--game` path takes priority for direct startup. See [file locations](#file-locations) for AppImage and Flatpak defaults.

<a id="running-on-linux"></a>

## Linux packages

Linux runs through Vulkan. The [v0.7.35 release](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.35) provides an AppImage and a standalone Flatpak bundle. Steam Deck and other Linux hardware remain only partially validated.

### AppImage

Download `LostOdysseyRecomp-linux-x64-v0.7.35.AppImage`, then run:

```bash
chmod +x LostOdysseyRecomp-linux-x64-v0.7.35.AppImage
./LostOdysseyRecomp-linux-x64-v0.7.35.AppImage
```

You can import game data from the graphical importer. For a direct launch, pass the game directory, `disc1`, or `default.xex`:

```bash
./LostOdysseyRecomp-linux-x64-v0.7.35.AppImage --game /path/to/game
```

A normally mounted AppImage stores saves and settings in your Linux user directories; see [file locations](#file-locations). `--game` chooses the game data and does not switch to portable storage. Putting `game-path.txt` beside the outer `.AppImage` file does not configure this mode.

### Flatpak

The bundle uses application ID `io.github.freefrank.LostOdysseyRecomp` and Freedesktop Platform 26.08. With Flatpak installed, add Flathub for the current user and install the runtime. These commands use a consistent per-user installation, following the [Flatpak setup guide](https://docs.flatpak.org/en/latest/first-build.html):

```bash
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Platform//26.08
```

Then install and run the downloaded bundle:

```bash
flatpak --user install --bundle LostOdysseyRecomp-linux-x64-v0.7.35.flatpak
flatpak run io.github.freefrank.LostOdysseyRecomp
```

The default Flatpak game directory is `/var/data/game`. The manifest grants access to the host, `/media`, `/run/media` and `/mnt` so the importer can reach a dump stored outside the sandbox. Standalone bundles do not configure an OSTree remote; install a newly downloaded bundle when upgrading.

## Running on Linux from source

See [BUILDING.md](BUILDING.md) for native build prerequisites and packaging commands. The runtime needs a Vulkan driver, `libdxcompiler.so` beside the executable, and an extracted Disc 1. A direct launch looks like this:

```bash
./LostOdysseyRecomp --game /path/to/game
```

Keep the ELF directory as the working directory when you want portable `save/`, `profile/`, `cache/` and `logs/` folders.

<a id="macos"></a>

## macOS (Apple Silicon, experimental)

The [v0.7.35 release](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.35) provides `LostOdysseyRecomp-macos-arm64-v0.7.35.dmg`, a disk image with `LostOdysseyRecomp.app` and an Applications link. It needs an Apple Silicon Mac with macOS 15 or later, and the game renders with Metal. The game itself has only run on macOS 26.6.2. The v0.7.35 app still declares macOS 14.0, but its bundled shader compiler (DXC) is built for macOS 15 and the game has never run on macOS 14, so macOS 14 is not supported ([details](MACOS_RELEASE.md#minimum-macos-version)). The app is ad-hoc signed and not notarized, so macOS blocks the first launch.

1. Download the disk image and open it.
2. Drag `LostOdysseyRecomp.app` onto the Applications link in the window, then eject the disk image.
3. Open `LostOdysseyRecomp` from Applications. macOS blocks it the first time. Open **System Settings → Privacy & Security**, scroll to the Security section and choose **Open Anyway** next to the app. The button appears only after a blocked launch attempt. Choose **Open** when macOS asks again; it may ask for your password. Later launches need no approval.
4. The built-in importer opens when no usable game installation is found. Import your game data as described in [Importing game data](#automatic-content-import).

To approve the app from Terminal instead, run `xattr -dr com.apple.quarantine /Applications/LostOdysseyRecomp.app`, then open the app again.

The app keeps game data, settings, saves, profiles and the shader cache in `~/Library/Application Support/LostOdysseyRecomp/`, and logs in `~/Library/Logs/LostOdysseyRecomp/logs/`; see [file locations](#file-locations). The first start may offer to download the Metal shaders; see [Shader preparation](#shader-preparation).

The game's update check only offers to open the release page when a newer release has a disk image. To update, download the new image and replace the app in Applications; your saves and settings live outside the app and stay.

Validation is limited to one Mac. On the maintainer's M1 Max (macOS 26.6.2) the opening new-game battle ran with Metal and used the downloaded Metal shader pack; HDR requested on an external display without EDR headroom stayed in SDR, and Metal HDR output itself has not been seen. Long play, broader scenes and other Macs have not been tested. To build the app yourself, see [BUILDING.md](BUILDING.md#building-on-macos); [MACOS_RELEASE.md](MACOS_RELEASE.md) describes how the disk image is made.

## First launch and settings

On Windows, the first-launch page sets the interface language, game language and graphics options before game initialization. Existing settings skip that page; run `LostOdysseyRecomp.exe --setup` to open it again. Linux and macOS have no first-launch page yet: the first run saves default settings, which you change on the in-game Settings page. The in-game Settings page offers a controlled restart for options that need a new process.

The interface language and game language are separate. USA/Europe data provides English, Japanese, German, French, Spanish and Italian; the audited Asian set provides English, Japanese, Korean, Traditional Chinese and Simplified Chinese. A game-language choice unavailable in the installed edition falls back to English.

Press **F1** or **LB+RB** to open the Debug Menu; opening it pauses the game. It is separate from ordinary Settings. See the [README's Debug Menu guide](../README.md#debug-menu) for captures, teleport, fast-forward, memory edits and Save Anywhere restrictions.

### Shader preparation

Release packages carry no precompiled shaders. When none matching the selected renderer (Direct3D 12, Vulkan or Metal) is installed and one is published for your version, the game offers to download it at startup, after the update check and before it prepares shaders. The window, titled Shader bundle, shows the download size.

- **Download (A)** fetches the pack from the `shader-packs` release on GitHub and shows progress; **Cancel (B)** stops it. The game uses the file only if its size and SHA-256 match the published list and it fits the game; otherwise it compiles the shaders on your PC. Accepting avoids several minutes of shader compilation.
- **Skip (B)** compiles the shaders on your PC. The choice is remembered until the shaders change, in `declined-downloads.txt` in the `shaders/` folder (see the [README's file list](../README.md#files-and-folders)). Closing the window without choosing asks again at the next start.

When you are offline, or no pack is published for your version, the game compiles the shaders on your PC without asking. Later launches reuse the compiled shaders. The [pack reference](PORTABLE_SHADER_PACK.md#startup-download) lists the files and the checks.

## File locations

| Layout | Default locations |
|---|---|
| Windows portable package; writable native Linux ELF directory | `save/`, `profile/`, `cache/`, `logs/`, `settings.ini` and `game-path.txt` beside the executable when launched normally. Imported data defaults to `game/`. |
| AppImage; native Linux ELF in a read-only directory | Saves, profiles, cache and game data: `~/.local/share/lost-odyssey-recomp/`. Settings and game path: `~/.config/lost-odyssey-recomp/`. Logs: `~/.local/state/lost-odyssey-recomp/logs/`. |
| Flatpak | Under `~/.var/app/io.github.freefrank.LostOdysseyRecomp/`: saves, profiles, cache and game files use `data/`; settings and game path use `config/lost-odyssey-recomp/`; logs use `.local/state/lost-odyssey-recomp/logs/`. |
| macOS `.app` (experimental) | Saves, profiles, cache, game data, settings and game path: `~/Library/Application Support/LostOdysseyRecomp/`. Logs: `~/Library/Logs/LostOdysseyRecomp/logs/`. |

F1 render captures go to `captures/` and mods to `mods/`. In a portable layout both sit beside the executable. Otherwise captures use the settings folder and mods the data folder. The [README](../README.md#files-and-folders) lists every file and folder and the [command-line options](../README.md#command-line-options).

In Flatpak, the host's `data/` folder appears as `/var/data` inside the sandbox; the default game directory is `/var/data/game`. This program's shader cache uses `data/cache/`. The host paths follow [Flatpak's XDG directory conventions](https://docs.flatpak.org/en/latest/conventions.html#xdg-base-directories).

Linux uses portable storage when the actual ELF directory is writable. Otherwise it uses the XDG directories above; custom `XDG_CONFIG_HOME`, `XDG_DATA_HOME` or `XDG_STATE_HOME` values change their roots. For portable Windows or native ELF launches with explicit `--game`, relative user-data paths follow the calling working directory, so launch from the same directory to keep using the same saves. `--game` selects the game source and does not change a non-portable installation's data layout; only F1 captures then follow the working directory.

## Updating and keeping user data

Formal release packages can check GitHub for a newer release at startup. Disable automatic checks if you prefer manual updates; an offline or failed check does not block launching. Standalone Flatpak bundles are installed manually because they do not use an OSTree remote. On macOS the check only offers to open the release page; replace the app in Applications with the copy from the new disk image.

Keep these when updating:

- saves, profiles and `settings.ini`;
- `logs/` and shader caches;
- `game-path.txt` and imported game data when stored beside the package.

For a manual update, close the game before replacing program files. Keep a copy of your saves and settings, and retain the previous package until the new one works.

## Reporting a startup or rendering failure

Attach the complete current runtime log from `logs/runtime-<timestamp>.log` and include the executable or source version, backend, GPU and driver details shown near startup. Set `LO_LOG_FILE=<path>` to choose another log path, or `LO_LOG_FILE=0` to disable the duplicate file sink.

For a visible rendering issue, open **F1 → Overview → Capture Render State**, confirm, then **close F1** so rendering resumes. The next three frames are captured and archived in the background. Reopen the menu to read the result, then attach the archive at the displayed path. Windows produces a `.zip`; Linux and macOS produce a `.tar.gz`. If archiving fails, the raw capture directory remains. Capture archives include screenshots and rendering/shader data; review their contents before sharing.

Include the scene, edition, disc, settings and exact package version in a report so the result can be reproduced.

## From source and CI

See [BUILDING.md](BUILDING.md) for source builds and [PUBLISHING.md](PUBLISHING.md) for repository synchronization. Release packaging is a separate step from local installation.
