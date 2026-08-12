// 态势页重构后离屏 UI 契约测试：验证空起步目标注入、左面板折叠、设备资源条、
// 目标详情浮层、PiP 控制、双向高亮、端到端四目标同步、目标表四列契约、
// 证据元数据填充、结束保留和重置清除。不连接设备、网络或持久化。

#include "MainWindow/MainWindow.h"
#include "MainWindow/LeftPanelWidget.h"
#include "MainWindow/DeviceResourceBar.h"
#include "MainWindow/TargetDetailOverlay.h"
#include "MainWindow/TacticalMapWidget.h"
#include "MainWindow/VideoStreamPanel.h"
#include "Core/Simulation/DetectionSimulator.h"
#include "Core/Simulation/DemoScenarioProvider.h"
#include "Core/Simulation/DroneTelemetrySimulator.h"

#include <QtTest>
#include <QSignalSpy>
#include <QtMath>
#include <QVideoFrame>

#include <QAbstractButton>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>

#include <memory>

namespace {

// MainWindow 生成目标 ID 格式为 T-001, T-002, ...
const QString kTargetId = QStringLiteral("T-001");

template <typename WidgetType>
WidgetType *contractWidget(MainWindow &window, const char *objectName)
{
    return window.findChild<WidgetType *>(QString::fromLatin1(objectName));
}

int columnWithHeader(const QTableWidget *table, const QString &headerText)
{
    for (int column = 0; column < table->columnCount(); ++column) {
        const QTableWidgetItem *header = table->horizontalHeaderItem(column);
        if (header != nullptr && header->text().contains(headerText)) {
            return column;
        }
    }
    return -1;
}

QString firstTargetStatus(const QTableWidget *table, int statusColumn)
{
    const QTableWidgetItem *item = table->item(0, statusColumn);
    return item == nullptr ? QString() : item->text();
}

bool clickFirstTarget(QTableWidget *table)
{
    if (table->rowCount() == 0 || table->columnCount() == 0) {
        return false;
    }
    QTableWidgetItem *item = table->item(0, 0);
    if (item == nullptr) {
        return false;
    }
    table->scrollToItem(item);
    const QRect itemRect = table->visualItemRect(item);
    if (!itemRect.isValid()) {
        return false;
    }
    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, itemRect.center());
    return true;
}

// 创建离屏窗口并停止 loadMockData 自动启动的检测模拟器和视频，
// 避免模拟时钟在测试期间注入非预期目标。需要检测的测试自行调用 start()。
std::unique_ptr<MainWindow> createOffscreenWindow()
{
    auto window = std::make_unique<MainWindow>();
    const QString requestedSize = qEnvironmentVariable("UXO_VISUAL_EVIDENCE_SIZE");
    const QStringList dimensions = requestedSize.toLower().split(QLatin1Char('x'));
    if (dimensions.size() == 2) {
        bool widthValid = false;
        bool heightValid = false;
        const int width = dimensions.at(0).toInt(&widthValid);
        const int height = dimensions.at(1).toInt(&heightValid);
        if (widthValid && heightValid && width > 0 && height > 0) {
            window->resize(width, height);
        }
    }
    window->show();

    // 停止检测模拟器（loadMockData 已通过 loadDetections 加载数据，但 running=false）
    if (auto *sim = window->findChild<Core::Simulation::DetectionSimulator *>()) {
        sim->stop();
    }
    if (auto *video = window->findChild<VideoStreamPanel *>("videoPiP")) {
        video->pause();
    }
    return window;
}

// 向 VideoStreamPanel 注入一帧测试图像，使检测证据快照非空
void injectVideoFrame(MainWindow &window)
{
    auto *videoPanel = window.findChild<VideoStreamPanel *>("videoPiP");
    QVERIFY2(videoPanel != nullptr, "主窗口必须包含 VideoStreamPanel");
    QImage testFrame(320, 240, QImage::Format_RGB32);
    testFrame.fill(Qt::red);
    QVideoFrame frame(testFrame);
    videoPanel->onFrameProbed(frame);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QVERIFY2(videoPanel->hasFrame(), "注入帧后 VideoStreamPanel 必须持有帧");
}

