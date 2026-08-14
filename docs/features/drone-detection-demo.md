# 无人机探测态势演示

状态：Implemented
关联产品需求：REQ-009（requirements/REQ-009.md，Approved）
关联版本：待确认

## 1. 问题与目标

### 1.1 问题

当前态势页已有初步实现（基于 REQ-008 时间线驱动方案），但存在以下问题：

- 探测脚本以视频时间点（10s/25s/42s/60s/78s）硬编码驱动，不符合真实无人机工作流。
- 战术地图使用本地米坐标系（0-5000m），无法与真实经纬度对接。
- 地图无无人机位置和航迹显示，缺乏态势感知。
- 需要同时提供发现瞬间提示和按目标回看证据，避免检测框长期遮挡视频画面。
- 目标坐标硬编码在脚本中，不是从无人机位置推算。

### 1.2 目标

改造为贴合真实无人机工作流的探测态势演示：无人机传输视频信号和 GPS 遥测，视频用于目标识别，坐标用于态势显示。检测事件驱动四区同步，目标地表坐标由无人机 GPS + 画面位置推算。

### 1.3 与 REQ-008 的关系

REQ-008（时间线驱动方案）已被标记为 `Superseded`，本功能（REQ-009）取代它。REQ-008 已实现的组件（VideoStreamPanel、TacticalMapWidget、VideoOverlayWidget 等）在本功能中改造复用，不从头重建。

### 1.4 与 SRS 四阶段的关系

同 REQ-008：本功能只做**探测阶段**。识别/规划/排弹属于后续独立功能，各自走审批流程，不在本功能范围内。

## 2. 范围与非目标

### 2.1 范围

- 改造态势页中心区：2D 战术地图（卫星底图 + 经纬度坐标）+ UAV-1 视频区（单画面 + HUD-only 叠加）。
- 新建无人机遥测模拟器：沿预设航线持续输出 GPS 坐标（经纬度、高度、航向）。
- 新建检测模拟器：处理视频画面，在目标出现时输出检测结果（类型 + 画面红框 + 置信度）。
- 目标地表坐标推算：无人机 GPS + 画面位置 -> 目标经纬度（简化投影模型）。
- 地图显示：卫星底图 + 无人机位置标记 + 航迹 + 目标标点（脉冲动画 + ID 标签）。
- 视频显示：真实视频播放 + HUD-only 叠加（十字准星、REC、遥测）；冻结标注证据在检测时捕获，仅选中目标时在详情浮层显示。
- 检测事件驱动四区同步：冻结标注证据捕获 + 目标表插行 + 地图标点 + 告警/日志。
- 双向联动：点击地图标点/目标表行，二者双向高亮。
- 探测工具栏：[重置][开始][结束] 三按钮。
- 空起步：启动时目标表/地图/告警/日志全空。
- 视频结束：目标流入停止，已有目标保留。
- 重置：清空所有，回到初始状态。
- 所有模拟数据标注"模拟"或"演示"。

### 2.2 非目标

- **不新建探测页**：不改导航行为，不实现 `docs/ui/pages/detection.md` 的独立页面规格。
- **不做识别证据 Tab**：识别证据/探测来源/状态历史三 Tab 属于后续识别阶段。
- **不做决策页/规划工作区**：跑道画布/三方案/算法参数属于后续规划阶段。
- **不做数据层全量重构**：不新建 SimulationSession，不把 SimulationWorkflow 改为 QObject + 六信号（那是 REQ-001/004 的全量方案）。本功能在现有 MainWindow 手工同步模式上增量改造。
- **不删除 Qt3D 代码**：`SituationView.cpp/h` 和 Qt3D 依赖保留，后续确认不再用 Qt3D 再处理。
- **不做真实设备控制、真实排爆、外部通信、持久化、UXR、MOS**（requirements/ 排除范围）。
- **不做识别/规划/排弹阶段**（各自独立功能）。
- **不做真实视频分析/AI 目标识别**：检测模拟器内部用预设数据驱动，但接口贴合真实检测器输出格式。
- **不做真实无人机飞控**：遥测模拟器内部用预设航线，但接口贴合真实遥测输出格式。

## 3. 用户流程

### 3.1 主流程

