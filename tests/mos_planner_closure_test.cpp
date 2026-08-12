// MOS P0 规划器闭合测试：独立小域 oracle、supplied-tier 校验顺序与完成结果重算守卫。

#include "Core/MOS/MosPlanner.h"
#include "MainWindow/MosPlanningController.h"

#include <QtTest>
#include <QSignalSpy>
#include <algorithm>
#include <cmath>

namespace {

Core::MOS::MosRunwayParams params()
{
    Core::MOS::MosRunwayParams p;
    p.L = 500.0; p.W = 20.0; p.K = 1.5; p.expand = 1.0; p.step = 5.0;
    p.minLength = 1.0; p.minWidth = 5.0; p.backfill = 50.0;
    p.uxoHours = 8.0; p.tiers = 3;
    return p;
}

Core::MOS::MosCrater crater(const QString &id, int x, int y, double radius)
{
    Core::MOS::MosCrater c;
    c.id = id; c.visibleRadius = radius; c.x = x; c.y = y;
    c.threat = Core::MOS::MosThreatLevel::High; c.influenceRadius = radius;
    return c;
}

bool better(const Core::MOS::MosRectangleResult &a,
            const Core::MOS::MosRectangleResult &b)
{
    if (a.area != b.area) return a.area > b.area;
    if (a.yStart != b.yStart) return a.yStart < b.yStart;
    if (a.xStart != b.xStart) return a.xStart < b.xStart;
    if (a.length != b.length) return a.length > b.length;
    return a.width > b.width;
}

void consider(Core::MOS::MosRectangleResult &best, double rawLow, double rawHigh,
              bool runwayLow, bool runwayHigh, double yLow, double yHigh)
{
    const double xLow = runwayLow ? rawLow : std::nextafter(rawLow, rawHigh);
    const double xHigh = runwayHigh ? rawHigh : std::nextafter(rawHigh, rawLow);
    if (xLow >= xHigh) return;
    Core::MOS::MosRectangleResult candidate;
    candidate.valid = true; candidate.reason = Core::MOS::MosPlannerReason::Accepted;
    candidate.xStart = xLow; candidate.xEnd = xHigh;
    candidate.yStart = yLow; candidate.yEnd = yHigh;
    candidate.length = xHigh - xLow; candidate.width = yHigh - yLow;
    candidate.area = candidate.length * candidate.width;
    if (!best.valid || better(candidate, best)) best = candidate;
}

// 独立单圆盘 oracle：所有障碍物按 influenceRadius 投影到 X 轴，Y 固定为跑道全宽
Core::MOS::MosRectangleResult oracle(const Core::MOS::MosCrater &c,
                                     const Core::MOS::MosRunwayParams &p)
{
    Core::MOS::MosRectangleResult best;
    best.reason = Core::MOS::MosPlannerReason::NoFeasibleRectangle;
    const double yLow = -p.W / 2.0;
    const double yHigh = p.W / 2.0;
    const double low = std::max(0.0, static_cast<double>(c.x) - c.influenceRadius);
    const double high = std::min(p.L, static_cast<double>(c.x) + c.influenceRadius);
    if (low > high) {
        consider(best, 0.0, p.L, true, true, yLow, yHigh);
    } else {
        consider(best, 0.0, low, true, false, yLow, yHigh);
        consider(best, high, p.L, false, true, yLow, yHigh);
    }
    return best;
}

bool sameRectangle(const Core::MOS::MosRectangleResult &a,
                   const Core::MOS::MosRectangleResult &b)
{
    return a.valid == b.valid && a.reason == b.reason && a.xStart == b.xStart &&
           a.xEnd == b.xEnd && a.yStart == b.yStart && a.yEnd == b.yEnd &&
           a.length == b.length && a.width == b.width && a.area == b.area;
}

Core::MOS::MosObstacleSet threeObstacles()
{
    Core::MOS::MosObstacleSet obstacles;
    obstacles.craters = {crater(QStringLiteral("a"), 100, 0, 5.0),
                         crater(QStringLiteral("b"), 250, 0, 5.0),
                         crater(QStringLiteral("c"), 400, 0, 5.0)};
    return obstacles;
}

QVector<Core::MOS::MosTierPlan> validPlans()
{
    return {{}, {{QStringLiteral("a")}},
            {{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}}};
}

} // namespace

class MosPlannerClosureTest : public QObject
{
    Q_OBJECT
private slots:
    void exhaustiveSingleObstacleMatchesIndependentOracle();
    void suppliedTierValidationOrder_data();
    void suppliedTierValidationOrder();
    void controllerRejectsNonNestedCompletionAtomically();
    void controllerRejectsGeometryMismatchAtomically();
};

void MosPlannerClosureTest::exhaustiveSingleObstacleMatchesIndependentOracle()
{
    // Given: 45 个边界、中心、相切半径组合。
    const auto p = params();
    for (int x : {0, 125, 250, 375, 500}) {
        for (int y : {-10, 0, 10}) {
            for (double radius : {1.0, 5.0, 10.0}) {
                Core::MOS::MosObstacleSet obstacles;
                obstacles.craters.append(crater(QStringLiteral("c"), x, y, radius));
                // When: 生产规划器与独立单圆盘 oracle 分别求解。
                const auto actual = Core::MOS::MosPlanner::planSingle(obstacles, p);
                const auto expected = oracle(obstacles.craters.first(), p);
                // Then: 最大矩形及总排序结果逐位一致。
                QVERIFY2(sameRectangle(actual, expected),
                         qPrintable(QStringLiteral("oracle mismatch x=%1 y=%2 r=%3")
                                        .arg(x).arg(y).arg(radius)));
            }
        }
    }
}