// 注入 T-001 到窗口（先注入视频帧使证据快照非空，再启动检测模拟器 ->
// onPositionChanged 触发 detectionOccurred -> 处理事件循环）
void injectFirstTarget(MainWindow &window)
{
    injectVideoFrame(window);
    auto *sim = window.findChild<Core::Simulation::DetectionSimulator *>();
    QVERIFY2(sim != nullptr, "主窗口必须包含 DetectionSimulator");
    sim->start();
    const auto detections = Core::Simulation::DemoScenarioProvider::create().detections;
    QVERIFY2(!detections.isEmpty(), "检测数据不得为空");
    sim->onPositionChanged(detections.first().videoPositionMs);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

QString visualEvidenceFileName(QString fileName)
{
    const QString suffix = qEnvironmentVariable("UXO_VISUAL_EVIDENCE_SUFFIX");
    if (suffix.isEmpty()) {
        return fileName;
    }
    const int pos = fileName.lastIndexOf(QLatin1Char('.'));
    fileName.insert(pos < 0 ? fileName.size() : pos, suffix);
    return fileName;
}

QString geometryText(const QRect &rect)
{
    return QStringLiteral("%1,%2 %3x%4")
        .arg(rect.x())
        .arg(rect.y())
        .arg(rect.width())
        .arg(rect.height());
}

QString widgetText(const QWidget &widget)
{
    QString text;
    if (const auto *label = qobject_cast<const QLabel *>(&widget)) {
        text = label->text();
    } else if (const auto *button = qobject_cast<const QAbstractButton *>(&widget)) {
        text = button->text();
    }
    return text.replace(QLatin1Char('\t'), QLatin1Char(' '))
        .replace(QLatin1Char('\n'), QStringLiteral("\\n"));
}

QString widgetPath(const QWidget &widget)
{
    QStringList path;
    const QWidget *current = &widget;
    while (current != nullptr) {
        QString segment = QString::fromLatin1(current->metaObject()->className());
        if (!current->objectName().isEmpty()) {
            segment += QStringLiteral("#") + current->objectName();
        }
        path.prepend(segment);
        current = current->parentWidget();
    }
    return path.join(QLatin1Char('/'));
}

QString overflowText(const QRect &bounds, const QRect &container)
{
    QStringList overflow;
    if (bounds.left() < container.left()) {
        overflow.append(QStringLiteral("left=%1").arg(container.left() - bounds.left()));
    }
    if (bounds.top() < container.top()) {
        overflow.append(QStringLiteral("top=%1").arg(container.top() - bounds.top()));
    }
    if (bounds.right() > container.right()) {
        overflow.append(QStringLiteral("right=%1").arg(bounds.right() - container.right()));
    }
    if (bounds.bottom() > container.bottom()) {
        overflow.append(QStringLiteral("bottom=%1").arg(bounds.bottom() - container.bottom()));
    }
    return overflow.join(QLatin1Char(','));
}

void writeOverflowGeometryReport(MainWindow &window, const QString &imageFileName)
{
    if (!qEnvironmentVariableIsSet("UXO_VISUAL_GEOMETRY_REPORT")) {
        return;
    }

    const QString evidenceDirectory = qEnvironmentVariable("UXO_VISUAL_EVIDENCE_DIR");
    const QString reportName = QFileInfo(imageFileName).completeBaseName()
        + QStringLiteral("-geometry.tsv");
    QFile report(QDir(evidenceDirectory).filePath(reportName));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream output(&report);
    output.setCodec("UTF-8");
    output << "class\ttext\tobjectName\tparentClass\tpath\tgeometry\tparentRect\twindowBounds\twindowRect\tparentOverflow\twindowOverflow\n";

    const auto widgets = window.findChildren<QWidget *>();
    for (const QWidget *widget : widgets) {
        const QWidget *parent = widget->parentWidget();
        if (parent == nullptr || !widget->isVisibleTo(&window)) {
            continue;
        }

        const QRect parentBounds(widget->mapTo(parent, QPoint(0, 0)), widget->size());
        const QRect windowBounds(widget->mapTo(&window, QPoint(0, 0)), widget->size());
        const bool exceedsParent = !parent->rect().contains(parentBounds);
        const bool exceedsWindow = !window.rect().contains(windowBounds);
        if (!exceedsParent && !exceedsWindow) {
            continue;
        }

        output << widget->metaObject()->className() << '\t'
               << widgetText(*widget) << '\t'
               << widget->objectName() << '\t'
               << parent->metaObject()->className() << '\t'
               << widgetPath(*widget) << '\t'
               << geometryText(widget->geometry()) << '\t'
               << geometryText(parent->rect()) << '\t'
               << geometryText(windowBounds) << '\t'
               << geometryText(window.rect()) << '\t'
               << overflowText(parentBounds, parent->rect()) << '\t'
               << overflowText(windowBounds, window.rect()) << '\n';
    }
}

void captureVisualEvidence(MainWindow &window, const QString &fileName)
{
    const QString evidenceDirectory = qEnvironmentVariable("UXO_VISUAL_EVIDENCE_DIR");
    if (evidenceDirectory.isEmpty()) {
        return;
    }

    QCoreApplication::processEvents(QEventLoop::AllEvents);
    const QString outputFileName = visualEvidenceFileName(fileName);
    window.grab().save(QDir(evidenceDirectory).filePath(outputFileName), "PNG");
    writeOverflowGeometryReport(window, outputFileName);
}

} // namespace

