# 探测阶段动态演示执行计划

状态：Approved for execution
关联功能设计：[docs/features/detection-stage-demo.md](../../docs/features/detection-stage-demo.md)
关联需求：REQ-008（PRODUCT.md §9.2，Approved；REQ-007 已被 MOS 占用）

## 背景

### 当前事实

- MVP 已完成单目标四状态流转闭环（4/4 CTest 通过）。
- 态势页中心区：上半 4 格视频分屏（REC 文本占位），下半告警 + 操作日志。
- 右侧面板：Qt3D SituationView（用户已判定"完全不可用"）+ 设备状态 + 决策建议。
- 启动即加载 1 目标 1 任务 2 设备（`MainWindow::loadMockData`）。
- `DemoScenarioProvider` 坐标混用经纬度 (108.9, 34.2) 和本地米坐标系 (0-5000)。
- 探测/识别/规划三阶段全缺，演示效果"悬浮"。

### 决策依据

- 用户选择"预录视频回放"作为视频画面方案。
- 用户决定放弃 Qt3D 改 2D 战术地图（QGraphicsView + 卫星底图）。
- 探测阶段全部决策已锁定（控制/布局/2D 地图/视频联动/5 目标/空起步）。
- 本功能只做探测阶段，识别/规划/排弹各自独立功能。
- 不做数据层全量重构（REQ-001/004 全量方案），在现有 MainWindow 手工同步模式上增量改造。

## 目标

把态势页改造为探测阶段动态演示：视频驱动目标流入、2D 战术地图显示态势、四区同步、80 秒 5 目标。演示从"悬浮枚举变化"变成"有真实感的探测过程"。

## 非目标

- 不新建探测页（不改导航）。
- 不做识别证据 Tab（识别阶段）。
- 不做决策页/规划工作区（规划阶段）。
- 不做数据层全量重构（不新建 SimulationSession，不改 SimulationWorkflow 为 QObject + 六信号）。
- 不删除 Qt3D 代码（保留，后续清理）。
- 不做真实设备控制/排弹/外部通信/持久化/UXR/MOS。
- 不做识别/规划/排弹阶段。

## 安全边界

- 所有探测阶段产生的目标/告警/日志标注"模拟"或"演示"。
- 视频为本地预录文件，不接外部视频流。
- 2D 地图底图为本地图片，不接外部地图服务。
- 无真实设备控制命令、外部通信、数据库写入、明文密钥。

## 外部依赖

| 素材 | 规格 | 用途 | 用户提供时机 |
|------|------|------|-------------|
| 机场航拍视频 | 80 秒，机场俯视，MP4 | UAV-1 视频画面 | 实现阶段（可先用占位） |
| 机场卫星图 | 通用民用机场俯视图，PNG/JPG | 2D 地图底图 | 实现阶段（可先用占位） |

如用户暂无法提供，先用占位素材（纯色背景 + 测试视频）开发，后续替换。

## 执行任务

### 阶段 1：数据层与坐标修复

- [ ] 1.1 修复 DemoScenarioProvider 坐标 bug
  - 把经纬度 (108.9, 34.2) 改为本地米坐标系（0-5000 范围）。
  - 新增 5 个目标的脚本数据：时间点（10s/25s/42s/60s/78s）+ 坐标 + 类型 + 置信度 + 威胁等级。
  - 类型从 `TargetInfo::Type` 枚举选取（AntiRunwayBomb / ArtilleryShell / Rocket / ClusterBomb / Unknown）。
  - 验证：新增 `demo_scenario_provider` 测试断言米坐标系和 5 目标脚本数据。
  - 证据：`build/demo_scenario_provider_test` 通过。

- [ ] 1.2 改造 MainWindow 启动为空起步
  - 删除 `loadMockData` 中 1 目标 1 任务 2 设备的预加载。
  - 启动时目标表/告警/日志清空（设备表保留静态设备展示，因为设备不属于探测阶段流入）。
  - 验证：启动后截图确认各区域为空。
  - 证据：`startup_visible` 测试更新断言空起步。

### 阶段 2：2D 战术地图控件

