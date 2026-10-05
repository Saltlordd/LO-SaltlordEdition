# 首次使用管线的卡顿：方案（2026-10-05）

> 状态：方案，未实现。影响所有平台：Windows（D3D12 / Vulkan）、Linux、macOS、Android。

## 现象

第一次进入一张没去过的地图、第一次和新敌人战斗、第一次看某段过场时，会卡一帧到一秒多。按[Shader 准备](shader-preparation.md)里的配方机制收集管线，再多也收不全。

## 证据

**管线键。** `gpu::pipeline_cache::Key` 由以下字段组成：

- 着色器对：VS、PS；
- 渲染状态：混合、深度控制（含 stencil 操作）、剔除与 polygon offset 模式、颜色写掩码、图元类型；
- 格式：RT 格式、深度格式；
- stencil 参考值与掩码（正面、背面）；
- depth bias、slope bias。

注释里写明“不做任何规范化”。

**配方统计（Windows 上的 `pipelines.bin`，统计方法见文末）：**

| 来源 | 配方 | 无关位规范化后 | 去掉所有可动态化状态后 | VS/PS 对 | VS | PS |
|---|---|---|---|---|---|---|
| i212-run（316 张地图巡游） | 1950 | 1903 | 1246 | 1241 | 183 | 1088 |
| 本机游玩安装（Vulkan） | 920 | 888 | 548 | 545 | 114 | 497 |

结论：
- 状态排列只占约三分之一。把所有能做成动态状态的字段都拿掉，配方也只少 36%。
- 漏掉的管线主要来自新的 VS/PS 组合。组合数随地图、敌人、特效增长，录制收不全。
- 不同的着色器本身少得多（183 个 VS、1088 个 PS），shader pack 里总共 28687 条记录，是有限且已知的集合。

**Android（TB321FU，Turnip Mesa 26，Vulkan，已装匹配的 shader pack）：**

| 情况 | 进场景首帧间隔 | 当帧新建管线 | `pipeline_ms` |
|---|---|---|---|
| 第一次进 Highlands of Wohl | 1.34 s | 37 | 1056 |
| 第二次启动、同一场景 | 0.40 s | 5 | — |

- 第二次启动时，启动阶段预建 213 个配方用了 2.5 s（7 个 worker），平均每个约 80 ms。说明驱动没有任何磁盘缓存命中，每次启动都在重编。
- 剩下的 0.4 s 主要是 `bind_ms` 200 ms 和 `taa_ms` 161 ms，不属于管线问题，单独处理。

**各后端现在都没有持久化驱动缓存：**

| 后端 | 现状 |
|---|---|
| plume Vulkan | `vkCreateGraphicsPipelines` 传 `VK_NULL_HANDLE`，没有 `VkPipelineCache`；Android 也没设置 `MESA_SHADER_CACHE_DIR` |
| plume D3D12 | 没有 `ID3D12PipelineLibrary`。NVIDIA/AMD 驱动自带隐式磁盘缓存，所以桌面上重复进入还好，第一次仍然卡 |
| plume Metal | 着色器用 `newLibraryWithSource` 编译 MSL，PSO 用 `newRenderPipelineState`，没有 `MTLBinaryArchive` |

**其他已知限制：**
- `SDL_QUIT` 路径用 `_Exit` 绕过 Shutdown，最后一次 60 帧 checkpoint 之后学到的配方会丢失（见[Shader 准备](shader-preparation.md)）。这会让“收集过的管线”实际上没存下来。
- 配方上限 16384 条。

**驱动能力：** 平板上的 Turnip 驱动二进制里有 `VK_EXT_graphics_pipeline_library`、`VK_EXT_extended_dynamic_state` 1/2/3、`VK_KHR_pipeline_binary` 和 `MESA_SHADER_CACHE_DIR`。实现时要用设备扩展查询确认。

## 目标与验收

- 平板和 PC：第一次进入没去过的地图时，管线造成的单帧耗时 `pipeline_ms` 小于 30 ms。
- 第一次和新敌人战斗：任何一帧的 `pipeline_ms` 都小于 16 ms。
- 启动准备时间不比现在长；持久化缓存的磁盘占用有上限，并且会被清理。
- 测量方法：`LO_RENDER_TIMING=1`，按帧间隔和 `pipelines=` / `pipeline_ms=` 统计。

## 方案

### P0　运行时未命中日志（半天，先做）

每次在绘制时新建管线都打一行日志，包含：

- 帧号、当前地图 ID 或战斗编号；
- VS/PS 哈希，耗时；
- 这个键在配方里有没有、两个着色器有没有见过。

再写一个汇总脚本，按场景列出未命中。后面每个阶段都用它验收。

### P1　持久化驱动缓存（全平台，1~2 天）

- **Vulkan**：plume 为每个设备建一个 `VkPipelineCache`，所有 `vkCreate*Pipelines` 都传进去。
  - 缓存放在着色器缓存目录，比如 `pipeline_cache_vk.bin`。
  - 读入前校验 Vulkan 缓存头：`vendorID`、`deviceID`、`pipelineCacheUUID`。
  - 写盘随 `SavePipelineRecipes` 异步进行，并在正常退出时再写一次。
  - 以后可以换成 `VK_KHR_pipeline_binary`。
