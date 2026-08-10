// MOS P0 输入包络校验测试：覆盖每条包络规则的合法/非法/边界用例，并验证拒绝时输入不变。
// 本测试在实现前编写，期望先失败再通过（failing-first TDD）。

#include "Core/MOS/MosTypes.h"
#include "Core/MOS/MosValidation.h"

#include <QtTest>
#include <cmath>
#include <limits>

namespace {

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

// 默认合法生成器参数（合同常量）
Core::MOS::MosGeneratorParams defaultGeneratorParams()
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

// 默认合法障碍物集合（2 弹坑 + 2 UXO，N=4 <= 13）
Core::MOS::MosObstacleSet defaultObstacleSet()
{
    Core::MOS::MosObstacleSet o;
    Core::MOS::MosCrater c1;
    c1.id = QStringLiteral("crater-1");
    c1.visibleRadius = 5.0;
    c1.x = 1000;
    c1.y = 0;
    c1.threat = Core::MOS::MosThreatLevel::High;
    c1.influenceRadius = 7.5;
    o.craters.append(c1);

    Core::MOS::MosCrater c2;
    c2.id = QStringLiteral("crater-2");
    c2.visibleRadius = 4.0;
    c2.x = 2000;
    c2.y = 10;
    c2.threat = Core::MOS::MosThreatLevel::Medium;
    c2.influenceRadius = 6.0;
    o.craters.append(c2);

    Core::MOS::MosUxo u1;
    u1.id = QStringLiteral("uxo-1");
    u1.syntheticYield = 30.0;
    u1.x = 500;
    u1.y = -20;
    u1.threat = Core::MOS::MosThreatLevel::High;
    u1.influenceRadius = 1.5 * std::cbrt(30.0);
    o.uxo.append(u1);

    Core::MOS::MosUxo u2;
    u2.id = QStringLiteral("uxo-2");
    u2.syntheticYield = 20.0;
    u2.x = 2500;
    u2.y = 30;
    u2.threat = Core::MOS::MosThreatLevel::Medium;
    u2.influenceRadius = 1.5 * std::cbrt(20.0);
    o.uxo.append(u2);

    return o;
}

} // namespace

