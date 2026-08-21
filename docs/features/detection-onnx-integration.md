# UXO 检测算法嵌入

状态：Implemented
关联产品需求：REQ-009（requirements/REQ-009.md）
关联版本：待确认

## 1. 问题与目标

### 1.1 问题

当前系统存在两个缺口：

1. **态势页检测是模拟数据**：REQ-009 实现的 `DetectionSimulator` 用预设脚本驱动检测时机，接口注释写道"真实系统替换为 AI 推理引擎即可"，但从未接入真实算法。检测到的目标类型、位置、置信度都是硬编码的模拟值。

2. **探测页工作流错误**：导航 index 1 的探测页已接入页面栈，但 `DetectionView` 当前实现为手动图像分析工作站（手动导入图像 -> 点击分析），不符合真实工作流。正确工作流是：只要无人机有视频信号，探测流程就自动开始--态势页视频抽帧，探测页对抽帧图片自动分析，结果列在左侧，点击查看大图做人工二次校验。

UXO 检测算法（Phase 1 完成的 PatchCore + YOLOv8-cls 两阶段管线）已验证可导出为 ONNX 格式，数值精度与 Python 推理一致（最大差异 1.2e-6），可在 C++ 端直接运行，无需 Python 运行时。Detection 静态库已实现并编译通过。

### 1.2 目标

将真实 UXO 检测算法以 ONNX Runtime 嵌入客户端，实现自动视频驱动检测流程：

1. **态势页**：视频播放时定时抽帧（默认每 3 秒），抽取的帧自动送入 DetectionEngine。
2. **DetectionEngine**：接收帧，异步执行两阶段检测（PatchCore 异常检测 + YOLOv8-cls 分类），发射结果信号。
3. **探测页**：检测结果自动填充左侧表格；点击结果在中心区查看干净原图 + 分类结果，右侧热力图模块独立展示异常热力图；人工确认/拒绝做二次校验。
4. **态势页四区同步**：当 DetectionEngine 检出异常时，触发四区同步（冻结标注证据捕获 + 目标表插行 + 地图标点 + 告警/日志），替代 DetectionSimulator 的模拟检测事件。

### 1.3 与 REQ-009 的关系

REQ-009（无人机探测态势演示）的 `DetectionSimulator` 接口设计已预留替换点--`detectionOccurred(DetectionResult)` 信号的语义与真实检测器输出一致。本功能将 `DetectionSimulator` 替换为 `DetectionEngine`：相同的四区同步机制，真实 AI 推理替代模拟脚本。

REQ-009 的功能文档 `drone-detection-demo.md` 明确写道："接口贴合真实检测器--真实系统替换为 AI 推理引擎即可"。本功能即是该替换的实现。

### 1.4 与 ARCHITECTURE.md 的关系

ARCHITECTURE.md §5 描述了未来 `Integration` 库用于适配外部系统（AI、传感器），"仅在批准需求后引入"。本功能是用户明确批准的 AI 嵌入需求。由于算法以 ONNX 模型文件形式嵌入（不是外部服务），使用已实现的 `Detection` 静态库而非 `Integration` 库--`Integration` 预留给未来外部服务适配（传感器、无人机遥测等）。

### 1.5 与 detection.md 的关系

`docs/ui/pages/detection.md`（状态：TARGET / Draft）是探测页面的完整 UI 设计契约，定义了三栏布局（左面板搜索+筛选+目标表 / 中心区证据Tab+确认操作条 / 右面板目标详情+时间线）和 `DET-*` 控件 ID。本功能以该文档为 UI 设计依据，实现时对齐其布局结构和控件规格。

## 2. 范围与非目标

### 2.1 范围

