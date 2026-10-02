# Asset inventory

`inventory.py` builds a metadata-only catalog of the imported Lost Odyssey game data for mod development. It reads the four-disc `LO.fpi` file trees, hashes complete archive entries, and attempts to decode UE3/CPX package metadata with the standalone native decoder. It does not extract or copy textures, models, audio, scripts, or other original payloads.

The scanner is read-only with respect to the game data. Build and report directories must be outside the imported game root; use an ignored directory such as `out/asset-inventory/` or a separate scratch directory. It runs with the native Windows toolchain used by this repository and does not require WSL, Docker, or a running game.

## Build and scan

Build the decoder with a native compiler:

```powershell
python -B tools/asset_inventory/build.py --output out/asset-inventory-build
```

For a complete inventory, point `--game` at a directory containing the matching `disc1` through `disc4` directories (each with its `LO.fpi` and archive files):

```powershell
python -B tools/asset_inventory/inventory.py scan `
  --game "D:\LostOdyssey\game" `
  --output "out/asset-inventory" `
  --decoder "out/asset-inventory-build/asset_decoder.exe"
```

The command writes `catalog.sqlite`, `summary.json`, `REPORT.md`, CSV catalog files, and failure records. Large category catalogs are written as `.csv.gz`. The SQLite database is the authoritative query source; generated databases and full CSV catalogs should remain local and should not be committed.

The scanner requires all four discs by default. `--allow-partial` explicitly marks a bounded partial scan and must not be used for a complete game resource census:

```powershell
python -B tools/asset_inventory/inventory.py scan --allow-partial `
  --game "D:\LostOdyssey\disc1-only" `
  --output "out/asset-inventory-partial" `
  --decoder "out/asset-inventory-build/asset_decoder.exe"
```

After decoder improvements, revalidate registered UE3 candidates and regenerate a report in a new directory:

```powershell
python -B tools/asset_inventory/inventory.py reparse `
  --db "out/asset-inventory/catalog.sqlite" `
  --decoder "out/asset-inventory-build/asset_decoder.exe"
python -B tools/asset_inventory/inventory.py report `
  --db "out/asset-inventory/catalog.sqlite" `
  --output "out/asset-inventory-report-2"
```

`reparse` rechecks full payload hashes and package metadata while retaining the existing non-package file statistics. `report` requires a new output directory so an earlier report is not silently overwritten.

`reparse --pending-only` limits the pass to failures and previously undecoded CPX signatures. Both `scan` and `report` reject output inside the recorded game-data root. A completed scan may still return exit code 2 when it contains parse/property errors or unsupported Mod keys; inspect the separate counters in `summary.json`. Unsupported keys do not invalidate successful package enumeration.

## Focused checks

```powershell
python -B -m unittest tools.tests.asset_inventory_test -v
python -B tools/tests/asset_decoder_test.py --decoder out/asset-inventory-build/asset_decoder.exe -v
python -B -m unittest tools.tests.mod_catalog_test tools.tests.mod_tools_test -v
```

The first command checks synthetic FPI traversal, content variants, queries, replay and input/output boundaries. The second exercises the actual decoder with synthetic UE3/CPX data, UTF-16 names, texture metadata, and damaged bounds/references. The third checks SQLite catalog selection, filters, image keys, manifest/overlay layouts, and LOTEX1 packaging; it requires Pillow but no game data. None of these checks requires original game data.

## Querying the catalog

Use `query` to inspect categories, UE3 export classes, names, and UI heuristics without opening the raw game files:

```powershell
python -B tools/asset_inventory/inventory.py query --db "out/asset-inventory/catalog.sqlite" --category models --limit 30
python -B tools/asset_inventory/inventory.py query --db "out/asset-inventory/catalog.sqlite" --class-name Texture2D --limit 50
python -B tools/asset_inventory/inventory.py query --db "out/asset-inventory/catalog.sqlite" --search UI_MAIN_00 --ui
```