class SimulationWorkflowUiTest : public QObject
{
    Q_OBJECT

private slots:
    void initialSurfaceIsEmptyAndExpanded();
    void unsafeControlsAreAbsent();
    void detectingTargetInjectsFourZoneSync();
    void leftPanelStartsExpanded();
    void leftPanelToggleChangesWidth();
    void deviceResourceBarLoadsDevices();
    void targetDetailOverlayShowsOnSelection();
    void targetDetailOverlayHidesOnClose();
    void pipControlsPresent();
    void targetRowClickHighlightsTacticalMap();
    void tacticalMapClickHighlightsTargetRow();
    void detectionStageE2EFourTargetsFourZoneSync();
    void targetTableHasExactlyFourColumns();
    void targetTableItemsAreNotCheckable();
    void targetTableSelectsRowsAndSingleSelection();
    void targetRowClickEmitsExactlyOneSignal();
    void targetSelectionShowsEvidenceMetadata();
    void stopPreservesStateAndResetClears();
    void tacticalMapSatelliteImageAspectFit();
    void detectionMarkerProjectsFromDroneTelemetry();
};

// 空起步: 启动时目标表 0 行, 左面板默认展开（探测阶段需可见目标列表）
void SimulationWorkflowUiTest::initialSurfaceIsEmptyAndExpanded()
{
    auto window = createOffscreenWindow();
    QTRY_COMPARE(window->isVisible(), true);
    captureVisualEvidence(*window, QStringLiteral("refactor-initial.png"));

    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    QVERIFY2(targetTable != nullptr, "缺少对象 targetTable");
    QCOMPARE(targetTable->rowCount(), 0);

    auto *leftPanel = window->findChild<LeftPanelWidget *>();
    QVERIFY2(leftPanel != nullptr, "主窗口必须包含 LeftPanelWidget");
    QVERIFY2(!leftPanel->isCollapsed(), "左面板默认必须展开（探测阶段需可见目标列表）");
    QCOMPARE(leftPanel->width(), 320);

    auto *tacticalMap = window->findChild<TacticalMapWidget *>("tacticalMap");
    QVERIFY2(tacticalMap != nullptr, "缺少对象 tacticalMap");
    QCOMPARE(tacticalMap->targetCount(), 0);
}

// 不安全设备控制按钮不得出现在模拟专用界面
void SimulationWorkflowUiTest::unsafeControlsAreAbsent()
{
    auto window = createOffscreenWindow();
    QTRY_COMPARE(window->isVisible(), true);

    const QStringList forbiddenTexts = {
        QStringLiteral("起飞"), QStringLiteral("降落"), QStringLiteral("返航"),
        QStringLiteral("直接启动")
    };
    QStringList visibleUnsafeControls;
    const auto buttons = window->findChildren<QPushButton *>();
    for (const QPushButton *button : buttons) {
        if (button->isVisibleTo(window.get()) && forbiddenTexts.contains(button->text())) {
            visibleUnsafeControls.append(button->text());
        }
    }

    QVERIFY2(visibleUnsafeControls.isEmpty(),
             qPrintable(QStringLiteral("模拟专用界面仍暴露不安全设备控制：%1")
                            .arg(visibleUnsafeControls.join(QStringLiteral(", ")))));
}

// 检测模拟器注入 T-001 后四区同步: 目标表 1 行, 2D 地图 1 个红点
void SimulationWorkflowUiTest::detectingTargetInjectsFourZoneSync()
{
    auto window = createOffscreenWindow();

    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *tacticalMap = window->findChild<TacticalMapWidget *>("tacticalMap");
    QVERIFY2(targetTable != nullptr && tacticalMap != nullptr,
             "缺少目标表或战术地图, 无法验证四区同步");

    injectFirstTarget(*window);

    QTRY_COMPARE(targetTable->rowCount(), 1);
    QCOMPARE(tacticalMap->targetCount(), 1);

    const int statusColumn = columnWithHeader(targetTable, QStringLiteral("状态"));
    QVERIFY2(statusColumn >= 0, "targetTable 必须提供模拟状态列");
    QCOMPARE(firstTargetStatus(targetTable, statusColumn), QStringLiteral("[模拟] 已发现"));

    captureVisualEvidence(*window, QStringLiteral("refactor-detected.png"));
}

// 左面板默认展开 (isCollapsed == false, width == 320)
void SimulationWorkflowUiTest::leftPanelStartsExpanded()
{
    auto window = createOffscreenWindow();
    auto *leftPanel = window->findChild<LeftPanelWidget *>();
    QVERIFY2(leftPanel != nullptr, "主窗口必须包含 LeftPanelWidget");

    QVERIFY2(!leftPanel->isCollapsed(), "左面板启动时必须默认展开");
    QCOMPARE(leftPanel->width(), 320);
}

