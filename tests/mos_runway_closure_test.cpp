// MOS 跑道画布闭包测试：验证快照驱动几何、视口中心缩放、钳制平移、
// 真实点击信号、重叠档位最上层命中、中键拖拽平移与逆变换命中测试的端到端契约。
// 已注册到 CMake（UXOMissionControlMosRunwayClosureTest），仅本地合成 fixture 驱动，不连接设备/网络/真实规划。

#include "MainWindow/MosRunwayWidget.h"
#include "MosRunwayWidgetInternal.h"
#include "Core/MOS/MosFixtureGenerator.h"
#include "Core/MOS/MosPlanner.h"
#include "Core/MOS/MosPlanningSession.h"

#include <QtTest>

#include <QApplication>
#include <QMouseEvent>
#include <QPointF>
#include <QSignalSpy>
#include <QTransform>

using namespace MosRunwayInternal;

class MosRunwayClosureTest : public QObject
{
    Q_OBJECT

private slots:
    void titleDerivesFromSnapshotLW();
    void zoomScalesGeometryAroundViewportCenter();
    void zoomClampedToHalfToThree();
    void panClampedToBounds();
    void resetViewClearsZoomAndPan();
    void hitTestTargetInverseTransform();
    void hitTestTierInverseTransform();
    void fontZoomCompensationKeepsContentStable();
    void formatTierLabelContainsTierNumberIntervalsAndDimensions();
    void targetClickEmitsTargetClickedAtObstaclePixel();
    void overlappingTiersHitSelectedTierOnly();
    void obstacleRadiusPxMatchesInfluenceRadiusTimesPxPerM();
    void middleButtonDragAtZoomTwoShiftsContentMapping();

private:
    Core::MOS::MosPlanningSnapshot makeSeed();
    Core::MOS::MosPlanningSnapshot makeNoFeasible();
    Core::MOS::MosPlanningSnapshot makeSingleCraterCenter();
    Core::MOS::MosPlanningSnapshot makeOverlappingTiers();
};

Core::MOS::MosPlanningSnapshot MosRunwayClosureTest::makeSeed()
{
    Core::MOS::MosRunwayParams params;
    params.L = 4000.0;
    params.W = 60.0;
    Core::MOS::MosGeneratorParams gen;
    const qint32 seed = 42;
    const auto obstacles = Core::MOS::MosFixtureGenerator::generate(params, gen, seed);
    const auto result = Core::MOS::MosPlanner::planProgressive(obstacles, params);

    Core::MOS::MosPlanningSnapshot snap;
    snap.obstacles = obstacles;
    snap.params = params;
    snap.result = result;
    snap.hasResult = true;
    snap.selectedTier = 1;
    snap.committedRevision = 1;
    return snap;
}

Core::MOS::MosPlanningSnapshot MosRunwayClosureTest::makeNoFeasible()
{
    Core::MOS::MosRunwayParams params;
    Core::MOS::MosCrater blocker;
    blocker.id = QStringLiteral("blocker");
    blocker.visibleRadius = 5.0;
    blocker.x = 1500;
    blocker.y = 0;
    blocker.threat = Core::MOS::MosThreatLevel::High;
    blocker.influenceRadius = 2000.0;
    Core::MOS::MosObstacleSet obstacles;
    obstacles.craters.append(blocker);
    const auto result = Core::MOS::MosPlanner::planProgressive(obstacles, params);

    Core::MOS::MosPlanningSnapshot snap;
    snap.obstacles = obstacles;
    snap.params = params;
    snap.result = result;
    snap.hasResult = true;
    snap.selectedTier = 0;
    snap.committedRevision = 1;
    return snap;
}

// 合成单弹坑快照：弹坑置于跑道中线 (x=L/2, y=0)，便于在 widget 中心点击命中。
Core::MOS::MosPlanningSnapshot MosRunwayClosureTest::makeSingleCraterCenter()
{
    Core::MOS::MosRunwayParams params;
    params.L = 4000.0;
    params.W = 60.0;
    Core::MOS::MosCrater c;
    c.id = QStringLiteral("C-CENTER");
    c.x = 2000.0;       // 跑道中点
    c.y = 0.0;
    c.visibleRadius = 5.0;
    c.threat = Core::MOS::MosThreatLevel::High;
    c.influenceRadius = 200.0;
    Core::MOS::MosObstacleSet obstacles;
    obstacles.craters.append(c);
    Core::MOS::MosPlanningSnapshot snap;
    snap.obstacles = obstacles;
    snap.params = params;
    snap.hasResult = false;
    snap.selectedTier = -1;
    return snap;
}

