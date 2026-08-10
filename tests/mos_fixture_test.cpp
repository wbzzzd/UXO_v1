// MOS P0 确定性生成器与合成估算测试：
// - 五个种子向量与冻结 oracle（mos_rng_vectors.json）逐字段比对
// - 确定性：同种子同参数两次生成字节一致
// - canonical JSON 字段顺序与公式
// - 嵌套 fixture 顺序 floor(tierIndex * N / (T-1))
// - 估算器球体体积、合成回填/UXO 工时、序数难度
// 本测试在实现前编写，期望先失败再通过（failing-first TDD）。

#include "Core/MOS/MosTypes.h"
#include "Core/MOS/MosFixtureGenerator.h"
#include "Core/MOS/MosEstimator.h"

#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <limits>

namespace {

// 默认合同参数（与 mos_rng_vectors.json 一致）
Core::MOS::MosRunwayParams defaultParams()
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

Core::MOS::MosGeneratorParams defaultGenParams()
{
    Core::MOS::MosGeneratorParams g;
    g.craterCount = 2;
    g.craterRMin = 3;
    g.craterRMax = 6;
    g.uxoCount = 2;
    g.uxoYMin = 10.0;
    g.uxoYMax = 50.0;
    return g;
}

// 浮点模糊比较：JS Number 与 C++ double 应在 1e-9 内一致
bool fuzzyEq(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) <= eps;
}

} // namespace

class MosFixtureTest : public QObject
{
    Q_OBJECT

private slots:
    // === 五个种子向量与冻结 oracle 比对 ===
    void seed0MatchesOracle();
    void seed42MatchesOracle();
    void seedNegative1MatchesOracle();
    void seedInt32MinMatchesOracle();
    void seedInt32MaxMatchesOracle();
    // === 确定性 ===
    void sameSeedProducesIdenticalBytes();
    void differentSeedsProduceDifferentBytes();
    // === 计数与抽取顺序 ===
    void generatorProducesExpectedCounts();
    void firstCraterThreatIsFixedHigh();
    // === 公式正确性 ===
    void craterInfluenceRadiusFormula();
    void uxoInfluenceRadiusFormula();
    void coordinateRangesRespected();
    // === canonical JSON 字段顺序 ===
    void serializedCraterHasCanonicalFieldOrder();
    void serializedUxoHasCanonicalFieldOrder();
    // === 嵌套 fixture 顺序 ===
    void nestedFixtureOrderT5N10();
    void nestedFixtureOrderT1();
    void nestedFixtureOrderT0();
    // === 估算器 ===
    void sphereVolumeFormula();
    void estimateBackfillAndUxoHours();
    void estimateDifficultyForTier();
};

// === 五个种子向量与冻结 oracle 比对 ===
// 数据来源：tests/fixtures/mos_rng_vectors.json
void MosFixtureTest::seed0MatchesOracle()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, 0);

    QCOMPARE(obs.craters.size(), 2);
    QCOMPARE(obs.uxo.size(), 2);

    // crater 0: {visibleRadius:4, x:1, y:-22, threat:high, influenceRadius:6}
    QCOMPARE(obs.craters[0].visibleRadius, 4.0);
    QCOMPARE(obs.craters[0].x, 1);
    QCOMPARE(obs.craters[0].y, -22);
    QCOMPARE(obs.craters[0].threat, Core::MOS::MosThreatLevel::High);
    QVERIFY(fuzzyEq(obs.craters[0].influenceRadius, 6.0));

    // crater 1: {visibleRadius:3, x:1402, y:4, threat:medium, influenceRadius:4.5}
    QCOMPARE(obs.craters[1].visibleRadius, 3.0);
    QCOMPARE(obs.craters[1].x, 1402);
    QCOMPARE(obs.craters[1].y, 4);
    QCOMPARE(obs.craters[1].threat, Core::MOS::MosThreatLevel::Medium);
    QVERIFY(fuzzyEq(obs.craters[1].influenceRadius, 4.5));

    // uxo 0: {syntheticYield:35.959415193647146, x:1368, y:6, threat:medium, influenceRadius:4.95102895017792}
    QVERIFY(fuzzyEq(obs.uxo[0].syntheticYield, 35.959415193647146));
    QCOMPARE(obs.uxo[0].x, 1368);
    QCOMPARE(obs.uxo[0].y, 6);
    QCOMPARE(obs.uxo[0].threat, Core::MOS::MosThreatLevel::Medium);
    QVERIFY(fuzzyEq(obs.uxo[0].influenceRadius, 4.95102895017792));

    // uxo 1: {syntheticYield:13.914067791774869, x:2710, y:-7, threat:high, influenceRadius:3.6078014661363853}
    QVERIFY(fuzzyEq(obs.uxo[1].syntheticYield, 13.914067791774869));
    QCOMPARE(obs.uxo[1].x, 2710);
    QCOMPARE(obs.uxo[1].y, -7);
    QCOMPARE(obs.uxo[1].threat, Core::MOS::MosThreatLevel::High);
    QVERIFY(fuzzyEq(obs.uxo[1].influenceRadius, 3.6078014661363853));
}