1. 启动客户端，态势页可见。中心区为 2D 战术地图（卫星底图，显示机场全貌，无目标标记），左下角浮动 UAV-1 视频区（黑屏，显示"等待开始"）。目标表/告警/日志全空。
2. 点击探测工具栏 [开始] 按钮。
3. UAV-1 视频开始播放。无人机遥测模拟器开始沿预设航线输出 GPS 坐标。地图上出现无人机位置标记，随时间移动并留下航迹。
4. 视频播放过程中，检测模拟器在画面中发现目标时输出检测结果。每次检测触发：
   - MainWindow 捕获当前视频帧并标注检测结果（冻结标注证据，内存持有）。
   - 目标表插入新行。
   - 地图在推算的目标地表坐标处加标点（脉冲动画 + ID 标签）。
   - 告警面板插入 1 条，日志追加 1 条。
5. 视频播放结束，自动暂停。目标流入停止，已有目标保留。无人机标记停在航线终点。
6. 全程地图上无人机标记随遥测移动，航迹实时延伸。

### 3.2 重置流程

1. 点击 [重置] 按钮。
2. 视频停止回 0s，地图清空所有目标标点和无人机航迹，目标表/告警/日志清空。
3. 回到启动初始状态。

### 3.3 结束流程

1. 点击 [结束] 按钮。
2. 视频停止并回 0s，模拟器停止，已有目标和冻结证据保留。
3. 区别于 [重置]：[结束] 保留现状（目标/地图/侧栏/选中），[重置] 清空一切。

### 3.4 双向联动

- 点击地图上的目标标点 -> 该标点高亮，目标表对应行高亮选中，详情浮层显示该目标的冻结标注证据。
- 点击目标表行 -> 地图标点高亮，详情浮层显示该目标的冻结标注证据。

## 4. 需求增量

### REQ-009 无人机探测态势演示

**解决的当前缺口**：REQ-008 时间线驱动方案不符合真实无人机工作流，坐标使用本地米坐标系无法对接真实经纬度，地图无无人机位置显示。

**验收结果**：
- 态势页中心区为 2D 战术地图（卫星底图 + 经纬度坐标）+ UAV-1 视频区。
- 启动时空起步：目标表/地图/告警/日志全空。
- 点击 [开始] 后视频播放，无人机遥测模拟器沿预设航线输出 GPS 坐标，地图实时显示无人机位置和航迹。
- 检测模拟器在视频画面中发现目标时输出检测结果（类型 + 画面红框 + 置信度），触发四区同步：冻结标注证据捕获 + 目标表插行 + 地图标点（脉冲动画 + ID 标签）+ 告警插入 + 日志追加。
- 目标地表坐标由无人机 GPS + 画面位置推算，使用真实经纬度。
- 视频结束，目标流入停止，已有目标保留。
- [重置] 清空所有，[结束] 停止视频并回 0s，保留已有目标。
- 双向联动：点击地图标点/目标表行，二者双向高亮。
- 所有模拟数据明确标注"模拟"或"演示"。

## 5. 架构增量

### 5.1 新增组件

| 组件 | 类型 | 职责 |
|------|------|------|
| `DroneTelemetrySimulator` | 新建（QObject 子类） | 无人机遥测模拟器：沿预设航线（经纬度航点序列）持续输出 GPS 坐标（经纬度、高度、航向）。定时器驱动，start/stop/reset 控制接口。发射 `telemetryUpdated(double lat, double lng, double alt, double heading)` 信号。接口贴合真实遥测源——真实系统替换为接收 MAVLink 或其他遥测协议即可。 |
| `DetectionSimulator` | 新建（QObject 子类） | 检测模拟器：处理视频画面，在目标出现时输出检测结果 `DetectionResult{类型, 画面红框(归一化坐标), 置信度}`。内部用视频位置驱动检测时机，但**接口不暴露时间点概念**——只输出检测结果。发射 `detectionOccurred(const DetectionResult& result)` 信号。接口贴合真实检测器——真实系统替换为 AI 推理引擎即可。 |

### 5.2 修改组件