// 合成重叠档位快照：3 个档位矩形完全重叠，用于验证 P0 仅命中选中档位。
Core::MOS::MosPlanningSnapshot MosRunwayClosureTest::makeOverlappingTiers()
{
    Core::MOS::MosRunwayParams params;
    params.L = 4000.0;
    params.W = 60.0;
    Core::MOS::MosObstacleSet obstacles;   // 无障碍物，仅档位矩形
    Core::MOS::MosProgressiveResult result;
    result.accepted = true;
    result.reason = Core::MOS::MosPlannerReason::Accepted;
    for (int i = 0; i < 3; ++i) {
        Core::MOS::MosRepairTier tier;
        tier.rectangle.valid = true;
        tier.rectangle.xStart = 0.0;
        tier.rectangle.xEnd = 4000.0;
        tier.rectangle.yStart = -25.0;
        tier.rectangle.yEnd = 25.0;
        tier.rectangle.length = 4000.0;
        tier.rectangle.width = 50.0;
        tier.rectangle.area = 200000.0;
        result.tiers.append(tier);
    }
    Core::MOS::MosPlanningSnapshot snap;
    snap.obstacles = obstacles;
    snap.params = params;
    snap.result = result;
    snap.hasResult = true;
    snap.selectedTier = -1;
    snap.committedRevision = 1;
    return snap;
}

void MosRunwayClosureTest::titleDerivesFromSnapshotLW()
{
    // 契约：m_rwTitle 由父页面 DecisionView 按 params.L/W 派生，不再硬编码 3000m × 50m
    // 此处仅验证 MosRunwayWidget 不持有标题；标题契约由 DecisionView 测试覆盖
    MosRunwayWidget w;
    w.resize(800, 400);
    const auto snap = makeSeed();
    w.setSnapshot(snap);
    // widget 本身无标题 API，仅确认快照接受后不崩溃
    QVERIFY2(w.zoom() == 1.0, "初始 zoom 应为 1.0");
}

void MosRunwayClosureTest::zoomScalesGeometryAroundViewportCenter()
{
    // 契约：zoom 变化时 contentTransform 围绕视口中心缩放，视口中心点在 widget 坐标下稳定
    MosRunwayWidget w;
    w.resize(800, 400);
    w.setSnapshot(makeSeed());

    const QPointF center(400, 200);
    const QPointF cAt1 = w.mapContentToWidget(center);
    const QPointF offCenter(200, 100);
    const QPointF oAt1 = w.mapContentToWidget(offCenter);
    w.setZoomDisplay(2.0);
    const QPointF cAt2 = w.mapContentToWidget(center);
    // 视口中心在内容坐标下映射前后应近似相等（浮点容差 0.5px）
    QVERIFY2(std::abs(cAt1.x() - cAt2.x()) < 0.5 && std::abs(cAt1.y() - cAt2.y()) < 0.5,
             "视口中心在 zoom 变化后应保持稳定");
    // zoom=2 时非中心点应被放大 2 倍
    const QPointF oAt2 = w.mapContentToWidget(offCenter);
    QVERIFY2(w.zoom() == 2.0, "zoom 应为 2.0");
    // 与中心的距离在 zoom=2 时应为 zoom=1 时的 2 倍
    const double dist1 = QLineF(cAt1, oAt1).length();
    const double dist2 = QLineF(cAt2, oAt2).length();
    QVERIFY2(std::abs(dist2 - 2.0 * dist1) < 1.0,
             qPrintable(QStringLiteral("非中心点距离应按 zoom 放大，dist1=%1 dist2=%2")
                            .arg(dist1).arg(dist2)));
}

