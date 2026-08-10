// MOS P0 合成规划器测试：覆盖单次最大空矩形、稳定档位与递进规划，
// 并用独立归约域 oracle 校验生产规划器几何与总排序契约。
// 本测试在实现前编写，期望先失败再通过（failing-first TDD）。

#include "Core/MOS/MosPlanner.h"
#include "Core/MOS/MosEstimator.h"

#include <QtTest>
#include <cmath>
#include <algorithm>
#include <vector>

namespace {

// 默认合法跑道参数（合同常量，与 mos_validation_test 保持一致）
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

// 构造弹坑障碍物（简化构造，合成语义）
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

// 构造 UXO 障碍物（简化构造，合成语义）
Core::MOS::MosUxo makeUxo(const QString &id, double yield, int x, int y, double influenceR)
{
    Core::MOS::MosUxo u;
    u.id = id;
    u.syntheticYield = yield;
    u.x = x;
    u.y = y;
    u.threat = Core::MOS::MosThreatLevel::High;
    u.influenceRadius = influenceR;
    return u;
}

// 比较两个矩形结果是否相等（紧容差，检测几何/排序契约违背）
bool rectanglesEqual(const Core::MOS::MosRectangleResult &a,
                     const Core::MOS::MosRectangleResult &b)
{
    if (a.valid != b.valid) return false;
    if (a.reason != b.reason) return false;
    if (!a.valid) return true; // 无解只需 reason 一致
    const double eps = 1e-9;
    return std::fabs(a.xStart - b.xStart) < eps
        && std::fabs(a.xEnd - b.xEnd) < eps
        && std::fabs(a.yStart - b.yStart) < eps
        && std::fabs(a.yEnd - b.yEnd) < eps
        && std::fabs(a.length - b.length) < eps
        && std::fabs(a.width - b.width) < eps
        && std::fabs(a.area - b.area) < eps;
}

// === 独立归约域 oracle：不复用生产规划器几何辅助 ===
// 所有障碍物按 influenceRadius 投影到 X 轴（不论 Y 位置），Y 固定为跑道全宽 [-W/2, W/2]。
// 闭 X 区间合并、自由空隙枚举与 nextafter 向内规范化（障碍侧端点），
// 按 area 降序 > yStart 升序 > xStart 升序 > length 降序 > width 降序 选优。
Core::MOS::MosRectangleResult oraclePlanSingle(const Core::MOS::MosObstacleSet &obstacles,
                                               const Core::MOS::MosRunwayParams &params,
                                               const QVector<QString> &repairedIds = {})
{
    using namespace Core::MOS;
    MosRectangleResult best;
    best.valid = false;
    best.reason = MosPlannerReason::NoFeasibleRectangle;

    const double L = params.L;
    const double W = params.W;
    const double halfW = W / 2.0;

    // 收集未修复障碍物为圆盘 (cx, cy, r)
    struct Disk { double cx, cy, r; };
    std::vector<Disk> disks;
    for (const auto &c : obstacles.craters)
        if (!repairedIds.contains(c.id))
            disks.push_back({static_cast<double>(c.x), static_cast<double>(c.y), c.influenceRadius});
    for (const auto &u : obstacles.uxo)
        if (!repairedIds.contains(u.id))
            disks.push_back({static_cast<double>(u.x), static_cast<double>(u.y), u.influenceRadius});

    // 五级总排序：area 降序 > yStart 升序 > xStart 升序 > length 降序 > width 降序
    auto better = [](const MosRectangleResult &c, const MosRectangleResult &b) {
        if (c.area != b.area) return c.area > b.area;
        if (c.yStart != b.yStart) return c.yStart < b.yStart;
        if (c.xStart != b.xStart) return c.xStart < b.xStart;
        if (c.length != b.length) return c.length > b.length;
        return c.width > b.width;
    };

    // 所有障碍物投影到 X 轴，Y 固定为跑道全宽 [-halfW, halfW]
    const double yLow = -halfW;
    const double yHigh = halfW;
    const double width = W;

    struct Interval { double low, high; };
    std::vector<Interval> blocked;
    for (const auto &d : disks)
        blocked.push_back({d.cx - d.r, d.cx + d.r});

    // 裁剪到 [0, L] 并合并闭区间（含 singleton 相切）
    std::sort(blocked.begin(), blocked.end(),
              [](const Interval &a, const Interval &b) { return a.low < b.low; });
    std::vector<Interval> merged;
    for (const auto &iv : blocked) {
        double lo = std::max(iv.low, 0.0);
        double hi = std::min(iv.high, L);
        if (lo > hi) continue;
        if (!merged.empty() && lo <= merged.back().high)
            merged.back().high = std::max(merged.back().high, hi);
        else
            merged.push_back({lo, hi});
    }

    // 枚举自由空隙：跑道边界端点保持精确，障碍侧端点 nextafter 向内
    auto consider = [&](double xRawLo, double xRawHi, bool loAtRunway, bool hiAtRunway) {
        double xLo = loAtRunway ? xRawLo : std::nextafter(xRawLo, xRawHi);
        double xHi = hiAtRunway ? xRawHi : std::nextafter(xRawHi, xRawLo);
        if (xLo >= xHi) return;
        double len = xHi - xLo;
        if (len < params.minLength) return;
        MosRectangleResult cand;
        cand.valid = true;
        cand.reason = MosPlannerReason::Accepted;
        cand.xStart = xLo;
        cand.xEnd = xHi;
        cand.yStart = yLow;
        cand.yEnd = yHigh;
        cand.length = len;
        cand.width = width;
        cand.area = len * width;
        if (!best.valid || better(cand, best))
            best = cand;
    };

    if (merged.empty()) {
        consider(0.0, L, true, true);
    } else {
        consider(0.0, merged.front().low, true, false);
        for (size_t k = 1; k < merged.size(); ++k)
            consider(merged[k - 1].high, merged[k].low, false, false);
        consider(merged.back().high, L, false, true);
    }

    return best;
}

} // namespace

