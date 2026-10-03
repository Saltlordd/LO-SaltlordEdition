<div align="center">

<img src="assets/lost-odyssey-recomp.png" alt="Lost Odyssey Recomp 标志" width="112">

# Lost Odyssey Recomp

**《失落的奥德赛》Xbox 360 版的实验性原生 PC 移植。**

Windows x64 · Linux x64 · macOS arm64（实验性） · Direct3D 12 · Vulkan · Metal

### [下载](https://github.com/freefrank/LostOdysseyRecomp/releases/latest) · [安装指南](docs/INSTALLING.zh-CN.md) · [English](README.md)

[功能](#当前功能) · [操作按键](#操作按键) · [调试菜单](#调试菜单)

[更新日志](CHANGELOG.md) · [反馈问题](https://github.com/freefrank/LostOdysseyRecomp/issues) · [项目看板](https://github.com/users/freefrank/projects/3) · [从源码构建](docs/BUILDING.md)

</div>

> [!IMPORTANT]
> **本项目仍处于早期测试阶段。** 已测试开场区域和部分场景，尚未完整通关。渲染和稳定性仍有问题。请自行提供受支持版本的游戏文件。

## 开始游戏

从[最新发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/latest)选择对应平台的安装包。当前已发布版本为 **v0.8.6**。

| 平台 | 安装包 | 首次启动 |
| :--- | :--- | :--- |
| Windows x64 | `LostOdysseyRecomp-windows-x64-v0.8.6.zip` | 将完整 ZIP 解压到可写目录，运行 `LostOdysseyRecomp.exe`。需要支持 AVX 的 CPU；默认使用 Direct3D 12，也可选择 Vulkan。 |
| Linux x64 | `LostOdysseyRecomp-linux-x64-v0.8.6.AppImage` | 用 `chmod +x` 赋予执行权限后运行。使用 Vulkan。 |
| Linux x64 | `LostOdysseyRecomp-linux-x64-v0.8.6.flatpak` | 安装 Freedesktop 26.08 运行时，再安装下载的 bundle。见 [Flatpak 安装命令](docs/INSTALLING.zh-CN.md#flatpak)。使用 Vulkan。 |
| macOS arm64（实验性） | `LostOdysseyRecomp-macos-arm64-v0.8.6.dmg` | 打开磁盘映像，把 `LostOdysseyRecomp.app` 拖到 Applications 链接（即“应用程序”文件夹）。应用未经公证，macOS 会拦截首次启动：先尝试打开应用，再到“系统设置 → 隐私与安全性”点击 **仍要打开**。需要 macOS 15 或更高版本的 Apple Silicon Mac；游戏目前只在 macOS 26.6.2 上运行过。使用 Metal。见 [macOS 安装步骤](docs/INSTALLING.zh-CN.md#macos)。 |
| Android arm64（实验性） | `LostOdysseyRecomp-android-arm64-v0.8.6.apk` | 安装 APK（按提示允许来自浏览器或文件管理器的安装），先打开一次应用（它会建好 `Android/data/io.github.freefrank.lostodyssey/files/game/disc1`–`disc4`），用 USB 把解出的光盘复制到这些目录（当前源码也可以在 **Game folder** 页面改选其他文件夹），再打开应用。高通设备首次启动前会先显示 **GPU driver** 页面。需要支持 Vulkan 的 64 位 Android 8.0 及以上设备，使用 Vulkan。见 [Android 步骤](docs/INSTALLING.zh-CN.md#android)。 |

1. **导入游戏数据。** 未找到可用的游戏安装时会打开内置导入器。用 **Files** 或 **Folder** 选择已提取的游戏文件夹、`default.xex`、XDVDFS ISO 或 GOD 数据。
2. **选择界面语言、游戏语言和图形设置。** 完成设置和着色器预编译后进入游戏；如果没有装好所选渲染器的预编译着色器，游戏会先询问是否下载，选择跳过则在本机编译。后续启动会复用着色器缓存。
3. **按需追加其他光盘和 DLC。** 在普通设置中打开 **Gameplay → Import discs & DLC（导入光盘与 DLC）**。四张同版本光盘全部导入后，游戏会自动读取所需光盘。

启动需要 Disc 1。请使用已核对的亚洲多语言版或 USA/Europe 四盘套装，不要混装不同版本。[安装指南](docs/INSTALLING.zh-CN.md)介绍光盘识别、Linux 安装、文件位置和更新方法。发布包不需要 Python 或 Visual Studio；更新时请保留存档和个人配置。

### Apple Silicon macOS（实验性）

v0.7.35 是第一个带有 macOS 安装包的版本，之后是 v0.8.0、v0.8.5 和 v0.8.6：使用 Metal 渲染的实验性 arm64 路径。打开 `LostOdysseyRecomp-macos-arm64-v0.8.6.dmg`，把 `LostOdysseyRecomp.app` 拖到 Applications 链接（即“应用程序”文件夹）后启动。应用需要 macOS 15 或更高版本的 Apple Silicon Mac（游戏目前只在 macOS 26 上运行过）。应用仅做 ad-hoc 签名、未经公证，并且会一直如此：维护者于 2026-10-03 决定不做 Developer ID 签名和公证。因此 macOS 会拦截首次启动。请先尝试打开应用一次，再到“系统设置 → 隐私与安全性”，为它点击 **仍要打开** 并确认；也可以运行 `xattr -dr com.apple.quarantine /Applications/LostOdysseyRecomp.app`。具体步骤和文件位置见[安装指南](docs/INSTALLING.zh-CN.md#macos)。游戏内的更新检查只会提示打开发布页，替换应用需要自己动手。目前只在一台 Mac 上验证过：在维护者的 M1 Max（macOS 26.6.2）上，开场新游戏战斗用 Metal 运行，v0.7.35 时 Metal 着色器包已下载并使用；另外也用源码构建在该机器上跑过开启 GTAO 和 4× 阴影的开场战斗。长时间游玩、更广场景、其他 Mac、画质和性能尚未测试。需要自行构建时，请按[macOS 构建说明](docs/BUILDING.md#building-on-macos)。

### 实验性 HDR

v0.8.6 包含 Windows D3D12／Vulkan、Linux Vulkan 和 macOS Metal 的实验性 HDR 输出路径。在图像设置中开启 **HDR**，保存并重启。**HDR 最高亮度**会打开冻结的确定性对比帧：左侧是截断在参考白位的 SDR 亮度预览，右侧是正常 HDR tone mapping，并实时更新峰值。可用鼠标或手柄 **X** 键在 Scene 和 Test pattern 间切换；没有有效场景时使用标准图案。菜单打开时只复制一次源画面，不会每帧复制。自动模式优先使用当前显示器回报的峰值；无法获取时使用 1000 nit 内容参考值。当前 Linux Vulkan 路径无法取得显示器峰值回报，因此自动模式使用该参考值。macOS 的自动值根据 EDR 亮度余量估算，并非面板实测尼特值。峰值调整可在校准页预览；峰值和纸白更改保存后无需重启即可生效。自 v0.8.0 起，HDR 可以和任一抗锯齿模式（FXAA、SMAA 和 TAA 在 HDR 场景上运行）以及任一超分同时使用（DLSS、FSR 和 MetalFX 在 SDR 场景上超分，HDR 画面由超分后的画面加上游戏超分前的高光重建）。Vulkan 上的 DLSS 插帧保持 HDR（在 RTX 5080 上只通过 SDR 预览检查过，没有在 HDR 显示器上验证；`LO_HDR_FG=0` 可改为保留 SDR 交换链）。其他插帧路径（Vulkan 上的 FSR，以及 Direct3D 12 和 Metal 上的全部）保留 SDR 交换链。不支持的格式／色彩空间组合会回退到 SDR。维护者已于 2026-10-02 确认存在 HDR 实机验证，但未记录具体平台、后端和显示器范围，因此不代表跨平台覆盖。在 v0.7.35 的那次运行中，外接显示器没有 EDR 余量的 Mac 上请求 HDR 后输出仍保持 SDR，当时尚未见到 Metal 的 HDR 输出本身。跨平台语义和验证限制见 [HDR 技术说明](docs/notes/hdr-output.md)。

### Android ARM64 运行时（实验性）

实验性的 arm64 APK `LostOdysseyRecomp-android-arm64-v0.8.6.apk` 是 [v0.8.6 发布](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.6)的一部分：发布流程会和其他安装包一起构建它。它最初是在 v0.8.0 发布后手动补充上传的。它是开发构建（应用名显示为 "Lost Odyssey (development)"），不是普遍支持的 Android 版本。应用使用项目固定的 debug 签名并一直保持，因此每个版本都能覆盖安装上一个版本。运行时使用 SDL Android activity、应用专属 external storage、Android ARM64 FFmpeg 配置和 Android DXC 构建。工具链和当前验证边界见 [Android 构建说明](packaging/android/README.md) 与 [Android 移植研究记录](docs/notes/android-port-research-2026-10-02.md)。

**安装。** 装好 APK 后，把游戏数据放到应用读取的位置：可以直接在设备上导入（当前源码，v0.8.6 之后：**Game folder** 页面有 **Import disc images…** 按钮，运行和桌面版相同的导入器，来源可以是内部存储或 SD 卡上的 `.iso` 镜像或已解出的光盘；需要“所有文件访问”权限，默认导入到自定义游戏文件夹，没有设置则导入到下面的应用目录，完成后直接启动游戏），也可以先用任一桌面版的导入器生成 `game/disc1`–`disc4`（以及 `game/dlc/…`），再用 USB 文件传输模式把这个 `game` 目录复制到 `Android/data/io.github.freefrank.lostodyssey/files/`（应用一打开就会在内部存储和每张 SD 卡上建好 `game/disc1`–`disc4` 和一个 `README.txt`；四张光盘约 20 GB）。必须有第 1 张光盘。游戏读取第一个存在 `game/disc1/default.xex` 的存储（内部存储或 SD 卡），否则用内部存储。当前源码中，开启 **Automatic updates** 时应用启动会检查新版本，并显示和桌面版相同的更新提示（**Download (A)** / **Later (B)**）；接受后在浏览器里打开 APK 下载，再覆盖安装即可。下个版本发布。首次启动时高通设备会先显示 **GPU driver** 页面（见下文），然后游戏会提示下载 Vulkan 着色器包，用屏幕上的 **A** 键接受。设置在游戏内的设置页面修改。步骤见[安装指南](docs/INSTALLING.zh-CN.md#android)，Android 的目录见[文件与目录](#文件与目录)。

**日志。** 应用把日志写到 `Android/data/io.github.freefrank.lostodyssey/files/logs/`。报告闪退或黑屏时，先再打开一次应用，再用 USB 文件传输模式连接电脑，附上该目录中最新的 `runtime-*.log`、`native-stderr.log` 以及所有 `java-crash-*.txt`；详见 [Android 日志](docs/INSTALLING.zh-CN.md#android-logs)。

Android 运行时现在能在测试平板加载开发资源、播放开场视频并进入首战。触摸输入已通过标题／菜单导航和首战两次攻击，画面显示伤害；一次首战 shader 准备过程中观测到约 40 秒停顿。长时间游玩、超出 native 队列的音频、其他 GPU、16 KB 设备和实体手柄仍待验证。详见[Android 移植研究记录](docs/notes/android-port-research-2026-10-02.md)及 [Android DXC 构建记录](docs/notes/android-dxc-build-2026-10-02.md)。

运行时保留 SDL 实体手柄支持，并加入 Android 屏幕触摸手柄。源码实现现在支持 `Controller settings` 中的大小／透明度设置，以及可拖动、保存、重置和逐个显示或隐藏控件的 `Edit layout` 页面；布局模型检查和设备 UI 流程已在测试平板验证。这条源码路径尚未通过完整游戏流程验收。生成的 PPC 代码仍可作为独立的 Android ARM64/PIC 静态库目标构建。

高通设备上的专有 Vulkan 驱动会把菜单光标所在行的文字画成全透明。因此应用会在首次启动游戏前打开 **GPU driver** 页面（游戏中也可从 **CTRL → GPU driver** 进入），在那里可以从和 Eden 模拟器相同的来源下载 Mesa Turnip 驱动包、或从 zip 安装并选用；修复已在开发平板上用 KIMCHI Turnip v26.0.0 R8 验证。详见 [Android 说明](packaging/android/README.md)。

连接 USB 或蓝牙手柄后会自动隐藏触摸控件，并保留 `CTRL` 入口；打开 **Show touch controls** 可让两者同时使用，最后一只手柄断开后恢复已保存的触摸偏好。Android 也会隐藏不可用的桌面画面选项，并在从后台返回时重建 Vulkan Surface。Vulkan shader 可在电脑上按 Android 专用接口预编译，桌面 Vulkan bundle 与其不兼容；生成和安装方法见 [Android 说明](packaging/android/README.md)。

### 最新更新

[v0.8.6](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.6) 修复了 Lunar Palace（第 4 张光盘的大古代遗迹）中放着柱子的升降台在随机战斗后自行升起、柱子悬空、谜题无法继续的问题（#171）。原因是 v0.7.35 为 #114 加入的“战斗开始时禁止与物体互动”改动，现已撤销。完整列表和更早的版本见[更新日志](CHANGELOG.md)，其中包括 [v0.8.5](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.5) 的 90／120 FPS 战斗台词修复（#148、#173）、“不遇敌”开关和随发布提供的 Android APK，以及 [v0.8.0](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.0) 对所有抗锯齿模式和超分辨率的 HDR 支持；验证范围见[开发状态](docs/STATUS.md)。

## 当前功能

| 功能 | 可用选项与行为 |
| :--- | :--- |
| 导入与首次设置 | 支持文件夹、XEX、ISO、GOD 和受支持的 DLC，可替换所选光盘；首次启动前设置语言和图形选项。原始来源文件保持不变。 |
| 语言 | 界面提供英语、日语、韩语、繁体中文和简体中文。游戏语言取决于安装的版本。 |
| 显示与画质 | 16:9／21:9 分辨率预设、宽屏开关、Off／FXAA／SMAA／实验性 TAA、DLSS 或 FSR 3.1 超分、滤波和 RGB Range 选项。自 v0.7.35 起，比 16:9 更高的屏幕（例如 16:10、3:2 和 4:3）会铺满 3D 画面，菜单和电影保持 16:9 布局，较高屏幕上的 minimap 靠近顶部，超宽屏的菜单两侧加有黑边（见[技术说明](docs/notes/tall-aspect-layout.md)）。另有 Windows D3D12／Vulkan、Linux Vulkan 和 macOS Metal 的实验性 HDR 及自动／手动峰值校准，详见 [HDR 技术说明](docs/notes/hdr-output.md)；Linux HDR 实机验证仍待完成。在 Linux 上，v0.7.25 安装包在 NVIDIA 显卡上被报告无法使用 DLSS（[#116](https://github.com/freefrank/LostOdysseyRecomp/issues/116)）；v0.7.35 包含修复，尚未在 Linux 的 NVIDIA 显卡上运行过。 |
| 阴影与环境光遮蔽 | 阴影分辨率 1×／2×／4×（默认 1×）及实验性 AO Off／SSAO／GTAO（默认 Off）分别以 `shadow_resolution` 和 `ambient_occlusion` 写入 `settings.ini`。AO 是屏幕空间效果；场景覆盖、画质和性能仍需评估。见[技术说明](docs/notes/shadow-ambient-occlusion.md)。 |
| 帧率 | 30／60／90／120 FPS 目标，以及 FreeSync／G-SYNC Compatible VRR 控制。实际性能取决于场景和硬件。 |
| 帧生成 | Windows D3D12 提供关／DLSS／FSR、受支持的 DLSS 倍率和固定 2× FSR。保存后应用支持即时切换的选项；从 DLSS FG 切到 FSR FG 需要重启。自 v0.7.35 起，Windows Vulkan 上也提供关／DLSS（固定 2×–6×；启动时为关而之后开启，或切换提供者，需要重启），另有实验性的 Vulkan FSR 2×（仅限源码构建）和实验性的 macOS MetalFX 2×（尚未在 Mac 硬件上运行），见[技术笔记](docs/notes/vulkan-fg-fsr4-metalfx.md)。 |
| 普通设置 | 使用原版字体，长列表可滚动，提供保存并应用。在图像页按 **Start／Enter** 只把焦点移到 **Save（保存）**，还需确认该项才会保存。需要重启的选项提供 **Now／Later**。 |
| 着色器预编译 | 多线程编译、跳过和缓存复用。v0.7.25 安装包内的 Vulkan 着色器包与该版本运行时不匹配，因此其首次启动编译了全部着色器（16 线程 CPU 上约 3 分钟）。v0.7.35 安装包不再附带着色器包：如果没有装好与所选渲染器匹配的包，游戏会在启动时询问是否下载；选择跳过则改为在本机编译。自 v0.8.0 起，Vulkan（Windows、Linux）和 Metal（macOS）共用一个包 `portable_vk.lospv`，DirectX 12 仍用 `portable_dx12.lospd`；着色器 contract 变了，需要新的包，所以更新后第一次启动会提示下载（[详情](docs/PORTABLE_SHADER_PACK.md#startup-download)）。 |
| Mod | Mod API v1、LOTEX1／PNG 工具、原生菜单图集和字体纹理页替换，以及 PlayStation 按键提示。支持范围和安装方法见 [Mod 指南](docs/wiki/Modding.md)，资源清单见[资源统计](docs/wiki/Asset-Inventory.md)。 |
| 输入与工具 | SDL 已映射手柄、键盘输入和震动；英文／简体中文[调试菜单](#调试菜单)，提供画面捕获、同地图传送、快进和游戏数据修改。 |

全屏、混合 DPI 显示器、更广的超分场景、Linux 硬件、较高屏幕／超宽屏布局在更多硬件上的表现，以及后续光盘流程仍需更多测试。当前工作见[路线图](docs/ROADMAP.zh-CN.md)和[项目看板](https://github.com/users/freefrank/projects/3)。

## 操作按键

SDL 已映射手柄和键盘可以同时用于玩家 1。未映射的摇杆需要 SDL 手柄映射，详见[输入说明](docs/notes/controller-input.md)。

| 游戏操作 | 键盘 |
| :--- | :--- |
| Start／Back | Enter／Backspace |
| A／B／X／Y | Z／X／A／S |
| 十字键／左摇杆 | 方向键／I、J、K、L |
| 左／右肩键 | Q／W |
| 左／右扳机 | E／R |
| 调试菜单 | F1 |

Ring 操作用手柄**右扳机**或键盘 **R**。震动默认开启，设置 `LO_CONTROLLER_RUMBLE=0` 可关闭。

## 调试菜单

按 **F1**，或手柄 **LB+RB**（PlayStation 布局为 **L1+R1**）打开或关闭调试菜单。**菜单打开时游戏会暂停。** 菜单分为 **Overview（概览）**、**Teleport（传送）** 和 **Cheats（修改）** 三页，与普通 Settings 分开，使用键盘或手柄操作，不支持鼠标。

| 操作 | 键盘 | 手柄 |
| :--- | :--- | :--- |
| 选择项目 | ↑／↓ | 十字键上／下 |
| 修改数值 | ←／→ | 十字键左／右 |
| 确认 | Enter | A |
| 返回或关闭 | Esc | B |
| 上一页／下一页 | Q 或 Tab／E | LB／RB |
| 切换 Cheats 类别 | 选中类别行后按 ←／→ | LT／RT |
| 打开或关闭菜单 | F1 | LB+RB |

### Overview：捕获与游戏操作

**Overview** 显示当前地图名称和 ID，也提供菜单语言、**Capture render state（捕获渲染状态）**、**Save Anywhere（随时存档）**、**No Random Encounters（不遇敌）**。还可以请求将当前战斗判为胜利，或撤销尚未执行的判胜请求。

遇到画面问题时，选择 **Capture render state** 并确认，然后**关闭菜单，让渲染继续**。程序会捕获三帧，在后台将结果归档到 `captures/`。状态消息会显示绝对路径：Windows 为 `.zip`，Linux 为 `.tar.gz`。归档失败时会保留原始捕获目录。捕获内容包括截图、渲染数据、着色器和日志，分享前请检查内容。

**Save Anywhere** 会开放原作 **System → Save（系统 → 存档）** 操作。关闭调试菜单后，重新打开游戏的 System 菜单再存档。它不会另建一套快速存档。

**No Random Encounters** 会停止在场景中行走时触发的随机战斗。剧情战斗和其他脚本安排的战斗仍会发生。

> [!WARNING]
> **随时存档存在已知的队伍状态问题。** 分队探索后存档再读档，可能丢失 RB 切换角色的功能（[#74](https://github.com/freefrank/LostOdysseyRecomp/issues/74)）。请另外保留一份正常存档。自 v0.7.25 起，分队期间随时存档保持关闭；读取以前这类存档后，可用 F1 菜单中的“强制开启 RB 换人”按钮恢复换人。

### Teleport：当前地图内移动

**Teleport** 提供位置书签、X／Y／Z 坐标和步长编辑，以及当前地图可用的兴趣点（POI）。在坐标行按 **Enter** 选择 X、Y 或 Z，再按 **←／→** 按所选步长调整该轴。确认传送操作或某个兴趣点后，关闭菜单即可移动。场景变化会清除书签，并取消尚未执行的传送。

### Cheats：快进与游戏数据工具

**Cheats** 按用途分为六组：

| 类别 | 功能 |
| :--- | :--- |
| **Quick tools** | 快进模式和倍率、**Allow memory edits（允许内存修改）**、金币和 HP／MP 操作。 |
| **Characters** | 角色 HP／MP、0–99 的 EXP 数值和技能。EXP 输入框不是等级选择器。 |
| **Inventory** | 将物品和素材数量设为 1、10、50 或 99，或填充已知类别。显示未刷新时，可在游戏背包里执行整理。 |
| **Equipment** | 实验性的武器、指环和已有饰品槽修改。 |
| **Party** | 实验性的五槽队伍编成、前后排和场景角色控制。部分修改可能需要重新读档才会显示。 |
| **Developer** | 实验性的原版 **EDIT MENU** 入口。开启后关闭 F1，再按 **LT+RT**；退出编辑器后应关闭此选项。 |

快进不需要开启内存修改，目前需要使用手柄。**Hold（按住）** 模式下按住 **LT** 加速；**Toggle（切换）** 模式下，每按一次 LT 就切换加速开关。选择 **2×、3×、4×、6× 或 8×** 后，关闭调试菜单即可使用。菜单打开、窗口失焦或场景编辑器运行时会停止加速；**LT+RT** 不会触发加速。音频不做时间拉伸。

内存修改默认关闭。修改游戏数据前，先备份存档，并进入战斗外可控制角色的场景。开启 **Allow memory edits**，选择操作并确认 **Yes**。状态提示待执行时关闭 F1，让操作执行，再重新打开菜单查看结果。场景变化会取消尚未执行的修改。

菜单语言、Save Anywhere 和 No Random Encounters 选项写入 `settings.ini`；快进设置和内存修改许可在程序重启后重置。对游戏数值的修改可能随正常存档保存。

## 文件与目录

Windows ZIP 是**便携式**的，所有文件都留在解压目录里。Linux 下，如果程序所在目录可写（例如源码构建或解压后的 AppImage），同样按便携方式存放。AppImage、Flatpak 和 macOS `.app` 不能在程序旁写文件，因此使用当前用户的目录。

| 安装包 | 配置目录 | 数据目录 | 日志目录 |
| :--- | :--- | :--- | :--- |
| Windows ZIP | `LostOdysseyRecomp.exe` 所在目录 | 同左 | 同左 |
| Linux，程序目录可写 | 程序所在目录 | 同左 | 同左 |
| Linux AppImage | `~/.config/lost-odyssey-recomp/` | `~/.local/share/lost-odyssey-recomp/` | `~/.local/state/lost-odyssey-recomp/` |
| Linux Flatpak | `~/.var/app/io.github.freefrank.LostOdysseyRecomp/config/lost-odyssey-recomp/` | `~/.var/app/io.github.freefrank.LostOdysseyRecomp/data/`（沙盒内为 `/var/data`） | `~/.var/app/io.github.freefrank.LostOdysseyRecomp/.local/state/lost-odyssey-recomp/` |
| macOS `.app`（实验性） | `~/Library/Application Support/LostOdysseyRecomp/` | 同左 | `~/Library/Logs/LostOdysseyRecomp/` |
| Android APK（实验性） | 应用私有存储 `files/config/` | 应用私有存储 `files/` | 应用私有存储 `files/state/`；日志在 `Android/data/io.github.freefrank.lostodyssey/files/logs/` |

Linux 上可以用 `XDG_CONFIG_HOME`、`XDG_DATA_HOME` 或 `XDG_STATE_HOME` 改变 AppImage 使用的目录。

Android 的应用私有存储是 `/data/data/io.github.freefrank.lostodyssey/files/`，文件管理器看不到。游戏数据和日志是例外：它们在应用的外部文件目录里，电脑通过 USB 连接可以访问。应用从内部存储或 SD 卡上的 `Android/data/io.github.freefrank.lostodyssey/files/game/` 读取游戏数据（用第一个存在 `game/disc1/default.xex` 的），把日志写到 `Android/data/io.github.freefrank.lostodyssey/files/logs/`。当前源码（v0.8.6 之后）中，**Game folder** 页面（找不到游戏数据时自动打开，也可从 **CTRL → Game folder** 进入）可以改为指向内部存储或 SD 卡上的任意文件夹，游戏会先在那里查找，页面上的 **Import disc images…** 按钮可以直接导入到设备上。

| 内容 | 位置 | 说明 |
| :--- | :--- | :--- |
| 导入的游戏数据 | 数据目录的 `game/`，内含 `disc1/`–`disc4/` 和 `dlc/` | 导入器的默认目标，也可以导入到其他位置。Android：`Android/data/io.github.freefrank.lostodyssey/files/game/`（内部存储或 SD 卡），应用首次打开时建好，光盘需手动复制进去，或在 **Game folder** 页面用 **Import disc images…**（当前源码）。当前源码：在 **CTRL → Game folder** 选定的文件夹（含 `disc1`）优先使用，需要“所有文件访问”权限。 |
| 所选游戏目录 | `game-path.txt`：便携方式在程序旁，否则在配置目录 | 由导入器写入。 |
| 设置 | 配置目录的 `settings.ini` 和 `taa-collection.ini` | 没有 `settings.ini` 时会运行首次启动设置。 |
| 存档 | 数据目录的 `save/` | 更新时保留。 |
| 个人配置（profile） | 数据目录的 `profile/` | 更新时保留。可用 `LO_PROFILE_DIR` 改变位置。 |
| 着色器与管线缓存 | 数据目录的 `cache/shaders/` | 删除后会重新生成。可用 `LO_SHADER_CACHE_DIR` 改变位置。 |
| 日志 | 日志目录的 `logs/runtime-*.log` 和 `logs/shader-*.jsonl` | 保留本次和之前两次运行的日志。Android：`Android/data/io.github.freefrank.lostodyssey/files/logs/`，另有 `native-stderr.log` 和 `java-crash-*.txt`。 |
| F1 渲染捕获 | 配置目录的 `captures/` | Windows 为 `.zip`，Linux 和 macOS 为 `.tar.gz`。 |
| Mod | `mods/`：便携方式在程序旁，否则在数据目录 | 可用 `LO_MODS_DIR` 改变位置。 |
| 着色器包 | 安装目录：便携方式为程序旁的 `shaders/`，否则为数据目录的 `shaders/` | 游戏会把所选渲染器的包下载到这里（Vulkan、Metal 和 Android 用 `portable_vk.lospv`，DirectX 12 用 `portable_dx12.lospd`）；选择跳过会记录在同一目录的 `declined-downloads.txt` 中。v0.7.25 及更早版本在程序旁附带 `shaders/portable_vk.lospv`。 |
| 更新程序临时文件 | Windows：程序旁的 `.update\`；AppImage：日志目录的 `.update/` | Flatpak 和 macOS 安装包需要手动更新。当前源码中 Android 会显示更新提示并在浏览器里打开 APK 下载，没有临时文件。 |
| GPU 驱动包（Android） | 应用私有存储 `files/gpu_driver/<驱动包>/` | 从 **GPU driver** 页面安装；所选驱动记录在应用偏好中。 |

未指定 `--game` 时，**游戏目录的查找顺序**是：

1. 读取 `game-path.txt`。
2. 没有这个文件时，在数据目录的 `game/` 中查找 `default.xex`（仅限按用户目录存放的安装包）。
3. 再依次检查程序旁的 `game/`、程序所在目录和上一级的 `../game`。
4. 都找不到时打开导入器。

Android 应用启动时总是用 `--game` 指向有游戏数据的存储（先内部存储，再 SD 卡）的外部文件目录里的 `game/disc1`，其余光盘在它旁边查找。使用 `--game` 启动时不会切换工作目录。便携方式下，设置、存档、个人配置、缓存、日志和捕获都会跟随启动时所在的目录；按用户目录存放的安装包只有 `captures/` 会这样。

## 命令行参数

| 参数 | 作用 |
| :--- | :--- |
| `--game <路径>` | 使用指定的游戏：包含 `default.xex` 或 `disc1/` 的文件夹，或 `default.xex` 文件本身。跳过 `game-path.txt`、自动查找和自动导入；找不到 `default.xex` 时报错退出。 |
| `--install` | 即使已经设置好游戏也打开导入器，完成后退出：导入成功返回 0，取消或失败返回 1。**Gameplay → Import discs & DLC** 就是用这个参数重新启动的。 |
| `--setup` | 重新运行首次启动设置，然后进入游戏。Windows 上是设置对话框；Linux 和 macOS 还没有设置界面，只会保存当前设置。 |
| `--setup-only` | 同 `--setup`，完成后退出。 |
| `--prepare-shaders-only` | 加载游戏数据并准备着色器和管线，然后不启动游戏直接退出：成功返回 0，失败返回 1。可用于预热着色器缓存。 |
| `--quiet-kernel` | 日志中不记录内核跟踪行。 |

参数必须完全一致：`--game <路径>` 要写成两个参数，`--game=<路径>` 和其他无法识别的参数都会被忽略。没有 `--help` 或 `--version`。更新程序和重启逻辑会使用内部参数（`--apply-plan`、`--wait-process`、`--restart-ready`、`--restart-parent-fd`、`--restart-ready-fd`），请不要手动传入。`LostOdysseyRecomp.exe` 是图形界面程序，不会向控制台输出内容，请查看日志。

```bash
LostOdysseyRecomp.exe --game "D:\Games\Lost Odyssey"
./LostOdysseyRecomp-linux-x64-v0.8.6.AppImage --game ~/Games/LostOdyssey
flatpak run io.github.freefrank.LostOdysseyRecomp --game ~/Games/LostOdyssey
LostOdysseyRecomp.app/Contents/MacOS/LostOdysseyRecomp --game ~/Games/LostOdyssey
```

环境变量提供更多启动选项，每个变量都只在本次运行中覆盖已保存的设置。

| 变量 | 作用 |
| :--- | :--- |
| `LO_GRAPHICS_API` | Windows 上为 `d3d12` 或 `vulkan`。Linux 始终使用 Vulkan，macOS 始终使用 Metal。 |
| `LO_FPS` | 帧率上限，0 到 1000；`0` 表示不限制。 |
| `LO_FG_PROVIDER`、`LO_FG_MODE`、`LO_FG_MULTIPLIER`、`LO_FG_TARGET_FPS` | Windows 插帧：`off`/`dlss`/`fsr`；`off`/`fixed`/`dynamic`；2–6 倍；目标帧率。自 v0.7.35 起，Vulkan 支持 DLSS 固定 2–6 倍，用 `LO_ENABLE_VULKAN_FSR_FG` 构建时还支持 FSR 固定 2×；macOS 支持 `metalfx`（固定 2×，实验性）。动态模式仅限 D3D12 的 DLSS。[详情](docs/notes/vulkan-fg-fsr4-metalfx.md)。 |
| `LO_OPTISCALER_PATH` | 实验性 Windows OptiScaler 接入：填写自备 `OptiScaler.dll` 的绝对路径。构建须包含 DLSS/NGX，并设置 `LO_FG_PROVIDER=off`；游戏内选择 DLSS。[配置方法与限制](docs/notes/vulkan-fg-fsr4-metalfx.md#optional-optiscaler-loading-on-windows)。 |
| `LO_NO_UPDATE` | 设为 `0` 以外的任何值即跳过更新检查。 |
| `LO_PROFILE_DIR`、`LO_SHADER_CACHE_DIR`、`LO_MODS_DIR` | 使用其他个人配置、着色器缓存或 Mod 目录。`LO_SHADER_CACHE_DIR` 设为空值会关闭着色器缓存。 |
| `LO_MODS` | `0` 或 `false` 关闭 Mod。 |
| `LO_LOG_FILE` | 把日志写到指定路径；设为 `0` 则不写日志文件。 |
| `LO_AUDIO_MUTE`、`LO_CONTROLLER_RUMBLE` | `LO_AUDIO_MUTE=1` 静音；`LO_CONTROLLER_RUMBLE=0` 关闭震动。 |

## 反馈问题

请提供确切的安装包或源码版本、操作系统、图形后端、GPU／驱动、游戏版本和光盘，以及复现步骤或场景。附上本次 `logs/runtime-<timestamp>.log` 的完整日志，其中包含启动和图形信息。`LO_LOG_FILE=<path>` 可指定其他日志路径，`LO_LOG_FILE=0` 可关闭重复文件输出。

画面问题请在问题出现时按[捕获步骤](#overview捕获与游戏操作)保存现场，检查内容后再分享归档。不要附上游戏程序、资源包、存档或个人数据。

可选诊断默认关闭，也可在设置中关闭。采集内容和分享控制见[隐私说明](PRIVACY.zh-CN.md)。

## 实机画面

<img src="docs/images/title-screen.png" alt="失落的奥德赛标题画面 — Press START" width="960">

| Ring 战斗 | 城市探索 |
| :---: | :---: |
| ![凯姆攻击时的 Ring 判定界面](docs/images/ring-battle.png) | ![工业城市探索场景](docs/images/city-exploration.png) |

*截图来自 v0.1 发布前的开发构建，未经修图。*

## 开发导航

[构建指南](docs/BUILDING.md)介绍依赖和构建命令，[开发工具](tools/README.md)列出可用工具，[文档索引](docs/README.md)汇总当前参考文档和历史记录。

| 目录 | 内容 |
| :--- | :--- |
| `LostOdysseyRecomp/` | 宿主内核、图形、音频、输入与调试 |
| `LostOdysseyRecompLib/` | 配置；Git 忽略的 `private/` 游戏数据和 `ppc/` 生成代码 |
| `tools/` | 重编译工具、依赖补丁、Ghidra 脚本，以及可选的[汇编采样分析器](tools/asm-profiler/README.zh-CN.md) |
| `thirdparty/` | 渲染、音频及其他依赖 |
| `docs/` | 当前状态、指南、研究记录与历史归档 |

## 赞助者

感谢 **Cristian** 和 **Whitesun** 在 Ko-fi 上支持本项目。

## 致谢与游戏数据

本项目参考 [UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp)、[re:Blue](https://github.com/zolaware/reblue)、[XenonRecomp](https://github.com/hedge-dev/XenonRecomp)、[XenosRecomp](https://github.com/hedge-dev/XenosRecomp)、[plume](https://github.com/renderbag/plume) 和 [Xenia](https://github.com/xenia-project/xenia)。音频采用固定版本的 [Xenia FFmpeg 分支](https://github.com/xenia-project/FFmpeg)，已附[许可证](thirdparty/ffmpeg-LICENSE.txt)。Android 上的自定义 Adreno GPU 驱动（Mesa Turnip 构建）来自 [Eden](https://git.eden-emu.dev/eden-emu/eden) 模拟器提供的驱动列表，通过 [libadrenotools](https://github.com/bylaws/libadrenotools) 加载。

《失落的奥德赛》及其资产归各自权利人所有，本项目为非官方移植。请从自己拥有的光盘提取数据，不要向仓库提交游戏程序、资源包、纹理、音视频、生成的游戏代码或捕获数据。依赖保留各自许可证。