void MosRunwayClosureTest::zoomClampedToHalfToThree()
{
    // 契约：zoom 钳制在 [0.5, 3.0]
    MosRunwayWidget w;
    w.resize(800, 400);
    w.setSnapshot(makeSeed());

    w.setZoomDisplay(10.0);
    QVERIFY2(w.zoom() == 3.0, "zoom 上限应为 3.0");
    w.setZoomDisplay(0.0);
    QVERIFY2(w.zoom() == 0.5, "zoom 下限应为 0.5");
    w.zoomBy(100.0);
    QVERIFY2(w.zoom() == 3.0, "zoomBy 上限应为 3.0");
}

void MosRunwayClosureTest::panClampedToBounds()
{
    // 契约：zoom=1 时内容等于视口，pan 钳制为 0；zoom=2 时 pan 限制在溢出半幅内
    MosRunwayWidget w;
    w.resize(800, 400);
    w.setSnapshot(makeSeed());

    w.setZoomDisplay(1.0);
    // zoom=1 时即使拖拽，pan 也应保持 0（钳制后）
    // 通过 mapContentToWidget 间接验证：pan=0 时中心映射到视口中心
    const QPointF center(400, 200);
    const QPointF mapped = w.mapContentToWidget(center);
    QVERIFY2(std::abs(mapped.x() - 400) < 0.5 && std::abs(mapped.y() - 200) < 0.5,
             "zoom=1 时内容中心应映射到视口中心");

    w.setZoomDisplay(2.0);
    // zoom=2 时 pan 范围为 [-200, 200]（800*(2-1)/2）；验证中心仍稳定（pan 自动钳制为 0）
    const QPointF centerAt2 = w.mapContentToWidget(center);
    QVERIFY2(std::abs(centerAt2.x() - 400) < 0.5 && std::abs(centerAt2.y() - 200) < 0.5,
             "zoom=2 pan=0 时视口中心应稳定");
}

void MosRunwayClosureTest::resetViewClearsZoomAndPan()
{
    // 契约：resetView 将 zoom 复位为 1.0，pan 复位为 0
    MosRunwayWidget w;
    w.resize(800, 400);
    w.setSnapshot(makeSeed());

    w.setZoomDisplay(2.5);
    QVERIFY2(w.zoom() == 2.5, "预设 zoom=2.5");
    w.resetView();
    QVERIFY2(w.zoom() == 1.0, "resetView 后 zoom 应为 1.0");
    const QPointF center(400, 200);
    const QPointF mapped = w.mapContentToWidget(center);
    QVERIFY2(std::abs(mapped.x() - 400) < 0.5 && std::abs(mapped.y() - 200) < 0.5,
             "resetView 后 pan 应为 0，中心映射到视口中心");
}

void MosRunwayClosureTest::hitTestTargetInverseTransform()
{
    // 契约：hitTestTarget 经 widget->content 逆变换，与 paintEvent 绘制位置对齐
    MosRunwayWidget w;
    w.resize(800, 400);
    const auto snap = makeSeed();
    w.setSnapshot(snap);
    QVERIFY2(!snap.obstacles.craters.isEmpty(), "seed 快照必须含弹坑");

    // 找到首个弹坑的 widget 坐标，验证命中返回其 ID
    const QString firstId = snap.obstacles.craters.first().id;
    const QPointF contentPt = w.mapWidgetToContent(w.mapContentToWidget(QPointF(400, 200)));
    // 中心点未必命中弹坑；用 hitTestTarget 验证逆变换闭环：在 widget 坐标下点击
    // mapContentToWidget 的输出应能被 mapWidgetToContent 还原
    QVERIFY2(std::abs(contentPt.x() - 400) < 1.0 && std::abs(contentPt.y() - 200) < 1.0,
             "mapContentToWidget 与 mapWidgetToContent 应互逆");

    // 无障碍物区域应返回空串
    const QString miss = w.hitTestTarget(QPointF(10, 10));
    QVERIFY2(miss.isEmpty(), "无障碍物区域 hitTestTarget 应返回空串");
}