// 切换折叠状态: 展开 320px -> 折叠 40px -> 展开 320px
void SimulationWorkflowUiTest::leftPanelToggleChangesWidth()
{
    auto window = createOffscreenWindow();
    auto *leftPanel = window->findChild<LeftPanelWidget *>();
    QVERIFY2(leftPanel != nullptr, "主窗口必须包含 LeftPanelWidget");

    QVERIFY2(!leftPanel->isCollapsed(), "起始态必须为展开");
    QCOMPARE(leftPanel->width(), 320);

    leftPanel->setCollapsed(true);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QVERIFY2(leftPanel->isCollapsed(), "setCollapsed(true) 后必须处于折叠态");
    QCOMPARE(leftPanel->width(), 40);

    leftPanel->setCollapsed(false);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QVERIFY2(!leftPanel->isCollapsed(), "setCollapsed(false) 后必须处于展开态");
    QCOMPARE(leftPanel->width(), 320);
}

// 设备资源条加载演示场景的 UAV + UGV 卡片
void SimulationWorkflowUiTest::deviceResourceBarLoadsDevices()
{
    auto window = createOffscreenWindow();
    auto *bar = window->findChild<DeviceResourceBar *>("deviceResourceBar");
    QVERIFY2(bar != nullptr, "主窗口必须包含 DeviceResourceBar");

    const auto scenario = Core::Simulation::DemoScenarioProvider::create();
    QCOMPARE(static_cast<int>(scenario.devices.size()), 2);

    for (const Core::DeviceInfo &dev : scenario.devices) {
        const QString cardName = QStringLiteral("deviceCard_%1").arg(dev.id);
        auto *card = window->findChild<QWidget *>(cardName);
        QVERIFY2(card != nullptr,
                 qPrintable(QStringLiteral("设备资源条缺少卡片 %1").arg(dev.id)));
    }
}

// 选中目标后浮层显现, 显示目标 ID 和类型
void SimulationWorkflowUiTest::targetDetailOverlayShowsOnSelection()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *overlay = window->findChild<TargetDetailOverlay *>("targetDetailOverlay");
    QVERIFY2(targetTable != nullptr && overlay != nullptr,
             "缺少目标表或目标详情浮层");

    injectFirstTarget(*window);
    QTRY_COMPARE(targetTable->rowCount(), 1);

    QVERIFY2(!overlay->isVisibleTo(window->centralWidget()),
             "注入目标但未选中时浮层不得显现");

    QVERIFY2(clickFirstTarget(targetTable), "targetTable 第一行必须可点击");
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    QVERIFY2(overlay->isVisibleTo(window->centralWidget()),
             "选中目标后浮层必须显现");
    QCOMPARE(overlay->currentTargetId(), kTargetId);

    auto *idLabel = contractWidget<QLabel>(*window, "targetDetailIdLabel");
    QVERIFY2(idLabel != nullptr, "浮层缺少目标 ID 标签");
    QCOMPARE(idLabel->text(), kTargetId);

    captureVisualEvidence(*window, QStringLiteral("refactor-overlay-shown.png"));
}

// 点击浮层关闭按钮后浮层隐藏
void SimulationWorkflowUiTest::targetDetailOverlayHidesOnClose()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *overlay = window->findChild<TargetDetailOverlay *>("targetDetailOverlay");
    QVERIFY2(targetTable != nullptr && overlay != nullptr,
             "缺少目标表或目标详情浮层");

    injectFirstTarget(*window);
    QTRY_COMPARE(targetTable->rowCount(), 1);
    QVERIFY2(clickFirstTarget(targetTable), "点击目标行");
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QVERIFY2(overlay->isVisibleTo(window->centralWidget()),
             "选中目标后浮层必须显现");

    auto *closeBtn = contractWidget<QPushButton>(*window, "targetDetailCloseButton");
    QVERIFY2(closeBtn != nullptr, "浮层缺少关闭按钮");
    QTest::mouseClick(closeBtn, Qt::LeftButton);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    QVERIFY2(!overlay->isVisibleTo(window->centralWidget()),
             "点击关闭按钮后浮层必须隐藏");
}

// PiP 控制按钮存在: 最小化 / 主次切换 / 关闭
void SimulationWorkflowUiTest::pipControlsPresent()
{
    auto window = createOffscreenWindow();

    auto *minimizeBtn = contractWidget<QPushButton>(*window, "pipMinimizeButton");
    auto *swapBtn = contractWidget<QPushButton>(*window, "pipSwapButton");
    auto *closeBtn = contractWidget<QPushButton>(*window, "pipCloseButton");
    QVERIFY2(minimizeBtn != nullptr && swapBtn != nullptr && closeBtn != nullptr,
             "PiP 控制条必须包含最小化/主次切换/关闭三个按钮");
}

