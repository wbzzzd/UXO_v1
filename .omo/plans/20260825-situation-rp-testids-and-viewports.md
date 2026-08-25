# 态势页 SIT-RP-* 契约补齐与六页三视口截图交付

- 状态：Completed（HTML + 文档 + 截图 + 验证全部完成；git 提交待用户确认）
- 日期：2026-08-25
- 工作区：/home/lin/UXO_v1-ui-visual-upgrade（分支 feature/prototype-baseline-sync，基于 origin/main 240f5bf）
- 关联需求：REQ-010（Implemented，本批为原型侧契约收尾，不新增产品功能）
- 前置计划：.omo/plans/20260824-prototype-structure-resync.md（已完成；本批解决其 §7.3 全部三项遗留）
- 执行依据：用户续跑指令，承接前置两批已批计划的遗留项

## 1. 背景与目标

结构同步批次（2026-08-24）完成后遗留三项：situation 原型右面板浮层无 SIT-RP-* testid；situation.md §11 行号滞后；1280×720 与 3840×2160 三视口截图未交付（ui/README §8 后续任务）。

**目标**：

1. 态势页补齐 8 枚 SIT-RP-* testid 并以运行时断言验证契约；
2. situation.md 同步描述浮层放置、位姿/缩放模拟行为并校准 §11 行号；
3. screenshot.js 视口参数化，交付六页 × 三视口截图，README 如实记录三视口现状。

**非目标**：不实现响应式布局；不改 Qt 源码；不新增需求或功能；不提交 git（待用户）。

**安全边界**：本批仅涉及 docs/ui 原型与文档；无 C++ 变更、无真实设备控制、无数据库/外部通信；所有演示行为保持"模拟"标注。

## 2. 变更内容

### 2.1 prototypes/situation/index.html（11 处编辑）

- 8 枚 SIT-RP-* testid 挂载：`SIT-RP-MAP`（原 SIT-MAP 更名）、`SIT-RP-TOOLBAR`、`SIT-RP-TOP`、`SIT-RP-SIDE`、`SIT-RP-3D`、`SIT-RP-RESET`、`SIT-RP-FULLSCREEN`、`SIT-RP-ZOOM`。
- 工具条贴地图舞台右缘通高（60px、z-index:10）；右缘浮层避让（right:16px→76px、角标 right:4px→64px）。
- JS §10 视角控制模拟：`VT_CAM_DEFAULT={x:2500,y:600,z:2000}`；位姿 TOP(f.x,1500,f.y)/SIDE(100,200,f.y)/3D(f.x+500,500,f.y+700)；缩放按 `600/cam.y` 折算；f 取选中目标（fixture TGT-002 x:2100,y:1440）否则地图中心(2500,1500)；RESET 镜像 SIT-TB-RESET 行为。

### 2.2 pages/situation.md（10 处编辑）

- §5.1/§5.1.1 改写为原型 2D 地图模拟行为契约（右缘放置、位姿公式、缩放折算、RESET 镜像、浮层避让）。
- §11 对照表 16 行行号校准：LeftPanelWidget.cpp:72/276-347/435-453；populateMissionList/populateDeviceList 不存在；AlertPanel/DetectionControlPanel/BatchOperationBar 未被实例化（死代码）；RightPanelWidget.cpp:26-112 未挂主窗口；loadMockData 493-538；onSelectTargetEverywhere 659-688（连接 436/461）；SimulationWorkflow.cpp requestSelectedTargetStatus 61-110、MainWindow.cpp:858 调用。
- §12 ID 索引确认已含全部 8 枚 SIT-RP-*，无需编辑。

### 2.3 prototypes/screenshot.js（4 处编辑）

- 视口参数化：`node screenshot.js [page] [宽x高]`；省略页面跑全部六页；省略视口默认 1920x1080；输出 `overview-<宽x高>.png`。

### 2.4 docs/ui/README.md（7 处编辑）

- §3 文件索引：截图行改三视口说明、脚本行补视口用法；§4.4 截图路径与固定画布说明；§7 新增"HTML 原型三视口实测现状"段；§8 后续任务改写（截图已交付，响应式达标仍为后续）；§9.1 重截图行补视口行为同步规则；§9.2 步骤 5 补三视口补拍命令。

### 2.5 截图交付（13 张）

- situation overview-1920x1080.png 重拍（HTML 已变）；六页 × 1280x720、六页 × 3840x2160 新增；其余五页 1920 图未动（HTML 未变）。

## 3. 验证（证据：.omo/evidence/situation-rp-testids-and-viewports/）

1. **Playwright 运行时断言 29/29 全过**（verify-sit-rp.js + verify-output.txt，2026-08-25 复跑落盘）：8 枚 SIT-RP-* testid 各恰 1 处；SIT-MAP 0 处；初始/TOP/SIDE/3D 标签逐字断言（含 TGT-002 选中态，如 3D 时 `缩放: 120%`、`X:2600 Y:500 Z:2140`）；RESET 镜像全项；FULLSCREEN 恒禁用；无 pageerror。
2. **三视口截图目检**（look_at 抽检 7 张：situation 三视口 + detection/configuration@1280 + detection/statistics@3840）：1920×1080 完整铺满、右缘工具条与指北针/比例尺/PiP 无遮挡；1280×720 按原尺寸裁切右侧约 640px、底部约 360px（六页一致）；3840×2160 内容原尺寸锚定左上角、右侧约 1920px/下方约 1080px 深色留空（六页一致）。
3. **data-testid 全量重算**（sort -u 唯一值口径）：HTML 63 个唯一 testid（82 处出现，无动态挂载）；situation.md 70 个唯一 SIT-* ID；交集 37；文档独有 33（均为 §11 已标注的 CURRENT/死代码映射）；HTML 独有 26（SIT-TD-* 13、SIT-PIP-* 9、SIT-CENTER、SIT-DR-ROBOT1、SIT-DR-UAV1、SIT-LP-TOGGLE，全部为历史遗留，本批未新增未登记 ID）。
4. `git status --short` 仅预期变更：5 个修改（README.md、situation.md、screenshot.js、situation/index.html、situation 1920 图）+ 12 张新视口截图。

### 验证命令

```bash
# 29 项契约断言（需 chromium NSS 库 workaround，见 §4.3）
LD_LIBRARY_PATH=<chromium-libs 目录> node .omo/evidence/situation-rp-testids-and-viewports/verify-sit-rp.js

# 截图（默认 1920x1080；可指定视口）
cd docs/ui/prototypes
node screenshot.js situation
node screenshot.js situation 1280x720
node screenshot.js situation 3840x2160
```

## 4. 遗留与建议（超出本批范围，仅报告不修改）

1. 26 个 HTML 独有 testid 未入 situation.md §12 索引（README §9.3 禁止"只改原型不改文档"）；建议下批补登记或显式退役。
2. 六页原型均为固定 1920×1080 设计画布，无响应式缩放；README §7 已如实记录，三视口达标仍列 §8 后续任务。
3. 本机缺 libnspr4/libnss3，chromium 启动需 LD_LIBRARY_PATH 指向自备库目录（本批用 /tmp/opencode/chromium-libs，临时目录，重启即失）；永久修复建议 `sudo apt-get install libnspr4 libnss3`。
4. git 提交与 PR 待用户确认后进行。