- **Android**：在加载 Turnip 之前，把 `MESA_SHADER_CACHE_DIR` 设到应用缓存目录。改动很小，能直接用上 Mesa 的磁盘缓存。
- **D3D12**：用 `ID3D12PipelineLibrary`，按键名 `StorePipeline` / `LoadGraphicsPipeline`。驱动已有隐式缓存，优先级较低。
- **Metal**：用 `MTLBinaryArchive` 存 PSO。
- **顺带修掉**：`_Exit` 退出路径前把配方和管线缓存 flush 到磁盘。

**效果**：第二次及以后几乎不卡，启动阶段的预建从秒级降到百毫秒级。第一次见到的组合仍然会卡，这一阶段不解决。

### P2　Vulkan graphics pipeline library（Vulkan 的主修复：Windows、Linux、Android）

思路：把编译成本从“每个组合一次”变成“每个着色器一次”。着色器集合有限而且已知（就是 pack 里的记录）。

管线拆成四部分：

| 部分 | 内容 | 粒度 |
|---|---|---|
| vertex input | 顶点输入（Xenos 的 vfetch 在着色器里做，这部分很小） | 几乎只有一个 |
| pre-raster | VS + 光栅状态 | 每个 VS 一个；剔除、正反面、depth bias 改成动态状态（EDS1 + Vulkan 1.0 核心） |
| fragment shader | PS + 深度/stencil 状态 | 每个 PS 一个；深度测试/写入/比较、stencil 操作用 EDS1，stencil 参考值/掩码用核心动态状态 |
| fragment output | RT/深度格式 + 混合 + 写掩码 | 不含着色器，创建很便宜，按需建并缓存 |

- **绘制时未命中**：fast-link 四个库（不开 LTO），预期亚毫秒级。同时把开了 `LINK_TIME_OPTIMIZATION` 的完整管线放到后台编译，编好后替换。
- **库的预编译**：
  - 启动时先为配方里出现过的着色器编库；
  - 之后在后台逐步为 pack 里的其余着色器编库；有 P1 的缓存，每个驱动版本只编一次；
  - 遇到新着色器（`GetShader` 第一次看到它）时立即编它的库。
- **plume 改动**：新增“创建库管线”和“链接管线”接口。改动量中等偏大。
- **回退**：设备不支持 GPL 或 EDS 时，走现在的完整管线路径。
- **风险**：某些驱动上 fast-link 出来的管线运行时偏慢，后台的优化编译可以弥补。28k 个库在 Android 上占多少内存需要实测，必要时只给配方和 pack 里的高频着色器常驻。

### P3　配方语料 + 按场景预取（全平台；D3D12 和 Metal 主要靠它）

- **无关位规范化**：stencil 关闭时清掉 stencil 字段；深度关闭时清掉比较函数；polygon offset 关闭时清掉 bias；写掩码为 0 时清掉混合。收益小（1950 → 1903），但没有成本，还能减少重复编译。要同时升级 `kPipelineRecipeVersion`。
- **分发语料**：用自动巡游收集配方，随 shader pack 一起发布。配方键里的格式是 `RenderFormat`，和后端无关。
  - 已有的地图巡游（316 张）和过场巡游（178 段）；
  - **新增战斗巡游**：通过调试入口逐个触发敌人编组。新敌人正是现在漏得最多的地方；
  - 玩家本地学到的配方继续合并进来。
- **按场景打标签和预取**：每条配方记录它出现在哪个地图 ID、战斗编组或过场里。游戏开始读某张地图、某场战斗时，在读盘画面期间用 worker 线程预建这个场景的配方。启动时只预建全局常用的那部分。

### P4　剩余未命中：并行化与上限

- 现在同一帧里的多个未命中是在渲染线程上串行创建的（37 × 约 28 ms）。遇到一个未命中时，可以在语料里找出共享同一个 VS 或 PS 的兄弟配方，并行预建。
- 可选开关：管线还没编好时跳过这次绘制（类似 DXVK async）。会导致短暂缺物体，默认关闭。

## 平台矩阵

| 平台 / 后端 | P1 | P2 | P3 | 预期 |
|---|---|---|---|---|
| Android / Vulkan（Turnip） | VkPipelineCache + Mesa 缓存 | GPL | 语料预取 | P1 后重复访问基本不卡；P2 后首次也基本不卡 |
| Windows / Vulkan、Linux / Vulkan | VkPipelineCache | GPL | 语料预取 | 同上 |
| Windows / D3D12 | ID3D12PipelineLibrary（驱动已有缓存） | 不适用 | 主要手段 | 首次卡顿取决于语料覆盖 |
| macOS / Metal | MTLBinaryArchive | MSL 已经按着色器编译，PSO 仍按组合 | 主要手段 | 同 D3D12 |

## 推荐顺序

P0 → P1 → P2 → P3 → P4。

- P1 改动小，立刻改善 Android 和所有平台的重复访问。
- P2 解决 Vulkan 平台的首次卡顿。
- P3 是 D3D12 和 Metal 的主要手段，同时提高所有后端的启动预建命中率。

## 统计方法

记录格式：48 字节文件头，之后每条 72 字节，前 64 字节是键（`Encode` 的顺序：vs、ps，再是 12 个 32 位字段）。

- 只用 Vulkan 核心动态状态和 EDS1 时，保留 (vs, ps, 有写掩码时的 blend, 写掩码, RT 格式, 深度格式, 是否 rect list)：i212-run 剩 1371 条。
- 再用 EDS3 把混合和写掩码也做成动态：剩 1246 条，即表中“去掉所有可动态化状态后”。