- **DetectionEngine 替换 DetectionSimulator**：态势页视频抽帧驱动 DetectionEngine，真实 ONNX 推理替代模拟脚本。DetectionSimulator 代码保留但不接线（备用回退）。
- **视频抽帧**：VideoStreamPanel 新增定时抽帧机制（默认 3 秒间隔），发射 `frameExtracted(QImage, qint64)` 信号。
- **DetectionEngine 扩展**：新增 `analyzeFrame(QImage, qint64)` 方法，接收内存帧（不限于文件路径）。
- **DetectionView 重新设计**：从手动图像队列改为自动结果填充的三栏布局，对齐 detection.md 设计契约。
- **探测页三栏布局**：左侧检测结果表（自动填充）、中心证据查看器（干净原图 + 分类结果）、右侧异常热力图模块 + 目标详情 + 状态时间线。
- **人工二次校验**：确认/拒绝操作条，目标状态机 `Detected -> Confirmed / Rejected`。
- **四区同步适配**：DetectionEngine 检出异常时，MainWindow 将结果映射为 `Core::DetectionResult`，触发既有四区同步机制。
- **异步推理**：检测在后台线程执行，UI 不阻塞。
- **阈值修正**：PatchCore 归一化阈值从代码中硬编码的 0.25 修正为从 `patchcore_params.json` 的 `image_threshold` 推导（(2.4904−1.5988)/(3.3820−1.5988) = 0.500）。
- **检测结果标注"AI 分析"来源标识**，与态势页的"模拟"/"演示"标识区分。

### 2.2 非目标

- **不做实时逐帧检测**：抽帧间隔 3 秒，不是每帧检测。CPU 上 PatchCore 16-patch 推理约 800ms/帧，无法实时。
- **不修改态势页 UI 布局**：态势页的中心区、视频 PiP、地图、工具栏布局保持不变。仅替换检测源（DetectionSimulator -> DetectionEngine）。
- **不删除 DetectionSimulator 代码**：保留为备用回退，仅停止接线。
- **不做模型训练/微调**：嵌入的是 Phase 1 已训练好的模型，不在客户端内训练。
- **不做检测结果持久化**：当前项目无数据库，检测结果仅内存持有。
- **不做真实设备控制、外部通信、排弹动作**（AGENTS.md 安全边界）。
- **不做 detection.md 的全部细节实现**：detection.md 中搜索栏、三维筛选器、排序按钮、证据 Tab 等控件属于后续迭代，本次只实现核心检测流程相关的控件。

## 3. 用户流程

### 3.1 主流程（自动检测）

1. 用户在态势页点击 [开始] 按钮，视频开始播放，无人机遥测模拟器开始输出 GPS 坐标。
2. VideoStreamPanel 内部定时器每 3 秒抽取一帧视频画面，发射 `frameExtracted(QImage, qint64)` 信号。
3. MainWindow 接收 `frameExtracted`，调用 `DetectionEngine.analyzeFrame(frame, timestampMs)`。
4. DetectionEngine 在后台线程执行两阶段检测（PatchCore + YOLO），完成后发射 `imageAnalyzed(ImageDetectionResult)` 信号。
5. MainWindow 接收 `imageAnalyzed`：
   - 转发结果到 DetectionView，自动插入左侧检测结果表。
   - 若 `hasAnomaly == true` 且 YOLO 分类确认（两阶段门控），将结果映射为 `Core::DetectionResult`，触发四区同步（冻结标注证据捕获 + 目标表插行 + 地图标点 + 告警/日志）；YOLO 未确认的异常帧仅记录到探测页。
6. 全程无需用户手动操作，检测随视频播放自动进行。

### 3.2 探测页人工校验流程

1. 用户点击导航 [探测] 进入探测页。
2. 左侧检测结果表已自动填充（来自 3.1 的自动检测），每行显示：目标 ID、类型、威胁等级、置信度、时间戳、状态。
3. 用户点击表格中某行：
   - 中心区显示该帧的干净原图（不叠加标注）。
   - 中心区底部显示 UXO 分类结果（Top-3 类名 + 置信度）。
   - 右侧热力图模块显示异常热力图（原图 + 半透明 jet 伪彩色），下方显示目标详情字段和状态历史时间线。
4. 用户审核证据后：
   - 点击 [确认]：目标状态 `Detected -> Confirmed`。
   - 点击 [拒绝]：目标状态 `Detected -> Rejected`（误报）。
5. 确认/拒绝结果同步回态势页目标表。

### 3.3 态势页联动流程

当 DetectionEngine 检出异常（`hasAnomaly == true`）时：

1. MainWindow 将 `ImageDetectionResult` 映射为 `Core::DetectionResult`：
   - UXO 紧包框（`PatchResult::targetRect`，amap 峰值连通域推导，与红框/证据标注同源）-> `videoRect`（归一化画面坐标）；无有效框时回退格子坐标。
   - YOLO 分类结果 -> `TargetType`（类名映射）。
   - YOLO 置信度 -> `confidence`（YOLO 未确认时不生成目标）。
