# Issue #74：分队期间随时存档后无法换人（2026-09-29 调查）

> **调查记录。** 结论与待办只对应文中日期、版本和捕获；现行问题、交付与验收状态见[项目状态](../STATUS.md)和[路线图](../ROADMAP.md)。

[issue #74](https://github.com/freefrank/LostOdysseyRecomp/issues/74) 报告在 The Great Ancient Ruins（Astral Square，地图 244）分队期间用随时存档保存，读档后右下角的 "RB Change Character" 提示消失，RB 无反应。本轮用报告附带的 `user01` 存档和 USA/Europe 数据在隔离目录复现并做了逆向。随时存档的行为没有改动，只在 F1 菜单开关下方加了两行常驻警告；中英文布局用 `LoDebugOverlayTest` 离线渲染截图检查过。该测试在 main 上本来就会在后续步骤报 `concurrent menu actions closed overlay`，撤下本次改动后结果相同，与警告文字无关。

## 结论

换人功能由一段一次性剧情脚本开启，开启状态只存在于运行时，不进入存档。随时存档只是打开原生 System 菜单的 Save 项（见[随时存档笔记](save-anywhere.md)），存档内容与原生存档相同，所以问题出在"在原作不允许保存的时刻保存"，而不是存档写坏。读档后剧情标志和分队成员仍然正确：走到出口时 Cooke 会说要等队长那组，但 RB 换人不会回来。

已排除的恢复途径：

- 离开 Astral Square 到相邻的 Legacy of the Eastern Tribe 再走回来，提示仍不出现，RB 仍无反应。
- rpPlayData 的 `CmdDrawState` / `CmdEnableState` 位（`PlayData+0x31F7C..0x31F8B`，由 `SetCmdDrawState` 等原生接口经虚表 `+0x1C4..+0x1D8` 读写）确实不在 EasySave 复制进存档的区段（`+0x4C..+0x30BF8`），但把 64 位全部置 1 也不能让提示出现，说明换人开关不在这里。
- 当前关卡几乎没有 UE Kismet 逻辑（堆里只有 13 个 `SeqEvent_Touch` 实例，`fsSeqAct_DrawControl`、`fsSeqAct_KeyControl`、`fsSeqAct_CheckButtonInfo` 只有默认对象），换人逻辑应在游戏自带的事件脚本系统里。
- `rpBaseController.XPad_RB`（原生 `828173B0`）只在按下 RB 时被原生 C++ 以新建的 FFrame 调用，调用者不是 UnrealScript 函数，无法从脚本侧找到开关。

## 已确认的地址

| 项目 | 值 |
|---|---|
| rpPlayData 虚表 | `0x820108FC`，由 GEngine 虚表 `+0x160` 取得，与作弊功能同一路径 |
| EasySave 实现 | 虚表 `+0x220` → `82906E18`，复制 `+0x4C` 起五段数据 |
| MaskMember | 虚表 `+0x1EC` → `82908308`，位于 `PlayData+0x64`，会存档 |
| ChangeCtrlChara | 虚表 `+0x1DC` → `829087B0`，写 `PlayData+0x58/+0x5C`，会存档 |
| 提示文本 | "Change Character" 与 "Pick Up Firewood"、"Enter" 同在一张本地化 FString 表 |
| UFunction 反射 | UField `SuperField +0x3C`、`Next +0x40`；UStruct `Children +0x4C`、`PropertiesSize +0x50`、脚本字节码 `+0x54/+0x58`；UProperty `Offset +0x68` |

`PlayData+0x64` 的成员遮罩不能用来判断是否分队：报告存档为 `0x596`，Numara Palace 原生存档点的正常存档也有 `0x58C`。

## 后续方向

要么阻止随时存档在分队期间覆盖原生的 Save 禁用，要么在读档后重建换人状态，两者都需要先找到运行时开关。缺的是一个"可以换人"的活体状态：需要一份分队开始之前的存档，进入分队后与读档后的状态做内存对照。本轮临时探针（FName 检索、反射字段遍历、对象/字符串/指针扫描、`XPad_RB` 调用者记录）保存在 ignored 的 `out/issue74/issue74-probe.patch`，源码已还原。
