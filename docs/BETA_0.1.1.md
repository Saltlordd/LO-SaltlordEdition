# LO: Saltlord Edition — v0.1.1 Beta

Our second Android beta is here! This update brings a substantial pipeline-preparation upgrade, mobile performance fixes, more graphics controls and a cleaner setup experience.

**Crashing before you reach the game? You can now save a diagnostic report from every setup-wizard page and the shader-preparation screen.** Reopen the app after a crash or error, look for **“Crashed during setup? Save log”**, and export a report to send us. This helps us investigate what is stopping more devices from running the app. Every tester and report is appreciated.

## What's new since v0.1.0

### Performance, shaders and upstream integration

- Integrated selected Android-relevant changes from freefrank's LostOdysseyRecomp, reviewed through upstream **v0.9.22**, including earlier pipeline dependencies.
- Retained upstream CPU-side improvements involving register-state handling, synchronization epochs, validation/lookup caches and bounded request polling. Performance still depends on the scene, hardware, driver and heat.
- Bundled **3,974 portable pipeline recipes** to prepare many known graphics-state combinations before their first draw. These are recipes compiled locally by your driver, not a universal precompiled GPU cache or coverage of the entire game.
- Added Vulkan graphics pipeline libraries and fast linking where supported by the driver, normalized pipeline keys, migration of older learned recipes, scene-tagged prefetch, per-worker shader readers, sibling pipeline jobs and a more efficient completed-job queue.
- Preserved separate shipped recipes and locally learned pipelines. New encounters can add to the learned file without replacing it with the bundled corpus.
- Improved shared Vulkan driver-cache loading and persistence, with driver-identity validation and per-driver storage. Switching drivers can require more preparation.
- Kept Android startup preparation on the preparation screen, with bounded mobile workers and cache saving before gameplay. Desktop background startup compilation and optional missing-draw skipping are not enabled.
- Corrected the custom-driver handoff so the selected custom loader stays active, and retained Android surface/resize handling.
- Included upstream native-menu text alignment and an asynchronous motion-vector pipeline dependency fix. The latter is inactive in normal Android Off/SMAA profiles; it is not an extra mobile FPS promise.

### Graphics and saved preferences

- **Motion blur** and **dynamic shadows** switches in Graphics, applied live and saved.
- Saved **edge culling**, with 100% retaining original behavior. Lower values keep objects visible longer and can increase workload; higher values can cause pop-in.
- Graphics profile/preferences persistence improvements. **Expanded RGB range** is the default for all profiles.
- Removed the output-scaling option from Graphics; the mobile path uses basic scaling. Development-only cold-cache and detailed-timing switches are absent from the release UI.

Profiles remain:

| Profile | Resolution | Frame target | Anti-aliasing |
| --- | --- | --- | --- |
| Low-spec — conservative default | 720p | Steady 30 | Off |
| Balanced | 720p | Dynamic 60 | Off |
| Performance | 720p | Dynamic 60 | SMAA |
| Quality | 1080p | Steady 30 | SMAA |

Dynamic 60 targets **up to 60 FPS**; it does not mean dynamic resolution or guaranteed 60 FPS. More demanding combinations, including 1080p with Dynamic 60 and SMAA, remain selectable after workload confirmation.

### Setup, reporting and polish

- Local diagnostic export is available before gameplay, on every wizard page and during shader preparation, as well as in Options.
- A compact **“Crashed during setup? Save log”** shortcut keeps reporting easy to find without filling the page.
- Shorter wizard descriptions and reporting messages, with a clearer **Performance** page.
- Help buttons beside settings replace large standalone hint rows where appropriate.
- Auto-hidden touch controls return with a quick fade; the waking touch still activates controls.
- Renamed **Feedback & auto-hide** and **Export pre-restore saves**. The latter exports the safety copy saved before your latest save restore; it does not change your current saves.

## Features carried forward

Custom full-colour/monochrome Lost Odyssey touch artwork; layout editing with grid snapping/free placement, sizes, opacity, floating sticks and haptics; manual/automatic hiding and controller-aware hiding; separate foldable cover/inner layouts, adaptive fitting and live 16:9 lock; experimental Flex mode with saved layouts; controller remapping and named profiles; guided game/driver setup; Save Anywhere and save/layout backup/restore; graphics profiles; integrated Fast Forward; gameplay-pausing Options; performance monitoring; dedicated main menu and custom branding.

## Installing and preparing

Android 9+ / ARM64. Supply your own supported game files; no game assets are included. Install this APK over the existing Saltlord Edition build. Back up saves before updating; uninstalling or clearing app data is not required.

Allow shader/pipeline preparation to finish, then restart when instructed. First preparation can take time, and driver changes or updates may trigger additional work. Known pipeline recipes reduce first-use hitches, but unseen combinations and other graphics/resource stalls can still occur.

## Beta limits and compatibility

Primarily developed and playtested on Saltlord's Samsung Galaxy Z Fold 8 Ultra. The tested route runs very smoothly on that device with the chosen driver/settings; this does not validate all devices or the entire game. Hardware and driver compatibility vary, and launch crashes on other devices are still being investigated.

Wizard report export can help after a native launch/setup failure **if the Android UI can reopen**. It cannot guarantee recovery or a report when the app cannot open at all, and some Android exit information may be unavailable.

Save Anywhere is a normal game-save feature, **not a save state**. Original Xbox 360 save compatibility remains unverified. Flex auto-hide is temporarily disabled; manual Hide works and cold launches return to normal fullscreen. Thermal monitoring reports Android thermal pressure, not a precise chipset temperature.

Experimental Windows/NVIDIA features, desktop mod management, DLSS/frame-generation UI and desktop background-startup compilation were not imported as Android features. This is a selected upstream integration, not a claim of full upstream parity or a fully reproducible end-to-end game-code build.

## Help improve compatibility

Save a report using **“Crashed during setup? Save log”**, or **Options → Setup, features & reports → Save diagnostic report**.

Attach it to a [GitHub issue](https://github.com/Saltlordd/LO-SaltlordEdition/issues) or email **saltlordstrikes@gmail.com**. Include the app version, device model/chipset/RAM, storage/free space, selected GPU driver, graphics settings, screen/Flex mode, controller details, reproduction steps, expected/actual behavior and whether it happens only on first encounter. If setup fails, tell us which page or preparation stage you reached.

Review the report before sharing. Reports are saved locally and sent only by you. This beta requests **no INTERNET permission** and sends nothing automatically.

Huge thanks to **freefrank and all LostOdysseyRecomp contributors**, plus the projects and contributors credited in the app's notices. Android adaptation and integration by **Saltlord**, with **AI-assisted development**. Thanks to everyone testing, reporting bugs and helping this passion project grow.

[Repository](https://github.com/Saltlordd/LO-SaltlordEdition) · [Upstream](https://github.com/freefrank/LostOdysseyRecomp)
