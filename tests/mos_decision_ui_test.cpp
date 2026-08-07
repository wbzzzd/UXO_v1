// MOS 决策页集成离屏 UI 契约测试：通过 MainWindow 外壳验证页面栈、导航、决策页与
// MosPlanningController 的稳定对象名与集成路径。MainWindow::loadMockData 在构造时用
// 默认跑道参数 + 默认生成器参数 + seed=42 发起一次 replan，产出 revision 1 已提交
// 快照，因此每个场景启动时控制器已处于基线状态（hasResult=true、revision=1、log=1）。
// happy/invalid/no-solution/route-regression 在基线上+N 断言；no-output 仅观测导出。
// 全程本地合成 fixture 驱动，不连接设备、网络或真实规划会话；不使用 sleep/
// QSignalSpy::wait/QTRY/QTimer/线程 API，所有断言同步完成（控制器与 worker 均为同步）。
// 通过环境变量 MOS_UI_SCENARIO 选择场景：happy|invalid|no-solution|no-output|route-regression。

#include "MainWindow/DecisionView.h"
#include "MainWindow/MainWindow.h"
#include "MainWindow/MosGeneratorDialog.h"
#include "MainWindow/MosParamsPanel.h"
#include "MainWindow/MosPlanningController.h"
#include "Core/MOS/MosFixtureGenerator.h"
#include "Core/MOS/MosPlanner.h"
#include "Core/MOS/MosPlanningSession.h"
#include "Core/MOS/MosTypes.h"

#include <QtTest>

#include <QAbstractButton>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QObject>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QVariant>

#include <memory>

namespace {

// === 合成 fixture 构造助手（与 mos_session_test/mos_planner_test 对齐，合成语义）===

// 默认合法跑道参数（合同常量）
Core::MOS::MosRunwayParams defaultRunwayParams()
{
    Core::MOS::MosRunwayParams p;
    p.L = 3000.0;
    p.W = 50.0;
    p.K = 1.5;
    p.expand = 1.5;
    p.step = 1.0;
    p.minLength = 460.0;
    p.minWidth = 15.0;
    p.backfill = 50.0;
    p.uxoHours = 8.0;
    p.tiers = 3;
    return p;
}

// 构造合成弹坑（简化构造，合成语义）
Core::MOS::MosCrater makeCrater(const QString &id, double visibleR, int x, int y, double influenceR)
{
    Core::MOS::MosCrater c;
    c.id = id;
    c.visibleRadius = visibleR;
    c.x = x;
    c.y = y;
    c.threat = Core::MOS::MosThreatLevel::High;
    c.influenceRadius = influenceR;
    return c;
}

// 合法有解障碍物集合：两个分离弹坑，递进规划可接受
Core::MOS::MosObstacleSet solvableObstacles()
{
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("c1"), 5.0, 1000, 0, 50.0));
    obs.craters.append(makeCrater(QStringLiteral("c2"), 5.0, 2000, 0, 50.0));
    return obs;
}

// 合法无解障碍物集合：单个超大影响半径圆盘覆盖全跑道
Core::MOS::MosObstacleSet blockedObstacles()
{
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("blocker"), 5.0, 1500, 0, 2000.0));
    return obs;
}

// 非法跑道参数：L < 100（触发 InvalidParams 拒绝）
Core::MOS::MosRunwayParams invalidParams()
{
    auto p = defaultRunwayParams();
    p.L = 50.0;
    return p;
}

// === 外壳稳定对象名清单：MainWindow 必须暴露的全部命名对象 ===
const QStringList &shellObjectNames()
{
    static const QStringList names = {
        QStringLiteral("mainPageStack"),
        QStringLiteral("situationWorkspacePage"),
        QStringLiteral("mosDecisionPage"),
        QStringLiteral("mosPlanningController"),
        QStringLiteral("DEC-NAV-LOGO"),
        QStringLiteral("DEC-NAV-01"),
        QStringLiteral("DEC-NAV-02"),
        QStringLiteral("DEC-NAV-03"),
        QStringLiteral("DEC-NAV-04"),
        QStringLiteral("DEC-NAV-05"),
        QStringLiteral("DEC-NAV-06")
    };
    return names;
}

// 决策页静态稳定对象名清单（DecisionView 构造后即存在，不依赖 setSnapshot）
const QStringList &decisionStaticObjectNames()
{
    static const QStringList names = {
        QStringLiteral("DEC-TB-GEN"),
        QStringLiteral("DEC-LP-TARGET-LIST"),
        QStringLiteral("DEC-CE-PARAM-MINLENGTH"),
        QStringLiteral("DEC-CE-VALIDATION"),
        QStringLiteral("DEC-CE-PLAN-STATE"),
        QStringLiteral("DEC-GEN-MODAL"),
        QStringLiteral("DEC-GEN-APPLY"),
        QStringLiteral("DEC-GEN-JSON"),
        QStringLiteral("DEC-SB-TARGET")
    };
    return names;
}