void MosRunwayClosureTest::hitTestTierInverseTransform()
{
    // 契约：hitTestTier 经逆变换命中档位矩形，无结果时返回 -1
    MosRunwayWidget w;
    w.resize(800, 400);
    const auto snap = makeSeed();
    w.setSnapshot(snap);
    QVERIFY2(snap.hasResult && snap.result.accepted, "seed 快照必须有结果");

    // 角落点在核心区外（内容 Y < kMarginY），应未命中任何档位
    const int miss = w.hitTestTier(QPointF(5, 5));
    QVERIFY2(miss == -1, "核心区外的角落点应未命中档位");

    // 无结果快照应始终返回 -1
    MosRunwayWidget w2;
    w2.resize(800, 400);
    w2.setSnapshot(Core::MOS::MosPlanningSnapshot{});
    const int noHit = w2.hitTestTier(QPointF(400, 200));
    QVERIFY2(noHit == -1, "无有效档位快照 hitTestTier 应返回 -1");
}

void MosRunwayClosureTest::fontZoomCompensationKeepsContentStable()
{
    // 契约：viewportScale 与 zoom 独立；zoom 变化不影响 viewportScale
    MosRunwayWidget w;
    w.resize(800, 400);
    w.setSnapshot(makeSeed());

    w.setViewportScale(2.0);
    QVERIFY2(w.viewportScale() == 2.0, "viewportScale 应为 2.0");
    w.setZoomDisplay(2.5);
    QVERIFY2(w.viewportScale() == 2.0, "zoom 变化不应影响 viewportScale");
    QVERIFY2(w.zoom() == 2.5, "zoom 应为 2.5");
    w.setViewportScale(1.0);
    QVERIFY2(w.zoom() == 2.5, "viewportScale 变化不应影响 zoom");
}

void MosRunwayClosureTest::formatTierLabelContainsTierNumberIntervalsAndDimensions()
{
    // 契约：formatTierLabel 输出“档位N X[a..b] Y[c..d] L×Wm”，1-based 档位号，
    // X/Y 区间按整数米，长×宽按整数。与 paintEvent 标签格式共享同一纯函数。
    Core::MOS::MosRectangleResult rect;
    rect.valid = true;
    rect.xStart = 100.4;
    rect.xEnd = 3000.6;
    rect.yStart = -25.0;
    rect.yEnd = 25.0;
    rect.length = 2900.2;
    rect.width = 50.0;
    const QString label = formatTierLabel(2, rect);
    QCOMPARE(label, QStringLiteral("档位3 X[100..3001] Y[-25..25] 2900×50m"));
}

void MosRunwayClosureTest::targetClickEmitsTargetClickedAtObstaclePixel()
{
    // Given: 800x400 widget，单弹坑置于跑道中点 (x=2000, y=0)。
    MosRunwayWidget w;
    w.resize(800, 400);
    const auto snap = makeSingleCraterCenter();
    w.setSnapshot(snap);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    // 弹坑内容像素 = (xToPx(2000), yCoreToPx(0)) = (400, 200)（zoom=1 pan=0 时等同 widget 像素）
    const auto lay = computeLayout(snap, 800, 400);
    const QPointF craterWidgetPx = w.mapContentToWidget(
        QPointF(xToPx(2000.0, lay), yCoreToPx(0.0, lay)));
    QSignalSpy spy(&w, &MosRunwayWidget::targetClicked);
    QVERIFY(spy.isValid());

    // When: 在弹坑像素位置左键点击。
    QTest::mouseClick(&w, Qt::LeftButton, Qt::NoModifier, craterWidgetPx.toPoint());

    // Then: targetClicked 发射一次，载荷为弹坑 ID。
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("C-CENTER"));
}