void MosFixtureTest::seed42MatchesOracle()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, 42);

    QCOMPARE(obs.craters.size(), 2);
    QCOMPARE(obs.uxo.size(), 2);

    // crater 0: {visibleRadius:5, x:1345, y:28, threat:high, influenceRadius:7.5}
    QCOMPARE(obs.craters[0].visibleRadius, 5.0);
    QCOMPARE(obs.craters[0].x, 1345);
    QCOMPARE(obs.craters[0].y, 28);
    QCOMPARE(obs.craters[0].threat, Core::MOS::MosThreatLevel::High);
    QVERIFY(fuzzyEq(obs.craters[0].influenceRadius, 7.5));

    // crater 1: {visibleRadius:5, x:524, y:2, threat:high, influenceRadius:7.5}
    QCOMPARE(obs.craters[1].visibleRadius, 5.0);
    QCOMPARE(obs.craters[1].x, 524);
    QCOMPARE(obs.craters[1].y, 2);
    QCOMPARE(obs.craters[1].threat, Core::MOS::MosThreatLevel::High);
    QVERIFY(fuzzyEq(obs.craters[1].influenceRadius, 7.5));

    // uxo 0: {syntheticYield:34.989786157384515, x:2596, y:-2, threat:medium, influenceRadius:4.906122130897645}
    QVERIFY(fuzzyEq(obs.uxo[0].syntheticYield, 34.989786157384515));
    QCOMPARE(obs.uxo[0].x, 2596);
    QCOMPARE(obs.uxo[0].y, -2);
    QCOMPARE(obs.uxo[0].threat, Core::MOS::MosThreatLevel::Medium);
    QVERIFY(fuzzyEq(obs.uxo[0].influenceRadius, 4.906122130897645));

    // uxo 1: {syntheticYield:45.28235333971679, x:2237, y:-15, threat:medium, influenceRadius:5.346475595556224}
    QVERIFY(fuzzyEq(obs.uxo[1].syntheticYield, 45.28235333971679));
    QCOMPARE(obs.uxo[1].x, 2237);
    QCOMPARE(obs.uxo[1].y, -15);
    QCOMPARE(obs.uxo[1].threat, Core::MOS::MosThreatLevel::Medium);
    QVERIFY(fuzzyEq(obs.uxo[1].influenceRadius, 5.346475595556224));
}

void MosFixtureTest::seedNegative1MatchesOracle()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    // -1 归一化为 uint32 4294967295
    const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, -1);

    QCOMPARE(obs.craters.size(), 2);
    QCOMPARE(obs.uxo.size(), 2);

    // crater 0: {visibleRadius:6, x:568, y:17, threat:high, influenceRadius:9}
    QCOMPARE(obs.craters[0].visibleRadius, 6.0);
    QCOMPARE(obs.craters[0].x, 568);
    QCOMPARE(obs.craters[0].y, 17);
    QCOMPARE(obs.craters[0].threat, Core::MOS::MosThreatLevel::High);
    QVERIFY(fuzzyEq(obs.craters[0].influenceRadius, 9.0));

    // crater 1: {visibleRadius:6, x:2536, y:3, threat:medium, influenceRadius:9}
    QCOMPARE(obs.craters[1].visibleRadius, 6.0);
    QCOMPARE(obs.craters[1].x, 2536);
    QCOMPARE(obs.craters[1].y, 3);
    QCOMPARE(obs.craters[1].threat, Core::MOS::MosThreatLevel::Medium);
    QVERIFY(fuzzyEq(obs.craters[1].influenceRadius, 9.0));

    // uxo 0: {syntheticYield:29.02288385666907, x:408, y:39, threat:high, influenceRadius:4.609687100133543}
    QVERIFY(fuzzyEq(obs.uxo[0].syntheticYield, 29.02288385666907));
    QCOMPARE(obs.uxo[0].x, 408);
    QCOMPARE(obs.uxo[0].y, 39);
    QCOMPARE(obs.uxo[0].threat, Core::MOS::MosThreatLevel::High);
    QVERIFY(fuzzyEq(obs.uxo[0].influenceRadius, 4.609687100133543));

    // uxo 1: {syntheticYield:17.573938351124525, x:1031, y:19, threat:medium, influenceRadius:3.8998475052151935}
    QVERIFY(fuzzyEq(obs.uxo[1].syntheticYield, 17.573938351124525));
    QCOMPARE(obs.uxo[1].x, 1031);
    QCOMPARE(obs.uxo[1].y, 19);
    QCOMPARE(obs.uxo[1].threat, Core::MOS::MosThreatLevel::Medium);
    QVERIFY(fuzzyEq(obs.uxo[1].influenceRadius, 3.8998475052151935));
}