class MosValidationTest : public QObject
{
    Q_OBJECT

private slots:
    // === 跑道参数：合法基线 ===
    void runwayParamsDefaultIsValid();
    // === L: 100..6000 ===
    void runwayLBoundaryLowIsValid();
    void runwayLBoundaryHighIsValid();
    void runwayLBelowLowIsRejected();
    void runwayLAboveHighIsRejected();
    // === W: 15..100 ===
    void runwayWBoundaryLowIsValid();
    void runwayWBoundaryHighIsValid();
    void runwayWBelowLowIsRejected();
    void runwayWAboveHighIsRejected();
    // === minLength: 1..L ===
    void minLengthBoundaryLowIsValid();
    void minLengthBoundaryHighIsValid();
    void minLengthBelowLowIsRejected();
    void minLengthAboveLIsRejected();
    // === minWidth: 1..W ===
    void minWidthBoundaryLowIsValid();
    void minWidthBoundaryHighIsValid();
    void minWidthBelowLowIsRejected();
    void minWidthAboveWIsRejected();
    // === step: 0.5..5 且 W/step 整数 且 S<=200 ===
    void stepBoundaryLowIsValid();
    void stepBoundaryHighIsValid();
    void stepBelowLowIsRejected();
    void stepAboveHighIsRejected();
    void stepNonIntegralSIsRejected();
    void stepSCountBoundaryIsValid();
    // === K: 0.1..10 ===
    void KBoundaryLowIsValid();
    void KBoundaryHighIsValid();
    void KBelowLowIsRejected();
    void KAboveHighIsRejected();
    // === expand: 0.1..10 ===
    void expandBoundaryLowIsValid();
    void expandBoundaryHighIsValid();
    void expandBelowLowIsRejected();
    void expandAboveHighIsRejected();
    // === backfill: >0 且 <=10000 ===
    void backfillBoundaryLowIsValid();
    void backfillBoundaryHighIsValid();
    void backfillZeroIsRejected();
    void backfillNegativeIsRejected();
    void backfillAboveHighIsRejected();
    // === uxoHours: 0..1000 ===
    void uxoHoursBoundaryLowIsValid();
    void uxoHoursBoundaryHighIsValid();
    void uxoHoursBelowLowIsRejected();
    void uxoHoursAboveHighIsRejected();
    // === tiers: 2..5 ===
    void tiersBoundaryLowIsValid();
    void tiersBoundaryHighIsValid();
    void tiersBelowLowIsRejected();
    void tiersAboveHighIsRejected();
    // === 非有限值 ===
    void nanInRunwayParamsIsRejected();
    void infinityInRunwayParamsIsRejected();
    // === 生成器参数：合法基线 ===
    void generatorParamsDefaultIsValid();
    // === craterCount: 1..8 ===
    void craterCountBoundaryLowIsValid();
    void craterCountBoundaryHighIsValid();
    void craterCountBelowLowIsRejected();
    void craterCountAboveHighIsRejected();
    // === craterRMin/craterRMax: 1..100 且 min<=max ===
    void craterRadiusBoundaryLowIsValid();
    void craterRadiusBoundaryHighIsValid();
    void craterRadiusMinAboveMaxIsRejected();
    // === uxoCount: 0..5 ===
    void uxoCountBoundaryLowIsValid();
    void uxoCountBoundaryHighIsValid();
    void uxoCountBelowLowIsRejected();
    void uxoCountAboveHighIsRejected();
    // === uxoYMin/uxoYMax: 0.1..1000 且 min<=max ===
    void uxoYieldBoundaryLowIsValid();
    void uxoYieldBoundaryHighIsValid();
    void uxoYieldMinAboveMaxIsRejected();
    // === 障碍物集合：合法基线 ===
    void obstacleSetDefaultIsValid();
    // === visibleRadius: 0.1..100 ===
    void craterVisibleRadiusBoundaryLowIsValid();
    void craterVisibleRadiusBoundaryHighIsValid();
    void craterVisibleRadiusBelowLowIsRejected();
    void craterVisibleRadiusAboveHighIsRejected();
    // === syntheticYield: 0.1..1000 ===
    void uxoSyntheticYieldBoundaryLowIsValid();
    void uxoSyntheticYieldBoundaryHighIsValid();
    void uxoSyntheticYieldBelowLowIsRejected();
    void uxoSyntheticYieldAboveHighIsRejected();
    // === influenceRadius: 有限 0.1..6000 ===
    void influenceRadiusBoundaryLowIsValid();
    void influenceRadiusBoundaryHighIsValid();
    void influenceRadiusBelowLowIsRejected();
    void influenceRadiusAboveHighIsRejected();
    void influenceRadiusNonFiniteIsRejected();
    // === 总障碍物 N <= 13 ===
    void totalObstaclesBoundaryIsValid();
    void totalObstaclesExceeds13IsRejected();
    // === 唯一非空 ID ===
    void emptyObstacleIdIsRejected();
    void duplicateObstacleIdIsRejected();
    // === 种子：signed int32 ===
    void seedBoundaryLowIsValid();
    void seedBoundaryHighIsValid();
    void seedBelowInt32MinIsRejected();
    void seedAboveInt32MaxIsRejected();
    // === 拒绝时输入不变 ===
    void rejectedRunwayParamsLeavesInputUnchanged();
    void rejectedObstacleSetLeavesInputUnchanged();
};

// === 跑道参数：合法基线 ===
void MosValidationTest::runwayParamsDefaultIsValid()
{
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Valid);
}

// === L: 100..6000 ===
void MosValidationTest::runwayLBoundaryLowIsValid()
{
    auto params = defaultRunwayParams();
    params.L = 100.0;
    params.minLength = 100.0; // minLength <= L
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::runwayLBoundaryHighIsValid()
{
    auto params = defaultRunwayParams();
    params.L = 6000.0;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::runwayLBelowLowIsRejected()
{
    auto params = defaultRunwayParams();
    params.L = 99.9;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::L);
}

void MosValidationTest::runwayLAboveHighIsRejected()
{
    auto params = defaultRunwayParams();
    params.L = 6000.1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::L);
}