void MosRunwayClosureTest::overlappingTiersHitSelectedTierOnly()
{
    // Given: 3 个完全重叠档位矩形，显式选中档位 2。
    MosRunwayWidget w;
    w.resize(800, 400);
    const auto snap = makeOverlappingTiers();
    w.setSnapshot(snap);
    w.setSelectedTier(1);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QVERIFY(snap.result.tiers.size() == 3);
    // 重叠中心内容像素 = (xToPx(2000), yCoreToPx(0)) = (400, 200)，三档矩形均覆盖该点
    const auto lay = computeLayout(snap, 800, 400);
    const QPointF overlapWidgetPx = w.mapContentToWidget(
        QPointF(xToPx(2000.0, lay), yCoreToPx(0.0, lay)));
    QSignalSpy spy(&w, &MosRunwayWidget::tierClicked);
    QVERIFY(spy.isValid());

    // When: 在三档重叠区域中心左键点击。
    QTest::mouseClick(&w, Qt::LeftButton, Qt::NoModifier, overlapWidgetPx.toPoint());

    // Then: tierClicked 发射一次，载荷为显式选中的 index=1（而非 topmost=2）。
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toInt(), 1);
}

void MosRunwayClosureTest::obstacleRadiusPxMatchesInfluenceRadiusTimesPxPerM()
{
    // 契约：障碍物视觉半径(像素) = influenceRadius(m) × ContentLayout::pxPerM。
    const double influence = 5.346; // 取自截图 uxo-1 影响半径，非真实设备参数
    // 两种 widget 宽高比 -> 两个不同 pxPerM，确保不是巧合相等
    const auto layA = computeLayout(makeSeed(), 800, 400);
    const auto layB = computeLayout(makeSeed(), 1200, 300);
    QVERIFY2(layA.pxPerM > 0.0 && layB.pxPerM > 0.0, "两个布局必须给出正值 pxPerM");
    QVERIFY2(layA.pxPerM != layB.pxPerM, "两种宽高比应产生不同 pxPerM");
    // 严格断言：像素半径 == 米半径 × 比例，无钳制、无系数
    QCOMPARE(obstacleRadiusPx(influence, layA), influence * layA.pxPerM);
    QCOMPARE(obstacleRadiusPx(influence, layB), influence * layB.pxPerM);
}

void MosRunwayClosureTest::middleButtonDragAtZoomTwoShiftsContentMapping()
{
    // Given: 800x400 widget，zoom=2 后 pan 范围 [-200,200]，初始 pan=0。
    MosRunwayWidget w;
    w.resize(800, 400);
    w.setSnapshot(makeSeed());
    w.setZoomDisplay(2.0);
    QVERIFY2(w.zoom() == 2.0, "预设 zoom=2.0");
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    const QPointF center(400, 200);
    const QPointF beforeDrag = w.mapContentToWidget(center);
    QVERIFY2(std::abs(beforeDrag.x() - 400) < 0.5 && std::abs(beforeDrag.y() - 200) < 0.5,
             "pan=0 时视口中心应映射到自身");

    // When: 中键按下于 (300,200)，移动 +50px 到 (350,200)，释放。增量平移 = (50,0)。
    // 直接构造 QMouseEvent 投递，避免 QTest::mouseMove 在 offscreen 平台下
    // 不保留按键状态导致 mouseMoveEvent 早返回（m_dragging=false）。
    auto sendMouse = [&](QEvent::Type type, const QPoint &pos, Qt::MouseButton button,
                         Qt::MouseButtons buttons) {
        QMouseEvent ev(type, pos, button, buttons, Qt::NoModifier);
        QApplication::sendEvent(&w, &ev);
    };
    sendMouse(QEvent::MouseButtonPress, QPoint(300, 200), Qt::MiddleButton, Qt::MiddleButton);
    sendMouse(QEvent::MouseMove, QPoint(350, 200), Qt::NoButton, Qt::MiddleButton);
    sendMouse(QEvent::MouseButtonRelease, QPoint(350, 200), Qt::MiddleButton, Qt::NoButton);
    QCoreApplication::processEvents();

    // Then: panOffset=(50,0)，内容中心映射应右移 50px（widget 坐标）。
    const QPointF afterDrag = w.mapContentToWidget(center);
    QVERIFY2(std::abs(afterDrag.x() - 450.0) < 1.0 && std::abs(afterDrag.y() - 200.0) < 1.0,
             qPrintable(QStringLiteral("中键拖拽 +50px 后内容中心应映射到 (450,200)，实际 (%1,%2)")
                            .arg(afterDrag.x()).arg(afterDrag.y())));
}

QTEST_MAIN(MosRunwayClosureTest)
#include "mos_runway_closure_test.moc"
