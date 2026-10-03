# 安装 Lost Odyssey Recomp

[English](INSTALLING.md)

本文说明已发布的 v0.8.6 安装包（含实验性 Android APK）和当前源码路径。按平台下载程序，导入自己的游戏数据；更新时保留存档和个人配置目录。

## Windows 快速开始

需要 Windows x64 和支持 AVX 的 CPU。Windows 默认使用 Direct3D 12。发布包已经包含导入器、更新器、DXC v1.8.2407 DLL 对及依赖许可证；游玩发布包不需要安装 Python 或 Visual Studio。

1. 从 [v0.8.6 发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.6)下载 `LostOdysseyRecomp-windows-x64-v0.8.6.zip`。
2. 将完整 ZIP 解压到可写目录，避免放在 `Program Files` 下。
3. 运行 `LostOdysseyRecomp.exe`。未找到可用的游戏安装时，会打开内置导入器。
4. 首次设置中选择界面语言、游戏语言和图形选项，完成着色器准备后进入游戏。游戏可能会先询问是否下载所选渲染器的预编译着色器，见[着色器准备](#shader-preparation)。

下载包不包含游戏文件。启动游戏需要 Disc 1。

<a id="automatic-content-import"></a>

## 导入游戏数据

在导入器的来源页面选择 **Files** 导入指定文件，或选择 **Folder** 扫描目录。导入器会根据光盘元数据识别来源，不需要手动选择光盘或 DLC 模式。确认前先检查识别结果。

<a id="supported-sources"></a>

支持的来源包括：

- 解压后的游戏目录或其中的 `default.xex`；
- XDVDFS ISO，包括 descriptor 位于前 512 MiB 内的带填充镜像；
- GOD/SVOD header、对应的 `.data` 目录，或包含多个 GOD 光盘的目录。

已核对的版本均使用 Title ID `4D5307FA`：

| 版本 | Version | Disc 1–4 的 Media ID |
|---|---:|---|
| Asian multilingual | 4 | `39F7D748`、`0EF8CEA8`、`309E3386`、`7B21A91D` |
| USA/Europe | 3 | `368DE6DD`、`1888BE4E`、`6DD59D08`、`0C0E80B5` |

两套版本的光盘不能混用。其他区域版本、Title Update 和修改过的 XEX 不在已核对范围内。

四张光盘全部导入后，游戏会自动选择请求的光盘，不需要手动换盘。后续光盘的完整流程尚未完成验证。

## 追加或替换光盘和 DLC

需要追加光盘或 DLC 时，从**设置 → 游戏 → 导入光盘与 DLC**（Settings → Gameplay → Import discs & DLC）重新打开导入器。选择要添加或替换的内容，检查合并后的结果，再确认导入。未选中的其他光盘、存档、个人配置和设置仍可使用。

DLC 可以直接选择，也可以放在扫描目录中。支持的包需要包含 Lost Odyssey Title ID `4D5307FA`、Marketplace content type `2` 和 STFS volume。DLC 不包含在程序下载包中。

导入器会先暂存所选内容，再写入安装目录。导入取消或失败后，从导入器重试剩余内容。确认安装可用前请保留原始来源文件；导入器会复制 dump，不会移动它。

在目标目录页，选择 **New folder**、按 **F2** 或手柄 **Y** 可新建文件夹，继续前可以编辑名称。

默认游戏数据位于 `game/disc1` 至 `game/disc4`，DLC 位于 `game/dlc/<content-id>`。也可以选择其他目标目录。便携模式使用可执行文件旁的 `game-path.txt`；文件为空或不存在时先查找旁边的 `game`，再查找旧版 `../game` 位置。直接启动时，显式的 `--game` 路径优先级最高。AppImage 和 Flatpak 的默认目录见[文件位置](#file-locations)。

<a id="running-on-linux"></a>

## Linux 安装包

Linux 只使用 Vulkan。 [v0.8.6 发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.6)提供 AppImage 和独立 Flatpak。Steam Deck 及其他 Linux 硬件仍只有限定范围的验证。不发布 Linux AArch64 安装包；实验性的源码构建和交叉编译路径见 [LINUX_ARM64.md](LINUX_ARM64.md)。

### AppImage

下载 `LostOdysseyRecomp-linux-x64-v0.8.6.AppImage` 后运行：

```bash
chmod +x LostOdysseyRecomp-linux-x64-v0.8.6.AppImage
./LostOdysseyRecomp-linux-x64-v0.8.6.AppImage
```

可以直接使用图形导入器。若要指定路径启动，可传入游戏目录、`disc1` 或 `default.xex`：

```bash
./LostOdysseyRecomp-linux-x64-v0.8.6.AppImage --game /path/to/game
```

正常挂载运行的 AppImage 会把存档和设置放在 Linux 用户目录，见[文件位置](#file-locations)。`--game` 只选择游戏数据，不会切换为便携存储；把 `game-path.txt` 放在外层 `.AppImage` 文件旁不能配置这种运行方式。

### Flatpak

Flatpak 的 App ID 是 `io.github.freefrank.LostOdysseyRecomp`，使用 Freedesktop Platform 26.08。安装好 Flatpak 后，为当前用户添加 Flathub 并安装运行时。下面统一使用用户级安装，参照 [Flatpak 官方指南](https://docs.flatpak.org/en/latest/first-build.html)：

```bash
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Platform//26.08
```

再安装并运行下载的 bundle：

```bash
flatpak --user install --bundle LostOdysseyRecomp-linux-x64-v0.8.6.flatpak
flatpak run io.github.freefrank.LostOdysseyRecomp
```

Flatpak 默认游戏目录是 `/var/data/game`。manifest 允许访问 host、`/media`、`/run/media` 和 `/mnt`，因此导入器可以读取沙盒外的 dump。独立 bundle 不配置 OSTree remote，更新时需要重新下载并安装新的 bundle。

## 从源码运行 Linux

原生构建依赖和打包命令见 [BUILDING.md](BUILDING.md)。运行时需要 Vulkan 驱动、放在可执行文件旁的 `libdxcompiler.so`，以及已解压的 Disc 1。直接启动示例：

```bash
./LostOdysseyRecomp --game /path/to/game
```

如果要使用便携的 `save/`、`profile/`、`cache/` 和 `logs/`，请把 ELF 所在目录作为当前工作目录。

<a id="macos"></a>

## macOS（Apple Silicon，实验性）

[v0.8.6 发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.6)提供 `LostOdysseyRecomp-macos-arm64-v0.8.6.dmg`，这是内含 `LostOdysseyRecomp.app` 和 Applications 链接的磁盘映像。需要 macOS 15 或更高版本的 Apple Silicon Mac，游戏使用 Metal 渲染。游戏本身只在 macOS 26.6.2 上运行过。v0.8.0 和 v0.8.6 应用声明的最低系统都是 macOS 15.0。v0.7.35 应用声明的最低系统仍是 macOS 14.0，但内置的着色器编译器（DXC）按 macOS 15 构建，游戏也从未在 macOS 14 上运行过，因此不支持 macOS 14（[详情](MACOS_RELEASE.md#minimum-macos-version)）。应用仅做 ad-hoc 签名、未经公证，并且会一直如此：维护者于 2026-10-03 决定不做 Developer ID 签名和公证。因此 macOS 会拦截首次启动。

1. 下载磁盘映像并打开。
2. 把 `LostOdysseyRecomp.app` 拖到窗口中的 Applications 链接上，然后推出磁盘映像。
3. 从“应用程序”打开 `LostOdysseyRecomp`。macOS 第一次会拦截它。打开 **系统设置 → 隐私与安全性**，向下滚动到“安全性”部分，点击应用旁的 **仍要打开**。这个按钮只在启动被拦截之后才会出现。macOS 再次询问时点击 **打开**，可能需要输入密码。之后的启动不再需要批准。
4. 找不到可用的游戏安装时会打开内置导入器。按[导入游戏数据](#automatic-content-import)中的方法导入。

也可以在终端运行 `xattr -dr com.apple.quarantine /Applications/LostOdysseyRecomp.app` 来批准应用，然后再次打开它。

应用把游戏数据、设置、存档、个人配置和着色器缓存放在 `~/Library/Application Support/LostOdysseyRecomp/`，日志放在 `~/Library/Logs/LostOdysseyRecomp/logs/`，见[文件位置](#file-locations)。首次启动可能会询问是否下载 Metal 着色器，见[着色器准备](#shader-preparation)。

游戏内的更新检查只在新版本带有磁盘映像时提示打开发布页。更新时请下载新的磁盘映像，并替换“应用程序”中的应用；存档和设置在应用之外，会保留。

验证范围仅限一台 Mac。在维护者的 M1 Max（macOS 26.6.2）上，开场新游戏战斗用 Metal 运行，并使用了下载的 Metal 着色器包；在没有 EDR 余量的外接显示器上请求 HDR 时输出保持 SDR，当时尚未见到 Metal 的 HDR 输出本身。另外也用源码构建在这台 Mac 上跑过开启 GTAO 和 4× 阴影的开场战斗。长时间游玩、更广场景和其他 Mac 尚未测试。需要自行构建时见 [BUILDING.md](BUILDING.md#building-on-macos)；磁盘映像的制作方法见 [MACOS_RELEASE.md](MACOS_RELEASE.md)。

<a id="android"></a>

## Android（实验性）

[v0.8.6 发布](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.6)随安装包一起提供 `LostOdysseyRecomp-android-arm64-v0.8.6.apk`。需要支持 Vulkan 的 64 位（arm64）Android 8.0 或更高版本设备，四张光盘约占 20 GB 空间，画面用 Vulkan 渲染。应用名显示为 "Lost Odyssey (development)"，使用项目固定的 debug 签名，每个版本都沿用（维护者决定不使用正式签名 keystore），因此可以互相覆盖安装；Android 不允许覆盖安装签名不同的 APK（例如你自己构建的版本），需要先卸载。验证只在一台平板（联想 TB321FU，Adreno 750）上做过，边界见本节末尾。

1. 在设备上下载 APK 并打开，按 Android 的提示允许来自浏览器或文件管理器的安装。打开应用一次让它建好目录，然后关闭。
2. 在电脑上准备游戏数据。Android 上没有导入器：用任一桌面版的导入器生成 `game/disc1`–`disc4`，有 DLC 的话还有 `game/dlc/<content-id>`，方法见[导入游戏数据](#automatic-content-import)。
3. 用 USB 把设备连到电脑，选文件传输（MTP）模式，把这个 `game` 目录复制到 `Android/data/io.github.freefrank.lostodyssey/files/`，使 `…/files/game/disc1/default.xex` 存在。必须有第 1 张光盘，其余光盘在它旁边查找。也可以 `adb push` 到 `/sdcard/Android/data/io.github.freefrank.lostodyssey/files/game`。设备上的文件管理器一般写不进 `Android/data`。
4. 打开应用。高通设备会先显示 **GPU driver** 页面：设备自带的 Vulkan 驱动会让菜单光标所在行的文字消失，所以下载一个 Mesa Turnip 驱动包（KIMCHI `Turnip_v26.0.0_R8.zip` 在 Adreno 750 上验证过；页面会显示 Eden 模拟器对你的型号推荐哪个），或者保留 **System GPU driver**，然后按 **Start game**。之后可从 **CTRL → GPU driver** 再进这个页面；切换驱动会重启游戏。其他设备直接进入游戏。
5. 游戏会用和桌面相同的窗口提示下载 Vulkan 着色器包，用屏幕上的 **A** 键接受（**B** 跳过并在设备上编译，需要几分钟）。之后启动会复用缓存。

游戏画面上有触摸手柄。**CTRL** 打开手柄设置（大小、透明度、布局编辑器和 GPU driver 页面）；连接 USB 或蓝牙手柄后触摸手柄自动隐藏。设置在游戏内的设置页面修改，没有首次设置页面。

应用把设置放在私有存储的 `files/config/`，存档、档案、着色器包和缓存放在 `files/` 下，下载的 GPU 驱动包在 `files/gpu_driver/`。私有存储文件管理器看不到；卸载应用会连同 `Android/data` 里的游戏数据一起删除。更新时直接覆盖安装新 APK，存档和设置会保留。

<a id="android-logs"></a>
日志写在 `Android/data/io.github.freefrank.lostodyssey/files/logs/`，电脑通过 USB 连接可以复制：`runtime-<timestamp>.log` 是本次和之前两次运行的日志，`native-stderr.log`（以及上一次运行的 `native-stderr.previous.log`）包含 native 输出和崩溃报告，应用本身出错时还会有 `java-crash-<time>.txt`。运行日志开头记录了手机型号、Android 版本、GPU、Vulkan 驱动和所选的 GPU 驱动包。游戏闪退或一直黑屏时，复制前先再打开一次应用，让电脑看到完整的文件，然后把这些文件附到报告里。Android 11 及以上设备上的文件管理器一般打不开 `Android/data`。使用 adb 时，`adb logcat -s LostOdyssey` 可以实时看到同样的内容。

目前的验证：在维护者的平板上，开发构建能播放开场视频、进入首战，并在 Turnip 驱动下正常显示菜单。长时间游玩、Turnip 下的性能、其他 GPU、16 KB 页设备和实体手柄尚未测试。

## 首次设置和普通设置

Windows 上，首次设置页面会在游戏初始化前保存界面语言、游戏语言和图形选项。已有设置时会跳过该页面；运行 `LostOdysseyRecomp.exe --setup` 可以重新打开。Linux 和 macOS 还没有首次设置页面：首次运行会保存默认设置，之后在游戏内设置页面修改。游戏内设置页面会提示哪些改动需要重启。

界面语言和游戏语言是两套选项。USA/Europe 数据提供英、日、德、法、西、意语言；已核对的亚洲版提供英、日、韩、繁中、简中。当前版本没有的游戏语言会回退到英语。

按 **F1** 或 **LB+RB** 打开调试菜单，打开时暂停游戏。它与普通设置分开；捕获、传送、快进、内存修改和随时存档限制见 [README 的调试菜单说明](../README.zh-CN.md#调试菜单)。

<a id="shader-preparation"></a>

### 着色器准备

发布包不附带预编译着色器。如果没有装好与所选渲染器（Direct3D 12、Vulkan 或 Metal）匹配的包，并且已为你的版本发布了对应的包，游戏会在启动时询问是否下载，时机在更新检查之后、准备着色器之前。窗口标题为“着色器包”，会显示下载大小。

- **下载 (A)** 从 GitHub 的 `shader-packs` 发布页下载并显示进度，**取消 (B)** 可中止。只有文件的大小和 SHA-256 与发布清单一致、并且与游戏匹配时才会使用，否则游戏改为在本机编译着色器。选择下载可以省去数分钟的着色器编译。
- **跳过 (B)** 在本机编译着色器。这个选择会一直记到着色器更新为止，记录在 `shaders/` 目录的 `declined-downloads.txt` 中（见 [README 的文件列表](../README.zh-CN.md#文件与目录)）。不做选择直接关闭窗口，下次启动会再次询问。

自 v0.8.0 起，Vulkan（Windows、Linux）和 Metal（macOS）共用一个包 `portable_vk.lospv`，DirectX 12 仍用 `portable_dx12.lospd`。着色器 contract 变了，需要新的包，所以更新后第一次启动会提示下载。

离线或没有为你的版本发布着色器包时，游戏不会询问，直接在本机编译着色器。之后的启动会复用编译好的着色器。文件和校验见[着色器包参考](PORTABLE_SHADER_PACK.md#startup-download)。

<a id="file-locations"></a>

## 文件位置

| 运行方式 | 默认位置 |
|---|---|
| Windows 便携包；位于可写目录的 Linux 原生 ELF | 正常启动时，`save/`、`profile/`、`cache/`、`logs/`、`settings.ini` 和 `game-path.txt` 位于可执行文件旁；游戏数据默认导入到 `game/`。 |
| AppImage；位于只读目录的 Linux 原生 ELF | 存档、档案、缓存和游戏数据：`~/.local/share/lost-odyssey-recomp/`。设置与游戏路径：`~/.config/lost-odyssey-recomp/`。日志：`~/.local/state/lost-odyssey-recomp/logs/`。 |
| Flatpak | 主机目录为 `~/.var/app/io.github.freefrank.LostOdysseyRecomp/`：存档、档案、缓存和游戏数据在 `data/`；设置与游戏路径在 `config/lost-odyssey-recomp/`；日志在 `.local/state/lost-odyssey-recomp/logs/`。 |
| macOS `.app`（实验性） | 存档、档案、缓存、游戏数据、设置与游戏路径：`~/Library/Application Support/LostOdysseyRecomp/`。日志：`~/Library/Logs/LostOdysseyRecomp/logs/`。 |
| Android APK（实验性） | 游戏数据：`Android/data/io.github.freefrank.lostodyssey/files/game/`（手动复制）。应用私有存储 `files/`：设置在 `config/`，存档、档案、着色器包和缓存在旁边，GPU 驱动包在 `gpu_driver/`。日志在 `Android/data/io.github.freefrank.lostodyssey/files/logs/`。 |

F1 渲染捕获保存在 `captures/`，Mod 放在 `mods/`。便携方式下两者都在可执行文件旁；否则捕获在设置目录，Mod 在数据目录。[README](../README.zh-CN.md#文件与目录) 列出了全部文件、目录和[命令行参数](../README.zh-CN.md#命令行参数)。

Flatpak 的主机 `data/` 目录在沙盒内显示为 `/var/data`，因此默认游戏目录是 `/var/data/game`。本程序的着色器缓存使用 `data/cache/`。主机路径遵循 [Flatpak 的 XDG 目录约定](https://docs.flatpak.org/en/latest/conventions.html#xdg-base-directories)。

Linux 根据实际 ELF 所在目录是否可写来选择便携存储，否则使用上方 XDG 目录；自定义 `XDG_CONFIG_HOME`、`XDG_DATA_HOME` 或 `XDG_STATE_HOME` 会改变对应根目录。便携 Windows 或原生 ELF 显式使用 `--game` 启动时，相对用户数据路径按调用者的工作目录计算；请保持启动目录一致，以免读到另一套存档。`--game` 只选择游戏来源，不改变非便携安装的数据布局；此时只有 F1 捕获会跟随工作目录。

## 更新和保留个人数据

正式发布包启动时可以检查 GitHub 是否有新版本。可以关闭自动检查；离线或检查失败不应阻止启动。独立 Flatpak 没有 OSTree remote，需要手动安装新 bundle。macOS 上的检查只会提示打开发布页；请用新磁盘映像中的应用替换“应用程序”中的旧应用。

更新时保留以下内容：

- 存档、个人配置和 `settings.ini`；
- `logs/` 与着色器缓存；
- `game-path.txt` 以及放在程序目录旁的导入数据。

手动更新时，先关闭游戏再替换程序文件。另存一份存档和设置，并在确认新包可用前保留旧包。

## 报告启动或画面问题

附上 `logs/runtime-<timestamp>.log` 的完整当前日志，并提供启动日志附近记录的 executable/source version、backend、GPU 和 driver 信息。设置 `LO_LOG_FILE=<path>` 可指定日志路径；设置 `LO_LOG_FILE=0` 可关闭重复文件输出。Android 的日志在 `Android/data/io.github.freefrank.lostodyssey/files/logs/`，见 [Android 日志](#android-logs)。

遇到画面问题时，打开 **F1 → 常规 → 捕获渲染状态**，确认后**关闭 F1** 恢复渲染。程序捕获接下来的三个帧并在后台归档。重新打开菜单查看结果，附上所示路径的归档文件。Windows 为 `.zip`，Linux 和 macOS 为 `.tar.gz`；归档失败时会保留原始目录。归档包含截图和渲染／着色器数据，分享前请检查内容。

报告时请写明场景、游戏版本、光盘、设置和确切安装包版本，方便复现。

## 源码和 CI

源码构建见 [BUILDING.md](BUILDING.md)，仓库同步见 [PUBLISHING.md](PUBLISHING.md)。发布打包和本地安装是两个步骤。
