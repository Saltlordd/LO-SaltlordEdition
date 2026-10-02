# 资源统计 / Asset inventory

这份页面说明如何为 Mod 开发建立游戏资源目录。统计工具只读取游戏数据，不启动游戏、不修改原始光盘文件，也不需要 WSL 或 Docker。输出是元数据、哈希和分类结果，不包含原始纹理、模型、音频、脚本或其他游戏资源正文。

This page describes the metadata catalog used for Mod development. The inventory is read-only: it does not launch the game or modify imported disc data, and it runs with the native host toolchain without WSL or Docker. Reports contain metadata, hashes, and classifications only; they do not redistribute original textures, models, audio, scripts, or other payloads.

## 使用 / Usage

先在仓库根目录构建原生 decoder，再扫描包含 `disc1` 到 `disc4` 的游戏目录。构建和输出目录应位于游戏目录之外，推荐使用被忽略的 `out/`：

Build the native decoder, then scan a game root containing `disc1` through `disc4`. Keep build and report directories outside the game root; an ignored `out/` directory is recommended:

```powershell
python -B tools/asset_inventory/build.py --output out/asset-inventory-build
python -B tools/asset_inventory/inventory.py scan `
  --game "D:\LostOdyssey\game" `
  --output "out/asset-inventory" `
  --decoder "out/asset-inventory-build/asset_decoder.exe"
```

默认要求四张光盘，以便得到完整统计。`--allow-partial` 只适用于明确标注范围的局部扫描；局部结果不能当作完整资源清单。扫描会生成 `catalog.sqlite`、`summary.json`、`REPORT.md`、分类 CSV（超大分类可能是 `.csv.gz`）以及失败记录。完整数据库和 CSV 留在本地，不应提交到 Git。

The default requires all four discs for a complete census. Use `--allow-partial` only for an explicitly bounded partial scan. The scan writes `catalog.sqlite`, `summary.json`, `REPORT.md`, category CSV files (large categories may use `.csv.gz`), and failure records. Keep the full database and CSV files local; do not commit them.

## 已完成快照 / Completed snapshot

以下数字来自四张光盘的完整扫描，source commit 为 `fe89910300b92a59006924987e8586ba157cc20c`，包解析定版时间为 `2026-10-02T19:58:52.608051Z`（UTC）。另有 1,062 个第四字节非零的 CPX 签名在 `2026-10-02T20:01:33Z` 完成核验，全部判定为 non-UE3 且无错误。

The following figures are from the complete four-disc scan at source commit `fe89910300b92a59006924987e8586ba157cc20c`. Package parsing was finalized at `2026-10-02T19:58:52.608051Z` UTC. An additional 1,062 non-zero-fourth-byte CPX signatures were checked at `2026-10-02T20:01:33Z`; all were classified as non-UE3 with zero errors.

| 项目 / Item | 数量 / Count | 口径 / Scope |
| --- | ---: | --- |
| FPI 文件 / FPI files | 53,103 | 四盘文件出现次数 / file occurrences across four discs |
| 目录记录 / Directory records | 2,318 | FPI 目录记录 / FPI directory records |
| 虚拟路径 / Virtual paths | 16,904 | 去重路径 / distinct paths |
| 完整条目内容 / Complete entry contents | 16,203 | SHA-256 去重 / unique complete payload hashes |
| UE3 包光盘条目 / UE3 package disc entries | 25,687 | 文件出现次数 / package occurrences |
| UE3 包内容 / UE3 package contents | 9,134 | 完整包内容去重 / unique package contents |
| 包内 exports / Package exports | 1,624,436 | 按完整包内容和 export 索引去重；含组件、默认模板等 / unique package-content exports, including components and templates |
| Models | 51,862 | `StaticMeshRPG` 50,898；`StaticMesh` 349；`SkeletalMesh` 615 |
| Textures | 29,457 | 纹理类 exports；其中 `Texture2D`、`LightMapTexture2D`、`ShadowMapTexture2D` 的宽高和 format 全部解析 |
| Materials | 31,635 | UE3 material exports |
| Fonts | 73 | UE3 font exports |
| Animations | 12,981 | `AnimSequence` 10,912；`AnimSet` 1,207；`FaceFXAnimSet` 862 |
| Animation graphs | 1,878 | 单独记录的动画图 / separately counted animation graphs |
| Particle systems | 4,161 | `ParticleSystem`；不按模块对象重复计数 |
| UI class objects | 1,213 | UI export classes |
| UI usage candidates | 4,372 | 用途提示，和 textures 等类别重叠；含 215 个默认模板 |
| DDS | 51 unique / 204 entries | 独立纹理文件 / standalone texture files |
| XWV audio | 958 unique / 1,527 entries | 音频流 / audio streams |
| XSE audio | 1,520 unique / 6,476 entries | 音频 bank / audio banks |
| Video | 66 unique / 72 entries | WMV 文件 / WMV files |
| JMD files | 1,734 unique / 8,924 entries | 消息和对白文件，按后缀统计 / message and dialogue files, counted by extension |
| Subtitles | 1,118 unique / 4,548 entries | 字幕文件 / subtitle files |
| BIN files | 703 unique / 2,860 entries | 含事件脚本及其他二进制数据；按后缀统计 / event scripts and other binary data, counted by extension |

`StaticMeshRPG` 是游戏专属 resource class；统计确认其 export 数量，但尚未验证网格几何解码。读取类定义确认 `SkeletalMesh_VC` 的 `super_ref` 指向 `fpPawn`，因此归入 `scene_actors`，不计入 models。当前有 1,933 个对象名不满足 Mod key 语法；这些条目保留 `key_error`，这不是包解析失败。

`StaticMeshRPG` is a game-specific resource class; its export count is confirmed, but mesh geometry decoding is not. Reading the class definition confirms that `SkeletalMesh_VC` has `super_ref` pointing to `fpPawn`, so it belongs to `scene_actors`. There are 1,933 object names that do not satisfy the current Mod key grammar; they retain `key_error`, which is separate from package parse failure.

更新 decoder 后，可以对已登记的 UE3 候选执行完整 payload 哈希和元数据复核，然后在新目录生成报告：

After updating the decoder, revalidate registered UE3 candidates and regenerate a report in a new directory:

```powershell
python -B tools/asset_inventory/inventory.py reparse `
  --db "out/asset-inventory/catalog.sqlite" `
  --decoder "out/asset-inventory-build/asset_decoder.exe"