class MosPlannerTest : public QObject
{
    Q_OBJECT

private slots:
    // === planSingle：空跑道返回全长全宽 ===
    void planSingleEmptyRunwayIsFullLength();
    // === planSingle：完全阻塞为合法无解 ===
    void planSingleFullyBlockedIsLegalNoSolution();
    // === planSingle：相切视为碰撞（oracle 对比） ===
    void planSingleTangentIsCollision();
    // === planSingle：重叠（oracle 对比） ===
    void planSingleOverlap();
    // === planSingle：边界（oracle 对比） ===
    void planSingleBoundary();
    // === planSingle：对称布局确定性排序 ===
    void planSingleSymmetricOrdering();
    // === planSingle：N=13 最大障碍物数 ===
    void planSingleMaxObstacles();
    // === planSingle：最小长宽 + 确定性重复 ===
    void planSingleMinLengthWidthDeterministic();
    // === oracle 对比：空跑道 ===
    void oracleMatchesProductionEmpty();
    // === oracle 对比：相切 ===
    void oracleMatchesProductionTangent();
    // === oracle 对比：边界 ===
    void oracleMatchesProductionBoundary();
    // === oracle 对比：重叠 ===
    void oracleMatchesProductionOverlap();
    // === oracle 对比：对称 ===
    void oracleMatchesProductionSymmetric();
    // === buildStableRepairTiers：T=5/N=10 前缀档位 ===
    void buildStableTiersT5N10Prefix();
    // === buildStableRepairTiers：N=0 全空档位 ===
    void buildStableTiersEmptyObstacles();
    // === planProgressive：合法接受 ===
    void planProgressiveAccepted();
    // === planProgressive：合法无解档位仍接受 ===
    void planProgressiveNoSolutionTierAccepted();
    // === planProgressive：非法参数拒绝 ===
    void planProgressiveInvalidParamsRejected();
    // === planProgressive：T>N 档位（前缀分布） ===
    void planProgressiveTiersExceedObstacles();
};

// === planSingle：空跑道返回全长全宽 ===
void MosPlannerTest::planSingleEmptyRunwayIsFullLength()
{
    auto params = defaultRunwayParams();
    Core::MOS::MosObstacleSet empty;
    const auto r = Core::MOS::MosPlanner::planSingle(empty, params);
    QVERIFY(r.valid);
    QCOMPARE(r.reason, Core::MOS::MosPlannerReason::Accepted);
    QCOMPARE(r.xStart, 0.0);
    QCOMPARE(r.xEnd, params.L);
    QCOMPARE(r.yStart, -params.W / 2.0);
    QCOMPARE(r.yEnd, params.W / 2.0);
    QCOMPARE(r.length, params.L);
    QCOMPARE(r.width, params.W);
    QCOMPARE(r.area, params.L * params.W);
}