// === W: 15..100 ===
void MosValidationTest::runwayWBoundaryLowIsValid()
{
    auto params = defaultRunwayParams();
    params.W = 15.0;
    params.minWidth = 15.0; // minWidth <= W
    params.step = 0.5;      // W/step = 30 整数
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::runwayWBoundaryHighIsValid()
{
    auto params = defaultRunwayParams();
    params.W = 100.0;
    params.step = 1.0; // W/step = 100 整数
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::runwayWBelowLowIsRejected()
{
    auto params = defaultRunwayParams();
    params.W = 14.9;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::W);
}

void MosValidationTest::runwayWAboveHighIsRejected()
{
    auto params = defaultRunwayParams();
    params.W = 100.1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::W);
}

// === minLength: 1..L ===
void MosValidationTest::minLengthBoundaryLowIsValid()
{
    auto params = defaultRunwayParams();
    params.minLength = 1.0;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::minLengthBoundaryHighIsValid()
{
    auto params = defaultRunwayParams();
    params.minLength = params.L;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::minLengthBelowLowIsRejected()
{
    auto params = defaultRunwayParams();
    params.minLength = 0.9;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::MinLength);
}

void MosValidationTest::minLengthAboveLIsRejected()
{
    auto params = defaultRunwayParams();
    params.minLength = params.L + 0.1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::MinLength);
}

// === minWidth: 1..W ===
void MosValidationTest::minWidthBoundaryLowIsValid()
{
    auto params = defaultRunwayParams();
    params.minWidth = 1.0;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::minWidthBoundaryHighIsValid()
{
    auto params = defaultRunwayParams();
    params.W = 50.0;
    params.minWidth = 50.0;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::minWidthBelowLowIsRejected()
{
    auto params = defaultRunwayParams();
    params.minWidth = 0.9;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::MinWidth);
}

void MosValidationTest::minWidthAboveWIsRejected()
{
    auto params = defaultRunwayParams();
    params.minWidth = params.W + 0.1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::MinWidth);
}

// === step: 0.5..5 且 W/step 整数 且 S<=200 ===
void MosValidationTest::stepBoundaryLowIsValid()
{
    auto params = defaultRunwayParams();
    params.W = 50.0;
    params.step = 0.5; // W/step = 100 整数, S=100<=200
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::stepBoundaryHighIsValid()
{
    auto params = defaultRunwayParams();
    params.W = 50.0;
    params.step = 5.0; // W/step = 10 整数
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::stepBelowLowIsRejected()
{
    auto params = defaultRunwayParams();
    params.step = 0.4;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Step);
}

void MosValidationTest::stepAboveHighIsRejected()
{
    auto params = defaultRunwayParams();
    params.step = 5.1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Step);
}

void MosValidationTest::stepNonIntegralSIsRejected()
{
    auto params = defaultRunwayParams();
    params.W = 50.0;
    params.step = 3.0; // W/step = 16.666... 非整数
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::StepIntegral);
}

void MosValidationTest::stepSCountBoundaryIsValid()
{
    auto params = defaultRunwayParams();
    params.W = 100.0;
    params.step = 0.5; // W/step = 200 边界合法
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);

    // S=201 越界：W=100.5, step=0.5 -> S=201
    params.W = 100.5;
    params.minWidth = 1.0; // minWidth <= W
    params.step = 0.5;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::StepSCount);
}

// === K: 0.1..10 ===
void MosValidationTest::KBoundaryLowIsValid()
{
    auto params = defaultRunwayParams();
    params.K = 0.1;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::KBoundaryHighIsValid()
{
    auto params = defaultRunwayParams();
    params.K = 10.0;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::KBelowLowIsRejected()
{
    auto params = defaultRunwayParams();
    params.K = 0.09;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::K);
}

void MosValidationTest::KAboveHighIsRejected()
{
    auto params = defaultRunwayParams();
    params.K = 10.1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::K);
}