// 前向链: 目标表行点击 -> 2D 地图高亮
void SimulationWorkflowUiTest::targetRowClickHighlightsTacticalMap()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    QVERIFY2(targetTable != nullptr, "缺少目标表");

    injectFirstTarget(*window);
    QTRY_COMPARE(targetTable->rowCount(), 1);

    QTableWidgetItem *item = targetTable->item(0, 0);
    QVERIFY2(item != nullptr, "目标表第0行无单元格");
    const QRect cellRect = targetTable->visualItemRect(item);
    QTest::mouseClick(targetTable->viewport(), Qt::LeftButton, Qt::NoModifier, cellRect.center());
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    auto *tacticalMap = window->findChild<TacticalMapWidget *>("tacticalMap");
    QVERIFY2(tacticalMap != nullptr, "未找到 tacticalMap 控件");
    QCOMPARE(tacticalMap->selectedTargetId(), kTargetId);
}

// 反向链: 2D 地图红点点击 -> 目标表行高亮
void SimulationWorkflowUiTest::tacticalMapClickHighlightsTargetRow()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    QVERIFY2(targetTable != nullptr, "缺少目标表");

    injectFirstTarget(*window);
    QTRY_COMPARE(targetTable->rowCount(), 1);

    auto *tacticalMap = window->findChild<TacticalMapWidget *>("tacticalMap");
    QVERIFY2(tacticalMap != nullptr, "未找到 tacticalMap 控件");

    // offscreen 下真实鼠标点击红点像素不稳定, 用元对象系统触发信号验证反向链
    QMetaObject::invokeMethod(tacticalMap, "targetClicked",
                              Qt::QueuedConnection,
                              Q_ARG(QString, kTargetId));
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    QVERIFY2(targetTable->selectionModel() != nullptr, "目标表无 selectionModel");
    QVERIFY2(targetTable->selectionModel()->isRowSelected(0),
             "目标表第0行应被 selectTargetRow 选中");
}

// 端到端: 推进 4 个检测时间点, 验证目标表和 2D 地图同步
void SimulationWorkflowUiTest::detectionStageE2EFourTargetsFourZoneSync()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *tacticalMap = window->findChild<TacticalMapWidget *>("tacticalMap");
    QVERIFY2(targetTable != nullptr && tacticalMap != nullptr,
             "缺少目标表或战术地图");

    auto *sim = window->findChild<Core::Simulation::DetectionSimulator *>();
    QVERIFY2(sim != nullptr, "主窗口必须包含 DetectionSimulator");

    const auto detections = Core::Simulation::DemoScenarioProvider::create().detections;
    QCOMPARE(static_cast<int>(detections.size()), 4);

    sim->start();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QVERIFY2(sim->isRunning(), "检测模拟器应处于运行态");
    QCOMPARE(targetTable->rowCount(), 0);
    QCOMPARE(tacticalMap->targetCount(), 0);

    // MainWindow::targetTypeName 返回的中文类型名（不含"模拟"前缀）
    const QStringList expectedTypeNames = {
        QStringLiteral("反跑道雷"),
        QStringLiteral("航弹"),
        QStringLiteral("子母弹"),
        QStringLiteral("简易爆炸装置")
    };

    for (int i = 0; i < static_cast<int>(detections.size()); ++i) {
        sim->onPositionChanged(detections[i].videoPositionMs);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        QCOMPARE(targetTable->rowCount(), i + 1);
        QCOMPARE(tacticalMap->targetCount(), i + 1);
        QCOMPARE(targetTable->item(i, 0)->text(), expectedTypeNames[i]);
    }

    // 超过最后一个检测时间点后不再产生新目标
    sim->onPositionChanged(80000);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCOMPARE(targetTable->rowCount(), 4);
    QCOMPARE(tacticalMap->targetCount(), 4);

    captureVisualEvidence(*window, QStringLiteral("refactor-e2e-four-targets.png"));
}

// 目标表必须恰好 4 列，表头为 类型/置信度/位置/模拟状态
void SimulationWorkflowUiTest::targetTableHasExactlyFourColumns()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    QVERIFY2(targetTable != nullptr, "缺少目标表");

    QCOMPARE(targetTable->columnCount(), 4);
    QCOMPARE(targetTable->horizontalHeaderItem(0)->text(), QStringLiteral("类型"));
    QCOMPARE(targetTable->horizontalHeaderItem(1)->text(), QStringLiteral("置信度"));
    QCOMPARE(targetTable->horizontalHeaderItem(2)->text(), QStringLiteral("位置"));
    QCOMPARE(targetTable->horizontalHeaderItem(3)->text(), QStringLiteral("模拟状态"));
}