2. 触发既有四区同步机制（与 DetectionSimulator 的 `detectionOccurred` 路径相同）：
   - 取该帧 AI 标注图（annotatedImage，无则热力图叠加图）-> 冻结标注证据。
   - 目标表插入新行。
   - 地图在推算的目标地表坐标处加标点。
   - 告警面板插入 1 条，日志追加 1 条。
3. 证据图片使用 DetectionEngine 的 `annotatedImage`（AI 红框标注图），无标注图时回退 `heatmapOverlay`（热力图叠加图）；不再使用 `currentFrameSnapshot()` + 手工标注。

### 3.4 停止与重置

- 视频播放结束：抽帧停止，DetectionEngine 完成已排队帧的分析，已有检测结果保留。
- 点击 [重置]：清空所有检测结果、目标、证据，回到初始状态。

## 4. 需求增量

### REQ-009 扩展：从检测模拟升级为真实 AI 推理

REQ-009 验收结果中的"检测模拟器在画面中发现目标时输出检测结果"升级为"DetectionEngine 对视频抽帧执行真实 AI 推理，检出异常时输出检测结果"。四区同步机制不变，检测源从模拟脚本变为真实算法。

**待确认**：此扩展是否需要新建 REQ-010 单独跟踪，或在 REQ-009 下直接实施。见 §9.1。

## 5. 架构增量

### 5.1 已实现组件（Detection 静态库）

以下组件已实现、编译通过、模型可加载，不需要重建：

| 组件 | 类型 | 职责 | 状态 |
|------|------|------|------|
| `DetectionEngine` | QObject 子类 | 检测编排器：加载模型，接收图像路径，异步执行两阶段管线，发射结果信号 | 已实现，需扩展 |
| `PatchCoreDetector` | 普通类 | PatchCore ONNX 推理：512x512 整帧单次推理 -> pred_score + anomaly_map -> 4x4 分区聚合出 16 个 patch 异常分数 + 热力图 | 已实现 |
| `YoloClassifier` | 普通类 | YOLOv8-cls ONNX 推理：224x224 输入 -> 多尺度 TTA -> 9 类概率聚合 | 已实现 |
| `DetectionTypes.h` | 头文件 | 数据结构定义：PatchResult、ClassificationResult、ImageDetectionResult | 已实现，需扩展 |

### 5.2 修改组件

#### 5.2.1 DetectionEngine

| 变更 | 说明 |
|------|------|
| 新增 `analyzeFrame(const QImage& frame, qint64 timestampMs)` | 接收内存帧（不限于文件路径），异步分析，复用 `imageAnalyzed` 信号 |
| 重构 `doAnalyze` | 拆分为 `doAnalyzeImage(const QImage& image, qint64 timestampMs)`，`doAnalyze(QString)` 改为先加载图像再调用 `doAnalyzeImage` |
| `ImageDetectionResult` 新增 `qint64 timestampMs` 字段 | 记录帧时间戳，用于探测页结果排序和时间线显示 |

#### 5.2.2 VideoStreamPanel

| 变更 | 说明 |
|------|------|
| 新增 `QTimer *m_extractionTimer` | 3 秒间隔定时器，play 时 start，pause/stop 时 stop |
| 新增 `frameExtracted(const QImage& frame, qint64 timestampMs)` 信号 | 定时器触发时，发射 QVideoProbe 缓存的 `m_lastFrame`，携带播放位置时间戳 |
| 新增 `startFrameExtraction()` / `stopFrameExtraction()` 方法 | 控制抽帧定时器，由 MainWindow 在 [开始]/[结束]/[重置] 时调用 |

VideoStreamPanel 已有 `QVideoProbe` 和 `m_lastFrame`，`currentFrameSnapshot()` 已实现，无需新增帧捕获机制--仅增加定时器和信号。

#### 5.2.3 DetectionView（重新设计）

当前实现为手动图像队列工作流，需要完全重新设计为自动结果填充工作流：