| 组件 | 变更 |
|------|------|
| `TacticalMapWidget` | 坐标系从本地米(0-5000)改为经纬度；底图从占位纯色改为卫星图 QPixmap；新增无人机位置标记（航向三角形）+ 航迹绘制；新增 `setDronePosition(lat,lng,heading)` 和 `addTrackPoint(lat,lng)` 接口；目标标点坐标改为经纬度 |
| `VideoOverlayWidget` | HUD-only 叠加（十字准星、REC 指示、遥测 LAT/LON/ALT/HDG、时间码）；不绘制检测框、不持有冻结证据、无点击命中信号；鼠标事件透传给下层视频控件 |
| `VideoStreamPanel` | 加载真实视频文件（`/home/lin/uxo-assets/video/perth_airport_drone_edit.mp4`）；移除模拟时钟模式；`positionChanged` 信号驱动 `DetectionSimulator` |
| `MainWindow` | 移除 `DetectionTimelineController` 接线；新增 `DroneTelemetrySimulator` + `DetectionSimulator` 接线；检测事件 -> 四区同步（冻结标注证据捕获 + 目标表插行 + 地图标点 + 告警/日志）；双向联动（地图标点/目标表行） |
| `DemoScenarioProvider` | 移除 `DetectionScriptEntry` 时间线脚本；新增无人机航线数据（经纬度航点序列）；新增检测数据（目标类型 + 画面位置 + 置信度，**不含时间点**）；新增机场区域边界（经纬度） |

### 5.3 删除组件

| 组件 | 处理 |
|------|------|
| `DetectionTimelineController` | 删除（被 `DroneTelemetrySimulator` + `DetectionSimulator` 取代） |

### 5.4 不新增

- 不新建 SimulationSession（requirements/ 排除范围，ARCHITECTURE.md §9.2 约束）。
- 不把 SimulationWorkflow 改为 QObject + 六信号（那是 REQ-001/004 全量方案）。
- 不新增通用 Repository、Store、服务容器、事件总线或预留外部接口（ARCHITECTURE.md §9.2）。
- 不删除 SituationView/Qt3D 代码（保留，后续清理）。

### 5.5 状态所有权

延续 CURRENT 状态所有权模式（ARCHITECTURE.md §4）：MainWindow 手工同步到各 UI。`DroneTelemetrySimulator` 和 `DetectionSimulator` 持有模拟数据，MainWindow 接收信号后手工同步到目标表/地图/视频/告警/日志。这是过渡方案，REQ-001/004 全量方案批准后再迁移到统一状态源。

### 5.6 目标坐标推算模型

简化投影模型（实现已采用，参数为演示定值）：

1. 假设无人机 nadir（垂直下视）拍摄，固定高度，画面覆盖固定地面幅宽（宽 600m × 高 400m）。
2. 画面中心对应无人机正下方地面点。
3. 检测框中心归一化坐标 (cx, cy) 换算为相机系地面偏移：右舷偏移 `rightOffset=(cx-0.5)×600m`，前向偏移 `forwardOffset=(0.5-cy)×400m`（画面顶部对应航向正前方）。
4. 按 UAV 当前航向 h（0=北，顺时针）把相机系偏移旋转到地面东/北系：`east=right×cos(h)+forward×sin(h)`；`north=-right×sin(h)+forward×cos(h)`。目标偏移先按航向旋转，再进入下一步 WGS84 转换。
5. 地面偏移转 WGS84 增量：`Δlat=north/111000`；`Δlng=east/(111000×cos(lat))`。目标经纬度 = 无人机经纬度 + 增量。

此模型仅用于本地模拟演示，不是真实 GIS 投影。真实系统应使用相机姿态 + DEM 精确投影。

### 5.7 数据流

```
[开始] -> VideoStreamPanel.play()
       -> DroneTelemetrySimulator.start()
       -> DetectionSimulator.start()

VideoStreamPanel.positionChanged(ms) -> DetectionSimulator.onPositionChanged(ms)

DetectionSimulator -> detectionOccurred(DetectionResult)
                   -> MainWindow::onDetectionOccurred(result)
                   -> 1. 捕获当前视频帧 currentFrameSnapshot() + 标注检测结果  // 冻结标注证据，内存持有（m_evidenceByTargetId）
                   -> 2. LeftPanelWidget.addTarget(target)               // 目标表插行
                   -> 3. TacticalMapWidget.addTarget(target)             // 地图标点
                   -> 4. AlertPanel.addAlert(...)                        // 告警
                   -> 5. SimulationWorkflow.log(...)                     // 日志

DroneTelemetrySimulator -> telemetryUpdated(lat, lng, alt, heading)
                        -> MainWindow::onTelemetry(lat, lng, alt, heading)
                        -> 1. TacticalMapWidget.setDronePosition(lat, lng, heading)
                        -> 2. TacticalMapWidget.addTrackPoint(lat, lng)

双向联动:
  TacticalMapWidget.targetClicked(id)     -> MainWindow::onSelectTargetEverywhere -> LeftPanelWidget.select(id) + TargetDetailOverlay.setEvidence(...)
  LeftPanelWidget.targetSelected(id)      -> MainWindow::onSelectTargetEverywhere -> TacticalMapWidget.select(id) + TargetDetailOverlay.setEvidence(...)
```

