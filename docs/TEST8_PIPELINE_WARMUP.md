# Test8: first-encounter pipeline preparation

Private candidate based on the successful Test7 phone test. All nine native libraries remain byte-for-byte identical to Test7, retaining its upstream CPU/rendering integrations and custom-driver dispatch fix. This build changes Android launch preparation, driver import identity and diagnostic UI only. Development and integration are AI-assisted, based on freefrank's LostOdysseyRecomp and contributors. Public identity: Saltlord.

## Changes
- Bundle the 202 portable pipeline recipes collected in Saltlord's four-run report. These contain state and shader hashes, no shader binaries, driver binaries, saves or game assets. The existing native renderer validates them, resolves available shaders, and creates driver-specific pipelines before guest startup.
- Validate framing, schema version 27/recipe version 1, checksums and supported state masks before merging. Preserve compatible learned recipes, deduplicate exact keys, cap records and replace the recipe file atomically. Invalid/incompatible old recipes are replaced by the valid seed. Failure is reported and leaves native preparation available.
- Importing identical driver loader inputs reuses the selected or another existing matching directory, including legacy randomly named directories. New imports use a SHA-256 identity directory. ZIP order is irrelevant; changed metadata/library content stays separate. Existing native device/driver cache UUID compatibility checks remain in effect.
- Graphics: **Rebuild pipelines on next launch · test route**. This one-shot diagnostic action deletes only the selected driver's binary pipeline cache and replaces learned recipe coverage with the 202 bundled recipes. It preserves SPIR-V preparation, other driver caches, saves, layouts, settings and game files. It simulates a cold pipeline test, not an entirely fresh installation.
- Diagnostic header reports bundled/merged coverage, previous recipe validity and whether that launch performed the cold test.

## Phone test
1. Install Test8 over Test7. Keep V34 selected; no uninstall or data clearing is needed.
2. Options → Graphics: turn **Collect detailed performance timings** off. Enable **Rebuild pipelines on next launch · test route**, then restart the app. Both settings apply on the next process launch.
3. Allow preparation and any requested restart to finish. Use the same recent save and route at 720p, Dynamic 60, AA Off, Basic scaling, with the same effect settings and screen mode. Let the phone cool before comparing.
4. Play the route once, then restart and repeat twice without importing another driver or changing graphics. Export the report after all runs: Options → Setup, features & reports → Save diagnostic report.
5. Separately, if desired, reimport the exact same V34 ZIP and restart. Its compatible existing pipeline cache should still load. A genuinely different driver may need preparation.

Expected: cold preparation creates approximately 202 known pipelines before gameplay; subsequent starts can reuse compatible driver binaries. Check `renderer: pipeline preparation`, `Android Vulkan pipeline cache loadedBytes`, actual Vulkan driver identity and runtime pipeline hitches. Timing overhead must remain off for normal FPS comparisons.

## Limits
Coverage is this captured route, not all scenes or the entire game. Unsupported/missing shaders and newly encountered state can still require runtime work. Texture uploads, asset streaming and other CPU/GPU spikes are separate from pipeline compilation. No zero-stutter or universal FPS promise. Existing Test7 diagnostic low graphics baseline behavior is retained. This is not a full fresh-install, comprehensive device or end-to-end reproducibility claim.

## Validation
Production Java compilation against Android 35 and D8 release/min API 28 passed. Host integration checks cover corrupt/truncated recipe rejection, learned merge, exact-key deduplication, legacy/stable same-driver reuse, changed-driver separation, one-shot reset and unrelated save/cache preservation. The native C++ recipe loader accepts all 202 seeded records. Original signer, APK integrity, ARM64/16KiB ELF and ZIP alignment, unchanged native libraries and previous assets/resources, and absence of INTERNET permission checked. Device validation is pending.