| 当前（废弃） | 新设计 |
|-------------|--------|
| 手动 [导入图像] 按钮 + 文件队列 | 自动填充检测结果表（来自 DetectionEngine） |
| [开始分析] / [停止分析] 手动触发 | 无手动触发，自动接收结果 |
| DetectionView 拥有 `m_engine`（DetectionEngine*） | DetectionView 不拥有 Engine，仅接收 MainWindow 转发的结果 |
| `m_imageList`（QListWidget 文件列表） | `m_resultTable`（QTableWidget 结果表，7 列） |
| `m_imageViewer`（QLabel 图像显示） | 证据查看器：干净原图 + 分类结果（热力图在右栏独立模块） |
| 无确认/拒绝操作 | 确认/拒绝操作条 |
| 无右侧详情面板 | 目标详情 + 状态历史时间线 |

新接口：

```cpp
class DetectionView : public QWidget {
public:
    explicit DetectionView(QWidget *parent = nullptr);

public slots:
    // 由 MainWindow 转发 DetectionEngine::imageAnalyzed 结果
    void onFrameAnalyzed(const ImageDetectionResult& result);
    // 清空所有结果（重置时调用）
    void clearResults();

signals:
    // 人工校验结果
    void targetConfirmed(const QString& targetId);
    void targetRejected(const QString& targetId);
    // 选中结果时通知 MainWindow（用于态势页联动）
    void resultSelected(const QString& targetId);
};
```

#### 5.2.4 MainWindow

| 变更 | 说明 |
|------|------|
| 新增 `DetectionEngine *m_detectionEngine` | 替代 `DetectionSimulator` 作为检测源。Engine 在 MainWindow 构造时初始化，加载 ONNX 模型。 |
| 连接 `VideoStreamPanel::frameExtracted -> DetectionEngine::analyzeFrame` | 视频抽帧驱动检测 |
| 连接 `DetectionEngine::imageAnalyzed -> DetectionView::onFrameAnalyzed` | 结果转发到探测页 |
| 连接 `DetectionEngine::imageAnalyzed -> MainWindow::onFrameAnalyzed` | 若 `hasAnomaly`，映射为 `Core::DetectionResult`，触发四区同步 |
| `DetectionSimulator` 接线移除 | `positionChanged -> DetectionSimulator` 断开。DetectionSimulator 代码保留但不实例化/不接线。 |
| `onDetectionOccurred` 适配 | 新增 `onFrameAnalyzed(ImageDetectionResult)` 方法，内部映射后复用四区同步逻辑 |

### 5.3 数据结构

#### 5.3.1 ImageDetectionResult 扩展

```cpp
struct ImageDetectionResult {
    QString   imagePath;
    QImage    originalImage;           // 512x512 original
    QImage    heatmapOverlay;          // 512x512 with heatmap overlaid
    QVector<PatchResult> patches;
    QVector<ClassificationResult> classifications;
    bool      hasAnomaly       = false;
    float     maxAnomalyScore  = 0.0f;
    qint64    processingTimeMs = 0;
    QString   error;
    qint64    timestampMs      = 0;    // 新增：视频帧时间戳（用于排序和时间线）
};
```

#### 5.3.2 ImageDetectionResult -> Core::DetectionResult 映射

当 `hasAnomaly == true` 时，MainWindow 将 `ImageDetectionResult` 映射为 `Core::DetectionResult` 以复用四区同步：

| ImageDetectionResult 字段 | Core::DetectionResult 字段 | 映射规则 |
|--------------------------|---------------------------|----------|
| `targetRect`（异常 patch） | `videoRect` (QRectF) | 紧包框（amap 峰值连通域，512 域）归一化到 [0,1]；无效时回退 `QRectF(col/4.0, row/4.0, 0.25, 0.25)` |
| `classifications[0].bestClassName` | `type` (TargetType) | 类名映射表（见下） |
| `classifications[0].confidence` 或 `maxAnomalyScore` | `confidence` (double) | 优先使用 YOLO 置信度；无分类结果时使用异常分数 |

YOLO 类名 -> TargetType 映射：

| CLASS_NAMES (ONNX) | Core::TargetType | 中文 |
|--------------------|------------------|------|
| aircraft-bombs | (航弹类型) | 航弹 |
| landmines | (地雷类型) | 反跑道雷 |
| rockets | (火箭弹类型) | 火箭弹 |
| submunitions | (集束弹类型) | 集束弹 |
| mortars | (迫击炮弹类型) | 迫击炮弹 |
| grenades | (手榴弹类型) | 手榴弹 |
| projectiles | (投射物类型) | 投射物 |
| fuzes | (引信类型) | 引信 |
| background | (不映射) | - |

