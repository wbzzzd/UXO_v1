// 阶段 4 离屏 UI 契约测试：只验证本地模拟流程，不连接设备、网络或持久化服务。

#include "MainWindow/MainWindow.h"
#include "MainWindow/DetectionControlPanel.h"
#include "MainWindow/DeviceStatusPanel.h"
#include "MainWindow/LeftPanelWidget.h"

#include <QtTest>

#include <QAbstractButton>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QTableWidget>
#include <QTextEdit>
#include <QTextStream>

#include <memory>

namespace {

const QString kTargetId = QStringLiteral("target-demo-001");

template <typename WidgetType>
WidgetType *contractWidget(MainWindow &window, const char *objectName)
{
    return window.findChild<WidgetType *>(QString::fromLatin1(objectName));
}

QStringList missingContractObjects(MainWindow &window)
{
    QStringList missing;
    if (contractWidget<QTableWidget>(window, "targetTable") == nullptr) {
        missing.append(QStringLiteral("targetTable"));
    }
    if (contractWidget<QLabel>(window, "simulationTargetLabel") == nullptr) {
        missing.append(QStringLiteral("simulationTargetLabel"));
    }
    if (contractWidget<QLabel>(window, "simulationStatusLabel") == nullptr) {
        missing.append(QStringLiteral("simulationStatusLabel"));
    }
    if (contractWidget<QPushButton>(window, "simulationConfirmButton") == nullptr) {
        missing.append(QStringLiteral("simulationConfirmButton"));
    }
    if (contractWidget<QPushButton>(window, "simulationStartButton") == nullptr) {
        missing.append(QStringLiteral("simulationStartButton"));
    }
    if (contractWidget<QPushButton>(window, "simulationCompleteButton") == nullptr) {
        missing.append(QStringLiteral("simulationCompleteButton"));
    }
    QWidget *operationLog = contractWidget<QWidget>(window, "simulationOperationLog");
    if (qobject_cast<QTextEdit *>(operationLog) == nullptr
        && qobject_cast<QPlainTextEdit *>(operationLog) == nullptr) {
        missing.append(QStringLiteral("simulationOperationLog"));
    }
    if (contractWidget<QLabel>(window, "decisionSimulationStatusLabel") == nullptr) {
        missing.append(QStringLiteral("decisionSimulationStatusLabel"));
    }
    return missing;
}

bool hasVisibleLabelContaining(MainWindow &window, const QString &text)
{
    const auto labels = window.findChildren<QLabel *>();
    for (const QLabel *label : labels) {
        if (label->isVisibleTo(&window) && label->text().contains(text)) {
            return true;
        }
    }
    return false;
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

QString operationLogText(const QWidget *log)
{
    if (const auto *textEdit = qobject_cast<const QTextEdit *>(log)) {
        return textEdit->toPlainText();
    }
    if (const auto *plainTextEdit = qobject_cast<const QPlainTextEdit *>(log)) {
        return plainTextEdit->toPlainText();
    }
    return QString();
}

QStringList simulationLogLines(const QWidget *log)
{
    QStringList simulationLines;
    const QStringList lines = operationLogText(log).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        const int markerIndex = trimmed.indexOf(QStringLiteral("[模拟]"));
        if (markerIndex >= 0) {
            simulationLines.append(trimmed.mid(markerIndex));
        }
    }
    return simulationLines;
}

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
    return window;
}

QString visualEvidenceFileName(QString fileName)
{
    const QString suffix = qEnvironmentVariable("UXO_VISUAL_EVIDENCE_SUFFIX");
    if (suffix.isEmpty()) {
        return fileName;
    }

    const int extensionPosition = fileName.lastIndexOf(QLatin1Char('.'));
    fileName.insert(extensionPosition < 0 ? fileName.size() : extensionPosition, suffix);
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

// 仅在显式请求视觉证据时调整尺寸、抓图并记录溢出几何，不影响常规测试行为。
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

}

class SimulationWorkflowUiTest : public QObject
{
    Q_OBJECT

private slots:
    void initialSurfaceIsSimulationOnly();
    void unsafeControlsAreAbsent();
    void selectingFirstTargetShowsDetected();
    void validActionsUpdateAllUiState();
    void operationLogPreservesSimulationOrder();
    void newWindowStartsWithFreshWorkflow();
    void targetTableColumnsPreserveCjkAndCoordinates();
};