// === planSingle：完全阻塞为合法无解 ===
void MosPlannerTest::planSingleFullyBlockedIsLegalNoSolution()
{
    auto params = defaultRunwayParams();
    Core::MOS::MosObstacleSet obs;
    // 单个超大影响半径圆盘覆盖全跑道
    obs.craters.append(makeCrater(QStringLiteral("blocker"), 5.0, 1500, 0, 2000.0));
    const auto r = Core::MOS::MosPlanner::planSingle(obs, params);
    QVERIFY(!r.valid);
    QCOMPARE(r.reason, Core::MOS::MosPlannerReason::NoFeasibleRectangle);
    QCOMPARE(r.area, 0.0);
}

// === planSingle：相切视为碰撞（oracle 对比） ===
void MosPlannerTest::planSingleTangentIsCollision()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet obs;
    // 圆盘中心 y=-26，r=1，相切于 Y 边界 -25（dy=1=r，half=0，singleton 投影）
    obs.craters.append(makeCrater(QStringLiteral("t"), 5.0, 1500, -26, 1.0));
    const auto prod = Core::MOS::MosPlanner::planSingle(obs, params);
    const auto orc = oraclePlanSingle(obs, params);
    QVERIFY(rectanglesEqual(prod, orc));
}

// === planSingle：重叠（oracle 对比） ===
void MosPlannerTest::planSingleOverlap()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("a"), 5.0, 1000, 0, 100.0));
    obs.craters.append(makeCrater(QStringLiteral("b"), 5.0, 1050, 0, 100.0));
    const auto prod = Core::MOS::MosPlanner::planSingle(obs, params);
    const auto orc = oraclePlanSingle(obs, params);
    QVERIFY(rectanglesEqual(prod, orc));
}

// === planSingle：边界（oracle 对比） ===
void MosPlannerTest::planSingleBoundary()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet obs;
    // 障碍物在跑道 X 边界 x=0
    obs.craters.append(makeCrater(QStringLiteral("edge"), 5.0, 0, 0, 50.0));
    const auto prod = Core::MOS::MosPlanner::planSingle(obs, params);
    const auto orc = oraclePlanSingle(obs, params);
    QVERIFY(rectanglesEqual(prod, orc));
}

// === planSingle：对称布局确定性排序 ===
void MosPlannerTest::planSingleSymmetricOrdering()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet obs;
    // 两个对称障碍物：X=1000 和 X=2000，相同半径
    obs.craters.append(makeCrater(QStringLiteral("left"), 5.0, 1000, 0, 100.0));
    obs.craters.append(makeCrater(QStringLiteral("right"), 5.0, 2000, 0, 100.0));
    const auto prod = Core::MOS::MosPlanner::planSingle(obs, params);
    const auto orc = oraclePlanSingle(obs, params);
    QVERIFY(rectanglesEqual(prod, orc));
    // 对称布局下三个空隙：左 [0,~900]、中 [~1100,~1900]、右 [~2100,3000]
    // 左右等面积，总排序选 yStart 最小后 xStart 最小 -> 最左空隙
    QVERIFY(prod.valid);
    QVERIFY(prod.xStart <= 1.0);
}

// === planSingle：N=13 最大障碍物数 ===
void MosPlannerTest::planSingleMaxObstacles()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet obs;
    for (int i = 0; i < 8; ++i)
        obs.craters.append(makeCrater(QStringLiteral("c%1").arg(i), 5.0, 100 + i * 200, 0, 30.0));
    for (int i = 0; i < 5; ++i)
        obs.uxo.append(makeUxo(QStringLiteral("u%1").arg(i), 30.0, 200 + i * 200, 10, 10.0));
    QCOMPARE(obs.craters.size() + obs.uxo.size(), 13);
    const auto prod = Core::MOS::MosPlanner::planSingle(obs, params);
    const auto orc = oraclePlanSingle(obs, params);
    QVERIFY(rectanglesEqual(prod, orc));
}

// === planSingle：最小长宽 + 确定性重复 ===
void MosPlannerTest::planSingleMinLengthWidthDeterministic()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("a"), 5.0, 1500, 0, 100.0));
    const auto r1 = Core::MOS::MosPlanner::planSingle(obs, params);
    const auto r2 = Core::MOS::MosPlanner::planSingle(obs, params);
    QVERIFY(r1.valid);
    QVERIFY(rectanglesEqual(r1, r2)); // 确定性：相同输入产生相同输出
    const auto orc = oraclePlanSingle(obs, params);
    QVERIFY(rectanglesEqual(r1, orc));
}