void MosFixtureTest::seedInt32MinMatchesOracle()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    // -2147483648 (INT32_MIN) 归一化为 uint32 2147483648
    const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, -2147483647 - 1);

    QCOMPARE(obs.craters.size(), 2);
    QCOMPARE(obs.uxo.size(), 2);

    // crater 0: {visibleRadius:5, x:1344, y:23, threat:high, influenceRadius:7.5}
    QCOMPARE(obs.craters[0].visibleRadius, 5.0);
    QCOMPARE(obs.craters[0].x, 1344);
    QCOMPARE(obs.craters[0].y, 23);
    QCOMPARE(obs.craters[0].threat, Core::MOS::MosThreatLevel::High);
    QVERIFY(fuzzyEq(obs.craters[0].influenceRadius, 7.5));

    // crater 1: {visibleRadius:5, x:2516, y:-6, threat:medium, influenceRadius:7.5}
    QCOMPARE(obs.craters[1].visibleRadius, 5.0);
    QCOMPARE(obs.craters[1].x, 2516);
    QCOMPARE(obs.craters[1].y, -6);
    QCOMPARE(obs.craters[1].threat, Core::MOS::MosThreatLevel::Medium);
    QVERIFY(fuzzyEq(obs.craters[1].influenceRadius, 7.5));

    // uxo 0: {syntheticYield:47.30770705267787, x:2024, y:-7, threat:high, influenceRadius:5.425026850330843}
    QVERIFY(fuzzyEq(obs.uxo[0].syntheticYield, 47.30770705267787));
    QCOMPARE(obs.uxo[0].x, 2024);
    QCOMPARE(obs.uxo[0].y, -7);
    QCOMPARE(obs.uxo[0].threat, Core::MOS::MosThreatLevel::High);
    QVERIFY(fuzzyEq(obs.uxo[0].influenceRadius, 5.425026850330843));

    // uxo 1: {syntheticYield:30.041590174660087, x:1021, y:37, threat:medium, influenceRadius:4.663001603423398}
    QVERIFY(fuzzyEq(obs.uxo[1].syntheticYield, 30.041590174660087));
    QCOMPARE(obs.uxo[1].x, 1021);
    QCOMPARE(obs.uxo[1].y, 37);
    QCOMPARE(obs.uxo[1].threat, Core::MOS::MosThreatLevel::Medium);
    QVERIFY(fuzzyEq(obs.uxo[1].influenceRadius, 4.663001603423398));
}