// === expand: 0.1..10 ===
void MosValidationTest::expandBoundaryLowIsValid()
{
    auto params = defaultRunwayParams();
    params.expand = 0.1;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::expandBoundaryHighIsValid()
{
    auto params = defaultRunwayParams();
    params.expand = 10.0;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::expandBelowLowIsRejected()
{
    auto params = defaultRunwayParams();
    params.expand = 0.09;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Expand);
}

void MosValidationTest::expandAboveHighIsRejected()
{
    auto params = defaultRunwayParams();
    params.expand = 10.1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Expand);
}

// === backfill: >0 且 <=10000 ===
void MosValidationTest::backfillBoundaryLowIsValid()
{
    auto params = defaultRunwayParams();
    params.backfill = 0.001; // >0
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::backfillBoundaryHighIsValid()
{
    auto params = defaultRunwayParams();
    params.backfill = 10000.0;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::backfillZeroIsRejected()
{
    auto params = defaultRunwayParams();
    params.backfill = 0.0;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Backfill);
}

void MosValidationTest::backfillNegativeIsRejected()
{
    auto params = defaultRunwayParams();
    params.backfill = -1.0;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Backfill);
}

void MosValidationTest::backfillAboveHighIsRejected()
{
    auto params = defaultRunwayParams();
    params.backfill = 10000.1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Backfill);
}

// === uxoHours: 0..1000 ===
void MosValidationTest::uxoHoursBoundaryLowIsValid()
{
    auto params = defaultRunwayParams();
    params.uxoHours = 0.0;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::uxoHoursBoundaryHighIsValid()
{
    auto params = defaultRunwayParams();
    params.uxoHours = 1000.0;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::uxoHoursBelowLowIsRejected()
{
    auto params = defaultRunwayParams();
    params.uxoHours = -0.1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::UxoHours);
}

void MosValidationTest::uxoHoursAboveHighIsRejected()
{
    auto params = defaultRunwayParams();
    params.uxoHours = 1000.1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::UxoHours);
}

// === tiers: 2..5 ===
void MosValidationTest::tiersBoundaryLowIsValid()
{
    auto params = defaultRunwayParams();
    params.tiers = 2;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::tiersBoundaryHighIsValid()
{
    auto params = defaultRunwayParams();
    params.tiers = 5;
    QVERIFY(Core::MOS::validateRunwayParams(params).valid);
}

void MosValidationTest::tiersBelowLowIsRejected()
{
    auto params = defaultRunwayParams();
    params.tiers = 1;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Tiers);
}

void MosValidationTest::tiersAboveHighIsRejected()
{
    auto params = defaultRunwayParams();
    params.tiers = 6;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Tiers);
}

// === 非有限值 ===
void MosValidationTest::nanInRunwayParamsIsRejected()
{
    auto params = defaultRunwayParams();
    params.L = std::numeric_limits<double>::quiet_NaN();
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::InvalidFinite);
}

void MosValidationTest::infinityInRunwayParamsIsRejected()
{
    auto params = defaultRunwayParams();
    params.W = std::numeric_limits<double>::infinity();
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::InvalidFinite);
}

// === 生成器参数：合法基线 ===
void MosValidationTest::generatorParamsDefaultIsValid()
{
    auto params = defaultGeneratorParams();
    const auto result = Core::MOS::validateGeneratorParams(params);
    QVERIFY(result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Valid);
}

// === craterCount: 1..8 ===
void MosValidationTest::craterCountBoundaryLowIsValid()
{
    auto params = defaultGeneratorParams();
    params.craterCount = 1;
    QVERIFY(Core::MOS::validateGeneratorParams(params).valid);
}

void MosValidationTest::craterCountBoundaryHighIsValid()
{
    auto params = defaultGeneratorParams();
    params.craterCount = 8;
    QVERIFY(Core::MOS::validateGeneratorParams(params).valid);
}

void MosValidationTest::craterCountBelowLowIsRejected()
{
    auto params = defaultGeneratorParams();
    params.craterCount = 0;
    const auto result = Core::MOS::validateGeneratorParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::CraterCount);
}