void SimulationWorkflowUiTest::initialSurfaceIsSimulationOnly()
{
    auto window = createOffscreenWindow();
    QTRY_COMPARE(window->isVisible(), true);
    captureVisualEvidence(*window, QStringLiteral("phase4-initial.png"));

    const QStringList missing = missingContractObjects(*window);
    QVERIFY2(missing.isEmpty(),
             qPrintable(QStringLiteral("缺少阶段 4 稳定对象名：%1").arg(missing.join(QStringLiteral(", ")))));

    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *targetLabel = contractWidget<QLabel>(*window, "simulationTargetLabel");
    auto *statusLabel = contractWidget<QLabel>(*window, "simulationStatusLabel");
    auto *confirmButton = contractWidget<QPushButton>(*window, "simulationConfirmButton");
    auto *startButton = contractWidget<QPushButton>(*window, "simulationStartButton");
    auto *completeButton = contractWidget<QPushButton>(*window, "simulationCompleteButton");
    auto *operationLog = contractWidget<QWidget>(*window, "simulationOperationLog");
    auto *decisionStatus = contractWidget<QLabel>(*window, "decisionSimulationStatusLabel");

    QCOMPARE(targetTable->rowCount(), 1);
    const int statusColumn = columnWithHeader(targetTable, QStringLiteral("状态"));
    QVERIFY2(statusColumn >= 0, "targetTable 必须提供可见的模拟状态列");
    QCOMPARE(firstTargetStatus(targetTable, statusColumn), QStringLiteral("[模拟] 已发现"));
    QVERIFY2(targetLabel->text().contains(QStringLiteral("未选择")), "初始时不得预选模拟目标");
    QCOMPARE(statusLabel->text(), QStringLiteral("模拟状态：未选择"));
    QCOMPARE(decisionStatus->text(), QStringLiteral("[模拟] 目标状态：未选择"));
    QVERIFY2(!confirmButton->isEnabled() && !startButton->isEnabled() && !completeButton->isEnabled(),
             "未选择目标时三个模拟操作按钮必须禁用");
    QVERIFY2(confirmButton->text().contains(QStringLiteral("模拟"))
                 && startButton->text().contains(QStringLiteral("模拟"))
                 && completeButton->text().contains(QStringLiteral("模拟")),
             "阶段 4 操作按钮必须明确标注为模拟操作");
    QCOMPARE(operationLogText(operationLog).trimmed(), QStringLiteral("暂无模拟操作记录（重启后清空）"));
    QVERIFY2(hasVisibleLabelContaining(*window, QStringLiteral("模拟模式")),
             "主窗口必须持续显示模拟模式标识");
}

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

    auto *detectionPanel = window->findChild<DetectionControlPanel *>();
    QVERIFY2(detectionPanel != nullptr, "主窗口必须包含 DetectionControlPanel 以限定设备控制检查范围");
    const auto sliders = detectionPanel->findChildren<QSlider *>();
    for (const QSlider *slider : sliders) {
        if (slider->isVisibleTo(detectionPanel)) {
            visibleUnsafeControls.append(QStringLiteral("探测控制高度滑块"));
        }
    }

    auto *deviceStatusPanel = window->findChild<DeviceStatusPanel *>();
    QVERIFY2(deviceStatusPanel != nullptr, "主窗口必须包含 DeviceStatusPanel 以限定设备控制台检查范围");
    const auto deviceButtons = deviceStatusPanel->findChildren<QPushButton *>();
    for (const QPushButton *button : deviceButtons) {
        if (button->isVisibleTo(deviceStatusPanel) && button->text() == QStringLiteral("控制台")) {
            visibleUnsafeControls.append(button->text());
        }
    }

    QVERIFY2(visibleUnsafeControls.isEmpty(),
             qPrintable(QStringLiteral("模拟专用界面仍暴露不安全设备控制：%1")
                            .arg(visibleUnsafeControls.join(QStringLiteral(", ")))));
}