- [ ] 2.1 新建 TacticalMapWidget 类
  - `include/MainWindow/TacticalMapWidget.h` + `src/MainWindow/TacticalMapWidget.cpp`。
  - 继承 QWidget，内部 QGraphicsView + QGraphicsScene。
  - 加载卫星底图 QPixmap（支持占位纯色背景）。
  - 米坐标系（0-5000），场景坐标到屏幕坐标的映射。
  - 验证：单元测试或手动验证坐标映射。
  - 证据：新增 `tactical_map_test`。

- [ ] 2.2 实现目标红点标记
  - `addTarget(QString id, QPointF position)` 方法：添加红色实心圆点 QGraphicsEllipseItem + ID 标签 QGraphicsTextItem。
  - 脉冲动画：QPropertyAnimation 驱动圆点半径周期性放大缩小。
  - `clearTargets()` 方法：清空所有红点。
  - `highlightTarget(QString id)` 方法：高亮指定红点（边框加粗）。
  - 点击拾取：QGraphicsScene 的 mousePressEvent 检测红点点击，发出 `targetSelected(QString id)` 信号。
  - 验证：手动运行验证红点显示和点击。
  - 证据：截图。

- [ ] 2.3 接入 MainWindow 中心区上半
  - MainWindow 中心区布局改为上下分割（QSplitter）。
  - 上半放 TacticalMapWidget，下半放 VideoStreamPanel（阶段 3 改造）。
  - 连接 targetSelected 信号到目标表行高亮。
  - 验证：启动后中心上半显示 2D 地图。
  - 证据：截图。

### 阶段 3：视频面板改造

- [ ] 3.1 改造 VideoStreamPanel 为单画面播放
  - 从 4 格 QLabel 占位改为 QMediaPlayer + QVideoWidget 单画面。
  - 暴露接口：`play()` / `pause()` / `stop()` / `seek(qint64 ms)` / `positionChanged` 信号。
  - 支持加载本地视频文件（支持占位测试视频）。
  - 空状态：黑屏显示"等待开始"文字。
  - 验证：手动加载测试视频播放。
  - 证据：截图。

- [ ] 3.2 新建 VideoOverlayWidget 红框叠加层
  - `include/MainWindow/VideoOverlayWidget.h` + `src/MainWindow/VideoOverlayWidget.cpp`。
  - 继承 QWidget，透明背景，叠加在 QVideoWidget 上。
  - `showRedBox(QPointF position, QSizeF size)` 方法：在指定位置绘制红框。
  - 闪烁动画：红框显示 250ms 消失 250ms，重复 2 次，1 秒后完全消失（QTimer 驱动）。
  - 验证：手动触发红框观察闪烁。
  - 证据：截图。

- [ ] 3.3 接入 MainWindow 中心区下半
  - VideoStreamPanel 放在中心下半。
  - VideoOverlayWidget 叠加在 VideoStreamPanel 上。
  - 验证：启动后中心下半显示视频区（空状态）。
  - 证据：截图。

### 阶段 4：探测脚本驱动器

- [ ] 4.1 新建 DetectionTimelineController 类
  - `include/Core/Simulation/DetectionTimelineController.h` + `src/Core/Simulation/DetectionTimelineController.cpp`。
  - 继承 QObject。
  - 持有 5 个目标的脚本数据（从 DemoScenarioProvider 获取）。
  - `start()` / `stop()` / `reset()` 接口。
  - 监听 VideoStreamPanel 的 positionChanged 信号，到预设时间点发出 `targetDetected(TargetInfo target, QPointF videoPosition)` 信号。
  - 验证：单元测试断言 5 个时间点触发。
  - 证据：新增 `detection_timeline_test`。

- [ ] 4.2 接入 MainWindow 四区同步
  - MainWindow 接收 DetectionTimelineController::targetDetected 信号。
  - 信号处理：
    1. VideoOverlayWidget 显示红框（使用 videoPosition）。
    2. 目标表插行（LeftPanelWidget 新增 addTargetRow 方法）。
    3. TacticalMapWidget 加红点（使用 target.position）。
    4. AlertPanel 插入告警条目（AlertPanel 新增 addAlert 方法）。
    5. SimulationWorkflow 追加日志条目。
  - 验证：运行时观察四区同步。
  - 证据：截图。