// === oracle 对比：空跑道 ===
void MosPlannerTest::oracleMatchesProductionEmpty()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet empty;
    const auto prod = Core::MOS::MosPlanner::planSingle(empty, params);
    const auto orc = oraclePlanSingle(empty, params);
    QVERIFY(rectanglesEqual(prod, orc));
}

// === oracle 对比：相切 ===
void MosPlannerTest::oracleMatchesProductionTangent()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet obs;
    // Y 相切：圆盘 y=-26, r=1 相切边界 -25
    obs.craters.append(makeCrater(QStringLiteral("yt"), 5.0, 1000, -26, 1.0));
    // X 相切：圆盘 x=3000, r=50 相切跑道端 L=3000
    obs.craters.append(makeCrater(QStringLiteral("xt"), 5.0, 3000, 0, 50.0));
    const auto prod = Core::MOS::MosPlanner::planSingle(obs, params);
    const auto orc = oraclePlanSingle(obs, params);
    QVERIFY(rectanglesEqual(prod, orc));
}

// === oracle 对比：边界 ===
void MosPlannerTest::oracleMatchesProductionBoundary()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("b0"), 5.0, 0, 0, 50.0));
    obs.craters.append(makeCrater(QStringLiteral("bL"), 5.0, 3000, 0, 50.0));
    const auto prod = Core::MOS::MosPlanner::planSingle(obs, params);
    const auto orc = oraclePlanSingle(obs, params);
    QVERIFY(rectanglesEqual(prod, orc));
}

// === oracle 对比：重叠 ===
void MosPlannerTest::oracleMatchesProductionOverlap()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("o1"), 5.0, 1000, 0, 80.0));
    obs.craters.append(makeCrater(QStringLiteral("o2"), 5.0, 1050, 5, 80.0));
    obs.craters.append(makeCrater(QStringLiteral("o3"), 5.0, 1100, -5, 80.0));
    const auto prod = Core::MOS::MosPlanner::planSingle(obs, params);
    const auto orc = oraclePlanSingle(obs, params);
    QVERIFY(rectanglesEqual(prod, orc));
}

// === oracle 对比：对称 ===
void MosPlannerTest::oracleMatchesProductionSymmetric()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    params.minWidth = 1.0;
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("s1"), 5.0, 750, 0, 60.0));
    obs.craters.append(makeCrater(QStringLiteral("s2"), 5.0, 2250, 0, 60.0));
    const auto prod = Core::MOS::MosPlanner::planSingle(obs, params);
    const auto orc = oraclePlanSingle(obs, params);
    QVERIFY(rectanglesEqual(prod, orc));
}

// === buildStableRepairTiers：T=5/N=10 前缀档位 ===
void MosPlannerTest::buildStableTiersT5N10Prefix()
{
    auto params = defaultRunwayParams();
    params.tiers = 5;
    Core::MOS::MosObstacleSet obs;
    // 5 弹坑 + 5 UXO = 10 障碍物，稳定顺序 c0..c4, u0..u4
    for (int i = 0; i < 5; ++i)
        obs.craters.append(makeCrater(QStringLiteral("c%1").arg(i), 5.0, 100 + i * 200, 0, 30.0));
    for (int i = 0; i < 5; ++i)
        obs.uxo.append(makeUxo(QStringLiteral("u%1").arg(i), 30.0, 200 + i * 200, 10, 10.0));

    const auto tiers = Core::MOS::MosPlanner::buildStableRepairTiers(obs, params);
    QCOMPARE(tiers.size(), 5);
    // floor(tierIndex * 10 / 4) = 0, 2, 5, 7, 10
    QCOMPARE(tiers[0].repairedIds.size(), 0);
    QCOMPARE(tiers[1].repairedIds.size(), 2);
    QCOMPARE(tiers[2].repairedIds.size(), 5);
    QCOMPARE(tiers[3].repairedIds.size(), 7);
    QCOMPARE(tiers[4].repairedIds.size(), 10);
    // 末档包含全部 ID
    QCOMPARE(tiers[4].repairedIds.size(), obs.craters.size() + obs.uxo.size());
    // 嵌套：每档是前一档的超集
    for (int t = 1; t < 5; ++t)
        for (const auto &id : tiers[t - 1].repairedIds)
            QVERIFY(tiers[t].repairedIds.contains(id));
    // 稳定顺序：前 5 个 craters，后 5 个 uxo
    QCOMPARE(tiers[4].repairedIds[0], QStringLiteral("c0"));
    QCOMPARE(tiers[4].repairedIds[4], QStringLiteral("c4"));
    QCOMPARE(tiers[4].repairedIds[5], QStringLiteral("u0"));
    QCOMPARE(tiers[4].repairedIds[9], QStringLiteral("u4"));
}