// 目标表单元格不得可勾选（无 ItemIsUserCheckable）
void SimulationWorkflowUiTest::targetTableItemsAreNotCheckable()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    QVERIFY2(targetTable != nullptr, "缺少目标表");

    injectFirstTarget(*window);
    QTRY_COMPARE(targetTable->rowCount(), 1);

    for (int col = 0; col < targetTable->columnCount(); ++col) {
        QTableWidgetItem *item = targetTable->item(0, col);
        QVERIFY2(item != nullptr, "目标表单元格不应为空");
        QVERIFY2(!(item->flags() & Qt::ItemIsUserCheckable),
                 "目标表单元格不得可勾选");
    }
}

// 目标表必须为 SelectRows + SingleSelection
void SimulationWorkflowUiTest::targetTableSelectsRowsAndSingleSelection()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    QVERIFY2(targetTable != nullptr, "缺少目标表");

    QCOMPARE(targetTable->selectionBehavior(), QAbstractItemView::SelectRows);
    QCOMPARE(targetTable->selectionMode(), QAbstractItemView::SingleSelection);
}

// 点击目标行必须恰好发射一次 targetSelected 信号
void SimulationWorkflowUiTest::targetRowClickEmitsExactlyOneSignal()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *leftPanel = window->findChild<LeftPanelWidget *>();
    QVERIFY2(targetTable != nullptr && leftPanel != nullptr,
             "缺少目标表或左面板");

    injectFirstTarget(*window);
    QTRY_COMPARE(targetTable->rowCount(), 1);

    QSignalSpy spy(leftPanel, &LeftPanelWidget::targetSelected);
    QVERIFY2(clickFirstTarget(targetTable), "targetTable 第一行必须可点击");
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    QCOMPARE(spy.count(), 1);
}

// 选中目标后证据元数据必须填充，且冻结标注截图 pixmap 必须非空
void SimulationWorkflowUiTest::targetSelectionShowsEvidenceMetadata()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *overlay = window->findChild<TargetDetailOverlay *>("targetDetailOverlay");
    QVERIFY2(targetTable != nullptr && overlay != nullptr,
             "缺少目标表或目标详情浮层");

    injectFirstTarget(*window);
    QTRY_COMPARE(targetTable->rowCount(), 1);

    QVERIFY2(clickFirstTarget(targetTable), "点击目标行");
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QVERIFY2(overlay->isVisibleTo(window->centralWidget()),
             "选中目标后浮层必须显现");

    // 证据元数据标签必须填充（注入视频帧后证据快照非空，捕获时间/视频时间/来源写入）
    auto *provenanceLabel = contractWidget<QLabel>(*window, "targetDetailProvenanceValue");
    auto *captureTimeLabel = contractWidget<QLabel>(*window, "targetDetailCaptureTimeValue");
    auto *videoTimeLabel = contractWidget<QLabel>(*window, "targetDetailVideoTimeValue");
    QVERIFY2(provenanceLabel != nullptr, "浮层缺少证据来源标签");
    QVERIFY2(captureTimeLabel != nullptr, "浮层缺少捕获时间标签");
    QVERIFY2(videoTimeLabel != nullptr, "浮层缺少视频时间标签");

    QVERIFY2(!captureTimeLabel->text().isEmpty()
             && captureTimeLabel->text() != QStringLiteral("-"),
             "捕获时间不得为空或占位符");
    QVERIFY2(!videoTimeLabel->text().isEmpty()
             && videoTimeLabel->text() != QStringLiteral("-"),
             "视频时间不得为空或占位符");
    QVERIFY2(provenanceLabel->text().contains(QStringLiteral("模拟")),
             "证据来源必须标注模拟");

    // 冻结标注截图 pixmap 必须非空：注入视频帧后 onDetectionOccurred 捕获非空快照，
    // onSelectTargetEverywhere 调用 setEvidence 将 pixmap 设置到证据图像标签
    auto *evidenceImageLabel = contractWidget<QLabel>(*window, "targetDetailEvidenceImage");
    auto *evidencePlaceholder = contractWidget<QLabel>(*window, "targetDetailEvidencePlaceholder");
    QVERIFY2(evidenceImageLabel != nullptr, "浮层缺少证据图像标签");
    QVERIFY2(evidencePlaceholder != nullptr, "浮层缺少证据占位标签");

    QVERIFY2(evidenceImageLabel->pixmap() != nullptr
             && !evidenceImageLabel->pixmap()->isNull(),
             "选中目标后证据图像 pixmap 必须非空（注入视频帧后证据快照非空）");
    QVERIFY2(evidenceImageLabel->isVisibleTo(overlay),
             "有证据时证据图像标签必须可见");
    QVERIFY2(!evidencePlaceholder->isVisibleTo(overlay),
             "有证据时占位标签必须隐藏");
}