Categories are export-class based. They include models, world geometry, textures, fonts, materials, material nodes, animations, effects, audio, UI objects, sequences, script objects, shader caches, levels/worlds, groups, redirectors, parameter curves, scene components, default templates, and other objects. Standalone files such as movies, sound streams, subtitles, camera data, localization data, and event scripts are recorded separately as file-level kinds.

The UI column is an additional usage hint derived from export class or package/path naming. It is not proof that a resource is consumed by the native Mod API. `Default__*` objects are kept in `default_templates`; `Model` and `Terrain` are kept in `world_geometry`; `StaticMeshRPG` is classified as models, while `SkeletalMesh_VC` is classified as `scene_actors`.

## Counting and modding limits

The report distinguishes file occurrences, distinct virtual paths, unique complete payloads, decoded exports, and path/content/export variants. A package copied to more than one disc path is therefore visible in both its occurrence count and its content-deduplicated count. Same-key content variants are retained and must be selected using path, disc, and SHA-256. Imports are dependency references and are not counted as assets.

The catalog does not establish runtime replacement support. The current Mod API wires the native settings `UI_MAIN_00` atlas and selected native font texture pages. Arbitrary guest textures, models, fonts/metrics, movies, and other listed resources remain extension points until a runtime consumer is implemented. Object names containing unsupported key characters are retained with `key_error` and an empty `mod_key`; the inventory does not invent a replacement key.

## Feeding the Mod API

`tools/modding/lo_mod.py catalog` and `init` accept the completed `catalog.sqlite` directly. The `--database` and legacy `--manifest` inputs are mutually exclusive. The database must be schema 1 with `complete=true` and is opened read-only. Catalog skips `key_error`, `property_error`, missing dimensions, and non-`Texture2D` rows; the legacy CSV path retains its `status=exported` filter. Output is limited to 100 rows by default; `--runtime-only` includes font-page candidates, while `--object`, `--package`, and `--content-sha256` select a narrower identity/content variant. The SHA filter selects the package variant used to derive authoring identity and dimensions; it is not written into the key or LOTEX1 payload.

```powershell
python tools/modding/lo_mod.py catalog --database catalog.sqlite --runtime-only --limit 100
python tools/modding/lo_mod.py init --database catalog.sqlite --object UI_MAIN_00 --package bin/xenon/loc/int/menu/rpmenurescommon_int.xxx --image art.png --id example --output mod.json
```

The confirmed consumer accepted by `init` is the native settings atlas `UI_MAIN_00`. Native font-page rows for the `Maru23`, `LocTit1`, and `Abc` owners are candidates only; `init` rejects them by default because native references are not indexed as complete runtime support. Other rows are marked `no_runtime_consumer`; `--allow-unwired` is required for an explicitly experimental specification. `init` requires exactly one identity and one content variant; `--content-sha256` chooses the variant used to derive the authoring identity and dimensions, while the key and LOTEX1 payload do not bind runtime resolution to that SHA. This connects the catalog to the existing `AssetProvider`/manifest/overlay contract; it does not register a new runtime provider or make the game load SQLite. The resulting JSON continues through the existing LOTEX1 `pack` command and supports `standalone` or `overlay` layouts.

The scanner covers the complete disc set passed to it. The `sources` field is auxiliary evidence for checking package provenance and does not change runtime resolution. The native settings-menu loader tries `gameRoot/disc1` first and falls back to `gameRoot`.

The offset in a catalog row is the original FPD archive extent. It must not be confused with a decoded or unpacked UE3 object offset. Reports contain metadata and hashes only; they do not include original artwork or package contents.

For the package layout background, see the upstream UEViewer UE3 package serializer: [UnrealPackage/UnPackage3.cpp](https://github.com/gildor2/UEViewer/blob/master/Unreal/UnrealPackage/UnPackage3.cpp).