void MosPlannerClosureTest::suppliedTierValidationOrder_data()
{
    QTest::addColumn<int>("scenario");
    QTest::addColumn<int>("expectedReason");
    QTest::newRow("params-first") << 0 << int(Core::MOS::MosPlannerReason::InvalidParams);
    QTest::newRow("obstacles-second") << 1 << int(Core::MOS::MosPlannerReason::InvalidObstacles);
    QTest::newRow("tier-count") << 2 << int(Core::MOS::MosPlannerReason::InvalidTierCount);
    QTest::newRow("empty-initial") << 3 << int(Core::MOS::MosPlannerReason::EmptyInitialTier);
    QTest::newRow("unknown-before-duplicate") << 4 << int(Core::MOS::MosPlannerReason::UnknownRepairedId);
    QTest::newRow("duplicate") << 5 << int(Core::MOS::MosPlannerReason::DuplicateRepairedId);
    QTest::newRow("complete-final") << 6 << int(Core::MOS::MosPlannerReason::IncompleteFinalTier);
    QTest::newRow("nesting") << 7 << int(Core::MOS::MosPlannerReason::NonNestedTiers);
}

void MosPlannerClosureTest::suppliedTierValidationOrder()
{
    QFETCH(int, scenario); QFETCH(int, expectedReason);
    auto p = params(); auto obstacles = threeObstacles(); auto plans = validPlans();
    if (scenario == 0) { p.L = 1.0; obstacles.craters[0].id.clear(); plans.clear(); }
    if (scenario == 1) { obstacles.craters[0].id.clear(); plans.clear(); }
    if (scenario == 2) plans.removeLast();
    if (scenario == 3) plans[0].repairedIds = {QStringLiteral("a")};
    if (scenario == 4) plans[1].repairedIds = {QStringLiteral("ghost"), QStringLiteral("ghost")};
    if (scenario == 5) plans[1].repairedIds = {QStringLiteral("a"), QStringLiteral("a")};
    if (scenario == 6) plans.last().repairedIds = {QStringLiteral("a"), QStringLiteral("b")};
    if (scenario == 7) {
        p.tiers = 4;
        plans = {{}, {{QStringLiteral("a"), QStringLiteral("b")}},
                 {{QStringLiteral("b")}},
                 {{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}}};
    }
    // When: supplied-tier 方案进入确定性边界校验。
    const auto result = Core::MOS::MosPlanner::planProgressive(obstacles, p, plans);
    // Then: 最早失败原因稳定，拒绝结果不泄漏部分 tiers。
    QVERIFY(!result.accepted);
    QCOMPARE(int(result.reason), expectedReason);
    QVERIFY(result.tiers.isEmpty());
}

void MosPlannerClosureTest::controllerRejectsNonNestedCompletionAtomically()
{
    // Given: pending revision 与 accepted=true 但 supplied tiers 非嵌套的完成结果。
    Core::MOS::MosPlanningController controller;
    controller.worker()->disconnect(&controller);
    QSignalSpy completed(controller.worker(), &Core::MOS::MosReplanWorker::replanCompleted);
    auto p = params(); p.tiers = 4;
    controller.requestReplan(threeObstacles(), p);
    auto completion = completed.first().first().value<Core::MOS::MosReplanCompletion>();
    completion.result.tiers[1].repairedIds = {QStringLiteral("a"), QStringLiteral("b")};
    completion.result.tiers[2].repairedIds = {QStringLiteral("b")};
    // When: controller 重算并处理完成结果。
    const auto disposition = controller.completeReplan(completion);
    // Then: 原子拒绝且只记录具体结构原因。
    QCOMPARE(disposition, Core::MOS::MosCompletionDisposition::Rejected);
    const auto snapshot = controller.snapshot();
    QVERIFY(!snapshot.hasResult); QCOMPARE(snapshot.committedRevision, quint64(0));
    QVERIFY(snapshot.result.tiers.isEmpty()); QCOMPARE(snapshot.logEntries.size(), 1);
    QCOMPARE(snapshot.logEntries.first().reason, Core::MOS::MosPlannerReason::NonNestedTiers);
}

void MosPlannerClosureTest::controllerRejectsGeometryMismatchAtomically()
{
    // Given: pending revision 与 repairedIds 合法但面积被篡改的完成结果。
    Core::MOS::MosPlanningController controller;
    controller.worker()->disconnect(&controller);
    QSignalSpy completed(controller.worker(), &Core::MOS::MosReplanWorker::replanCompleted);
    controller.requestReplan(threeObstacles(), params());
    auto completion = completed.first().first().value<Core::MOS::MosReplanCompletion>();
    completion.result.tiers[0].rectangle.area += 1.0;
    // When: controller 逐位比对重算结果。
    const auto disposition = controller.completeReplan(completion);
    // Then: 不提交业务状态，并记录 CompletionMismatch。
    QCOMPARE(disposition, Core::MOS::MosCompletionDisposition::Rejected);
    const auto snapshot = controller.snapshot();
    QVERIFY(!snapshot.hasResult); QCOMPARE(snapshot.committedRevision, quint64(0));
    QVERIFY(snapshot.result.tiers.isEmpty()); QCOMPARE(snapshot.logEntries.size(), 1);
    QCOMPARE(snapshot.logEntries.first().reason, Core::MOS::MosPlannerReason::CompletionMismatch);
}

QTEST_GUILESS_MAIN(MosPlannerClosureTest)
#include "mos_planner_closure_test.moc"