void MosFixtureTest::seedInt32MaxMatchesOracle()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    // 2147483647 (INT32_MAX) 归一化为 uint32 2147483647
    const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, 2147483647);

    QCOMPARE(obs.craters.size(), 2);
    QCOMPARE(obs.uxo.size(), 2);

    // crater 0: {visibleRadius:4, x:381, y:-9, threat:high, influenceRadius:6}
    QCOMPARE(obs.craters[0].visibleRadius, 4.0);
    QCOMPARE(obs.craters[0].x, 381);
    QCOMPARE(obs.craters[0].y, -9);
    QCOMPARE(obs.craters[0].threat, Core::MOS::MosThreatLevel::High);
    QVERIFY(fuzzyEq(obs.craters[0].influenceRadius, 6.0));

    // crater 1: {visibleRadius:4, x:359, y:36, threat:medium, influenceRadius:6}
    QCOMPARE(obs.craters[1].visibleRadius, 4.0);
    QCOMPARE(obs.craters[1].x, 359);
    QCOMPARE(obs.craters[1].y, 36);
    QCOMPARE(obs.craters[1].threat, Core::MOS::MosThreatLevel::Medium);
    QVERIFY(fuzzyEq(obs.craters[1].influenceRadius, 6.0));

    // uxo 0: {syntheticYield:49.83122006058693, x:1073, y:-17, threat:high, influenceRadius:5.519822332293598}
    QVERIFY(fuzzyEq(obs.uxo[0].syntheticYield, 49.83122006058693));
    QCOMPARE(obs.uxo[0].x, 1073);
    QCOMPARE(obs.uxo[0].y, -17);
    QCOMPARE(obs.uxo[0].threat, Core::MOS::MosThreatLevel::High);
    QVERIFY(fuzzyEq(obs.uxo[0].influenceRadius, 5.519822332293598));

    // uxo 1: {syntheticYield:46.80975499562919, x:2141, y:-27, threat:medium, influenceRadius:5.405925401394857}
    QVERIFY(fuzzyEq(obs.uxo[1].syntheticYield, 46.80975499562919));
    QCOMPARE(obs.uxo[1].x, 2141);
    QCOMPARE(obs.uxo[1].y, -27);
    QCOMPARE(obs.uxo[1].threat, Core::MOS::MosThreatLevel::Medium);
    QVERIFY(fuzzyEq(obs.uxo[1].influenceRadius, 5.405925401394857));
}

// === 确定性 ===
void MosFixtureTest::sameSeedProducesIdenticalBytes()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    const auto a = Core::MOS::MosFixtureGenerator::generate(p, g, 42);
    const auto b = Core::MOS::MosFixtureGenerator::generate(p, g, 42);
    const auto bytesA = Core::MOS::serializeObstacleSetBytes(a);
    const auto bytesB = Core::MOS::serializeObstacleSetBytes(b);
    QCOMPARE(bytesA, bytesB);
}

void MosFixtureTest::differentSeedsProduceDifferentBytes()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    const auto a = Core::MOS::MosFixtureGenerator::generate(p, g, 0);
    const auto b = Core::MOS::MosFixtureGenerator::generate(p, g, 42);
    const auto bytesA = Core::MOS::serializeObstacleSetBytes(a);
    const auto bytesB = Core::MOS::serializeObstacleSetBytes(b);
    QVERIFY(bytesA != bytesB);
}

// === 计数与抽取顺序 ===
void MosFixtureTest::generatorProducesExpectedCounts()
{
    const auto p = defaultParams();
    auto g = defaultGenParams();
    g.craterCount = 4;
    g.uxoCount = 3;
    const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, 7);
    QCOMPARE(obs.craters.size(), 4);
    QCOMPARE(obs.uxo.size(), 3);
}

void MosFixtureTest::firstCraterThreatIsFixedHigh()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    // 多个种子下首个弹坑 threat 必须固定为 high（不消耗 rng）
    for (qint32 seed : {0, 42, -1, -2147483647 - 1, 2147483647}) {
        const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, seed);
        QCOMPARE(obs.craters[0].threat, Core::MOS::MosThreatLevel::High);
    }
}

// === 公式正确性 ===
void MosFixtureTest::craterInfluenceRadiusFormula()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    // influenceRadius = visibleRadius * expand（float，不取整）
    for (qint32 seed : {0, 42, -1, -2147483647 - 1, 2147483647}) {
        const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, seed);
        for (const auto &c : obs.craters) {
            QVERIFY(fuzzyEq(c.influenceRadius, c.visibleRadius * p.expand));
        }
    }
}

void MosFixtureTest::uxoInfluenceRadiusFormula()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    // influenceRadius = K * cbrt(syntheticYield)（float，不取整）
    for (qint32 seed : {0, 42, -1, -2147483647 - 1, 2147483647}) {
        const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, seed);
        for (const auto &u : obs.uxo) {
            QVERIFY(fuzzyEq(u.influenceRadius, p.K * std::cbrt(u.syntheticYield)));
        }
    }
}