## 6. UI 增量

### 6.1 中心区布局

延续当前实现：导航(80px) | 左pane(可折叠) | 中心区(设备资源条36px + 地图主舞台)。
地图主舞台上浮动：视频PiP(左下480x270) + 目标详情浮层(右上340px)。

### 6.2 探测工具栏

态势页顶部探测工具栏（已实现，保留）：

| 控件 | 标签 | 行为 |
|------|------|------|
| 重置按钮 | [重置] | 视频停止回 0s，清空目标表/地图标点/无人机航迹/告警/日志 |
| 开始按钮 | [开始] | 视频播放，遥测模拟器和检测模拟器启动 |
| 结束按钮 | [结束] | 视频停止并回 0s，模拟器停止，保留已有目标/地图/侧栏/选中 |

### 6.3 2D 战术地图

- 底图：沈阳于洪全胜机场卫星图（用户提供，2000×1800，北朝上 PNG）。
- 底图铺放：aspect-fit，保持原始宽高比完整放入场景，不裁剪；上下/左右留对称信箱(letterbox)区。机场边界四角对齐底图四角，卫星图与 WGS84 叠加层共享同一显示矩形，像素严格对齐。
- 坐标系：经纬度（WGS84），本地线性映射，非真实 GIS 投影。
- 地图边界：N 41.840, S 41.805, W 123.278, E 123.320（约 4km × 3.5km）。
- 无人机航线：沿观察到的跑道轴向 out-and-back 巡航（本地模拟航点序列，非真实飞控）；航向取跑道轴向真方位（离场≈198.7°，返航≈18.7°），高度固定 300m，96s 内均分航段。
- 无人机标记：蓝色三角形（航向指示），随遥测移动。
- 航迹：蓝色虚线，记录无人机飞行路径。
- 目标标记：红色实心圆点 + 脉冲动画 + ID 标签（TGT-001 等）；目标坐标由检测画面偏移按 UAV 航向旋转后转 WGS84 推算（见 §5.6）。
- 交互：点击目标标点高亮，发出 `targetClicked` 信号驱动双向联动。
- 空状态：只有卫星底图，无标记。

### 6.4 视频区

- 单画面：UAV-1 视频流（QMediaPlayer + VideoSurfaceWidget）。
- 视频文件：`/home/lin/uxo-assets/video/perth_airport_drone_edit.mp4`（1920×1080, 96s, 30fps）。
- 视频叠加：HUD-only（十字准星、REC 指示、遥测 LAT/LON/ALT/HDG、时间码），不绘制检测框；鼠标事件透传给下层视频控件。
- 冻结标注证据：检测发生时 MainWindow 捕获当前视频帧并标注检测结果（红框 + 标签），内存持有（m_evidenceByTargetId），仅在目标被选中时由详情浮层 TargetDetailOverlay 显示。
- 空状态：黑屏，显示"等待开始"文字。

### 6.5 模拟标识

所有探测阶段产生的目标、告警、日志条目必须标注"模拟"或"演示"，符合 PRODUCT.md §10 安全边界。

## 7. 验收标准

### 7.1 功能验收