void MosValidationTest::craterCountAboveHighIsRejected()
{
    auto params = defaultGeneratorParams();
    params.craterCount = 9;
    const auto result = Core::MOS::validateGeneratorParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::CraterCount);
}

// === craterRMin/craterRMax: 1..100 且 min<=max ===
void MosValidationTest::craterRadiusBoundaryLowIsValid()
{
    auto params = defaultGeneratorParams();
    params.craterRMin = 1;
    QVERIFY(Core::MOS::validateGeneratorParams(params).valid);
}

void MosValidationTest::craterRadiusBoundaryHighIsValid()
{
    auto params = defaultGeneratorParams();
    params.craterRMax = 100;
    QVERIFY(Core::MOS::validateGeneratorParams(params).valid);
}

void MosValidationTest::craterRadiusMinAboveMaxIsRejected()
{
    auto params = defaultGeneratorParams();
    params.craterRMin = 7;
    params.craterRMax = 6;
    const auto result = Core::MOS::validateGeneratorParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::CraterRadius);
}

// === uxoCount: 0..5 ===
void MosValidationTest::uxoCountBoundaryLowIsValid()
{
    auto params = defaultGeneratorParams();
    params.uxoCount = 0;
    QVERIFY(Core::MOS::validateGeneratorParams(params).valid);
}

void MosValidationTest::uxoCountBoundaryHighIsValid()
{
    auto params = defaultGeneratorParams();
    params.uxoCount = 5;
    QVERIFY(Core::MOS::validateGeneratorParams(params).valid);
}

void MosValidationTest::uxoCountBelowLowIsRejected()
{
    auto params = defaultGeneratorParams();
    params.uxoCount = -1;
    const auto result = Core::MOS::validateGeneratorParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::UxoCount);
}

void MosValidationTest::uxoCountAboveHighIsRejected()
{
    auto params = defaultGeneratorParams();
    params.uxoCount = 6;
    const auto result = Core::MOS::validateGeneratorParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::UxoCount);
}

// === uxoYMin/uxoYMax: 0.1..1000 且 min<=max ===
void MosValidationTest::uxoYieldBoundaryLowIsValid()
{
    auto params = defaultGeneratorParams();
    params.uxoYMin = 0.1;
    QVERIFY(Core::MOS::validateGeneratorParams(params).valid);
}

void MosValidationTest::uxoYieldBoundaryHighIsValid()
{
    auto params = defaultGeneratorParams();
    params.uxoYMax = 1000.0;
    QVERIFY(Core::MOS::validateGeneratorParams(params).valid);
}

void MosValidationTest::uxoYieldMinAboveMaxIsRejected()
{
    auto params = defaultGeneratorParams();
    params.uxoYMin = 60.0;
    params.uxoYMax = 50.0;
    const auto result = Core::MOS::validateGeneratorParams(params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::UxoYield);
}

// === 障碍物集合：合法基线 ===
void MosValidationTest::obstacleSetDefaultIsValid()
{
    auto obstacles = defaultObstacleSet();
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Valid);
}

// === visibleRadius: 0.1..100 ===
void MosValidationTest::craterVisibleRadiusBoundaryLowIsValid()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].visibleRadius = 0.1;
    obstacles.craters[0].influenceRadius = 0.1 * 1.5;
    auto params = defaultRunwayParams();
    QVERIFY(Core::MOS::validateObstacleSet(obstacles, params).valid);
}

void MosValidationTest::craterVisibleRadiusBoundaryHighIsValid()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].visibleRadius = 100.0;
    obstacles.craters[0].influenceRadius = 100.0 * 1.5;
    auto params = defaultRunwayParams();
    QVERIFY(Core::MOS::validateObstacleSet(obstacles, params).valid);
}

void MosValidationTest::craterVisibleRadiusBelowLowIsRejected()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].visibleRadius = 0.09;
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::CraterRadius);
}

void MosValidationTest::craterVisibleRadiusAboveHighIsRejected()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].visibleRadius = 100.1;
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::CraterRadius);
}

