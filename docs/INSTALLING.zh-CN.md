# 安装 Lost Odyssey Recomp

[English](INSTALLING.md)

本文说明已发布的 v0.7.25 安装包和当前源码路径。按平台下载程序，导入自己的游戏数据；更新时保留存档和个人配置目录。

## Windows 快速开始

需要 Windows x64 和支持 AVX 的 CPU。Windows 默认使用 Direct3D 12。发布包已经包含导入器、更新器、DXC v1.8.2407 DLL 对及依赖许可证；游玩发布包不需要安装 Python 或 Visual Studio。

1. 从 [v0.7.25 发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.25)下载 `LostOdysseyRecomp-windows-x64-v0.7.25.zip`。
2. 将完整 ZIP 解压到可写目录，避免放在 `Program Files` 下。
3. 运行 `LostOdysseyRecomp.exe`。未找到可用的游戏安装时，会打开内置导入器。
4. 首次设置中选择界面语言、游戏语言和图形选项，完成着色器准备后进入游戏。

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

Linux 只使用 Vulkan。 [v0.7.25 发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.25)提供 AppImage 和独立 Flatpak。Steam Deck 及其他 Linux 硬件仍只有限定范围的验证。

### AppImage

下载 `LostOdysseyRecomp-linux-x64-v0.7.25.AppImage` 后运行：

```bash
chmod +x LostOdysseyRecomp-linux-x64-v0.7.25.AppImage
./LostOdysseyRecomp-linux-x64-v0.7.25.AppImage
```

可以直接使用图形导入器。若要指定路径启动，可传入游戏目录、`disc1` 或 `default.xex`：

```bash
./LostOdysseyRecomp-linux-x64-v0.7.25.AppImage --game /path/to/game
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
flatpak --user install --bundle LostOdysseyRecomp-linux-x64-v0.7.25.flatpak
flatpak run io.github.freefrank.LostOdysseyRecomp
```

Flatpak 默认游戏目录是 `/var/data/game`。manifest 允许访问 host、`/media`、`/run/media` 和 `/mnt`，因此导入器可以读取沙盒外的 dump。独立 bundle 不配置 OSTree remote，更新时需要重新下载并安装新的 bundle。

## 从源码运行 Linux

原生构建依赖和打包命令见 [BUILDING.md](BUILDING.md)。运行时需要 Vulkan 驱动、放在可执行文件旁的 `libdxcompiler.so`，以及已解压的 Disc 1。直接启动示例：

```bash
./LostOdysseyRecomp --game /path/to/game
```

如果要使用便携的 `save/`、`profile/`、`cache/` 和 `logs/`，请把 ELF 所在目录作为当前工作目录。

## 首次设置和普通设置

Windows 上，首次设置页面会在游戏初始化前保存界面语言、游戏语言和图形选项。已有设置时会跳过该页面；运行 `LostOdysseyRecomp.exe --setup` 可以重新打开。Linux 和 macOS 还没有首次设置页面：首次运行会保存默认设置，之后在游戏内设置页面修改。游戏内设置页面会提示哪些改动需要重启。

界面语言和游戏语言是两套选项。USA/Europe 数据提供英、日、德、法、西、意语言；已核对的亚洲版提供英、日、韩、繁中、简中。当前版本没有的游戏语言会回退到英语。

按 **F1** 或 **LB+RB** 打开调试菜单，打开时暂停游戏。它与普通设置分开；捕获、传送、快进、内存修改和随时存档限制见 [README 的调试菜单说明](../README.zh-CN.md#调试菜单)。

<a id="file-locations"></a>

## 文件位置

| 运行方式 | 默认位置 |
|---|---|
| Windows 便携包；位于可写目录的 Linux 原生 ELF | 正常启动时，`save/`、`profile/`、`cache/`、`logs/`、`settings.ini` 和 `game-path.txt` 位于可执行文件旁；游戏数据默认导入到 `game/`。 |
| AppImage；位于只读目录的 Linux 原生 ELF | 存档、档案、缓存和游戏数据：`~/.local/share/lost-odyssey-recomp/`。设置与游戏路径：`~/.config/lost-odyssey-recomp/`。日志：`~/.local/state/lost-odyssey-recomp/logs/`。 |
| Flatpak | 主机目录为 `~/.var/app/io.github.freefrank.LostOdysseyRecomp/`：存档、档案、缓存和游戏数据在 `data/`；设置与游戏路径在 `config/lost-odyssey-recomp/`；日志在 `.local/state/lost-odyssey-recomp/logs/`。 |
| macOS `.app`（实验性，需从源码构建） | 存档、档案、缓存、游戏数据、设置与游戏路径：`~/Library/Application Support/LostOdysseyRecomp/`。日志：`~/Library/Logs/LostOdysseyRecomp/logs/`。 |

F1 渲染捕获保存在 `captures/`，Mod 放在 `mods/`。便携方式下两者都在可执行文件旁；否则捕获在设置目录，Mod 在数据目录。[README](../README.zh-CN.md#文件与目录) 列出了全部文件、目录和[命令行参数](../README.zh-CN.md#命令行参数)。

Flatpak 的主机 `data/` 目录在沙盒内显示为 `/var/data`，因此默认游戏目录是 `/var/data/game`。本程序的着色器缓存使用 `data/cache/`。主机路径遵循 [Flatpak 的 XDG 目录约定](https://docs.flatpak.org/en/latest/conventions.html#xdg-base-directories)。

Linux 根据实际 ELF 所在目录是否可写来选择便携存储，否则使用上方 XDG 目录；自定义 `XDG_CONFIG_HOME`、`XDG_DATA_HOME` 或 `XDG_STATE_HOME` 会改变对应根目录。便携 Windows 或原生 ELF 显式使用 `--game` 启动时，相对用户数据路径按调用者的工作目录计算；请保持启动目录一致，以免读到另一套存档。`--game` 只选择游戏来源，不改变非便携安装的数据布局；此时只有 F1 捕获会跟随工作目录。

## 更新和保留个人数据

正式发布包启动时可以检查 GitHub 是否有新版本。可以关闭自动检查；离线或检查失败不应阻止启动。独立 Flatpak 没有 OSTree remote，需要手动安装新 bundle。

更新时保留以下内容：

- 存档、个人配置和 `settings.ini`；
- `logs/` 与着色器缓存；
- `game-path.txt` 以及放在程序目录旁的导入数据。

手动更新时，先关闭游戏再替换程序文件。另存一份存档和设置，并在确认新包可用前保留旧包。

## 报告启动或画面问题

附上 `logs/runtime-<timestamp>.log` 的完整当前日志，并提供启动日志附近记录的 executable/source version、backend、GPU 和 driver 信息。设置 `LO_LOG_FILE=<path>` 可指定日志路径；设置 `LO_LOG_FILE=0` 可关闭重复文件输出。

遇到画面问题时，打开 **F1 → 常规 → 捕获渲染状态**，确认后**关闭 F1** 恢复渲染。程序捕获接下来的三个帧并在后台归档。重新打开菜单查看结果，附上所示路径的归档文件。Windows 为 `.zip`，Linux 和 macOS 为 `.tar.gz`；归档失败时会保留原始目录。归档包含截图和渲染／着色器数据，分享前请检查内容。

报告时请写明场景、游戏版本、光盘、设置和确切安装包版本，方便复现。

## 源码和 CI

源码构建见 [BUILDING.md](BUILDING.md)，仓库同步见 [PUBLISHING.md](PUBLISHING.md)。发布打包和本地安装是两个步骤。