| # | 验收项 | 验证方法 |
|---|--------|---------|
| 1 | 启动后态势页中心区为 2D 战术地图（卫星底图），左下角浮动 UAV-1 视频区 | 启动客户端截图 |
| 2 | 启动时目标表/地图标点/无人机航迹/告警/日志全空 | 启动客户端检查各区域 |
| 3 | 点击 [开始] 后视频播放，地图上出现无人机标记并移动，航迹实时延伸 | 运行时观察 |
| 4 | 检测模拟器发现目标时：冻结标注证据捕获（内存持有）；目标表插入新行；地图加标点（脉冲动画 + ID 标签）；告警插 1 条；日志追加 1 条 | 运行时观察四区同步 |
| 5 | 视频结束，目标流入停止，已有目标保留，无人机标记停在航线终点 | 运行时观察 |
| 6 | [重置] 清空所有，回到启动初始状态 | 运行时点击重置 |
| 7 | [结束] 停止视频并回 0s，保留已有目标/地图/侧栏/选中 | 运行时点击结束 |
| 8 | 点击地图标点 -> 目标表行高亮 + 详情浮层显示冻结标注证据 | 运行时点击地图标点 |
| 9 | 点击目标表行 -> 地图标点高亮 + 详情浮层显示冻结标注证据 | 运行时点击目标表行 |
| 10 | 所有目标/告警/日志标注"模拟"或"演示" | 检查 UI 文案 |
| 11 | 地图坐标系为经纬度，目标标点落在机场区域内 | 代码审查 + 运行时观察 |

### 7.2 构建与测试验收

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` 通过。
- `cmake --build build --target UXOMissionControl -j2` 通过。
- `ctest --test-dir build --output-on-failure` 全部通过（现有测试 + 新增探测阶段测试）。
- 新增测试：遥测模拟器测试（验证航线输出）、检测模拟器测试（验证检测输出）、坐标推算测试（验证经纬度推算）。

### 7.3 安全验收

- 无真实设备控制命令。
- 无外部通信调用。
- 无数据库写入。
- 无明文密钥。
- 模拟数据明确标注。

## 8. 对核心文档的影响

| 文档 | 变更 |
|------|------|
| `docs/requirements/REQ-008.md` | REQ-008 标记为 `Superseded`，新增 REQ-009 |
| `docs/requirements/README.md` | 包含范围更新为 REQ-009 描述 |
| `docs/ARCHITECTURE.md` §4 | 状态所有权表更新：`DetectionTimelineController` -> `DroneTelemetrySimulator` + `DetectionSimulator` |
| `docs/UI.md` §4.3 | 中央工作区矩阵更新：卫星底图 + 经纬度坐标 + 无人机航迹 |

以上文档变更在实现完成后回写，不在功能审批阶段修改。

## 9. 待确认事项

### 9.1 视频与卫星图地点不一致

视频素材为 Perth Airport 航拍（澳大利亚），卫星图为沈阳于洪全胜机场（中国）。两者非同一地点。目标地表坐标由无人机 GPS（沈阳于洪航线）+ 画面位置推算，推算结果落在沈阳于洪机场区域内，但视频画面内容与地图底图不对应。这是模拟演示的已知限制，不影响机制验证。

### 9.2 无人机航线设计

已实现：预设航线为沿观察到的跑道轴向 out-and-back 巡航（3 航点 / 2 航段，96s 内均分，高度 300m），航向取跑道轴向真方位（离场≈198.7°，返航≈18.7°）。航线为本地模拟航点序列，非真实飞控；航点坐标由卫星图跑道像素端点按 2000×1800 北朝上标定线性映射到 WGS84。

### 9.3 检测数据

检测模拟器内部需要知道在视频的哪些位置出现目标。这是**实现细节**，不暴露在接口中。视频帧分析已完成（4 个目标位置已标注），实现阶段据此配置检测数据。检测模拟器的公开接口只输出 `DetectionResult{类型, 画面红框, 置信度}`，不暴露时间点。

### 9.4 目标坐标推算精度

简化投影模型（nadir + 固定高度）精度有限，仅用于模拟演示。真实系统应使用相机姿态 + DEM 精确投影。推算参数（高度、视场角）在实现阶段调整，使目标标点落在机场区域内。

### 9.5 DetectionResult 数据结构

新增 `DetectionResult` 结构体，定义在 `Core/Data/Types.h` 或独立头文件中：

```cpp
struct DetectionResult {
    Core::TargetInfo::Type type;  // 目标类型
    QRectF videoRect;             // 画面红框归一化坐标 [0,1]
    double confidence;            // 置信度 0.0-1.0
};
```

实现阶段确认最终归属和字段。