// === buildStableRepairTiers：N=0 全空档位 ===
void MosPlannerTest::buildStableTiersEmptyObstacles()
{
    auto params = defaultRunwayParams();
    params.tiers = 3;
    Core::MOS::MosObstacleSet empty;
    const auto tiers = Core::MOS::MosPlanner::buildStableRepairTiers(empty, params);
    QCOMPARE(tiers.size(), 3);
    for (const auto &t : tiers)
        QVERIFY(t.repairedIds.isEmpty());
}

// === planProgressive：合法接受 ===
void MosPlannerTest::planProgressiveAccepted()
{
    auto params = defaultRunwayParams();
    params.tiers = 3;
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("c1"), 5.0, 1000, 0, 50.0));
    obs.craters.append(makeCrater(QStringLiteral("c2"), 5.0, 2000, 0, 50.0));
    const auto result = Core::MOS::MosPlanner::planProgressive(obs, params);
    QVERIFY(result.accepted);
    QCOMPARE(result.reason, Core::MOS::MosPlannerReason::Accepted);
    QCOMPARE(result.tiers.size(), params.tiers);
    // 面积单调非减
    for (int t = 1; t < result.tiers.size(); ++t)
        QVERIFY(result.tiers[t].rectangle.area >= result.tiers[t - 1].rectangle.area);
}

// === planProgressive：合法无解档位仍接受 ===
void MosPlannerTest::planProgressiveNoSolutionTierAccepted()
{
    auto params = defaultRunwayParams();
    params.tiers = 3;
    Core::MOS::MosObstacleSet obs;
    // 单个超大障碍物：tier 0/1 无解（未修复），tier 2 修复后全空
    obs.craters.append(makeCrater(QStringLiteral("blocker"), 5.0, 1500, 0, 2000.0));
    const auto result = Core::MOS::MosPlanner::planProgressive(obs, params);
    QVERIFY(result.accepted);
    QCOMPARE(result.tiers.size(), params.tiers);
    // tier 0 无修复 -> 无解
    QVERIFY(result.tiers[0].repairedIds.isEmpty());
    QVERIFY(!result.tiers[0].rectangle.valid);
    QCOMPARE(result.tiers[0].rectangle.reason, Core::MOS::MosPlannerReason::NoFeasibleRectangle);
    // tier 2 修复全部 -> 有解
    QVERIFY(!result.tiers[2].repairedIds.isEmpty());
    QVERIFY(result.tiers[2].rectangle.valid);
}

// === planProgressive：非法参数拒绝 ===
void MosPlannerTest::planProgressiveInvalidParamsRejected()
{
    auto params = defaultRunwayParams();
    params.L = 50.0; // 非法：L < 100
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("c1"), 5.0, 50, 0, 10.0));
    const auto result = Core::MOS::MosPlanner::planProgressive(obs, params);
    QVERIFY(!result.accepted);
    QCOMPARE(result.reason, Core::MOS::MosPlannerReason::InvalidParams);
    QVERIFY(result.tiers.isEmpty());
}

// === planProgressive：T>N 档位（前缀分布） ===
void MosPlannerTest::planProgressiveTiersExceedObstacles()
{
    auto params = defaultRunwayParams();
    params.tiers = 5;
    Core::MOS::MosObstacleSet obs;
    // N=2 < T=5：floor(tierIndex * 2 / 4) = 0, 0, 1, 1, 2
    obs.craters.append(makeCrater(QStringLiteral("c1"), 5.0, 1000, 0, 50.0));
    obs.craters.append(makeCrater(QStringLiteral("c2"), 5.0, 2000, 0, 50.0));
    const auto result = Core::MOS::MosPlanner::planProgressive(obs, params);
    QVERIFY(result.accepted);
    QCOMPARE(result.tiers.size(), 5);
    // tier 0 和 tier 1 都为空（前缀分布）
    QVERIFY(result.tiers[0].repairedIds.isEmpty());
    QVERIFY(result.tiers[1].repairedIds.isEmpty());
    // 末档修复全部
    QCOMPARE(result.tiers[4].repairedIds.size(), 2);
    // 面积单调非减
    for (int t = 1; t < result.tiers.size(); ++t)
        QVERIFY(result.tiers[t].rectangle.area >= result.tiers[t - 1].rectangle.area);
}

QTEST_APPLESS_MAIN(MosPlannerTest)

#include "mos_planner_test.moc"