> 注：具体 TargetType 枚举值需对齐 `Core/Data/Types.h` 中的定义。若现有枚举不包含所有 UXO 类别，需扩展枚举。

### 5.4 数据流

```
[开始] -> VideoStreamPanel.play()
       -> VideoStreamPanel.startFrameExtraction()  // 3s 定时器启动
       -> DroneTelemetrySimulator.start()

VideoStreamPanel m_extractionTimer timeout
  -> m_lastFrame  // QVideoProbe 缓存帧
  -> frameExtracted(QImage, qint64 timestampMs)
  -> MainWindow::onFrameExtracted(frame, ts)
  -> DetectionEngine::analyzeFrame(frame, ts)
     -> QtConcurrent::run:
         -> QImage resize to 512x512
         -> PatchCoreDetector.detect(image)     // 16 patches
         -> YoloClassifier.classify(crops)      // 异常 patch 分类
         -> generateOverlay(original, patches)  // 热力图叠加
     -> QFutureWatcher::finished:
         -> imageAnalyzed(ImageDetectionResult with timestampMs)

DetectionEngine::imageAnalyzed(result)
  -> DetectionView::onFrameAnalyzed(result)     // 探测页结果表自动填充
  -> MainWindow::onFrameAnalyzed(result)        // 四区同步判断
     -> if result.hasAnomaly:
          -> 映射为 Core::DetectionResult
          -> 四区同步（冻结证据 + 目标表 + 地图 + 告警/日志）

[探测页] 用户点击结果行
  -> DetectionView::resultSelected(targetId)
  -> MainWindow::onSelectTargetEverywhere(targetId)  // 既有双向联动
```

### 5.5 ONNX Runtime 集成

- ONNX Runtime C++ SDK（v1.23.2，Linux x64 CPU）已放置于 `third_party/onnxruntime/`。
- CMake 通过 `IMPORTED` 目标引用，已配置完成。
- 仅使用 CPU Execution Provider，无 GPU 依赖。
- RPATH：根 CMakeLists 通过 `CMAKE_BUILD_RPATH` 指向源码树 `third_party/onnxruntime/lib`（绝对路径，仅开发期）；安装版通过 `CMAKE_INSTALL_RPATH = $ORIGIN/../lib` 相对解析，安装布局见 §9.5。

### 5.6 模型文件

| 文件 | 路径 | 大小 | 说明 |
|------|------|------|------|
| PatchCore ONNX | `assets/models/patchcore_512.onnx` | 22.2 MB | 输入 [B,3,512,512]，输出 anomaly_score[B] + anomaly_map[B,1,512,512]（patchcore_128 为历史实验版本，未入库） |
| YOLOv8-cls ONNX | `assets/models/yolov8_cls_224.onnx` | 19.4 MB | 输入 [1,3,224,224]，输出 probs[1,9] |
| 归一化参数 | `assets/models/patchcore_params.json` | < 1 KB | image_min, image_max, image_threshold |

### 5.7 异步策略

- `DetectionEngine::analyzeFrame()` 通过 `QtConcurrent::run` 在线程池执行。
- 推理完成后通过 `QFutureWatcher` 在主线程发射 `imageAnalyzed` 信号。
- 抽帧间隔 3 秒 > 推理耗时约 800ms，不会产生队列积压。
- 若前一帧仍在分析中，新帧到达时跳过（不排队），保证检测结果不滞后。

### 5.8 阈值修正

最初代码 `DetectionConst::DEFAULT_PC_THRESHOLD = 0.25f`，阈值过低会产生大量误报。`patchcore_params.json` 中 `image_threshold = 2.4904`、`image_min = 1.5988`、`image_max = 3.382`，归一化后应为：

```
normalized_threshold = (image_threshold - image_min) / (image_max - image_min)
                     = (2.4904 - 1.5988) / (3.382 - 1.5988)
                     = 0.500
```

实现为运行时从 `patchcore_params.json` 读取 `image_min` / `image_max` / `image_threshold` 并按上式推导归一化阈值（`DetectionEngine::initialize`，启动日志可见 "normalized threshold"）；`DetectionConst::DEFAULT_PC_THRESHOLD = 0.500f` 仅作推导值的常量对照，代码未直接引用。