// 收集 MainWindow 中缺失的外壳稳定对象名
QStringList missingShellObjects(MainWindow &window)
{
    QStringList missing;
    for (const auto &name : shellObjectNames()) {
        if (window.findChild<QObject *>(name) == nullptr) {
            missing.append(name);
        }
    }
    return missing;
}

// 收集决策页内缺失的静态稳定对象名（在 mosDecisionPage 子树中查找）
QStringList missingDecisionStaticObjects(QWidget &decisionPage)
{
    QStringList missing;
    for (const auto &name : decisionStaticObjectNames()) {
        if (decisionPage.findChild<QObject *>(name) == nullptr) {
            missing.append(name);
        }
    }
    return missing;
}

// === 视觉证据助手（与 simulation_workflow_ui_test 对齐，仅显式请求时抓图/记录）===

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
    // 同步刷新一次事件循环，确保布局与初始绘制就绪（不依赖 QTRY/定时器）
    QCoreApplication::processEvents(QEventLoop::AllEvents);
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

// 仅在 UXO_VISUAL_GEOMETRY_REPORT 设置时写出溢出几何 TSV 报告
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

// 仅在 UXO_VISUAL_EVIDENCE_DIR 设置时抓图并可选记录溢出几何。
// 抓图或写盘失败返回 false，由调用方用 QVERIFY2 转为测试失败；目录未设置时返回 true。
bool captureVisualEvidence(MainWindow &window, const QString &fileName)
{
    const QString evidenceDirectory = qEnvironmentVariable("UXO_VISUAL_EVIDENCE_DIR");
    if (evidenceDirectory.isEmpty()) {
        return true;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    const QPixmap snapshot = window.grab();
    if (snapshot.isNull()) {
        return false;
    }
    if (snapshot.size() != window.size()) {
        return false;
    }
    const QString outputFileName = visualEvidenceFileName(fileName);
    if (!snapshot.save(QDir(evidenceDirectory).filePath(outputFileName), "PNG")) {
        return false;
    }
    writeOverflowGeometryReport(window, outputFileName);
    return true;
}

// 模态对话框视觉证据助手：仅在 UXO_VISUAL_EVIDENCE_DIR 设置时抓图，
// pixmap 大小必须等于 dialog.size()；抓图或写盘失败返回 false，目录未设置时返回 true。
bool captureDialogEvidence(QWidget &dialog, const QString &fileName)
{
    const QString evidenceDirectory = qEnvironmentVariable("UXO_VISUAL_EVIDENCE_DIR");
    if (evidenceDirectory.isEmpty()) {
        return true;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    const QPixmap snapshot = dialog.grab();
    if (snapshot.isNull()) {
        return false;
    }
    if (snapshot.size() != dialog.size()) {
        return false;
    }
    const QString outputFileName = visualEvidenceFileName(fileName);
    if (!snapshot.save(QDir(evidenceDirectory).filePath(outputFileName), "PNG")) {
        return false;
    }
    return true;
}

// 决策页子树溢出硬断言：mosDecisionPage 任一可见子控件超出主窗口边界即测试失败
// 用于防止 1280x720 下 RightPanelWidget 高度调整回退导致溢出回归
QStringList collectDecisionPageOverflow(MainWindow &window)
{
    auto *decisionPage = window.findChild<QWidget *>(QStringLiteral("mosDecisionPage"));
    if (decisionPage == nullptr) {
        return QStringList{QStringLiteral("mosDecisionPage 缺失")};
    }
    const QRect windowRect = window.rect();
    QStringList offenders;
    const auto widgets = decisionPage->findChildren<QWidget *>();
    for (const QWidget *widget : widgets) {
        if (!widget->isVisibleTo(&window)) {
            continue;
        }
        const QRect bounds(widget->mapTo(&window, QPoint(0, 0)), widget->size());
        if (!windowRect.contains(bounds)) {
            offenders.append(QStringLiteral("%1 [%2] overflow=%3 bounds=%4")
                                 .arg(widget->objectName().isEmpty()
                                          ? QStringLiteral("<unnamed>")
                                          : widget->objectName(),
                                      widget->metaObject()->className(),
                                      overflowText(bounds, windowRect),
                                      geometryText(bounds)));
        }
    }
    return offenders;
}

} // namespace

class MosDecisionUiTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void scenarioContract();

private:
    // 场景实现
    void runHappy(MainWindow &window);
    void runInvalid(MainWindow &window);
    void runNoSolution(MainWindow &window);
    void runNoOutput(MainWindow &window);
    void runRouteRegression(MainWindow &window);

    // 外壳与决策页稳定对象名校验（外壳未接线时即失败）
    void requireShellAndStaticObjects(MainWindow &window);
    // 档位按钮校验（仅在推送含 tiers 的快照后调用）
    void requireTierButtons(DecisionView *view);

    // 从外壳定位控制器与决策视图
    Core::MOS::MosPlanningController *findController(MainWindow &window);
    DecisionView *findDecisionView(MainWindow &window);

    QString m_scenario;
};