### 阶段 5：探测工具栏与控制

- [ ] 5.1 新建探测工具栏
  - 态势页顶部新增工具栏（QToolBar 或自定义 QWidget）。
  - 三个按钮：[重置] [开始] [结束]。
  - 按钮状态：启动时 [开始] 可用，[重置]/[结束] 禁用；播放中 [开始] 禁用，[重置]/[结束] 可用。
  - 验证：按钮状态切换正确。
  - 证据：截图。

- [ ] 5.2 连接控制逻辑
  - [开始]：VideoStreamPanel::play() + DetectionTimelineController::start()。
  - [结束]：VideoStreamPanel::pause() + DetectionTimelineController::stop()（保留目标）。
  - [重置]：VideoStreamPanel::stop() + seek(0) + DetectionTimelineController::reset() + 清空目标表/2D 地图/告警/日志。
  - 80s 自动结束：VideoStreamPanel 到达结尾自动暂停 + DetectionTimelineController::stop()。
  - 验证：三个按钮功能正确。
  - 证据：运行时观察。

### 阶段 6：目标双向高亮

- [ ] 6.1 2D 地图红点点击 -> 目标表行高亮
  - TacticalMapWidget::targetSelected -> MainWindow -> LeftPanelWidget::selectTargetRow。
  - 验证：点击红点目标表行高亮。
  - 证据：截图。

- [ ] 6.2 目标表行点击 -> 2D 地图红点高亮
  - LeftPanelWidget::targetSelected -> MainWindow -> TacticalMapWidget::highlightTarget。
  - 验证：点击目标表行红点高亮。
  - 证据：截图。

### 阶段 7：集成测试与验收

- [ ] 7.1 端到端测试
  - 新增 `detection_stage_e2e` 测试：模拟视频位置变化，验证 5 个时间点触发四区同步。
  - 验证：测试通过。
  - 证据：`ctest --test-dir build --output-on-failure` 通过。

- [ ] 7.2 构建验收
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` 通过。
  - `cmake --build build --target UXOMissionControl -j2` 通过。
  - 证据：构建日志。

- [ ] 7.3 安全验收
  - 扫描无 `system()`/`popen`/`QProcess`/socket/真实设备命令。
  - 扫描无明文密钥。
  - 检查所有模拟数据标注"模拟"或"演示"。
  - 证据：扫描结果。

- [ ] 7.4 文档回写
  - 更新 `docs/PRODUCT.md` §6 REQ-008 状态为 Implemented（需用户验收后）。
  - 更新 `docs/ARCHITECTURE.md` §4 状态所有权表。
  - 更新 `docs/UI.md` §4.3 中央工作区矩阵。
  - 更新 `docs/DEVELOPMENT.md` 测试章节。
  - 证据：git diff。

## 最终验证

- [ ] F1. `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` 退出 0。
- [ ] F2. `cmake --build build --target UXOMissionControl -j2` 退出 0。
- [ ] F3. `ctest --test-dir build --output-on-failure` 全部通过（现有 4 + 新增 3）。
- [ ] F4. 启动客户端，点击 [开始]，观察 80 秒完整演示：5 个目标按时间点流入，四区同步。
- [ ] F5. [重置] 清空所有，[结束] 暂停保留，目标双向高亮均工作。
- [ ] F6. 所有模拟数据标注"模拟"或"演示"。
- [ ] F7. 无真实设备控制/外部通信/数据库写入/明文密钥。

## 完成条件

用户能在态势页点击 [开始] 后看到 80 秒完整探测演示：视频播放、红框闪烁、5 个目标按时间点流入、2D 地图红点脉冲、目标表/告警/日志四区同步。演示效果从"悬浮枚举变化"变成"有真实感的探测过程"。

完成后暂停，不继续做识别/规划/排弹阶段，等待用户验收和后续功能审批。