void MosFixtureTest::coordinateRangesRespected()
{
    const auto p = defaultParams();
    const auto g = defaultGenParams();
    // x ∈ [0, L], y ∈ [-40, 40]
    for (qint32 seed : {0, 42, -1, -2147483647 - 1, 2147483647}) {
        const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, seed);
        for (const auto &c : obs.craters) {
            QVERIFY2(c.x >= 0, "crater x below 0");
            QVERIFY2(c.x <= p.L, "crater x above L");
            QVERIFY2(c.y >= -40, "crater y below -40");
            QVERIFY2(c.y <= 40, "crater y above 40");
        }
        for (const auto &u : obs.uxo) {
            QVERIFY2(u.x >= 0, "uxo x below 0");
            QVERIFY2(u.x <= p.L, "uxo x above L");
            QVERIFY2(u.y >= -40, "uxo y below -40");
            QVERIFY2(u.y <= 40, "uxo y above 40");
        }
    }
}

// === canonical JSON 字段与值（Qt5 QJsonObject 按键排序，不保证插入顺序）===
void MosFixtureTest::serializedCraterHasCanonicalFieldOrder()
{
    Core::MOS::MosCrater c;
    c.id = QStringLiteral("test");
    c.visibleRadius = 5.0;
    c.x = 100;
    c.y = -50;
    c.threat = Core::MOS::MosThreatLevel::High;
    c.influenceRadius = 7.5;

    const QJsonObject obj = Core::MOS::serializeCrater(c);
    // 验证 5 个 canonical 字段全部存在且值正确（id 不含在 canonical 序列化中）
    QCOMPARE(obj.size(), 5);
    QVERIFY(obj.contains(QStringLiteral("visibleRadius")));
    QVERIFY(obj.contains(QStringLiteral("x")));
    QVERIFY(obj.contains(QStringLiteral("y")));
    QVERIFY(obj.contains(QStringLiteral("threat")));
    QVERIFY(obj.contains(QStringLiteral("influenceRadius")));
    QVERIFY(!obj.contains(QStringLiteral("id")));
    QCOMPARE(obj.value(QStringLiteral("visibleRadius")).toDouble(), 5.0);
    QCOMPARE(obj.value(QStringLiteral("x")).toInt(), 100);
    QCOMPARE(obj.value(QStringLiteral("y")).toInt(), -50);
    QCOMPARE(obj.value(QStringLiteral("threat")).toString(), QStringLiteral("high"));
    QCOMPARE(obj.value(QStringLiteral("influenceRadius")).toDouble(), 7.5);
}

void MosFixtureTest::serializedUxoHasCanonicalFieldOrder()
{
    Core::MOS::MosUxo u;
    u.id = QStringLiteral("test");
    u.syntheticYield = 30.0;
    u.x = 100;
    u.y = -50;
    u.threat = Core::MOS::MosThreatLevel::Medium;
    u.influenceRadius = 1.5 * std::cbrt(30.0);

    const QJsonObject obj = Core::MOS::serializeUxo(u);
    // 验证 5 个 canonical 字段全部存在且值正确（id 不含在 canonical 序列化中）
    QCOMPARE(obj.size(), 5);
    QVERIFY(obj.contains(QStringLiteral("syntheticYield")));
    QVERIFY(obj.contains(QStringLiteral("x")));
    QVERIFY(obj.contains(QStringLiteral("y")));
    QVERIFY(obj.contains(QStringLiteral("threat")));
    QVERIFY(obj.contains(QStringLiteral("influenceRadius")));
    QVERIFY(!obj.contains(QStringLiteral("id")));
    QVERIFY(fuzzyEq(obj.value(QStringLiteral("syntheticYield")).toDouble(), 30.0));
    QCOMPARE(obj.value(QStringLiteral("x")).toInt(), 100);
    QCOMPARE(obj.value(QStringLiteral("y")).toInt(), -50);
    QCOMPARE(obj.value(QStringLiteral("threat")).toString(), QStringLiteral("medium"));
    QVERIFY(fuzzyEq(obj.value(QStringLiteral("influenceRadius")).toDouble(), 1.5 * std::cbrt(30.0)));
}