python -B tools/asset_inventory/inventory.py report `
  --db "out/asset-inventory/catalog.sqlite" `
  --output "out/asset-inventory-report-2"
```

查询 SQLite 目录中的模型、贴图或 UI 候选：

Query model, texture, or UI candidates from the SQLite catalog:

```powershell
python -B tools/asset_inventory/inventory.py query --db "out/asset-inventory/catalog.sqlite" --category models --limit 30
python -B tools/asset_inventory/inventory.py query --db "out/asset-inventory/catalog.sqlite" --class-name Texture2D --limit 50
python -B tools/asset_inventory/inventory.py query --db "out/asset-inventory/catalog.sqlite" --search UI_MAIN_00 --ui
```

## 分类和统计口径 / Categories and counting

分类以 UE3 export class 为主，包括 `models`、`scene_actors`、`world_geometry`、`textures`、`fonts`、`materials`、`animations`、`effects`、`audio`、`ui_objects`、`sequences`、`script_objects`、`shader_caches`、`worlds_levels`、`default_templates` 等。电影、声音流、字幕、摄像机、事件脚本和本地化数据等非 UE3 文件按文件级用途单独记录。

Categories are based on UE3 export classes and include models, scene actors, world geometry, textures, fonts, materials, animations, effects, audio, UI objects, sequences, script objects, shader caches, levels/worlds, and default templates. Movies, sound streams, subtitles, camera data, event scripts, and localization data are recorded separately as file-level kinds.

报告同时区分光盘文件出现次数、不同虚拟路径、完整 payload 去重数、exports 数量和路径/内容/export 变体。Imports 是依赖引用，不计入资源数量。相同 Mod key 的不同完整包内容会保留为多个变体，选择时应结合 disc、path 和 SHA-256。

The report distinguishes disc file occurrences, distinct virtual paths, complete payloads, exports, and path/content/export variants. Imports are dependency references and are not counted as assets. Different complete package contents sharing a Mod key are retained as variants; select them using disc, path, and SHA-256.

UI 标签是额外的用途提示，来自 export class 或路径/名称启发式判断，不代表当前运行时一定能替换。当前 Mod API 只接入原生设置菜单的 `UI_MAIN_00` 图集和特定字体纹理页；任意 guest texture、模型、字体指标、电影及其他资源仍需要对应 runtime consumer。对象名包含当前 API 不支持的字符时会保留 `key_error` 并清空 `mod_key`，工具不会猜测可用 key。

The UI label is an additional usage hint from export classes or path/name heuristics; it does not prove runtime replacement support. The current Mod API wires only the native settings `UI_MAIN_00` atlas and selected font texture pages. Arbitrary guest textures, models, font metrics, movies, and other resources still require a runtime consumer. Names containing characters unsupported by the current API retain `key_error` and an empty `mod_key`; the tool does not guess a key.

FPD offset 是原始归档 extent，不能与解包后的 UE3 object offset 混用。`Default__*` 模板单独列为 `default_templates`；`Model`/`Terrain` 列为 `world_geometry`；`StaticMeshRPG` 列为 models，`SkeletalMesh_VC` 列为 `scene_actors`。

An FPD offset identifies the original archive extent and must not be confused with an unpacked UE3 object offset. `Default__*` templates are kept in `default_templates`; `Model` and `Terrain` are `world_geometry`; `StaticMeshRPG` is classified as models, while `SkeletalMesh_VC` is classified as `scene_actors`.

统计结果应与对应扫描的 source commit、光盘集合和完成时间一起引用。统计完成不等于玩家可替换、跨平台可用或完整通关验证。

Always cite the source commit, disc set, and completion time with a report. An inventory does not establish player-visible replacement support, cross-platform compatibility, or complete-playthrough validation.