void SimulationWorkflowUiTest::selectingFirstTargetShowsDetected()
{
    auto window = createOffscreenWindow();

    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *targetLabel = contractWidget<QLabel>(*window, "simulationTargetLabel");
    auto *statusLabel = contractWidget<QLabel>(*window, "simulationStatusLabel");
    auto *decisionStatus = contractWidget<QLabel>(*window, "decisionSimulationStatusLabel");
    QVERIFY2(targetTable != nullptr, "缺少对象 targetTable，无法通过真实点击选择模拟目标");
    QVERIFY2(targetLabel != nullptr && statusLabel != nullptr && decisionStatus != nullptr,
             "选择目标后必须同步显示目标、工作流状态和决策状态");

    QVERIFY2(clickFirstTarget(targetTable), "targetTable 第一行必须可通过 QTest::mouseClick 选择");
    QTRY_COMPARE(targetLabel->text().contains(kTargetId), true);
    QTRY_COMPARE(statusLabel->text(), QStringLiteral("模拟状态：已发现"));
    QTRY_COMPARE(decisionStatus->text(), QStringLiteral("[模拟] 目标状态：已发现"));
    captureVisualEvidence(*window, QStringLiteral("phase4-target-selected.png"));
}

void SimulationWorkflowUiTest::validActionsUpdateAllUiState()
{
    auto window = createOffscreenWindow();

    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *statusLabel = contractWidget<QLabel>(*window, "simulationStatusLabel");
    auto *decisionStatus = contractWidget<QLabel>(*window, "decisionSimulationStatusLabel");
    auto *confirmButton = contractWidget<QPushButton>(*window, "simulationConfirmButton");
    auto *startButton = contractWidget<QPushButton>(*window, "simulationStartButton");
    auto *completeButton = contractWidget<QPushButton>(*window, "simulationCompleteButton");
    QVERIFY2(confirmButton != nullptr && startButton != nullptr && completeButton != nullptr,
             "缺少阶段 4 三步模拟操作按钮");
    QVERIFY2(targetTable != nullptr && statusLabel != nullptr && decisionStatus != nullptr,
             "缺少阶段 4 状态展示对象");
    const int statusColumn = columnWithHeader(targetTable, QStringLiteral("状态"));
    QVERIFY2(statusColumn >= 0, "targetTable 必须提供模拟状态列");

    QVERIFY(clickFirstTarget(targetTable));
    QTRY_COMPARE(confirmButton->isEnabled(), true);
    QTRY_COMPARE(startButton->isEnabled(), false);
    QTRY_COMPARE(completeButton->isEnabled(), false);

    QTest::mouseClick(confirmButton, Qt::LeftButton);
    QTRY_COMPARE(statusLabel->text(), QStringLiteral("模拟状态：已确认"));
    QTRY_COMPARE(decisionStatus->text(), QStringLiteral("[模拟] 目标状态：已确认"));
    QTRY_COMPARE(firstTargetStatus(targetTable, statusColumn), QStringLiteral("[模拟] 已确认"));
    QTRY_COMPARE(confirmButton->isEnabled(), false);
    QTRY_COMPARE(startButton->isEnabled(), true);
    QTRY_COMPARE(completeButton->isEnabled(), false);

    QTest::mouseClick(startButton, Qt::LeftButton);
    QTRY_COMPARE(statusLabel->text(), QStringLiteral("模拟状态：处置中"));
    QTRY_COMPARE(decisionStatus->text(), QStringLiteral("[模拟] 目标状态：处置中"));
    QTRY_COMPARE(firstTargetStatus(targetTable, statusColumn), QStringLiteral("[模拟] 处置中"));
    QTRY_COMPARE(confirmButton->isEnabled(), false);
    QTRY_COMPARE(startButton->isEnabled(), false);
    QTRY_COMPARE(completeButton->isEnabled(), true);

    QTest::mouseClick(completeButton, Qt::LeftButton);
    QTRY_COMPARE(statusLabel->text(), QStringLiteral("模拟状态：已完成"));
    QTRY_COMPARE(decisionStatus->text(), QStringLiteral("[模拟] 目标状态：已完成"));
    QTRY_COMPARE(firstTargetStatus(targetTable, statusColumn), QStringLiteral("[模拟] 已完成"));
    QTRY_COMPARE(confirmButton->isEnabled(), false);
    QTRY_COMPARE(startButton->isEnabled(), false);
    QTRY_COMPARE(completeButton->isEnabled(), false);
    captureVisualEvidence(*window, QStringLiteral("phase4-workflow-completed.png"));
}

