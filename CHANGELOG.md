# Changelog / 更新日志

Brief release highlights, newest first. Dates are UTC. Technical validation is recorded in [development status](docs/STATUS.md); future plans are in the [roadmap](docs/ROADMAP.md).

按新到旧记录简短更新，日期采用 UTC。技术验证见[开发状态](docs/STATUS.md)，后续计划见[路线图](docs/ROADMAP.zh-CN.md)。

## Unreleased

### English

- Fixed character and object shadows shaking and flickering on AMD GPUs, including the Steam Deck, most visible in towns and cutscenes (#176).
- The shader packs change with this fix; the first start after updating offers to download the new packs.
- Android on Qualcomm devices no longer closes on every start when the selected GPU driver cannot run the game, such as the Adreno 650's own driver in the Retroid Pocket 5: the GPU driver page opens with the reason, and a Turnip driver can be picked there. Other devices show the reason instead of closing silently. Qualcomm drivers are also no longer turned down over a missing queue flag (#185).

### 简体中文

- 修复 AMD 显卡（包括 Steam Deck）上角色和物体阴影抖动、闪烁的问题，在城镇和过场动画中最明显（#176）。
- 着色器包随此修复更新，更新后第一次启动会提示下载新的着色器包。
- 高通设备上所选 GPU 驱动无法运行游戏时（例如 Retroid Pocket 5 中 Adreno 650 自带的驱动），Android 版不再每次启动都直接退出，而是打开 GPU driver 页面并说明原因，可在那里改选 Turnip 驱动；其他设备也会显示原因，不再无提示地关闭；高通驱动也不再因为缺少一个队列标志而被拒绝（#185）。

## [v0.8.7](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.7) — 2026-10-03

### English

- Android now checks for updates at startup and offers the new APK for download.
- Android now creates the `game` folder as soon as the app is opened, also on devices that start on the GPU driver page.
- Android can now read the game from an SD card, or from any folder chosen on the new **Game folder** page (**CTRL → Game folder**).
- Android can now import the game on the device from disc images or extracted discs (**Game folder → Import disc images…**).
- Android now writes its logs and crash reports to `Android/data/io.github.freefrank.lostodyssey/files/logs/`, where a PC can copy them over USB.
- The MetalFX settings texts are now translated into Japanese, Korean and Simplified Chinese.

### 简体中文

- Android 现在启动时检查更新，并提供新版 APK 下载。
- Android 现在一打开应用就建好 `game` 目录，先进入 GPU driver 页面的设备也一样。
- Android 现在可以从 SD 卡读取游戏，也可以在新的 **Game folder** 页面（**CTRL → Game folder**）选择任意文件夹。
- Android 现在可以在设备上从光盘镜像或已解出的光盘导入游戏（**Game folder → Import disc images…**）。
- Android 现在把日志和崩溃报告写到 `Android/data/io.github.freefrank.lostodyssey/files/logs/`，电脑可通过 USB 复制。
- MetalFX 相关设置文字现在有日文、韩文和简体中文翻译。

## [v0.8.6](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.6) — 2026-10-03

### English

- Fixed the pillar platforms in the Lunar Palace (Great Ancient Ruins, disc 4) rising on their own after a random battle, which left the pillars floating and the puzzle stuck (#171).
- Removed the v0.7.35 change that blocked object interactions while a battle was starting (#114); it caused the Lunar Palace problem above.

### 简体中文

- 修复 Lunar Palace（第 4 张光盘的大古代遗迹）中放着柱子的升降台在随机战斗后自行升起、柱子悬空、谜题无法继续的问题（#171）。
- 撤销 v0.7.35 加入的“战斗开始时禁止与物体互动”改动（#114）；上面的 Lunar Palace 问题正是它造成的。

## [v0.8.5](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.5) — 2026-10-03

### English

- Fixed battles with dialogue pausing for a long time between lines, or getting stuck, at 90 and 120 FPS (#148, #173).
- Added a **No Random Encounters** switch to the F1 Debug Menu, below Save Anywhere. Story battles still happen.
- The release now includes the experimental Android arm64 APK, built together with the other packages.
- Android on Qualcomm devices can now use a Mesa Turnip Vulkan driver. The device's own driver leaves the highlighted menu row's text invisible; Turnip draws it. A **GPU driver** page opens before the first game start and from **CTRL → GPU driver** while playing: download a driver from the same sources as the Eden emulator, or install a zip, and pick it. Changing the driver restarts the game. The Android app now has a launcher icon.
- Continue now finds a save whenever its `save.bin` is intact, including a renamed or copied slot folder and a slot whose `.lo-content` was damaged by a crash (#175).
- Saves are now written to disk before the game moves on, so a crash or power loss right after saving no longer wipes them (#175).

### 简体中文

- 修复 90／120 FPS 下带台词的战斗在台词之间长时间停顿甚至卡住的问题（#148、#173）。
- F1 调试菜单在随时存档下方新增“不遇敌”开关，剧情战斗仍会发生。
- 本版本起，实验性的 Android arm64 APK 随发布一起提供，和其他安装包一起构建。
- 高通设备上的 Android 版现在可以使用 Mesa Turnip Vulkan 驱动。设备自带的驱动会让菜单光标所在行的文字消失，Turnip 能正常显示。首次启动游戏前会打开 **GPU driver** 页面，游戏中也可从 **CTRL → GPU driver** 进入：从和 Eden 模拟器相同的来源下载驱动，或从 zip 安装，然后选用。切换驱动会重启游戏。Android 应用现在有了启动图标。
- 只要 `save.bin` 完好，“继续游戏”现在都能找到存档，包括被改名或复制过来的存档文件夹，以及 `.lo-content` 因崩溃损坏的存档（#175）。
- 存档现在会先写入磁盘再继续游戏，存档后立刻崩溃或断电不会再把存档清空（#175）。

## [v0.8.0](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.0) — 2026-10-03

### English

- HDR now works with every anti-aliasing mode (FXAA, SMAA, TAA) and every upscaler (DLSS, FSR, MetalFX), and with DLSS frame generation on Vulkan.
- Added Graphics settings for shadow resolution (1×, 2×, 4×) and experimental ambient occlusion (Off, SSAO, GTAO).
- Vulkan and Metal now share one shader pack, `portable_vk.lospv`. The first start after updating offers to download the new packs.
- Fixed the sun and its glare flashing through cliffs while sailing (#118).
- Added support for 8BitDo controllers such as the Ultimate 2 Wireless (#97, contributed by Xarishark).
- More Settings texts are translated into Japanese, Korean and Simplified Chinese.
- On Windows, a broken `OptiScaler.dll` no longer shows an error dialog at startup.
- The macOS app now requires macOS 15 or later.
- An experimental Android arm64 APK was added to this release after publication (see the Android section of the README).

### 简体中文

- HDR 现在可以和任一抗锯齿模式（FXAA、SMAA、TAA）、任一超分（DLSS、FSR、MetalFX）以及 Vulkan 上的 DLSS 插帧同时开启。
- 画面设置新增阴影分辨率（1×、2×、4×）和实验性环境光遮蔽（关闭、SSAO、GTAO）。
- Vulkan 和 Metal 现在共用一个着色器包 `portable_vk.lospv`。更新后第一次启动会提示下载新的着色器包。
- 修复开船时太阳和光晕穿过悬崖闪出来的问题（#118）。
- 支持 8BitDo 手柄，例如 Ultimate 2 Wireless（#97，由 Xarishark 贡献）。
- 设置菜单又有一批文字翻译为日语、韩语和简体中文。
- Windows 上，损坏的 `OptiScaler.dll` 不再在启动时弹出错误对话框。
- macOS 版本现在需要 macOS 15 或更高版本。
- 发布后补充上传了实验性的 Android arm64 APK（见 README 的 Android 一节）。

## [v0.7.35](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.35) — 2026-10-02

### English

- Added the first macOS package (Apple Silicon, Metal, experimental).
- Shaders compiled on your PC are now stored in one file per renderer, and the game offers to download precompiled shaders at startup.
- Added experimental HDR output for Windows, Linux and macOS.
- Screens taller than 16:9 (16:10, 3:2, 4:3) now show the 3D scene across the whole screen without bars.
- Direct3D 12 and Vulkan now run the game's occlusion queries, so effects like the sun's lens flare follow real visibility (#118).
- Vulkan DLSS frame generation now works like Direct3D 12; added experimental Vulkan FSR 3.1 and MetalFX frame generation.
- Fixed flicker in several skies, caves and cutscenes with TAA, FSR or DLSS (#121).
- Fixed the battle camera jumping at 90 and 120 FPS (#117) and DLSS being unavailable on NVIDIA GPUs in the Linux AppImage and Flatpak packages (#116).

### 简体中文

- 新增首个 macOS 安装包（Apple Silicon，Metal，实验性）。
- 本机编译的着色器现在每个渲染器只存一个文件，游戏也会在启动时询问是否下载预编译着色器。
- 新增 Windows、Linux 和 macOS 的实验性 HDR 输出。
- 比 16:9 更高的屏幕（16:10、3:2、4:3）现在让 3D 画面铺满整个屏幕，不再加黑边。
- Direct3D 12 和 Vulkan 现在会执行游戏的遮挡查询，太阳镜头光晕等效果会跟随真实可见度（#118）。
- Vulkan 的 DLSS 插帧现在与 Direct3D 12 一致；新增实验性的 Vulkan FSR 3.1 和 MetalFX 插帧。
- 修复多处天空、洞窟和过场动画在 TAA、FSR 或 DLSS 下的闪烁（#121）。
- 修复 90 和 120 FPS 下战斗镜头跳动（#117），以及 Linux AppImage 和 Flatpak 包在 NVIDIA 显卡上 DLSS 不可用的问题（#116）。

## [v0.7.25](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.25) — 2026-10-01

### English

- Added an experimental Apple Silicon macOS/Metal build.
- Added a Force RB Party Switch button to the F1 debug menu (#74); Save Anywhere is now off while the party is split.
- Fixed sky flicker with TAA and FSR in the Legacy of the Eastern Tribe area (#102).
- The Linux Flatpak update notice now explains how to install the new bundle.

### 简体中文

- 加入 Apple Silicon macOS／Metal 实验性构建。
- F1 调试菜单新增“强制开启 RB 换人”按钮（#74）；分队期间随时存档不再生效。
- 修复“东方部族的遗产”区域开启 TAA 或 FSR 时天空闪烁（#102）。
- Linux 的 Flatpak 更新提示现在会说明如何安装新的安装包。

## [v0.7.20](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.20) — 2026-09-30

### English

- Frame generation multiplier is capped at 6× (2× to 6×).
- Fixed a quit-to-desktop hang on Windows (#82).
- Improved performance on the default render path.
- Textures now load their mip chains (#87), which improves distant surfaces.

### 简体中文

- 插帧倍率上限为 6×（2× 到 6×）。
- 修复 Windows 上退出到桌面时卡死的问题（#82）。
- 提升默认渲染路径的性能。
- 纹理现在会加载 mip 链（#87），改善远处表面的显示。

## [v0.7.15 — 2026-09-29](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.15)

### English

- Added native 90/120 FPS targets and FreeSync / G-SYNC Compatible VRR pacing.
- Added an optional RGB Range expansion and a Hold/Toggle speed mode in F1 Cheats.
- Fixed the Hungry Man errand timer at higher frame rates.
- Portable installs now prefer the `game` directory beside the executable.

### 简体中文

- 新增原生 90／120 FPS 目标和 FreeSync／G-SYNC Compatible VRR 节奏控制。
- 新增可选的 RGB Range 扩展，以及 F1 Cheats 的按住／切换变速模式。
- 修复高帧率下 Hungry Man 差事计时。
- 便携式安装现在优先使用可执行文件旁的 `game` 目录。

## [v0.7.10 — 2026-09-28](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.10)

### English

- Updated the bundled Vulkan shaders and added a separate DX12 shader pack.

### 简体中文

- 更新内置 Vulkan 着色器，并新增独立的 DX12 着色器包。

## v0.7.9 — 2026-09-28

### English

- Added Windows D3D12 frame generation (Off/DLSS/FSR) in Graphics.
- Fixed Ubuntu 22.04 AppImage compatibility and improved Flatpak packaging.
- Simplified Windows updates, with rollback on failure.

### 简体中文

- 图像设置新增 Windows D3D12 插帧（关／DLSS／FSR）。
- 修复 Ubuntu 22.04 AppImage 兼容性，改进 Flatpak 打包。
- 简化 Windows 更新流程，失败时回滚。

## v0.7.3 — 2026-09-27

### English

- Reduced redundant D3D12 bindings for better performance.

### 简体中文

- 减少 D3D12 中的重复绑定以提升性能。

## v0.7.2 — 2026-09-27

### English

- Added experimental DLSS/DLAA and FSR upscaling on Windows D3D12.
- Fixed wrong DLAA resolution reporting.

### 简体中文

- Windows D3D12 新增实验性 DLSS／DLAA 与 FSR 超分。
- 修复 DLAA 报告分辨率错误的问题。

## v0.7.1 — 2026-09-26

### English

- Added official standalone Linux Flatpak packages and better PipeWire audio compatibility.
- Added in-game disc/DLC re-import with rollback on failure.

### 简体中文

- 新增官方 Linux Flatpak 独立安装包，改善 PipeWire 音频兼容性。
- 支持在游戏内重新导入光盘／DLC，失败时回滚。

## v0.7.0 — 2026-09-26

### English

- Added automatic PlayStation controller prompts in menus.
- Improved Vulkan DLSS/DLAA and FSR, with SMAA fallback when DLSS/DLAA is unavailable.
- Added the C++ Mod API and menu image/font-page replacement tools.

### 简体中文

- 菜单支持自动切换 PlayStation 手柄提示。
- 改进 Vulkan DLSS／DLAA 与 FSR，DLSS／DLAA 不可用时回退至 SMAA。
- 新增 C++ Mod API 和菜单图像／字体页面替换工具。

## v0.6.20 — 2026-09-25

### English

- Reduced flicker with TAA, FSR and DLSS in more scenes, including the exploration bridge.

### 简体中文

- 减少更多场景（包括探索场景桥面）在 TAA、FSR 和 DLSS 下的闪烁。

## v0.6.19 — 2026-09-25

### English

- Added live anisotropic filtering (Off to 16x) in Graphics, applied on save without a restart.
- Reordered DLSS and FSR quality choices and streamlined the settings menu.
- System menu now has "Quit to Desktop", and Settings > Game has "Quit to Main Menu".
- The debug Save Anywhere toggle is now remembered across sessions (#61).
- The mouse cursor now hides after 2 seconds of inactivity (#50); controller rumble is on by default; the Cheats sidebar uses LT/RT to switch categories.

### 简体中文

- 图形设置新增实时各向异性过滤（Off 到 16x），保存后生效，无需重启。
- 调整 DLSS 与 FSR 画质选项顺序，并精简设置菜单。
- 系统菜单新增“退出到桌面”，设置→游戏页新增“退出到主菜单”。
- 调试菜单的随时存档开关现在跨会话保存（#61）。
- 鼠标停顿 2 秒后自动隐藏（#50）；手柄震动默认开启；作弊菜单侧栏用 LT／RT 切换分类。

## v0.6.15 — 2026-09-24

### English

- Added experimental opt-in FSR upscaling on Windows and native Linux.
- Fixed screenshots showing swapped red and blue colors.
- Fixed DLSS SR and DLAA temporal history resetting spuriously after frame gaps.
- Fixed a crash when switching from DLSS Quality to DLAA.
- The DLSS menu status now shows `Active` only when DLSS output is really in use, with specific fallback reasons.

### 简体中文

- 增加实验性可选 FSR 超分，支持 Windows 与原生 Linux。
- 修复截图红蓝通道颠倒的问题。
- 修复 DLSS SR 与 DLAA 在帧间隔后时序历史被异常重置的问题。
- 修复从 DLSS Quality 切换到 DLAA 时的崩溃。
- DLSS 菜单状态现在仅在 DLSS 输出真正生效时显示为 `Active`，并给出具体回退原因。

## v0.6.11 — 2026-09-22

### English

- Added experimental native NVIDIA DLSS Super Resolution and DLAA, with NGX libraries included in the Windows and Linux packages.
- Settings -> Graphics now has **Upscaler** and **DLSS quality** (`Quality`, `Balanced`, `Performance`, `DLAA`) options.
- Removed the **Internal resolution** row from the menu.
- Menu lists longer than 11 rows now scroll.
- On the Graphics and Language tabs, Start (or Enter) jumps to **Save settings** without saving.

### 简体中文

- 增加实验性原生 NVIDIA DLSS 超分辨率与 DLAA，Windows 与 Linux 发布包内置 NGX 运行库。
- “设置” -> “图形”增加**缩放技术**与 **DLSS 质量**（质量、平衡、性能、DLAA）选项。
- 从菜单中移除“内部分辨率”行。
- 超过 11 行的菜单列表现在可滚动。
- 在“图形”与“语言”分页中，按 Start（或 Enter）跳到“保存设置”行而不保存。

## v0.6.7 — 2026-09-20

### English

- Added a **Widescreen** switch to Settings -> Graphics (Issue #17).
- Added 21:9 output resolution presets: 1720×720, 2560×1080, 3440×1440, 3840×1600 and 5120×2160.
- First-launch setup now offers the matching resolution presets.

### 简体中文

- “设置” -> “图形”增加**宽屏**开关（Issue #17）。
- 增加 21:9 输出分辨率预设：1720×720、2560×1080、3440×1440、3840×1600 与 5120×2160。
- 首次启动设置向导现在提供对应的分辨率预设。

## v0.6.6 — 2026-09-20

### English

- Added experimental native ultrawide (21:9) support (Issue #17); only 3440×1440 is supported.
- Fixed shadows at all aspect ratios and high internal resolutions.
- v0.6.6 was reissued on 2026-09-20 with the shadow fix; redownload it if you got the first package.
- On Linux, the AppImage updater now removes the old AppImage after a successful update and keeps it if the new one fails to run.

### 简体中文

- 增加实验性原生超宽屏（21:9）支持（Issue #17）；目前仅支持 3440×1440。
- 修复所有显示比例与高内部分辨率下的阴影问题。
- v0.6.6 已于 2026-09-20 重新发布以包含阴影修复；如果下载的是初版，请重新下载。
- Linux 上 AppImage 更新器在更新成功后删除旧版 AppImage，新版本无法运行时则保留旧版。

## v0.6.3 — 2026-09-19

### English

- Improved CPU performance during drawing by using sampled vertex-cache comparison for large buffers.
- Hardened the language menu (Issue #54): invalid language entries no longer change your selection.
- Fixed a possible file I/O locking problem (Issue #53).
- F1 render-state exports are now `.zip` on Windows and `.tar.gz` on Linux.

### 简体中文

- 对大缓冲使用采样式顶点缓存比较，降低绘制时的 CPU 开销。
- 加固语言菜单（Issue #54）：无效的语言条目不再改变你的选择。
- 修复一处可能的文件 I/O 锁问题（Issue #53）。
- F1 渲染状态导出在 Windows 上为 `.zip`，在 Linux 上为 `.tar.gz`。

## v0.6.2 — 2026-09-19

### English

- Applied the accepted TAA settings to the normal TAA path.
- Experimental motion-vector replay for TAA is now on by default; `LO_MV_ENABLE=0` turns it off.
- Improved performance with an index cache that reuses index fingerprints.

### 简体中文

- 将已接受的 TAA 设置应用到正常 TAA 路径。
- TAA 默认启用实验性运动矢量 replay，可用 `LO_MV_ENABLE=0` 关闭。
- 通过复用 index fingerprint 的 index cache 提升性能。

## v0.6.1 — 2026-09-18 / Published / 已发布

### English

- Windows and Linux now check for updates before importing game data.
- When a newer release exists, a prompt shows its release notes with **Install** and **Later**, and accepting updates and relaunches before import.

### 简体中文

- Windows 和 Linux 现在会在导入游戏资料前检查更新。
- 发现新版本时，提示显示发布说明以及“安装”和“稍后”，接受后在导入前更新并重新启动。

## v0.6.0 — 2026-09-18 / Published / 已发布

### English

- Fixed shader failures being treated as permanent, and shader/PSO preparation can now be cancelled.
- Fixed Linux updates so the previous AppImage is restored if the new one cannot start.
- Importer only publishes imported data after it was fully written, and the destination browser can create a new folder (button, `F2` or controller `Y`).

### 简体中文

- 修复 shader 暂时失败被当作永久失败的问题，并可取消 shader／PSO 准备。
- 修复 Linux 更新：新版本无法启动时恢复旧版 AppImage。
- 导入器仅在数据完整写入后才发布，目标目录页支持新建文件夹（按钮、`F2` 或手柄 `Y`）。

## v0.5.20 — 2026-09-17 / Published / 已发布

### English

- Fixed black light fixtures in Numara Castle's Philosopher's Chamber (Issue #38).
- Fixed TAA flicker on the stairs and save point in `f2358`.
- Added a portable Vulkan shader pack (`.lospv`) so the game starts without compiling shaders.
- Faster shader prebuilding, with `LO_SHADER_WORKERS` and `LO_PIPELINE_WORKERS` to set worker counts.
- The shader preparation screen can be skipped with ESC, Space or Controller B; `skip_shader_prebuild` in `settings.ini` disables it.
- Fixed debug overlay problems: pausing no longer hangs the game, close buttons no longer leak into the game, and Linux shows only the Vulkan backend.

### 简体中文

- 修复努玛拉城哲学者之间的黑光灯具问题（Issue #38）。
- 修复 `f2358` 场景阶梯与保存点处的 TAA 闪烁。
- 增加便携式 Vulkan 着色器包（`.lospv`），启动时无需编译着色器。
- 加快着色器预构建，可用 `LO_SHADER_WORKERS` 与 `LO_PIPELINE_WORKERS` 指定工作线程数。
- 着色器准备界面可按 ESC、空格键或手柄 B 键跳过；`settings.ini` 的 `skip_shader_prebuild` 可关闭该步骤。
- 修复调试浮层问题：暂停不再卡死游戏，关闭按键不再泄漏到游戏，Linux 仅显示 Vulkan 后端。

## Historical development checkpoints / 历史开发检查点

### English

- Added an in-game debug overlay (F1 or LB+RB) and a software-rendered settings menu that no longer depends on Windows GDI.
- Added Linux installer, updater and XDG user-path support; merged the importer and updater into `LostOdysseyRecomp.exe`; the SDL installer now imports discs and DLC together, with controller navigation.

### 简体中文

- 新增游戏内调试浮层（F1 或 LB+RB），设置菜单改为软件渲染，不再依赖 Windows GDI。
- 新增 Linux 安装器、更新器与 XDG 用户路径支持；导入器与更新器合并进 `LostOdysseyRecomp.exe`；SDL 安装器可一并导入光盘与 DLC，并支持手柄导航。

## v0.5.14 — 2026-09-16 / Published / 已发布

### English

- The main binary now includes the installer and updater, with safer DLC import and more reliable scan and retry.
- Added a Linux x64 AppImage alongside the Windows x64 ZIP.
- Replaced the Linux AppImage with one that includes the missing `AppRun`, fixing the launch error.

### 简体中文

- 主程序现已内置安装器与更新器，DLC 导入更安全，扫描与重试更可靠。
- 新增 Linux x64 AppImage，与 Windows x64 ZIP 并列提供。
- 已替换缺少 `AppRun` 的 Linux AppImage，修复启动报错。

## v0.5.13 — 2026-09-14 / Published / 已发布

### English

- Added **Alt+Enter** to switch between **Windowed** and **Borderless** modes.

### 简体中文

- 新增 **Alt+Enter**，在 **Windowed** 与 **Borderless** 模式间切换。

## v0.5.12 — 2026-09-14 / Published / 已发布

### English

- Fixed USA/Europe FMV and event subtitles showing English in other languages. Issue [#27](https://github.com/freefrank/LostOdysseyRecomp/issues/27).

### 简体中文

- 修复 USA/Europe FMV 与事件字幕在其他语言下显示英文的问题。[#27](https://github.com/freefrank/LostOdysseyRecomp/issues/27)。

## v0.5.11 — 2026-09-14 / Published / 已发布

### English

- Added startup and GPU failure diagnostics to help investigate Issues [#6](https://github.com/freefrank/LostOdysseyRecomp/issues/6) and [#22](https://github.com/freefrank/LostOdysseyRecomp/issues/22).

### 简体中文

- 增加启动与 GPU 失败诊断，用于排查 [#6](https://github.com/freefrank/LostOdysseyRecomp/issues/6) 与 [#22](https://github.com/freefrank/LostOdysseyRecomp/issues/22)。

## v0.5.10 — 2026-09-13 / Published / 已发布

### English

- Fixed more Vulkan TAA flicker cases.

### 简体中文

- 修复更多 Vulkan TAA 闪烁情况。

## v0.5.9 — 2026-09-13 / Published / 已发布

### English

- Improved Vulkan performance with depth-clear coalescing and fewer redundant descriptor bindings.

### 简体中文

- 通过合并深度清除并减少重复 descriptor 绑定，提升 Vulkan 性能。

## v0.5.8 — 2026-09-13 / Published / 已发布

### English

- Extended TAA jitter coverage to more main-camera shaders.

### 简体中文

- 扩展 TAA jitter 对更多主相机 shader 的覆盖。

## v0.5.7 — 2026-09-13 / Published / 已发布

### English

- The updater can recover from an updater-only install, bad local metadata or a missing game executable, then asks whether to launch the game (default **No**).
- Fixed lighting flicker in the reported scene with TAA and added a bloom prefilter fix.

### 简体中文

- 更新器可从仅有更新器的安装、损坏的本地 metadata 或缺少游戏可执行文件的状态恢复，随后询问是否启动游戏（默认**否**）。
- 修复报告场景中 TAA 下的光影闪烁，并加入 bloom prefilter 修复。

## v0.5.6-hotfix1 development record / 开发记录

This development record was never released separately; its scope was included in v0.5.7. / 本开发记录从未单独发布；其范围已纳入 v0.5.7。

### English

- Updater recovery and TAA flicker fixes later released in v0.5.7.

### 简体中文

- 更新器恢复与 TAA 闪烁修复，后于 v0.5.7 发布。

## v0.5.6 — 2026-09-13 / 已发布

### English

- Issue #16: fixed the freeze in the `xf_shd_aniflz.freeze` particle-material case.
- Reduced stutter in D3D12 and the renderer.

### 简体中文

- Issue #16：修复 `xf_shd_aniflz.freeze` particle-material 情况下的卡死。
- 降低 D3D12 与渲染器卡顿。

## v0.5.4 — 2026-09-11

### English

- Fixed installer window dragging.

### 简体中文

- 修复安装器窗口拖动。

## v0.5.3 — 2026-09-10

### English

- Fixed updater handling of ZIP packages with a root-directory entry.

### 简体中文

- 修复更新器处理带根目录条目的 ZIP 包。

## v0.5.2 — 2026-09-10

### English

- Manual F1 render captures now include the original shader microcode.

### 简体中文

- 手动 F1 渲染捕获现附带原始 shader 微码。

## v0.5.1 — 2026-09-10

### English

- Double-click `LostOdysseyUpdater.exe` beside the game to update without launching the game; it starts the game after a successful update.

### 简体中文

- 在游戏旁双击 `LostOdysseyUpdater.exe` 即可更新，无需先启动游戏；更新成功后会启动游戏。

## v0.5.0 — 2026-09-09

### English

- Added Windows Vulkan alongside D3D12, with automatic fallback.
- Discs and DLC are recognized automatically from files, folders or mixed selections; the game starts directly from the executable.
- Refreshed the installer, updater, first-run setup and Debug Menu; graphics settings save in one click, with Now/Later when a restart is needed.
- Fixed the Issue #12 crash and DLC directory filtering; Xenia saves can be copied directly to Recomp; added DPI-correct window sizing and Alt+Enter fullscreen switching.

### 简体中文

- 新增 Windows Vulkan，与 D3D12 并存，支持自动回退。
- 从文件、文件夹或混合选择中自动识别光盘与 DLC；可直接运行游戏程序。
- 改进安装器、更新器、首次设置和 Debug Menu；图形设置单击保存，需要重启时可选“现在”或“稍后”。
- 修复 Issue #12 的崩溃与 DLC 目录过滤；支持直接复制 Xenia 存档到 Recomp；窗口按 DPI 正确确定大小，并加入 Alt+Enter 全屏切换。

## [v0.4.2](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.4.2) — 2026-09-08

### English

- Fixed the Uhra Council cutscene crash.
- Corrected several game logic bugs from PowerPC translation.
- Added battle TAA coverage; F1 captures now compress in the background.

### 简体中文

- 修复乌拉议会过场崩溃。
- 修正 PowerPC 翻译导致的若干游戏逻辑错误。
- 补充战斗 TAA 覆盖；F1 捕获现于后台压缩。

## Published / 已发布

### [v0.4.1](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.4.1) — 2026-09-08

#### English

- Fixed Map3 tire-shadow flicker with TAA enabled.
- F1 now captures three consecutive frames into one ZIP; `LO_DEBUG_CAPTURE_DRAW_STEPS=1` restores draw previews.

#### 简体中文

- 修复 Map3 轮胎在启用 TAA 时的阴影闪烁。
- F1 现连续捕获三帧合并为一个 ZIP；`LO_DEBUG_CAPTURE_DRAW_STEPS=1` 可恢复逐绘制预览。

### [v0.4.0](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.4.0) — 2026-09-07

#### English

- Added real internal resolution up to 4K (Auto, or manual 720p to 2160p), with preview and rollback.
- Added SMAA 1x and experimental TAA.
- Added saved 30/60 FPS controls; the 120 FPS option requires `LO_EXPERIMENTAL_120=1`.
- Faster shader discovery for both editions; `LO_SHADER_FULL_SCAN=1` forces a full rescan.
- Added Debug menu language switching; removed the unimplemented DLSS/frame-generation controls.

#### 简体中文

- 新增最高 4K 的真实内部分辨率（Auto 或手动 720p 至 2160p），支持预览与回退。
- 新增 SMAA 1x 与实验性 TAA。
- 新增可保存的 30／60 FPS 控制；120 FPS 选项需要 `LO_EXPERIMENTAL_120=1`。
- 两个版本的 shader 发现更快；`LO_SHADER_FULL_SCAN=1` 可强制完整重扫。
- 新增 Debug 菜单语言切换；移除尚未实现的 DLSS／帧生成控件。

### [v0.3.0](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.3.0) — 2026-09-07

- Shaders are discovered before gameplay and used pipelines are prepared in parallel on later launches. / 游戏开始前发现 shader，后续启动并行预创建用过的管线。

### [v0.2.2](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.2.2) — 2026-09-07

- Fixed black/dark title, background and depth-of-field output on AMD; preserved Unicode Windows startup and save paths. / 修复 AMD 上标题、背景与景深全黑／偏黑；保留 Windows Unicode 启动与存档路径。

### [v0.2.1](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.2.1) — 2026-09-06

- Added F1 render-state capture; controllers and keyboard combine into player 1 with hotplug. / 增加 F1 渲染状态捕获；手柄与键盘合并到玩家 1，支持热插拔。

### [v0.2](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.2) — 2026-09-06

- Added USA/Europe 0.0.0.3 support alongside Asian 0.0.0.4; text and voice choices follow the installed edition. / 增加欧美 0.0.0.3 支持，保留亚洲 0.0.0.4；文本与语音选项按安装版本提供。

### [v0.1](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.1) — 2026-09-06

- First experimental portable Windows x64 release with graphical importer and first-launch setup for the four-disc Asian edition. / 首个实验性便携 Windows x64 版本，含图形导入器及亚洲四盘版首次设置。