// === 嵌套 fixture 顺序 ===
void MosFixtureTest::nestedFixtureOrderT5N10()
{
    // floor(tierIndex * N / (T-1))，T=5, N=10 -> [0, 2, 5, 7, 10]
    const auto order = Core::MOS::MosFixtureGenerator::nestedFixtureOrder(5, 10);
    QCOMPARE(order.size(), 5);
    QCOMPARE(order[0], 0);
    QCOMPARE(order[1], 2);
    QCOMPARE(order[2], 5);
    QCOMPARE(order[3], 7);
    QCOMPARE(order[4], 10);
}

void MosFixtureTest::nestedFixtureOrderT1()
{
    // T=1 时返回 [0]
    const auto order = Core::MOS::MosFixtureGenerator::nestedFixtureOrder(1, 10);
    QCOMPARE(order.size(), 1);
    QCOMPARE(order[0], 0);
}

void MosFixtureTest::nestedFixtureOrderT0()
{
    // T<1 时返回空
    const auto order = Core::MOS::MosFixtureGenerator::nestedFixtureOrder(0, 10);
    QVERIFY(order.isEmpty());
}

// === 估算器 ===
void MosFixtureTest::sphereVolumeFormula()
{
    // 4/3 · π · r³
    QVERIFY(fuzzyEq(Core::MOS::MosEstimator::sphereVolume(0.0), 0.0));
    QVERIFY(fuzzyEq(Core::MOS::MosEstimator::sphereVolume(1.0), 4.0 / 3.0 * M_PI));
    QVERIFY(fuzzyEq(Core::MOS::MosEstimator::sphereVolume(2.0), 4.0 / 3.0 * M_PI * 8.0));
}

void MosFixtureTest::estimateBackfillAndUxoHours()
{
    Core::MOS::MosObstacleSet obs;
    Core::MOS::MosCrater c;
    c.visibleRadius = 1.0; // volume = 4/3·π
    obs.craters.append(c);

    Core::MOS::MosUxo u;
    obs.uxo.append(u);
    obs.uxo.append(u); // uxoCount = 2

    auto p = defaultParams();
    p.backfill = 50.0;
    p.uxoHours = 8.0;

    const auto est = Core::MOS::MosEstimator::estimate(obs, p, 0, 3);
    const double expectedVolume = 4.0 / 3.0 * M_PI;
    QVERIFY(fuzzyEq(est.totalBackfillVolume, expectedVolume));
    QVERIFY(fuzzyEq(est.backfillHours, expectedVolume / 50.0));
    QVERIFY(fuzzyEq(est.uxoHours, 2 * 8.0));
    QVERIFY(fuzzyEq(est.totalHours, expectedVolume / 50.0 + 16.0));
}

void MosFixtureTest::estimateDifficultyForTier()
{
    // T=5: tier 0=无, tier 1=中等, tier 2=中等, tier 3=中等, tier 4=高
    Core::MOS::MosObstacleSet obs;
    auto p = defaultParams();

    QCOMPARE(Core::MOS::MosEstimator::estimate(obs, p, 0, 5).difficulty, Core::MOS::MosDifficulty::None);
    QCOMPARE(Core::MOS::MosEstimator::estimate(obs, p, 1, 5).difficulty, Core::MOS::MosDifficulty::Medium);
    QCOMPARE(Core::MOS::MosEstimator::estimate(obs, p, 2, 5).difficulty, Core::MOS::MosDifficulty::Medium);
    QCOMPARE(Core::MOS::MosEstimator::estimate(obs, p, 3, 5).difficulty, Core::MOS::MosDifficulty::Medium);
    QCOMPARE(Core::MOS::MosEstimator::estimate(obs, p, 4, 5).difficulty, Core::MOS::MosDifficulty::High);

    // T=3: tier 0=无, tier 1=中等, tier 2=高
    QCOMPARE(Core::MOS::MosEstimator::estimate(obs, p, 0, 3).difficulty, Core::MOS::MosDifficulty::None);
    QCOMPARE(Core::MOS::MosEstimator::estimate(obs, p, 1, 3).difficulty, Core::MOS::MosDifficulty::Medium);
    QCOMPARE(Core::MOS::MosEstimator::estimate(obs, p, 2, 3).difficulty, Core::MOS::MosDifficulty::High);

    // T=1: tier 0=无
    QCOMPARE(Core::MOS::MosEstimator::estimate(obs, p, 0, 1).difficulty, Core::MOS::MosDifficulty::None);
}

QTEST_APPLESS_MAIN(MosFixtureTest)

#include "mos_fixture_test.moc"