void SimulationWorkflowUiTest::operationLogPreservesSimulationOrder()
{
    auto window = createOffscreenWindow();

    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    auto *confirmButton = contractWidget<QPushButton>(*window, "simulationConfirmButton");
    auto *startButton = contractWidget<QPushButton>(*window, "simulationStartButton");
    auto *completeButton = contractWidget<QPushButton>(*window, "simulationCompleteButton");
    auto *operationLog = contractWidget<QWidget>(*window, "simulationOperationLog");
    QVERIFY2(operationLog != nullptr, "缺少对象 simulationOperationLog，无法展示阶段 4 内存日志");
    QVERIFY2(targetTable != nullptr && confirmButton != nullptr && startButton != nullptr && completeButton != nullptr,
             "缺少生成模拟操作日志所需的目标表或三步按钮");

    QVERIFY(clickFirstTarget(targetTable));
    QTRY_COMPARE(confirmButton->isEnabled(), true);
    QTest::mouseClick(confirmButton, Qt::LeftButton);
    QTRY_COMPARE(startButton->isEnabled(), true);
    QTest::mouseClick(startButton, Qt::LeftButton);
    QTRY_COMPARE(completeButton->isEnabled(), true);
    QTest::mouseClick(completeButton, Qt::LeftButton);

    const QStringList expected = {
        QStringLiteral("[模拟] 已选择目标 target-demo-001"),
        QStringLiteral("[模拟] 目标 target-demo-001：已发现 -> 已确认"),
        QStringLiteral("[模拟] 目标 target-demo-001：已确认 -> 处置中"),
        QStringLiteral("[模拟] 目标 target-demo-001：处置中 -> 已完成")
    };
    QTRY_COMPARE(simulationLogLines(operationLog), expected);
}

void SimulationWorkflowUiTest::newWindowStartsWithFreshWorkflow()
{
    auto firstWindow = createOffscreenWindow();
    auto *firstTargetTable = contractWidget<QTableWidget>(*firstWindow, "targetTable");
    auto *firstConfirmButton = contractWidget<QPushButton>(*firstWindow, "simulationConfirmButton");
    auto *firstStartButton = contractWidget<QPushButton>(*firstWindow, "simulationStartButton");
    auto *firstCompleteButton = contractWidget<QPushButton>(*firstWindow, "simulationCompleteButton");
    QVERIFY2(firstTargetTable != nullptr && firstConfirmButton != nullptr
                 && firstStartButton != nullptr && firstCompleteButton != nullptr,
             "第一个窗口缺少阶段 4 工作流控件");
    QVERIFY(clickFirstTarget(firstTargetTable));
    QTRY_COMPARE(firstConfirmButton->isEnabled(), true);
    QTest::mouseClick(firstConfirmButton, Qt::LeftButton);
    QTRY_COMPARE(firstStartButton->isEnabled(), true);
    QTest::mouseClick(firstStartButton, Qt::LeftButton);
    QTRY_COMPARE(firstCompleteButton->isEnabled(), true);
    QTest::mouseClick(firstCompleteButton, Qt::LeftButton);
    firstWindow.reset();

    auto freshWindow = createOffscreenWindow();
    auto *targetTable = contractWidget<QTableWidget>(*freshWindow, "targetTable");
    auto *targetLabel = contractWidget<QLabel>(*freshWindow, "simulationTargetLabel");
    auto *statusLabel = contractWidget<QLabel>(*freshWindow, "simulationStatusLabel");
    auto *decisionStatus = contractWidget<QLabel>(*freshWindow, "decisionSimulationStatusLabel");
    auto *confirmButton = contractWidget<QPushButton>(*freshWindow, "simulationConfirmButton");
    auto *startButton = contractWidget<QPushButton>(*freshWindow, "simulationStartButton");
    auto *completeButton = contractWidget<QPushButton>(*freshWindow, "simulationCompleteButton");
    auto *operationLog = contractWidget<QWidget>(*freshWindow, "simulationOperationLog");
    QVERIFY2(targetTable != nullptr && targetLabel != nullptr && statusLabel != nullptr
                 && decisionStatus != nullptr && operationLog != nullptr,
             "新窗口缺少阶段 4 重置状态展示对象");
    const int statusColumn = columnWithHeader(targetTable, QStringLiteral("状态"));
    QVERIFY2(statusColumn >= 0, "新窗口 targetTable 必须恢复模拟状态列");

    QTRY_COMPARE(firstTargetStatus(targetTable, statusColumn), QStringLiteral("[模拟] 已发现"));
    QTRY_COMPARE(targetLabel->text().contains(QStringLiteral("未选择")), true);
    QTRY_COMPARE(statusLabel->text(), QStringLiteral("模拟状态：未选择"));
    QTRY_COMPARE(decisionStatus->text(), QStringLiteral("[模拟] 目标状态：未选择"));
    QTRY_COMPARE(operationLogText(operationLog).trimmed(), QStringLiteral("暂无模拟操作记录（重启后清空）"));
    QTRY_COMPARE(confirmButton->isEnabled(), false);
    QTRY_COMPARE(startButton->isEnabled(), false);
    QTRY_COMPARE(completeButton->isEnabled(), false);
}