### 5.9 帧预处理

视频帧为 1920x1080，PatchCore 需要 512x512 输入（4x4 网格，每 patch 128x128）。预处理策略：

1. 方案 A：直接 resize 到 512x512（简单，轻微变形）。
2. 方案 B：等比缩放 + 填充（保持宽高比，有黑边）。
3. 方案 C：中心裁剪 1080x1080 -> resize 512x512（无变形，丢失边缘）。

建议方案 A，实现阶段确认。见 §9.4。

## 6. UI 增量

### 6.1 探测页三栏布局

对齐 `docs/ui/pages/detection.md` 设计契约：

```
┌─────────────────────────────────────────────────────────────┐
│ 导航 │  探测工具栏: [刷新探测]          │ 状态: AI 检测中  │
│      ├──────────┬───────────────────────────┬───────────────┤
│  态势 │          │                           │               │
│      │  检测结果  │     证据查看器             │  目标详情      │
│ ▶探测 │  表(自动  │    (干净原图)             │               │
│      │  填充)    │                           │  类型: 航弹    │
│  决策 │  ┌─────┐ │   ┌───────────────────┐  │  置信度: 82%   │
│      │  │T-001│ │   │                   │  │  异常分数: 0.73│
│  设备 │  │航弹  │ │   │   512x512 原图     │  ├───────────────┤
│      │  │82%  │ │   │                   │  │  异常热力图     │
│  统计 │  ├─────┤ │   │                   │  │ ┌───────────┐ │
│      │  │T-002│ │   └───────────────────┘  │ │ 原图+伪彩色 │ │
│  配置 │  │地雷  │ │                           │ └───────────┘ │
│      │  │91%  │ │  分类: mortars 0.82        │  状态历史      │
│      │  └─────┘ │                           │  ● 14:30:02   │
│      ├──────────┴───────────────────────────┴───────────────┤
│      │  [确认] [拒绝] [移除记录] 当前目标: T-001 · 状态: 已发现 │
└──────┴───────────────────────────────────────────────────────┘
```

### 6.2 左栏：检测结果表

- 宽 360px，对齐 `detection.md` 区域 A。
- 自动填充：每帧分析完成自动插入新行。
- 列：目标 ID | 类型 | 威胁 | 置信度 | 时间 | 状态 | 探测源。
- 点击行 -> 中心区显示原图，右栏显示热力图与详情。
- 状态：已发现(Detected) / 已确认(Confirmed) / 已拒绝(Rejected)。
- 异常行高亮（红色背景或红色类型标签）。

### 6.3 中栏：证据查看器

- 弹性宽度，对齐 `detection.md` 区域 B。
- 显示选中结果帧的干净原图（512x512，不叠加任何标注，热力图移至右栏）。
- 底部显示 UXO 分类结果（Top-3 类名 + 置信度条）。
- 底部确认操作条：[确认] [拒绝] [移除记录] + 当前目标状态标签。[移除记录] 为实现期补充的单项列表管理操作：从记录表移除当前行（含正常帧），不撤回已生成的目标与证据；全量清空仍走 [重置]（clearResults）。
- 空状态：提示"等待检测结果"。

### 6.4 右栏：目标详情 + 热力图 + 时间线

- 宽 380px，对齐 `detection.md` 区域 C。
- 上段：目标详情字段（目标 ID、类型、威胁等级、置信度、异常分数、探测时间、探测源）。
- 中段：异常热力图模块（原图 + 半透明 jet 伪彩色叠加），独立于主视图展示。
- 下段：状态历史时间线（已发现 -> 已确认/已拒绝）。

### 6.5 样式