// [结束] 保留目标/地图/浮层/证据图像, [重置] 清空全部并清除证据 pixmap
void SimulationWorkflowUiTest::stopPreservesStateAndResetClears()
{
    auto window = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *tacticalMap = window->findChild<TacticalMapWidget *>("tacticalMap");
    auto *overlay = window->findChild<TargetDetailOverlay *>("targetDetailOverlay");
    QVERIFY2(targetTable != nullptr && tacticalMap != nullptr && overlay != nullptr,
             "缺少目标表、战术地图或目标详情浮层");

    injectFirstTarget(*window);
    QTRY_COMPARE(targetTable->rowCount(), 1);
    QVERIFY2(clickFirstTarget(targetTable), "点击目标行");
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QVERIFY2(overlay->isVisibleTo(window->centralWidget()),
             "选中后浮层必须显现");

    // 选中后证据图像 pixmap 必须非空（injectFirstTarget 已注入视频帧）
    auto *evidenceImageLabel = contractWidget<QLabel>(*window, "targetDetailEvidenceImage");
    QVERIFY2(evidenceImageLabel != nullptr, "浮层缺少证据图像标签");
    QVERIFY2(evidenceImageLabel->pixmap() != nullptr
             && !evidenceImageLabel->pixmap()->isNull(),
             "选中后证据图像 pixmap 必须非空");

    const int mapTargetsBeforeStop = tacticalMap->targetCount();

    // [结束] 保留目标/地图/浮层/证据图像 pixmap
    auto *stopBtn = contractWidget<QPushButton>(*window, "mapToolbarStop");
    QVERIFY2(stopBtn != nullptr, "缺少结束按钮");
    QTest::mouseClick(stopBtn, Qt::LeftButton);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    QCOMPARE(targetTable->rowCount(), 1);
    QCOMPARE(tacticalMap->targetCount(), mapTargetsBeforeStop);
    QVERIFY2(overlay->isVisibleTo(window->centralWidget()),
             "结束后浮层必须保留（证据不丢失）");
    QVERIFY2(evidenceImageLabel->pixmap() != nullptr
             && !evidenceImageLabel->pixmap()->isNull(),
             "结束后证据图像 pixmap 必须保留（不丢失）");

    // [重置] 清空目标/地图/浮层/证据图像 pixmap
    auto *resetBtn = contractWidget<QPushButton>(*window, "mapToolbarReset");
    QVERIFY2(resetBtn != nullptr, "缺少重置按钮");
    QTest::mouseClick(resetBtn, Qt::LeftButton);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    QCOMPARE(targetTable->rowCount(), 0);
    QCOMPARE(tacticalMap->targetCount(), 0);
    QVERIFY2(!overlay->isVisibleTo(window->centralWidget()),
             "重置后浮层必须隐藏");
    QVERIFY2(evidenceImageLabel->pixmap() == nullptr
             || evidenceImageLabel->pixmap()->isNull(),
             "重置后证据图像 pixmap 必须清空");
}

// 战术地图卫星底图 aspect-fit：保持原始宽高比居中放入 SCENE_SIZE 正方形场景，
// 不裁剪。横向底图上下留信箱区，纵向底图左右留信箱区。
void SimulationWorkflowUiTest::tacticalMapSatelliteImageAspectFit()
{
    // 独立构造 TacticalMapWidget（构造函数默认机场边界，不影响 aspect-fit 几何）
    TacticalMapWidget mapWidget;

    auto *view = mapWidget.findChild<QGraphicsView *>();
    QVERIFY2(view != nullptr, "TacticalMapWidget 必须包含 QGraphicsView");
    QGraphicsScene *scene = view->scene();
    QVERIFY2(scene != nullptr, "QGraphicsView 必须有 scene");

    // 在场景中查找底图
    auto findPixmapItem = [scene]() -> QGraphicsPixmapItem * {
        const auto items = scene->items();
        for (QGraphicsItem *item : items) {
            if (auto *p = qgraphicsitem_cast<QGraphicsPixmapItem *>(item)) {
                return p;
            }
        }
        return nullptr;
    };

    QVERIFY2(findPixmapItem() == nullptr,
             "未加载底图时场景不得有 QGraphicsPixmapItem");

    // 横向 2:1 底图（200x100）-> 缩放为 1000x500，上下各留 250px 信箱区
    const QString widePath = QDir::tempPath() + QStringLiteral("/uxo_aspectfit_wide.png");
    QImage wideImg(200, 100, QImage::Format_RGB32);
    wideImg.fill(Qt::darkGreen);
    QVERIFY2(wideImg.save(widePath, "PNG"), "无法写入横向测试底图");
    mapWidget.setSatelliteImage(widePath);
    QFile::remove(widePath);

    QGraphicsPixmapItem *wideItem = findPixmapItem();
    QVERIFY2(wideItem != nullptr, "加载横向底图后场景必须含 QGraphicsPixmapItem");
    QCOMPARE(wideItem->sceneBoundingRect(), QRectF(0.0, 250.0, 1000.0, 500.0));

    // 纵向 1:2 底图（100x200）-> 缩放为 500x1000，左右各留 250px 信箱区
    const QString tallPath = QDir::tempPath() + QStringLiteral("/uxo_aspectfit_tall.png");
    QImage tallImg(100, 200, QImage::Format_RGB32);
    tallImg.fill(Qt::darkCyan);
    QVERIFY2(tallImg.save(tallPath, "PNG"), "无法写入纵向测试底图");
    mapWidget.setSatelliteImage(tallPath);
    QFile::remove(tallPath);

    QGraphicsPixmapItem *tallItem = findPixmapItem();
    QVERIFY2(tallItem != nullptr, "加载纵向底图后场景必须含 QGraphicsPixmapItem");
    QCOMPARE(tallItem->sceneBoundingRect(), QRectF(250.0, 0.0, 500.0, 1000.0));
}