// 校验目标表类型列与位置列在 320px 固定面板内不折行、不触发横向滚动，
// 且 CJK 类型名"模拟反跑道雷"和坐标"X:108 Y:0"作为完整单元格文本保留；
// 同时校验拉伸状态列实际宽度足以容纳"[模拟] 已发现"加内边距。
void SimulationWorkflowUiTest::targetTableColumnsPreserveCjkAndCoordinates()
{
    auto window = createOffscreenWindow();

    auto *targetTable = contractWidget<QTableWidget>(*window, "targetTable");
    QVERIFY2(targetTable != nullptr, "缺少对象 targetTable，无法校验列宽与单元格文本");

    const int typeColumn = columnWithHeader(targetTable, QStringLiteral("类型"));
    const int positionColumn = columnWithHeader(targetTable, QStringLiteral("位置"));
    const int statusColumn = columnWithHeader(targetTable, QStringLiteral("状态"));
    QVERIFY2(typeColumn >= 0, "targetTable 必须提供类型列");
    QVERIFY2(positionColumn >= 0, "targetTable 必须提供位置列");
    QVERIFY2(statusColumn >= 0, "targetTable 必须提供可见的模拟状态列");

    QCOMPARE(targetTable->columnWidth(typeColumn), 80);
    QCOMPARE(targetTable->columnWidth(positionColumn), 72);
    QCOMPARE(targetTable->wordWrap(), false);
    QCOMPARE(targetTable->horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);

    const QTableWidgetItem *typeItem = targetTable->item(0, typeColumn);
    const QTableWidgetItem *positionItem = targetTable->item(0, positionColumn);
    QVERIFY2(typeItem != nullptr, "targetTable 第一行类型单元格必须存在");
    QVERIFY2(positionItem != nullptr, "targetTable 第一行位置单元格必须存在");
    QCOMPARE(typeItem->text(), QStringLiteral("模拟反跑道雷"));
    QCOMPARE(positionItem->text(), QStringLiteral("X:108 Y:0"));

    auto *leftPanel = window->findChild<LeftPanelWidget *>();
    QVERIFY2(leftPanel != nullptr, "主窗口必须包含 LeftPanelWidget 以校验固定面板宽度");
    // min/max 均锁死 320px，无需布局事件即可校验固定面板约束。
    QCOMPARE(leftPanel->minimumWidth(), 320);
    QCOMPARE(leftPanel->maximumWidth(), 320);

    // 同步处理已入队布局事件，使拉伸状态列宽度收敛后再测量（不引入定时器/等待）。
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    const int statusWidth = targetTable->columnWidth(statusColumn);
    // 状态列需容纳"[模拟] 已发现"文本宽度加 2px 内边距（item padding 已减至 2px）。
    const QFontMetrics fontMetrics(targetTable->font());
    const int requiredStatusWidth = fontMetrics.horizontalAdvance(QStringLiteral("[模拟] 已发现")) + 4;
    QVERIFY2(statusWidth >= requiredStatusWidth,
             qPrintable(QStringLiteral("状态列实际宽度 %1 不足以容纳\"[模拟] 已发现\"所需 %2")
                            .arg(statusWidth)
                            .arg(requiredStatusWidth)));
}

QTEST_MAIN(SimulationWorkflowUiTest)

#include "simulation_workflow_ui_test.moc"