// 注册元类型并解析场景环境变量
void MosDecisionUiTest::initTestCase()
{
    qRegisterMetaType<Core::MOS::MosReplanRequest>("Core::MOS::MosReplanRequest");
    qRegisterMetaType<Core::MOS::MosReplanCompletion>("Core::MOS::MosReplanCompletion");

    m_scenario = qEnvironmentVariable("MOS_UI_SCENARIO").toLower();
    if (m_scenario.isEmpty()) {
        m_scenario = QStringLiteral("happy");
    }
    const QStringList valid = {
        QStringLiteral("happy"),
        QStringLiteral("invalid"),
        QStringLiteral("no-solution"),
        QStringLiteral("no-output"),
        QStringLiteral("route-regression")
    };
    QVERIFY2(valid.contains(m_scenario),
             qPrintable(QStringLiteral("未知的 MOS_UI_SCENARIO=%1，合法值：%2")
                            .arg(m_scenario, valid.join(QStringLiteral("|")))));
}

// 单一契约入口：按场景分发，最后抓取视觉证据
void MosDecisionUiTest::scenarioContract()
{
    auto window = createOffscreenWindow();

    // 外壳与决策页静态对象名校验：外壳未接线前每个场景在此失败
    requireShellAndStaticObjects(*window);

    if (m_scenario == QStringLiteral("happy")) {
        runHappy(*window);
    } else if (m_scenario == QStringLiteral("invalid")) {
        runInvalid(*window);
    } else if (m_scenario == QStringLiteral("no-solution")) {
        runNoSolution(*window);
    } else if (m_scenario == QStringLiteral("no-output")) {
        runNoOutput(*window);
    } else if (m_scenario == QStringLiteral("route-regression")) {
        runRouteRegression(*window);
    }

    // 抓取场景结束时所在页面的视觉证据（保持场景留下的当前页，不在此处导航）
    QVERIFY2(captureVisualEvidence(*window,
                                    QStringLiteral("mos-decision-%1-situation.png").arg(m_scenario)),
             qPrintable(QStringLiteral("态势页证据抓取失败：mos-decision-%1-situation.png").arg(m_scenario)));

    // 导航到 MOS 决策页并抓取决策页证据：DEC-NAV-03 切换页面栈到 mosDecisionPage
    auto *navDecision = window->findChild<QAbstractButton *>(QStringLiteral("DEC-NAV-03"));
    QVERIFY2(navDecision != nullptr, "DEC-NAV-03 缺失，无法导航到决策页");
    QTest::mouseClick(navDecision, Qt::LeftButton);
    auto *pageStack = window->findChild<QStackedWidget *>(QStringLiteral("mainPageStack"));
    QVERIFY2(pageStack != nullptr, "mainPageStack 缺失");
    auto *decisionPage = window->findChild<QWidget *>(QStringLiteral("mosDecisionPage"));
    QVERIFY2(decisionPage != nullptr, "mosDecisionPage 缺失");
    QCOMPARE(pageStack->currentWidget(), decisionPage);
    QVERIFY2(decisionPage->isVisible(), "导航到决策页后决策页应可见");

    // MOS 决策页（导航 index 2）须隐藏态势遗留工具栏，由 MOS 工具栏独占
    auto *situationToolBar = window->findChild<QWidget *>(QStringLiteral("mainSituationToolBar"));
    QVERIFY2(situationToolBar != nullptr, "mainSituationToolBar 缺失");
    QVERIFY2(!situationToolBar->isVisible(), "MOS 决策页应隐藏态势遗留工具栏");

    // 视口缩放生命周期契约：showEvent 在页面变可见时触发 applyViewportScale，
    // 4K 下缩放必须 >=1.9；1280x720 下 clamp 至 1.0。保存并恢复原始尺寸，不干扰后续证据与溢出断言。
    auto *scaleView = findDecisionView(*window);
    QVERIFY2(scaleView != nullptr, "缩放断言需要 DecisionView 实例");
    const QSize savedSize = window->size();
    window->resize(3840, 2160);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QVERIFY2(scaleView->viewportScale() >= 1.9,
             qPrintable(QStringLiteral("4K 下决策页视口缩放应 >=1.9，实际：%1")
                            .arg(scaleView->viewportScale())));
    window->resize(1280, 720);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCOMPARE(scaleView->viewportScale(), 1.0);
    window->resize(savedSize);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    QVERIFY2(captureVisualEvidence(*window,
                                    QStringLiteral("mos-decision-%1.png").arg(m_scenario)),
             qPrintable(QStringLiteral("决策页证据抓取失败：mos-decision-%1.png").arg(m_scenario)));

    // 决策页子树溢出硬断言：1280x720 下任一可见子控件超出主窗口即测试失败
    const QStringList decisionOverflow = collectDecisionPageOverflow(*window);
    QVERIFY2(decisionOverflow.isEmpty(),
             qPrintable(QStringLiteral("决策页存在溢出控件：%1")
                            .arg(decisionOverflow.join(QStringLiteral(" | ")))));

    // 生成器模态渲染证据：仅在证据目录设置时验证 DEC-GEN-MODAL 的 show() 渲染。
    // 同步 show() + 一次事件循环刷新，断言可见、默认种子 42、校验横幅包含"通过"，
    // 抓取 mos-generator-<scenario>.png（pixmap 大小必须等于 dialog 大小），随后 hide()。
    if (qEnvironmentVariableIsSet("UXO_VISUAL_EVIDENCE_DIR")) {
        auto *generatorModal = window->findChild<MosGeneratorDialog *>(QStringLiteral("DEC-GEN-MODAL"));
        QVERIFY2(generatorModal != nullptr, "DEC-GEN-MODAL 缺失，无法抓取生成器模态证据");
        generatorModal->show();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QVERIFY2(generatorModal->isVisible(), "生成器模态 show() 后应可见");
        auto *seedSpin = generatorModal->findChild<QSpinBox *>(QStringLiteral("DEC-GEN-SEED"));
        QVERIFY2(seedSpin != nullptr, "DEC-GEN-SEED 缺失");
        QCOMPARE(seedSpin->value(), 42);
        auto *validationBanner = generatorModal->findChild<QLabel *>(QStringLiteral("DEC-GEN-BANNER"));
        QVERIFY2(validationBanner != nullptr, "DEC-GEN-BANNER 缺失");
        QVERIFY2(validationBanner->text().contains(QStringLiteral("通过")),
                 qPrintable(QStringLiteral("生成器模态校验横幅应包含通过，实际：%1")
                                .arg(validationBanner->text())));
        QVERIFY2(captureDialogEvidence(*generatorModal,
                                        QStringLiteral("mos-generator-%1.png").arg(m_scenario)),
                 qPrintable(QStringLiteral("生成器模态证据抓取失败：mos-generator-%1.png").arg(m_scenario)));
        generatorModal->hide();
    }

    // 路由方向证据：导航回 DEC-NAV-01（态势工作区），验证页面栈、导航选中状态与工具栏可见性
    auto *navSituationReturn = window->findChild<QAbstractButton *>(QStringLiteral("DEC-NAV-01"));
    QVERIFY2(navSituationReturn != nullptr, "DEC-NAV-01 缺失，无法导航回态势工作区");
    QTest::mouseClick(navSituationReturn, Qt::LeftButton);
    auto *pageStackAfterReturn = window->findChild<QStackedWidget *>(QStringLiteral("mainPageStack"));
    QVERIFY2(pageStackAfterReturn != nullptr, "mainPageStack 缺失");
    auto *situationPage = window->findChild<QWidget *>(QStringLiteral("situationWorkspacePage"));
    QVERIFY2(situationPage != nullptr, "situationWorkspacePage 缺失");
    QCOMPARE(pageStackAfterReturn->currentWidget(), situationPage);
    QVERIFY2(situationPage->isVisible(), "导航回态势页后态势工作区应可见");
    // 选中态真实性：DEC-NAV-01 selected=true，DEC-NAV-02（决策）selected=false
    QCOMPARE(navSituationReturn->property("selected").toBool(), true);
    auto *navDecisionAfterReturn = window->findChild<QAbstractButton *>(QStringLiteral("DEC-NAV-03"));
    QVERIFY2(navDecisionAfterReturn != nullptr, "DEC-NAV-03 缺失");
    QCOMPARE(navDecisionAfterReturn->property("selected").toBool(), false);
    // 返回态势页后态势遗留工具栏应重新可见
    auto *situationToolBarAfterReturn = window->findChild<QWidget *>(QStringLiteral("mainSituationToolBar"));
    QVERIFY2(situationToolBarAfterReturn != nullptr, "mainSituationToolBar 缺失");
    QVERIFY2(situationToolBarAfterReturn->isVisible(), "态势页应显示态势遗留工具栏");
    QVERIFY2(captureVisualEvidence(*window,
                                    QStringLiteral("mos-decision-%1-situation-return.png").arg(m_scenario)),
             qPrintable(QStringLiteral("态势返回证据抓取失败：mos-decision-%1-situation-return.png").arg(m_scenario)));
}

