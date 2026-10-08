<p align="center"><img src="android/app/src/main/assets/branding/lo_saltlord_logo_full.png" alt="LO: Saltlord Edition" width="560"></p>

# LO: Saltlord Edition

An Android adaptation of [LostOdysseyRecomp](https://github.com/freefrank/LostOdysseyRecomp), with touch controls, foldable display layouts and guided setup. Developed and playtested by Saltlord on a Samsung Galaxy Z Fold 8 Ultra, with AI-assisted development.

**First beta preparation is underway.** A release APK will appear on the [Releases page](https://github.com/Saltlordd/LO-SaltlordEdition/releases) when published. This branch is a source/documentation candidate, not a published beta release.

## Features

- Guided disc/DLC import, custom GPU driver selection and first-run shader preparation.
- Full-colour or monochrome custom touch artwork; editable positions, sizes, opacity and grid spacing. Invisible floating sticks, haptics, manual/automatic hiding and optional hiding with a connected controller.
- Separate cover/inner display layouts and live Adaptive/16:9 fitting. Experimental Flex mode places the game above a separate control pane, with two orientations, its own editor and a cover-layout copy option.
- Graphics profiles, live resolution/frame-target/AA choices and an optional configurable performance monitor.
- Controller remapping and named profiles; saved Fast Forward settings.
- Save Anywhere, save ZIP backup/restore and overlay JSON backup/restore. Restore saves and layouts during setup or from Options.
- Options pauses gameplay without sending the game's Start button.
- Local diagnostic reports saved through Android's file picker. No automatic report sending; this build requests no INTERNET permission.

## Getting started

Use a compatible ARM64 Android device (Android 9/API 28 or newer). Hardware and driver compatibility vary; testing has primarily used the Z Fold 8 Ultra.

1. Install the release APK, then choose **Start game**.
2. Follow setup to import your own supported game discs and optional DLC. No game files are distributed here.
3. Restore backups if needed, choose graphics settings and your driver, then allow shader preparation to complete. Keeping the app foregrounded can finish faster; progress notifications are optional.
4. When initial compilation finishes, choose **Restart now** to begin with the saved cache.

Updates can be installed over an existing build signed with the same key. Before uninstalling or clearing app data, export saves and layouts somewhere outside app storage.

## Graphics profiles

| Profile | Render resolution | Frame target | Anti-aliasing |
| --- | --- | --- | --- |
| Low-spec | 720p | Steady 30 | Off |
| Balanced | 720p | Dynamic 60 | Off |
| Performance | 720p | Dynamic 60 | SMAA |
| Quality | 1080p | Steady 30 | SMAA |

Low-spec is the conservative fresh-install default. Performance and Quality suit more capable hardware. SMAA produced clearer 720p edges/text than FXAA in the tester's experience; FXAA can look soft or muddy.

**Dynamic 60 targets up to 60 FPS.** It neither changes resolution dynamically nor guarantees a minimum frame rate. 1080p/60, with or without AA, remains selectable after workload confirmation. In one Z Fold 8 Ultra test, 1080p/60/AA Off approached 60 in lighter sections and fell to roughly 32–42 in demanding cutscene intervals. These are scene/device observations, not guarantees. 1080p/30/SMAA provided steadier pacing in the supplied tests.

## Beta limitations

- First-encounter pipeline/graphics hitches can remain despite shader preparation and recorded-pipeline warmup. We do not promise a completely stutter-free playthrough.
- Flex mode is experimental and intended for a foldable's inner display. Auto-hide is temporarily suspended there; manual Hide still works. Ordinary cold launches return to the main menu/fullscreen flow.
- Some upstream PC settings are unavailable or inappropriate on Android. Change them cautiously for testing and revert if problems occur.
- Save Anywhere is not a save state. Compatibility of these saves with the original Xbox 360 release is not established; keep ordinary saves/backups.
- Monitor FPS counts game presentations, not screen refreshes. Thermal status reports Android heat pressure, not chipset temperature; GPU utilisation may be unavailable.

## Bug reports

Use **Options → Setup, features & reports → Save diagnostic report** and choose a location in Android's file picker. Review the file before sharing. Email it to **saltlordstrikes@gmail.com**, or attach it to an issue on this fork.

Include app version, device model, RAM, storage, chipset, driver, graphics choices, screen/Flex mode, controller, what happened, what you expected, and steps to reproduce. Mention whether it occurs every time or on first encounter; screenshots/video help. Reports contain local diagnostics and no save backup.

## Source, credits and licence

See [Android build notes](docs/ANDROID_BUILD.md), [credits](docs/SALTLORD_CREDITS.md) and the [upstream README](README.upstream.md). LostOdysseyRecomp and its contributors provide the underlying recompilation/runtime. This fork preserves upstream licensing and notices; see [LICENSE](LICENSE). The repository does not contain game assets, generated game code, private logs, saves or signing keys.