// === syntheticYield: 0.1..1000 ===
void MosValidationTest::uxoSyntheticYieldBoundaryLowIsValid()
{
    auto obstacles = defaultObstacleSet();
    obstacles.uxo[0].syntheticYield = 0.1;
    obstacles.uxo[0].influenceRadius = 1.5 * std::cbrt(0.1);
    auto params = defaultRunwayParams();
    QVERIFY(Core::MOS::validateObstacleSet(obstacles, params).valid);
}

void MosValidationTest::uxoSyntheticYieldBoundaryHighIsValid()
{
    auto obstacles = defaultObstacleSet();
    obstacles.uxo[0].syntheticYield = 1000.0;
    obstacles.uxo[0].influenceRadius = 1.5 * std::cbrt(1000.0);
    auto params = defaultRunwayParams();
    QVERIFY(Core::MOS::validateObstacleSet(obstacles, params).valid);
}

void MosValidationTest::uxoSyntheticYieldBelowLowIsRejected()
{
    auto obstacles = defaultObstacleSet();
    obstacles.uxo[0].syntheticYield = 0.09;
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::UxoYield);
}

void MosValidationTest::uxoSyntheticYieldAboveHighIsRejected()
{
    auto obstacles = defaultObstacleSet();
    obstacles.uxo[0].syntheticYield = 1000.1;
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::UxoYield);
}

// === influenceRadius: 有限 0.1..6000 ===
void MosValidationTest::influenceRadiusBoundaryLowIsValid()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].influenceRadius = 0.1;
    auto params = defaultRunwayParams();
    QVERIFY(Core::MOS::validateObstacleSet(obstacles, params).valid);
}

void MosValidationTest::influenceRadiusBoundaryHighIsValid()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].influenceRadius = 6000.0;
    auto params = defaultRunwayParams();
    QVERIFY(Core::MOS::validateObstacleSet(obstacles, params).valid);
}

void MosValidationTest::influenceRadiusBelowLowIsRejected()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].influenceRadius = 0.09;
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::InfluenceRadius);
}

void MosValidationTest::influenceRadiusAboveHighIsRejected()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].influenceRadius = 6000.1;
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::InfluenceRadius);
}

void MosValidationTest::influenceRadiusNonFiniteIsRejected()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].influenceRadius = std::numeric_limits<double>::quiet_NaN();
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::InfluenceRadius);
}

// === 总障碍物 N <= 13 ===
void MosValidationTest::totalObstaclesBoundaryIsValid()
{
    auto obstacles = defaultObstacleSet();
    auto params = defaultRunwayParams();
    // 13 个障碍物（8 弹坑 + 5 UXO）= 边界合法
    obstacles.craters.clear();
    obstacles.uxo.clear();
    for (int i = 0; i < 8; ++i) {
        Core::MOS::MosCrater c;
        c.id = QStringLiteral("c%1").arg(i);
        c.visibleRadius = 5.0;
        c.x = 100 + i * 200;
        c.y = 0;
        c.threat = Core::MOS::MosThreatLevel::High;
        c.influenceRadius = 7.5;
        obstacles.craters.append(c);
    }
    for (int i = 0; i < 5; ++i) {
        Core::MOS::MosUxo u;
        u.id = QStringLiteral("u%1").arg(i);
        u.syntheticYield = 30.0;
        u.x = 200 + i * 200;
        u.y = 10;
        u.threat = Core::MOS::MosThreatLevel::Medium;
        u.influenceRadius = 1.5 * std::cbrt(30.0);
        obstacles.uxo.append(u);
    }
    QCOMPARE(obstacles.craters.size() + obstacles.uxo.size(), 13);
    QVERIFY(Core::MOS::validateObstacleSet(obstacles, params).valid);
}