void MosDecisionUiTest::requireShellAndStaticObjects(MainWindow &window)
{
    const QStringList shellMissing = missingShellObjects(window);
    QVERIFY2(shellMissing.isEmpty(),
             qPrintable(QStringLiteral("缺少外壳稳定对象名：%1")
                            .arg(shellMissing.join(QStringLiteral(", ")))));

    auto *decisionPage = window.findChild<QWidget *>(QStringLiteral("mosDecisionPage"));
    QVERIFY2(decisionPage != nullptr,
             "mosDecisionPage 缺失，无法校验决策页静态对象名");
    const QStringList decisionMissing = missingDecisionStaticObjects(*decisionPage);
    QVERIFY2(decisionMissing.isEmpty(),
             qPrintable(QStringLiteral("缺少决策页稳定对象名：%1")
                            .arg(decisionMissing.join(QStringLiteral(", ")))));
}

void MosDecisionUiTest::requireTierButtons(DecisionView *view)
{
    QVERIFY2(view->findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-1")) != nullptr,
             "推送含 tiers 的快照后 DEC-TB-PLAN-1 必须存在");
    QVERIFY2(view->findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-2")) != nullptr,
             "推送含 tiers 的快照后 DEC-TB-PLAN-2 必须存在");
}

Core::MOS::MosPlanningController *MosDecisionUiTest::findController(MainWindow &window)
{
    return window.findChild<Core::MOS::MosPlanningController *>(
        QStringLiteral("mosPlanningController"));
}