延续项目 `GlobalStyle` 配色：
- 背景：`GlobalStyle::Colors::Background` (#1E1E1E)
- 面板背景：`GlobalStyle::Colors::PanelBackground` (#252526)
- 异常指示：`GlobalStyle::Colors::DangerRed` (#D32F2F)
- 正常指示：`GlobalStyle::Colors::PrimaryGreen` (#4A7A4C)
- 热力图：jet colormap（蓝 -> 绿 -> 黄 -> 红）

### 6.6 标识

探测页所有检测结果标注"AI 分析"来源，与态势页的"模拟"/"演示"标识区分。AI 分析结果是真实算法输出，不是模拟数据。

态势页的目标/告警/日志在 DetectionEngine 接入后不再标注"模拟"（因为检测源是真实 AI），但遥测模拟器和航线仍标注"模拟"。

## 7. 验收标准

### 7.1 功能验收

| # | 验收项 | 验证方法 |
|---|--------|---------|
| 1 | 视频播放时每 3 秒自动抽帧，无需手动操作 | 运行时观察日志 |
| 2 | DetectionEngine 对抽帧执行 ONNX 推理，UI 不卡顿 | 运行时观察 |
| 3 | 探测页左侧检测结果表自动填充，每帧分析完成插入新行 | 运行时观察 |
| 4 | 点击结果行 -> 中心区显示干净原图 + 分类结果，右栏热力图模块显示异常热力图 | 运行时操作 |
| 5 | 确认按钮 -> 目标状态变为"已确认" | 运行时操作 |
| 6 | 拒绝按钮 -> 目标状态变为"已拒绝" | 运行时操作 |
| 7 | 异常帧检测触发态势页四区同步（目标表 + 地图 + 告警 + 日志） | 运行时观察 |
| 8 | 正常帧（无异常）不触发四区同步 | 运行时观察 |
| 9 | DetectionSimulator 不再接线，态势页检测来自 DetectionEngine | 代码审查 |
| 10 | C++ ONNX 推理结果与 Python 基线一致（异常分数差异 < 0.01） | 回归测试 |
| 11 | 阈值使用 0.500（与 params.json 推导一致），非 0.25 | 代码审查 + 运行时验证 |
| 12 | 检测结果标注"AI 分析"来源 | 检查 UI 文案 |

### 7.2 构建与测试验收

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` 通过。
- `cmake --build build -j2` 通过。
- `ctest --test-dir build --output-on-failure` 全部通过（现有测试不受影响）。
- 未新增 C++ 单元测试（规划中的 DetectionEngine `analyzeFrame` 单测与 VideoStreamPanel 抽帧信号测试未实施）；Detection 行为回归目前依赖 Python 评估门与既有 17 项 CTest 工作流/UI 测试。

### 7.3 安全验收

- 无真实设备控制命令。
- 无外部通信调用。
- 无数据库写入。
- 模型文件不包含敏感信息。
- AI 分析结果标注来源，不冒充人工判断。
- 遥测模拟器仍标注"模拟"。

## 8. 对核心文档的影响

| 文档 | 变更 |
|------|------|
| `docs/ARCHITECTURE.md` §2 | 新增 `Detection` 构建目标到 CURRENT 工程概览 |
| `docs/ARCHITECTURE.md` §4 | 状态所有权表更新：`DetectionSimulator` -> `DetectionEngine` |
| `docs/ARCHITECTURE.md` §3 | 结构问题表更新：ONNX Runtime 为新增外部依赖 |
| `docs/PRODUCT.md` §6 | 功能清单新增"探测页 AI 自动检测" |
| `docs/UI.md` | 探测页从占位更新为已实现 |
| `docs/features/drone-detection-demo.md` | 备注：DetectionSimulator 已被 DetectionEngine 替代 |
| `docs/ui/pages/detection.md` | 状态从 TARGET/Draft 更新，标注已实现部分 |

以上文档变更在实现完成后回写，不在功能审批阶段修改。

## 9. 待确认事项

> **审批决策（Approved 时记录）**：以下各项按推荐方案执行——§9.1 方案 A（REQ-009 下直接实施）、§9.2 方案 A（接受无 UXO 视频，用测试图验证）、§9.3 硬编码 3 秒、§9.4 方案 A（直接 resize）、§9.5 当前部署方案、§9.6 直接提交、§9.7 核心控件优先、§9.8 扩展 TargetType 枚举。

### 9.1 需求关联：REQ-010 是否需要新建

本功能扩展了 REQ-009 的检测源（从模拟到真实 AI）。REQ-009 的功能文档 `drone-detection-demo.md` 已预留替换点（"真实系统替换为 AI 推理引擎即可"）。两种方案：

- 方案 A：在 REQ-009 下直接实施，不新建 REQ-010。理由：DetectionSimulator 接口设计已预留替换，本功能是 REQ-009 的自然演进。
- 方案 B：新建 REQ-010（真实 AI 检测嵌入），状态 Approved，关联本功能文档。理由：REQ-009 原始范围是"模拟演示"，真实 AI 嵌入是独立能力。

建议方案 A，由用户确认。

### 9.2 演示视频无 UXO 目标（实现阶段已按方案 B 解决）

设计阶段演示视频（`perth_airport_drone_edit.mp4`）为机场航拍，不含 UXO 目标。DetectionEngine 分析该视频帧时，PatchCore 应正确报告无异常（或低异常分数）。这意味着：

- 态势页四区同步不会被触发（无异常检测）。
- 探测页结果表会填充"正常"帧（无异常标记）。
- 无法演示"检出 UXO 目标"的场景。

解决方案（待用户确认）：

- 方案 A：接受现状。系统正确报告无异常，符合真实系统行为。用 `assets/detection_samples/` 中的合成测试图像验证检测能力。
- 方案 B：制作含 UXO 目标的演示视频（合成 UXO 到视频帧中）。
- 方案 C：保留 DetectionSimulator 作为演示回退，DetectionEngine 作为真实检测。用户可切换。

建议方案 A，由用户确认。

> 实现阶段（commit 1860e6e）演示视频已更换为 `perth_airport_drone_uxo.mp4`（1920×1080, 96s, 30fps），部分时段合成了 UXO 目标，实际效果等同方案 B：可演示异常检出与四区同步。
>
> 当前视频为 v3 运动补偿合成版（制作方案见 `uxo-detection-bench` 仓库 `reports/demo_video_composite_v3.md`）：四段目标为反跑道雷（8-14s）、迫击炮弹（37-43s）、投射物（46-52s）、火箭弹（64-69.5s），演示场景检测触发时刻对应 10s/40s/49s/66s（`DemoScenarioProvider`）。

### 9.3 抽帧间隔配置

默认 3 秒是否可接受？是否需要可配置（如配置页设置）？当前方案：硬编码 3 秒，不可配置。

### 9.4 帧预处理策略

1920x1080 -> 512x512 的预处理策略：

- 方案 A：直接 resize（简单，轻微变形）。
- 方案 B：等比缩放 + 填充（保持宽高比，有黑边）。
- 方案 C：中心裁剪 1080x1080 -> resize 512x512（无变形，丢失边缘）。

建议方案 A，由用户确认。

### 9.5 ONNX Runtime 部署方式

当前方案：共享库放在 `third_party/onnxruntime/lib/`，构建期通过根 CMakeLists 的 `CMAKE_BUILD_RPATH` 注入源码树绝对路径（仅开发期）。安装由根 CMakeLists 的 install 规则定义：`cmake --install --prefix <目录>` 产出 `bin/UXOMissionControl` + `lib/libonnxruntime.so.1`（真实库文件按 SONAME 重命名安装，避免符号链被解引用成多份拷贝）+ `share/uxo/assets/models/`（3 个运行时模型文件）；安装版 RPATH 为 `$ORIGIN/../lib`（须在 add_subdirectory 之前设置 `CMAKE_INSTALL_RPATH`，目标创建后设置不生效）。MainWindow 加载模型优先编译期源码树路径，源码树不存在时回退 `bin/../share/uxo/assets/models`（已实测）。Qt 运行库未打包，假定目标环境已安装。

### 9.6 模型文件版本管理

模型文件（36MB）已直接提交到 git 仓库。是否需要 git-lfs 管理？当前方案：直接提交，实现阶段确认。

### 9.7 detection.md 对齐范围

`detection.md` 定义了完整的探测页 UI 契约（搜索栏、三维筛选器、排序按钮、证据 Tab 等）。本次实现是否需要对齐全部控件，还是只实现核心检测流程相关的控件（结果表 + 证据查看器 + 确认/拒绝 + 详情/时间线）？

建议：本次只实现核心检测流程控件，搜索栏/筛选器/排序/证据 Tab 属于后续迭代。

### 9.8 TargetType 枚举扩展

ONNX 模型的 9 个类别（aircraft-bombs, background, fuzes, grenades, landmines, mortars, projectiles, rockets, submunitions）是否全部映射到 `Core::TargetType`？若现有枚举不包含所有类别，需扩展。实现阶段对齐。