void MosValidationTest::totalObstaclesExceeds13IsRejected()
{
    auto obstacles = defaultObstacleSet();
    auto params = defaultRunwayParams();
    // 14 个障碍物（8 弹坑 + 6 UXO）= 越界
    obstacles.craters.clear();
    obstacles.uxo.clear();
    for (int i = 0; i < 8; ++i) {
        Core::MOS::MosCrater c;
        c.id = QStringLiteral("c%1").arg(i);
        c.visibleRadius = 5.0;
        c.x = 100 + i * 200;
        c.y = 0;
        c.threat = Core::MOS::MosThreatLevel::High;
        c.influenceRadius = 7.5;
        obstacles.craters.append(c);
    }
    for (int i = 0; i < 6; ++i) {
        Core::MOS::MosUxo u;
        u.id = QStringLiteral("u%1").arg(i);
        u.syntheticYield = 30.0;
        u.x = 200 + i * 200;
        u.y = 10;
        u.threat = Core::MOS::MosThreatLevel::Medium;
        u.influenceRadius = 1.5 * std::cbrt(30.0);
        obstacles.uxo.append(u);
    }
    QCOMPARE(obstacles.craters.size() + obstacles.uxo.size(), 14);
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::TotalObstacles);
}

// === 唯一非空 ID ===
void MosValidationTest::emptyObstacleIdIsRejected()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].id = QString();
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::ObstacleId);
}

void MosValidationTest::duplicateObstacleIdIsRejected()
{
    auto obstacles = defaultObstacleSet();
    obstacles.craters[0].id = QStringLiteral("dup");
    obstacles.craters[1].id = QStringLiteral("dup");
    auto params = defaultRunwayParams();
    const auto result = Core::MOS::validateObstacleSet(obstacles, params);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::ObstacleId);
}

// === 种子：signed int32 ===
void MosValidationTest::seedBoundaryLowIsValid()
{
    QVERIFY(Core::MOS::validateSeed(-2147483648LL).valid);
}

void MosValidationTest::seedBoundaryHighIsValid()
{
    QVERIFY(Core::MOS::validateSeed(2147483647LL).valid);
}

void MosValidationTest::seedBelowInt32MinIsRejected()
{
    const auto result = Core::MOS::validateSeed(-2147483649LL);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Seed);
}

void MosValidationTest::seedAboveInt32MaxIsRejected()
{
    const auto result = Core::MOS::validateSeed(2147483648LL);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::Seed);
}

// === 拒绝时输入不变 ===
void MosValidationTest::rejectedRunwayParamsLeavesInputUnchanged()
{
    auto params = defaultRunwayParams();
    const auto originalL = params.L;
    const auto originalW = params.W;
    const auto result = Core::MOS::validateRunwayParams(params);
    QVERIFY(result.valid);
    // 验证校验函数不修改输入（const 引用）
    QCOMPARE(params.L, originalL);
    QCOMPARE(params.W, originalW);

    // 构造非法参数（不通过校验），验证调用方保持原始值
    auto badParams = defaultRunwayParams();
    badParams.L = 100.0; // 非法
    const auto badResult = Core::MOS::validateRunwayParams(badParams);
    QVERIFY(!badResult.valid);
    // 调用方在拒绝后应保留原始非法值（由调用方决定是否恢复，校验函数不修改）
    QCOMPARE(badParams.L, 100.0);
}

void MosValidationTest::rejectedObstacleSetLeavesInputUnchanged()
{
    auto obstacles = defaultObstacleSet();
    const auto originalCount = obstacles.craters.size();
    const auto result = Core::MOS::validateObstacleSet(obstacles, defaultRunwayParams());
    QVERIFY(result.valid);
    QCOMPARE(obstacles.craters.size(), originalCount);

    // 构造非法障碍物（N=14），验证校验函数不修改输入
    Core::MOS::MosObstacleSet badObstacles;
    for (int i = 0; i < 14; ++i) {
        Core::MOS::MosCrater c;
        c.id = QStringLiteral("c%1").arg(i);
        c.visibleRadius = 5.0;
        c.influenceRadius = 7.5;
        badObstacles.craters.append(c);
    }
    const auto badResult = Core::MOS::validateObstacleSet(badObstacles, defaultRunwayParams());
    QVERIFY(!badResult.valid);
    QCOMPARE(badObstacles.craters.size(), 14); // 未被修改
}

QTEST_APPLESS_MAIN(MosValidationTest)

#include "mos_validation_test.moc"