// mosDecisionPage 可以是 DecisionView 本身，也可以是包含 DecisionView 的页面容器
DecisionView *MosDecisionUiTest::findDecisionView(MainWindow &window)
{
    auto *page = window.findChild<QWidget *>(QStringLiteral("mosDecisionPage"));
    if (page == nullptr) {
        return nullptr;
    }
    if (auto *view = qobject_cast<DecisionView *>(page)) {
        return view;
    }
    return page->findChild<DecisionView *>();
}

// === happy：合法有解 replan，结果横幅与档位按钮契约 ===
void MosDecisionUiTest::runHappy(MainWindow &window)
{
    auto *controller = findController(window);
    auto *view = findDecisionView(window);
    QVERIFY2(controller != nullptr, "外壳必须暴露 mosPlanningController");
    QVERIFY2(view != nullptr, "外壳必须暴露 mosDecisionPage 中的 DecisionView");

    // 启动基线：loadMockData 已用 seed=42 完成 revision 1，log=1
    const auto baseline = controller->snapshot();
    QVERIFY2(baseline.hasResult, "启动后基线必须有已提交结果");
    QCOMPARE(baseline.committedRevision, quint64(1));
    QCOMPARE(baseline.logEntries.size(), 1);

    QSignalSpy stateSpy(controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QVERIFY2(stateSpy.isValid(), "mosStateChanged 信号不可用");

    // 合法有解 replan：基线之上 +1，同步完成，恰好一次状态通知
    const quint64 revision = controller->requestReplan(solvableObstacles(), defaultRunwayParams());
    QCOMPARE(revision, baseline.committedRevision + 1);
    QCOMPARE(stateSpy.count(), 1);

    const auto snap = controller->snapshot();
    QVERIFY2(snap.hasResult, "合法有解 replan 后必须有已提交结果");
    QCOMPARE(snap.committedRevision, baseline.committedRevision + 1);
    QVERIFY2(snap.result.accepted, "合法有解 replan 必须被接受");

    // 推送快照到视图：档位按钮与结果横幅必须反映接受状态
    view->setSnapshot(snap);
    requireTierButtons(view);

    auto *planState = view->findChild<QLabel *>(QStringLiteral("DEC-CE-PLAN-STATE"));
    QVERIFY2(planState != nullptr, "DEC-CE-PLAN-STATE 缺失");
    QVERIFY2(planState->text().contains(QStringLiteral("结果")),
             qPrintable(QStringLiteral("happy 场景应显示结果横幅，实际：%1").arg(planState->text())));

    // 真实点击档位2 按钮发出 tierSelected(1)（档位序号从 0 起）
    QSignalSpy tierSpy(view, &DecisionView::tierSelected);
    QVERIFY2(tierSpy.isValid(), "tierSelected 信号不可用");
    auto *tier2 = view->findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-2"));
    QVERIFY2(tier2 != nullptr, "DEC-TB-PLAN-2 缺失");
    QTest::mouseClick(tier2, Qt::LeftButton);
    QCOMPARE(tierSpy.count(), 1);
    QCOMPARE(tierSpy.takeFirst().at(0).toInt(), 1);
}

// === invalid：非法参数 replan 被拒绝，业务不变，追加一条拒绝日志 ===
void MosDecisionUiTest::runInvalid(MainWindow &window)
{
    auto *controller = findController(window);
    auto *view = findDecisionView(window);
    QVERIFY2(controller != nullptr, "外壳必须暴露 mosPlanningController");
    QVERIFY2(view != nullptr, "外壳必须暴露 mosDecisionPage 中的 DecisionView");

    // 启动基线：seed=42 revision 1，hasResult=true，log=1
    const auto baseline = controller->snapshot();
    QVERIFY2(baseline.hasResult, "启动后基线必须有已提交结果");
    QCOMPARE(baseline.committedRevision, quint64(1));
    QCOMPARE(baseline.logEntries.size(), 1);

    QSignalSpy stateSpy(controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QVERIFY2(stateSpy.isValid(), "mosStateChanged 信号不可用");

    // 非法参数 replan：基线之上分配 revision 2，同步完成，恰好一次状态通知（拒绝日志追加）
    const quint64 revision = controller->requestReplan(solvableObstacles(), invalidParams());
    QCOMPARE(revision, baseline.committedRevision + 1);
    QCOMPARE(stateSpy.count(), 1);

    const auto snap = controller->snapshot();
    // 业务字段保持不变（基线结果仍存在，revision 不前移）
    QCOMPARE(snap.committedRevision, baseline.committedRevision);
    QCOMPARE(snap.hasResult, baseline.hasResult);
    // 仅追加一条拒绝日志
    QCOMPARE(snap.logEntries.size(), baseline.logEntries.size() + 1);
    QCOMPARE(snap.logEntries.last().type, Core::MOS::MosSessionLogType::ReplanRejected);
    QCOMPARE(snap.logEntries.last().revision, baseline.committedRevision + 1);
    QCOMPARE(snap.logEntries.last().reason, Core::MOS::MosPlannerReason::InvalidParams);

    // 推送拒绝后的快照（仍有基线结果）到视图：横幅应显示结果状态
    view->setSnapshot(snap);
    auto *planState = view->findChild<QLabel *>(QStringLiteral("DEC-CE-PLAN-STATE"));
    QVERIFY2(planState != nullptr, "DEC-CE-PLAN-STATE 缺失");
    QVERIFY2(planState->text().contains(QStringLiteral("结果")),
             qPrintable(QStringLiteral("invalid 场景应显示结果横幅（基线结果保留），实际：%1").arg(planState->text())));
}

// === no-solution：合法无解 replan 被接受，档位 0 矩形无效，无可行横幅与禁用卡片 ===
void MosDecisionUiTest::runNoSolution(MainWindow &window)
{
    auto *controller = findController(window);
    auto *view = findDecisionView(window);
    QVERIFY2(controller != nullptr, "外壳必须暴露 mosPlanningController");
    QVERIFY2(view != nullptr, "外壳必须暴露 mosDecisionPage 中的 DecisionView");

    // 启动基线：seed=42 revision 1
    const auto baseline = controller->snapshot();
    QCOMPARE(baseline.committedRevision, quint64(1));

    QSignalSpy stateSpy(controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QVERIFY2(stateSpy.isValid(), "mosStateChanged 信号不可用");

    // 合法无解 replan：基线之上 +1，被接受但 tier 0 矩形无效
    const quint64 revision = controller->requestReplan(blockedObstacles(), defaultRunwayParams());
    QCOMPARE(revision, baseline.committedRevision + 1);
    QCOMPARE(stateSpy.count(), 1);

    const auto snap = controller->snapshot();
    QVERIFY2(snap.hasResult, "合法无解 replan 仍必须有已提交结果");
    QVERIFY2(snap.result.accepted, "合法无解 replan 必须被接受");
    QVERIFY2(!snap.result.tiers.isEmpty(), "合法无解 replan 必须产出档位列表");
    QVERIFY2(!snap.result.tiers.at(0).rectangle.valid,
             "合法无解 tier 0 矩形必须无效");
    QCOMPARE(snap.result.tiers.at(0).rectangle.reason,
             Core::MOS::MosPlannerReason::NoFeasibleRectangle);

    // 推送快照到视图：档位按钮与无可行横幅必须反映接受的无解状态
    view->setSnapshot(snap);
    requireTierButtons(view);

    // 选中档位（默认 0）无可行矩形时横幅必须显示无可行，而非结果
    auto *planState = view->findChild<QLabel *>(QStringLiteral("DEC-CE-PLAN-STATE"));
    QVERIFY2(planState != nullptr, "DEC-CE-PLAN-STATE 缺失");
    QVERIFY2(planState->text().contains(QStringLiteral("无可行")),
             qPrintable(QStringLiteral("no-solution 场景应显示无可行横幅，实际：%1").arg(planState->text())));

    // tier 0 按钮必须禁用，避免用户选中无法生成起降带的档位
    auto *tier1 = view->findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-1"));
    QVERIFY2(tier1 != nullptr, "DEC-TB-PLAN-1 缺失");
    QVERIFY2(!tier1->isEnabled(), "无可行档位的 DEC-TB-PLAN-1 必须禁用");

    // tier 0 卡片必须禁用且显示无可行方案占位，而非 0×0m 等合成假数据
    auto *plansContainer = view->findChild<QWidget *>(QStringLiteral("DEC-RP-PLANS"));
    QVERIFY2(plansContainer != nullptr, "DEC-RP-PLANS 容器缺失");
    auto cards = plansContainer->findChildren<QPushButton *>(QRegularExpression(QStringLiteral("^DEC-RP-PLAN-")));
    QVERIFY2(!cards.isEmpty(), "no-solution 推送后右面板应重建候选方案卡片");
    QVERIFY2(!cards.at(0)->isEnabled(), "无可行档位的 DEC-RP-PLAN-1 卡片必须禁用");
    QVERIFY2(cards.at(0)->text().contains(QStringLiteral("无可行方案")),
             qPrintable(QStringLiteral("无可行卡片应显示无可行方案，实际：%1").arg(cards.at(0)->text())));

    // 合法无解场景下更高档位修复 blocker 后矩形有效：tier-local 策略下该卡片必须可用，
    // 不应被 tier 0 的无可行态连带禁用
    QVERIFY2(cards.size() >= 3, "合法无解 3 档场景应至少产出 3 张候选卡片");
    QVERIFY2(cards.at(2)->isEnabled(),
             "合法无解场景下 tier 2（修复 blocker）卡片必须可用，实际 enabled=false");
    // tier 2 估算按 blocker 的 visibleRadius=100 计算，合同结果约 10.5h。
    QVERIFY2(cards.at(2)->text().contains(QStringLiteral("工时 10.5h")),
             qPrintable(QStringLiteral("tier 2 应显示 visibleRadius 合成估算，实际：%1").arg(cards.at(2)->text())));
    QVERIFY2(!cards.at(2)->text().contains(QStringLiteral("超出模拟范围")),
             "visibleRadius 合同结果不应被错误标记为超出模拟范围");
}

// === no-output：未发起额外 replan，导出为观测性写入，不改业务状态/revision/日志 ===
void MosDecisionUiTest::runNoOutput(MainWindow &window)
{
    auto *controller = findController(window);
    auto *view = findDecisionView(window);
    QVERIFY2(controller != nullptr, "外壳必须暴露 mosPlanningController");
    QVERIFY2(view != nullptr, "外壳必须暴露 mosDecisionPage 中的 DecisionView");

    // 启动基线：seed=42 revision 1，hasResult=true，log=1
    const auto baseline = controller->snapshot();
    QVERIFY2(baseline.hasResult, "启动后基线必须有已提交结果");
    QCOMPARE(baseline.committedRevision, quint64(1));
    QCOMPARE(baseline.logEntries.size(), 1);

    // 推送基线快照到视图：横幅应显示结果状态
    view->setSnapshot(baseline);
    auto *planState = view->findChild<QLabel *>(QStringLiteral("DEC-CE-PLAN-STATE"));
    QVERIFY2(planState != nullptr, "DEC-CE-PLAN-STATE 缺失");
    QVERIFY2(planState->text().contains(QStringLiteral("结果")),
             qPrintable(QStringLiteral("no-output 场景应显示结果横幅（基线），实际：%1").arg(planState->text())));

    // 单向导出：写入临时工件，不改变业务状态/revision/日志
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "无法创建临时导出目录");
    const QString exportPath = dir.path() + QStringLiteral("/mos-no-output.json");
    const auto result = controller->exportFixture(exportPath);
    QVERIFY2(result.success,
             qPrintable(QStringLiteral("no-output 导出应成功，消息：%1").arg(result.message)));
    QVERIFY2(QFile::exists(exportPath), "no-output 导出文件应存在");

    // 导出后业务状态完全不变
    const auto after = controller->snapshot();
    QCOMPARE(after.committedRevision, baseline.committedRevision);
    QCOMPARE(after.logEntries.size(), baseline.logEntries.size());
    QCOMPARE(after.hasResult, baseline.hasResult);
}

// === route-regression：导航路由与连续 replan revision 路由不回归 ===
void MosDecisionUiTest::runRouteRegression(MainWindow &window)
{
    auto *controller = findController(window);
    auto *view = findDecisionView(window);
    QVERIFY2(controller != nullptr, "外壳必须暴露 mosPlanningController");
    QVERIFY2(view != nullptr, "外壳必须暴露 mosDecisionPage 中的 DecisionView");

    auto *pageStack = window.findChild<QStackedWidget *>(QStringLiteral("mainPageStack"));
    QVERIFY2(pageStack != nullptr, "mainPageStack 缺失");
    auto *decisionPage = window.findChild<QWidget *>(QStringLiteral("mosDecisionPage"));
    QVERIFY2(decisionPage != nullptr, "mosDecisionPage 缺失");
    auto *situationPage = window.findChild<QWidget *>(QStringLiteral("situationWorkspacePage"));
    QVERIFY2(situationPage != nullptr, "situationWorkspacePage 缺失");

    // 启动基线：seed=42 revision 1
    const auto baseline = controller->snapshot();
    QCOMPARE(baseline.committedRevision, quint64(1));

    // 导航路由：DEC-NAV-02（态势）切到态势工作区，DEC-NAV-03（决策）切到 MOS 决策页
    auto *navSituation = window.findChild<QAbstractButton *>(QStringLiteral("DEC-NAV-02"));
    QVERIFY2(navSituation != nullptr, "DEC-NAV-02 缺失");
    auto *navDecision = window.findChild<QAbstractButton *>(QStringLiteral("DEC-NAV-03"));
    QVERIFY2(navDecision != nullptr, "DEC-NAV-03 缺失");

    // 点击决策导航：页面栈切到 MOS 决策页，态势工作区隐藏
    QTest::mouseClick(navDecision, Qt::LeftButton);
    QCOMPARE(pageStack->currentWidget(), decisionPage);
    QVERIFY2(!situationPage->isVisible(), "切换到决策页后态势工作区应隐藏");
    QVERIFY2(decisionPage->isVisible(), "切换到决策页后决策页应可见");

    // 点击态势导航：页面栈切回态势工作区，MOS 决策页隐藏
    QTest::mouseClick(navSituation, Qt::LeftButton);
    QCOMPARE(pageStack->currentWidget(), situationPage);
    QVERIFY2(situationPage->isVisible(), "切回态势页后态势工作区应可见");
    QVERIFY2(!decisionPage->isVisible(), "切回态势页后决策页应隐藏");

    // 其余导航 index（0,1,4,5）应保持态势工作区
    for (const auto &navName : {QStringLiteral("DEC-NAV-01"), QStringLiteral("DEC-NAV-04"),
                                 QStringLiteral("DEC-NAV-05"), QStringLiteral("DEC-NAV-06")}) {
        auto *btn = window.findChild<QAbstractButton *>(navName);
        QVERIFY2(btn != nullptr, qPrintable(QStringLiteral("%1 缺失").arg(navName)));
        QTest::mouseClick(btn, Qt::LeftButton);
        QCOMPARE(pageStack->currentWidget(), situationPage);
    }

    // revision 路由：基线之上连续两次 replan，第二次提交必须覆盖第一次
    QSignalSpy stateSpy(controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QVERIFY2(stateSpy.isValid(), "mosStateChanged 信号不可用");

    const quint64 rev1 = controller->requestReplan(solvableObstacles(), defaultRunwayParams());
    QCOMPARE(rev1, baseline.committedRevision + 1);
    const quint64 rev2 = controller->requestReplan(blockedObstacles(), defaultRunwayParams());
    QCOMPARE(rev2, baseline.committedRevision + 2);
    QCOMPARE(stateSpy.count(), 2);

    const auto snap = controller->snapshot();
    QVERIFY2(snap.hasResult, "连续 replan 后必须有已提交结果");
    QCOMPARE(snap.committedRevision, baseline.committedRevision + 2); // 第二次覆盖第一次
    QCOMPARE(snap.logEntries.size(), baseline.logEntries.size() + 2);
    QCOMPARE(snap.logEntries.last().type, Core::MOS::MosSessionLogType::ReplanAccepted);
    QCOMPARE(snap.logEntries.last().revision, baseline.committedRevision + 2);

    // 推送最新快照到视图：档位按钮必须重建
    view->setSnapshot(snap);
    requireTierButtons(view);
}

QTEST_MAIN(MosDecisionUiTest)

#include "mos_decision_ui_test.moc"
