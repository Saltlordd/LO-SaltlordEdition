# 路线图

[English](ROADMAP.md) · [开发状态](STATUS.md) · [更新日志](../CHANGELOG.md) · [维护者 Project](https://github.com/users/freefrank/projects/3)

Issue、发布记录和全部 Project 条目于 2026-09-30 通过 GitHub 复核；[Project 对账记录](project-management/reconciliation-2026-09-30.md)保存实际修正与回读结果。`[x]` 表示所述交付或跟踪范围已完成；`[~]` 表示仍有明确余项；`[ ]` 表示规划工作。关闭跟踪项不代表新增游戏或硬件验证。v0.7.25 发布后，发布记录和开放 Issue 列表已于 2026-10-01 重新读取，PR #128 合并后又于 2026-10-02 约 00:50 UTC 再次读取；Project 条目未重新读取。

## 当前交付

- [~] **已合并到 `main`、尚未发布（GitHub 于 2026-10-02 约 00:50 UTC 读取）：**[PR #119](https://github.com/freefrank/LostOdysseyRecomp/pull/119)（Vulkan DLSS 2×–6×、实验性的 Vulkan FSR 和 MetalFX 插帧、OptiScaler 加载）、[PR #122](https://github.com/freefrank/LostOdysseyRecomp/pull/122)（Direct3D 12 和 Vulkan 上的主机 GPU 遮挡查询，对应 [#118](https://github.com/freefrank/LostOdysseyRecomp/issues/118)）、[PR #120](https://github.com/freefrank/LostOdysseyRecomp/pull/120)（DLSS 库的查找路径，以及 Linux AppImage 和 Flatpak 中未被改写的 NGX 库，对应 [#116](https://github.com/freefrank/LostOdysseyRecomp/issues/116)）、[PR #127](https://github.com/freefrank/LostOdysseyRecomp/pull/127)（v0.7.25 附带的 Vulkan 着色器包与运行时不匹配而被忽略；现已修正着色器包的校验规则，发布构建会校验着色器包，并固定了重新生成的包）和 [PR #128](https://github.com/freefrank/LostOdysseyRecomp/pull/128)（一段过场动画和 Old Sorceress' Mansion 天空的抖动，对应 [#121](https://github.com/freefrank/LostOdysseyRecomp/issues/121)）。最新已发布版本仍是 v0.7.25。#116、#118 和 #121 仍开放，等待报告者在发布后确认。这些修复只按[开发状态](STATUS.md)记录的范围验证过，主要是捕获常量测试、CI 和 Windows 上的一块 NVIDIA RTX 5080；AMD 和 Intel 显卡、Linux 上的 NVIDIA 显卡，以及游戏内的过场和宅邸都没有检查。可选的 DX12 着色器包尚未按新的着色器包规则重新生成。
- [x] **v0.7.25 已发布（未验收）：**[发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.25)于 2026-10-01T08:04:23Z 发布，tag commit 为 `2c65f0b`，提供 Windows ZIP、Linux AppImage 和 Linux 独立 Flatpak，没有附带单独的 DX12 shader 包。内容包括：为 Issue [#74](https://github.com/freefrank/LostOdysseyRecomp/issues/74) 增加的 F1“强制开启 RB 换人”按钮和分队期间的随时存档保护（PR #106；维护者已于 2026-10-01 18:57 UTC 关闭该 Issue，晚于发布）；“东方部族的遗产”天空 jitter 修复（[#102](https://github.com/freefrank/LostOdysseyRecomp/issues/102)）和运行日志中的闪烁嫌疑记录（PR #107）；Windows ZIP 的 manifest 写入 SHA-256 字符串（[#105](https://github.com/freefrank/LostOdysseyRecomp/issues/105)，`43f4317`）；根据项目审查完成的 13 项运行时加固（PR #109，[修复记录](notes/PROJECT_REVIEW_FIXES_20260930.md)）；Flatpak 更新提示；以及实验性的 Apple Silicon macOS 源码构建路径（不提供 macOS 安装包）。这是首个由 Gitea Actions 构建并发布的版本（[run 67](https://git.zkx.ca/freefrank/LostOdysseyRecomp/actions/runs/67)，[CI 说明](notes/ci-gitea.md#releases)；PR #110 和 #115）。本次发布没有新增游戏实测，维护者尚未验收；发布不等于报告者验收，各项内容沿用[开发状态](STATUS.md)记录的验证范围。修复记录中的后续设想（显示模式定时恢复、待生效设置可见性、Linux 首次运行交互、异步目录枚举，以及基于测量的缓存与分配器优化）只是建议，不是已排期工作。
- [x] **v0.7.20 已发布：**[发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.20)于 2026-09-30T04:46:36Z 发布，提供 Windows ZIP、Linux AppImage 和 Linux 独立 Flatpak。包含 6× 插帧倍率上限、#82 退出修复、游戏纹理 mip 链（#87）、命令处理器开销降低和 D3D12 重复状态过滤。限定验证仍不等于完整游戏、跨 GPU 或实体显示验收。

- [x] **v0.7.0 已发布：**源码 `4142f23`，Release CI [36228746088](https://github.com/freefrank/LostOdysseyRecomp/actions/runs/36228746088)。Windows 与 Linux 包见[发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.0)。
- [x] **v0.7.1 已发布：**源码提交 `c585ef820cb72993ad87a90a1a03c1c648fb654c`，打标 `v0.7.1`，[发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.1)（2026-09-26T21:51:24Z），Release CI [36274702691](https://github.com/freefrank/LostOdysseyRecomp/actions/runs/36274702691)。包含 Gameplay → 导入光盘与 DLC、安全重启至导入器、选择性替换与失败回滚。公开产物已核实 SHA256：Windows 包 `LostOdysseyRecomp-windows-x64-v0.7.1.zip`（243,762,106 字节，SHA256 `e53753a71b06ab39c41a3a5b327a8477523db4b006543c54e70834b183c5291f`），Linux AppImage `LostOdysseyRecomp-linux-x64-v0.7.1.AppImage`（251,038,200 字节，SHA256 `878d04f9a530771fc2ba752842c1c9b5ba1cfc3fea63555a401dd53b46dd6e65`），正式独立 Flatpak `LostOdysseyRecomp-linux-x64-v0.7.1.flatpak`（265,618,800 字节，SHA256 `2efe0a4ba556037f9118894b36cba4b7667132b708c9ec3ea325db9c16f71775`，stable 分支），以及 Flathub 输入 runtime `LostOdysseyRecomp-linux-x64-v0.7.1-flatpak-runtime.tar.xz`（SHA256 `661838345ca5e1590dce99e35a9dba2bc1138d073c1c76d947aec34ea4db931f`）。Flatpak 经验证获 psvita 用户验收（严格限制于该验证范围，不推断性能或多场景兼容性）。
- [x] **v0.7.2 已发布：**merge 源码 `e2fc909dc15757aa5180566cecfd1ef2ff25dd18`，打标 `v0.7.2`，[发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.2)（2026-09-27T08:59:13Z），Release CI [36305268629](https://github.com/freefrank/LostOdysseyRecomp/actions/runs/36305268629)。新增有界的 Windows D3D12 DLSS/FSR SR、DLAA 尺寸修正，以及原生物体运动不可用时的相机／深度 hybrid motion。10 个资产均与 GitHub SHA-256 和大小记录一致；Windows ZIP、AppImage sidecar 和 Flatpak CI 核验通过。更广场景、画质、性能和其他 GPU 覆盖不在本次发布证据范围内。
- [x] **v0.7.9 已发布：**打标 `v0.7.9`，tag commit 为 `99fdcfa232e4deff2a80989d217524e7eb4bb365`，[发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.9)（2026-09-28T05:03:17Z），Release CI [36378342125](https://github.com/freefrank/LostOdysseyRecomp/actions/runs/36378342125)。新增 Windows D3D12 图像页 FG 分区，支持关／DLSS／FSR 和同进程即时生效，并加入 Ubuntu 22.04 AppImage 基线及 AppDir 复用 Flatpak。公开发布仅包含 Windows ZIP、Linux AppImage 和 stable Flatpak；更广游戏、跨 GPU、画质及物理显示验证仍待完成。
- [x] **v0.7.15 已发布并通过功能验收：**tag 与 Release CI head 均为 [`b074b689`](https://github.com/freefrank/LostOdysseyRecomp/commit/b074b689a3d2ffdbebabc1e14aad524d87e8c3ae)，[发布页](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.15)（2026-09-29T00:26:14Z），Release CI [36500844014](https://github.com/freefrank/LostOdysseyRecomp/actions/runs/36500844014)。5 个 job 均成功，4 个公开资产已上传；独立 DX12 shader 资产复用 v0.7.10 资产。维护者于 2026-09-29 确认本轮发布的全部功能通过验收。该验收限于已发布功能，不等于完整游戏、跨 GPU、实体显示或完整通关覆盖。
- [x] **PR #80 已随 v0.7.15 发布：**合并提交 [`e79a793`](https://github.com/freefrank/LostOdysseyRecomp/commit/e79a793530412633bc57b6fbd9b43097023deb3c) 新增原生 90／120 FPS 目标和 FreeSync／G-SYNC Compatible VRR 输出节奏控制。PR 最终 8 项 CI 检查通过；同场景用户证据确认输出节奏低于 144，且 G-SYNC／刷新率发生变化。v0.7.15 已发布功能已获维护者验收；更广游戏、退出生命周期、画质和实体显示覆盖仍作为后续回归工作。

## 已完成功能与已核对跟踪项

- [x] **Flatpak 独立发布：**原计划 v0.8.0，现提前随 v0.7.1 交付。独立包 `LostOdysseyRecomp-linux-x64-v0.7.1.flatpak` 已发布并通过 psvita 用户验收。独立 Flatpak 目标不以 Flathub 上架为前提。Flathub 商店提交流程单独跟踪（未创建 PR）：因 Flathub `requirements#generative-ai-policy` 严格禁止 AI 生成或协助编写 manifest 与 PR，且 PR 模板要求附带应用演示视频（application demonstration video），需由维护者本人人工另制 manifest、提供应用视频并提交 PR。

- [x] **DLSS/DLAA、FSR 超分与 D3D12 FG：**v0.7.0 范围已在记录的 Windows／原生 Linux 覆盖内通过用户验收；v0.7.9 新增有界的 Windows D3D12 DLSS／FSR FG 和游戏内同进程切换。更广 FG 硬件、场景、节奏、画质和物理显示覆盖仍待完成。
- [x] **PlayStation 按键提示：**宿主与游戏内面键、肩键、Start/Back 提示已验收并随 v0.7.0 发布。
- [x] **Mod API v1 与 Wiki：**`457ba24`／PR #68 已交付清单解析、禁用与回退、重载、LOTEX1/PNG 工具、原生菜单图集与字体替换及 Mod 指南。Windows/Linux Mod API 与 Wiki CI 已通过。任意游戏纹理／模型替换和真实 MO2 验收不属于此已完成范围。
- [x] **IME、闲置光标与手柄改进：**已随 v0.6.19 发布，旧 IME“未开始”条目已关闭。更广设备组合仍属回归覆盖。
- [x] **PortForge 清单：**`9abda30` 已提供 Windows/Linux 清单，Issue #37 已关闭，本地记录现与 Project 的 Done 一致。
- [x] **Issue 自动分析：**已部署 Issue-opened 与 `@codex` 回复，并核实[真实公开回复](https://github.com/freefrank/LostOdysseyRecomp/issues/21#issuecomment-5669773986)。后续缺陷修复和回复质量改进单独跟踪。
- [x] **音频跟踪项 #54/#55：**Issue 与 Project 均已关闭。此处记录维护者关闭状态，不将 PR #66 的诊断改动当作所有语言／缺失台词问题的修复证明；历史反馈仍保存在条目证据中。
- [x] **维护者已完成的 Project 记录：**两个 shader 翻译失败、纯资源 PSO 预备和临时主角伤害控制已在远端标为 Done。旧 Delivery 字段及证据不能独立证明发布版本或新增运行时验收。本地路线图遵循维护者完成状态，不再将这些记录列为当前 Todo。
- [x] **此前已交付：**实时 AF、退出桌面／标题菜单、随时存档偏好持久化、作弊导航、超宽屏控制、便携式 Vulkan shader 包、Linux AppImage、在线 PPC 编译及有界渲染／运行时修复。历史测量与版本细节保留在[开发状态](STATUS.md)和[更新日志](../CHANGELOG.md)，不再混在当前进行中队列。

## 当前工作

- [~] **未关闭报告：**[#49](https://github.com/freefrank/LostOdysseyRecomp/issues/49) 物理像素窗口坐标仍开放。[#74](https://github.com/freefrank/LostOdysseyRecomp/issues/74) 多队伍迷宫中的 Debug Save Anywhere 行为在 2026-10-01 08:06 UTC 的读取中仍开放，当天 18:57 UTC 被维护者按“已完成”关闭。#74 已查明原因：换人权限是运行时脚本变量，读档后不会恢复（见[调查笔记](notes/issue-74-save-anywhere-party-split.md)）。PR #106（已随 v0.7.25 发布）新增 F1“强制开启 RB 换人”按钮，并在分队期间让随时存档保持关闭；只用报告者的存档在 Astral Square 验证过，尚无报告者确认，关闭 Issue 也不构成确认。2026-10-01 新建的三个报告已有已合并、尚未发布的修复，等待报告者在发布后确认：[#116](https://github.com/freefrank/LostOdysseyRecomp/issues/116) Linux 上 NVIDIA 显卡 DLSS 不可用（PR #120）、[#118](https://github.com/freefrank/LostOdysseyRecomp/issues/118) 太阳的镜头光晕透过地形（PR #122）和 [#121](https://github.com/freefrank/LostOdysseyRecomp/issues/121) Old Sorceress' Mansion 天空闪烁（PR #128）。Issue [#64](https://github.com/freefrank/LostOdysseyRecomp/issues/64)、[#67](https://github.com/freefrank/LostOdysseyRecomp/issues/67) 和 [#77](https://github.com/freefrank/LostOdysseyRecomp/issues/77) 已在 GitHub 关闭（2026-09-29 核对）；关闭和 v0.7.15 功能验收不等于完整通关或更广硬件、场景覆盖。
- [~] **GitHub 于 2026-10-02 约 00:50 UTC 重新读取到的其他开放 Issue：**[#30](https://github.com/freefrank/LostOdysseyRecomp/issues/30) 景深控制、[#40](https://github.com/freefrank/LostOdysseyRecomp/issues/40) 剩余 Mod 范围、[#48](https://github.com/freefrank/LostOdysseyRecomp/issues/48) 晕动症体验、[#49](https://github.com/freefrank/LostOdysseyRecomp/issues/49) 物理像素坐标、[#85](https://github.com/freefrank/LostOdysseyRecomp/issues/85) 小地图开关和 [#90](https://github.com/freefrank/LostOdysseyRecomp/issues/90) HDR／存档状态，以及 [#103](https://github.com/freefrank/LostOdysseyRecomp/issues/103) 内置 SDL 手柄数据库和 [#104](https://github.com/freefrank/LostOdysseyRecomp/issues/104) 加快菜单动画（两者均于 2026-09-30 创建，晚于 Project 对账），还有 [#112](https://github.com/freefrank/LostOdysseyRecomp/issues/112) 原生 GTAO 请求、[#114](https://github.com/freefrank/LostOdysseyRecomp/issues/114) 摧毁容器时遇敌软锁和 [#117](https://github.com/freefrank/LostOdysseyRecomp/issues/117) 击中敌人时画面闪烁（三者均于 2026-10-01 创建；#117 此处未审查）。加上上文的 #116、#118 和 #121，共 14 个开放 Issue。2026-10-01 08:06 UTC 的读取列出 11 个：其中的 #74 已于当天 18:57 UTC 关闭，#116、#117、#118 和 #121 则晚于那次读取。[#88](https://github.com/freefrank/LostOdysseyRecomp/issues/88) 存档转换和 [#96](https://github.com/freefrank/LostOdysseyRecomp/issues/96) macOS Apple Silicon 兴趣请求在 2026-10-01 较早的读取中仍列为开放；GitHub 现显示两者均已按“已完成”关闭。这里只记录跟踪器状态，不推定实现、验收或共同优先级。
- [~] **Issue #40 剩余 Mod 范围：**PS 提示、v1 框架与 Wiki 已交付；更广游戏纹理／模型接入、真实外部管理器集成仍待完成，Issue 保持开放。
- [ ] **功能请求：**景深控制（#30）和晕动症选项（#48）。
- [~] **原生运动与时序颜色：**几何／刚体／骨骼 replay 基础和已确认的 SDR 输入已实现，并有有界战斗与 Hybrid SR 证据。余项为未映射 draw、更广骨骼／场景覆盖、HDR／曝光和 D3D12 replay PSO 错误 `0x80070057`。
- [~] **Linux／Steam Deck：**Linux x64 运行时、菜单、导入器、更新器与 AppImage 已发布，并有原生 AMD 8060S RADV 证据。Steam Deck 实机、Steam runtime／Flathub 及更广流程仍待覆盖；APEX 15W 不能等同 Deck 实机。NVIDIA 显卡上的 DLSS 在 v0.7.20 和 v0.7.25 的 AppImage 与 Flatpak 中被报告为不可用（[#116](https://github.com/freefrank/LostOdysseyRecomp/issues/116)）；修复（PR #120）已合并但尚未发布，DLSS 尚未在 Linux 的 NVIDIA 显卡上运行过。
- [~] **性能：**有界城市场景／缓存优化、通知等待和 0.7.20 命令处理器开销优化已进入 main。剩余实测停顿与更广 15W／全游戏性能目标需要各自的范围及证据；旧“未发布分支”描述仅为历史。
- [ ] **图形后续：**移除独立实验性 TAA，以及基于测量的缓存／运动优化。D3D12 便携 shader 包已在 v0.7.10 作为独立 `.lospd` 资产交付；它早于 v0.7.25 之后合并的着色器包规则（PR #127，未发布），该规则会拒绝按旧规则生成的包，目前尚无重新生成的 DX12 包。空间 AA 回退不等于移除 TAA 选项，也不证明所有闪烁已消除。

截至 2026-09-30 的对账，Project 将 #85、#88、#90 的 HDR／save states 和 #96 的 macOS 记录为 Paused／Deferred backlog；#88 和 #96 之后已在 GitHub 关闭，其 Project 条目未重新读取。Issue 开放不代表正在实施。全流程、章节／换盘／存档兼容、音频／语言、手柄／震动、混合 DPI／全屏与多 GPU 覆盖沿用各自验证记录的范围。更广覆盖不会自动成为新的进行中任务，也不构成重新挂起维护者已完成条目的理由。

## v0.8.0 计划

以下记录阶段进度与剩余执行顺序，已完成阶段保留作上下文。P0/P3/P4 为阶段标识，不是优先级；排列顺序也不代表每个阶段之间都有技术依赖。

1. [~] **P0：**共同时序契约已实现，Gate 1 宿主验证已由维护者于 2026-09-27 接受通过，依据为本地 `fe6f255` 加 Gate 1 宿主修复。原生构建和限定检查通过；FSR+FG 限定运行 exit 0、serial 为 820/820 且清理完整；获授权的 70 秒静音前台运行 exit 0、serial 为 2975/2975，生成区间 1,980 次、actual presents 4,955 次，采样 SDK／feature 创建错误为 0。已知 SDK 相关的 `PRESENT-AFTER-WRITE` 记录保留到 backlog，不再阻塞 Gate 1。`Application`／`ComposedFlip` 分类不足以证明生成帧到达物理显示；native failure injection 和 settings restart 仍属后续工作，native CPU 检查不等于 D3D12 GPU 验收。详见[Gate 1 宿主修复记录](notes/gate1-host-repair-20260927.md)。
2. [~] **P3：**Streamline 呈现接入、provider-neutral present／输入生命周期跟踪，以及最终合成 backbuffer 的 FG 路径已合入 [`81fe304`](https://github.com/freefrank/LostOdysseyRecomp/commit/81fe3048569f06bdeca4f1bd24c8fdc106428abc)／PR [#72](https://github.com/freefrank/LostOdysseyRecomp/pull/72)。v0.8.0 剩余工作是解决同步问题，并验证 resize、模式切换和退出生命周期。生产级 HUDless／UI 分离属于独立的 v1.0.0 目标，不再是 v0.8.0 完成条件。
3. [~] **P4：**Windows Vulkan 固定 2× DLSS 插帧已接入。经授权的 70 秒 Uhra 运行记录 48 个 enabled 周期、`actual_presents=2`、2,830 个生成区间和 0 个 SDK error；较早的同步 validation 运行失败，该历史结果仍保留在证据中。维护者之后已接受 Gate 1 通过，已知 SDK 例外转入 backlog。未发布更新（2026-10-01，[PR #119](https://github.com/freefrank/LostOdysseyRecomp/pull/119)）：Vulkan DLSS 现在由图形菜单和 `settings.ini` 控制，支持固定 2×–6× 请求，并受 SDK 报告的上限限制；另新增实验性的 Vulkan FSR 3.1 插帧路径（固定 2×，仅限用 `LO_ENABLE_VULKAN_FSR_FG` 构建的源码，发布包不含）。运行证据仅来自一块 NVIDIA RTX 5080（驱动 616.56，窗口 2560x1440，固定 Uhra 存档）：DLSS 2×、4×、6× 和 FSR 2× 均生成了插帧，包括 1080p 内部分辨率放大到 1440p，以及窗口缩放、最小化和还原（[笔记](notes/vulkan-fg-fsr4-metalfx.md#review-validation-2026-10-01)）。仍待完成：AMD 和 Intel 适配器、独占全屏、DLSS／DLAA／FSR 组合、首帧和镜头切换行为、在 GPU 上从菜单实时修改（Vulkan 下切换提供者需要重启）、validation layer、节奏、画质和实体显示节奏。
4. [x] **独立 FSR 插帧交付：**独立 D3D12 FSR FG provider 已随 v0.7.9 交付，并纳入 v0.7.15 功能验收；更广硬件、场景、节奏和画质覆盖不在该验收范围内。
5. [x] **D3D12 DLSS 插帧交付：**D3D12 DLSS FG 和图像菜单即时切换已随 v0.7.9 交付，并纳入 v0.7.15 功能验收；更广验证和 failure injection 覆盖另行保留。
6. [~] **动态 MFG 后续：**D3D12 adapter 已包含受能力限制的诊断动态 MFG 路径，游戏内菜单仍只提供固定模式。Vulkan 路径会拒绝动态请求，因为 Streamline 2.14.1 文档把动态 MFG 列为仅限 D3D12；Vulkan DLSS 只提供固定 2×–6×。更广 API、平台、倍率和硬件验证仍待完成。
7. [x] **原生 90／120 FPS 与 VRR：**原生游戏呈现和 FreeSync／G-SYNC Compatible 输出节奏已随 v0.7.15 发布，并获维护者验收。同场景输出节奏和硬件指示器变化已有有界用户证据；Ring、音频、过场、更广游戏、退出生命周期、FG 画质和独立 120 FPS 实体显示帧测量仍属后续覆盖，默认保留 30 FPS。
8. [ ] **移除 PM4 转换器：**替代架构可行性提前调查，执行排在插帧与呈现工作之后。
9. [ ] **Linux AArch64：**平台交付目标，尚不宣称官方包或实机验收。
10. [~] **macOS AArch64／Apple Silicon：**平台目标，关联 #96（GitHub 于 2026-10-01 按“已完成”关闭）。MikeRavenelle 的 `arm64-macos` 工作已整合为实验性的 arm64／Metal 路径，合并后包含在 v0.7.25 源码中，作为源码构建路径；v0.7.25 不提供 macOS 安装包。验证仅限[开发状态](STATUS.md#macos-integration-branch--2026-10-01)记录的 M1 Max 新游戏／首战脚本、冷启动 shader 预编译和缓存复用检查。长时间游玩、更广场景、画质、性能和已发布安装包仍待完成。实验性的 MetalFX 插帧（固定 2×，仅限 macOS 26）已在未发布的 [PR #119](https://github.com/freefrank/LostOdysseyRecomp/pull/119) 源码中实现，只针对 macOS 26 SDK 做过编译检查，尚未在 Mac 硬件上运行（[笔记](notes/vulkan-fg-fsr4-metalfx.md#metalfx-fg-on-the-existing-macos-port)）。
11. [ ] **实验性 Android：**探索目标，尚无 APK 或设备验证。

Gate 1 backlog：调查已知 SDK `PRESENT-AFTER-WRITE` 同步例外并补充显示分类，记录在[维护者 Project backlog](https://github.com/users/freefrank/projects/3?pane=issue&itemId=PVTI_lAHOAAsUY84Biy1azg9F10Q)。验证层记录的 10 条消息受 duplicate cap 限制，不是故障或帧数统计。

并行推进：

- [x] **Flatpak 发布：**已提前于 v0.7.1 交付完成（独立包已发布并经验证；Flathub 提交流程受 AI 政策限制，需人工另制 manifest、录制应用演示视频并提交，独立跟踪）。

## v1.0.0 延后计划

- [ ] **生产级 HUDless／UI 分离交接：**独立于 v0.8.0 的合成 backbuffer FG，建立并验证专用 scene／UI 合成契约。

## 后续积压

DX11、HDR 输出、高分辨率阴影、SSAO／深度访问、GI／反射、光追、WMV 播放与暂停的 Switch 工作保留各自 Project 范围。明确延期的研究与 SDK 同步例外使用 Paused／Deferred，移出当前 Todo，但不代表取消。

逐项证据见 [Project](https://github.com/users/freefrank/projects/3)，早期细节见[历史路线图](archive/ROADMAP-2026-09-10.md)。本轮整理没有重跑构建、游戏或测试。

<!-- Historical link compatibility. -->
<a id="v070-frame-generation"></a>
<a id="v070-upscaling"></a>
<a id="v080-frame-generation-macos"></a>
<a id="v090-pm4-translator"></a>
<a id="v050-pc-graphics"></a>
<a id="下一主版本v050--pc-vulkan-与-direct3d-11"></a>
<a id="近期优先事项"></a>
<a id="当前反馈与回归"></a>
<a id="已发布里程碑v042--修复与验证"></a>
<a id="阶段-1产出可编译代码"></a>
<a id="阶段-2进入主菜单"></a>
<a id="阶段-3推进完整通关"></a>
<a id="阶段-4现代化"></a>
<a id="阶段-5可选探索"></a>