// 端到端验证航向感知目标坐标投影：
// calculateTargetCoord 是 MainWindow 私有方法，无测试接口且不应为测试暴露公开 API；
// 改为端到端驱动：启动无人机遥测模拟器（同步发出 P1 遥测，航向 198.7°）-> 冻结 ->
// 注入视频帧并触发首个检测 -> onDetectionOccurred 按当前航向推算目标坐标 ->
// 选中目标读取浮层坐标标签 -> 断言坐标在机场边界内且接近 P1（航向投影偏移受 footprint 限制）。
void SimulationWorkflowUiTest::detectionMarkerProjectsFromDroneTelemetry()
{
    auto window = createOffscreenWindow();

    // 启动遥测模拟器：start() 同步发出航线起点 P1 的遥测（loadMockData 已加载航线但未启动）
    auto *droneSim = window->findChild<Core::Simulation::DroneTelemetrySimulator *>();
    QVERIFY2(droneSim != nullptr, "主窗口必须包含 DroneTelemetrySimulator");
    droneSim->start();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    droneSim->stop();  // 冻结遥测在 P1（航向 198.7°）

    injectVideoFrame(*window);
    auto *detSim = window->findChild<Core::Simulation::DetectionSimulator *>();
    QVERIFY2(detSim != nullptr, "主窗口必须包含 DetectionSimulator");
    detSim->start();
    const auto detections = Core::Simulation::DemoScenarioProvider::create().detections;
    QVERIFY2(!detections.isEmpty(), "检测数据不得为空");
    detSim->onPositionChanged(detections.first().videoPositionMs);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    QVERIFY2(targetTable != nullptr, "缺少目标表");
    QTRY_COMPARE(targetTable->rowCount(), 1);

    // 选中目标行以显示详情浮层（浮层 showTarget 写入坐标标签）
    QVERIFY2(clickFirstTarget(targetTable), "点击目标行以显示详情浮层");
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    auto *coordLabel = contractWidget<QLabel>(*window, "targetDetailCoordValue");
    QVERIFY2(coordLabel != nullptr, "浮层缺少坐标标签 targetDetailCoordValue");

    // 解析 "经度:<lng>° 纬度:<lat>°" 格式（6 位小数）
    const QString text = coordLabel->text();
    const QRegularExpression re(QStringLiteral("经度:(-?[0-9.]+)° 纬度:(-?[0-9.]+)°"));
    const QRegularExpressionMatch m = re.match(text);
    QVERIFY2(m.hasMatch(),
             qPrintable(QStringLiteral("坐标标签格式不符：") + text));
    const double lng = m.captured(1).toDouble();
    const double lat = m.captured(2).toDouble();

    const auto scenario = Core::Simulation::DemoScenarioProvider::create();

    // 目标坐标必须在机场边界内（航向投影后仍在机场范围内）
    QVERIFY2(lat > scenario.airportBounds.south && lat < scenario.airportBounds.north
             && lng > scenario.airportBounds.west && lng < scenario.airportBounds.east,
             qPrintable(QStringLiteral("目标坐标不在机场边界内：lat=%1 lng=%2")
                            .arg(lat).arg(lng)));

    // 目标坐标应接近 P1：航向投影偏移受 footprint 600m×400m 限制（<0.005°）
    const double p1Lat = scenario.droneRoute.first().lat;
    const double p1Lng = scenario.droneRoute.first().lng;
    QVERIFY2(qAbs(lat - p1Lat) < 0.005,
             qPrintable(QStringLiteral("纬度偏离 P1 超限：lat=%1 P1=%2")
                            .arg(lat).arg(p1Lat)));
    QVERIFY2(qAbs(lng - p1Lng) < 0.005,
             qPrintable(QStringLiteral("经度偏离 P1 超限：lng=%1 P1=%2")
                            .arg(lng).arg(p1Lng)));
}

QTEST_MAIN(SimulationWorkflowUiTest)

#include "simulation_workflow_ui_test.moc"
