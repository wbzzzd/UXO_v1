// MOS P0 合同闭合测试：验证三项 P0 闭合修复。
//   1. serializeObstacleSetBytes 显式字节写入器与 JS JSON.stringify 逐字节一致（5 个种子）
//   2. craterRMin/RMax 为 double，校验有限 0.1..100 且 min<=max
//   3. MosEstimator 立方 visibleRadius 而非 influenceRadius
// 本测试未注册到 CMakeLists，需手动加入构建。

#include "Core/MOS/MosTypes.h"
#include "Core/MOS/MosFixtureGenerator.h"
#include "Core/MOS/MosEstimator.h"
#include "Core/MOS/MosValidation.h"

#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

// 基于源文件路径定位 tests/fixtures/mos_rng_vectors.json
QString fixturePath()
{
    return QFileInfo(QLatin1String(__FILE__)).dir().absoluteFilePath(
        QStringLiteral("fixtures/mos_rng_vectors.json"));
}

QJsonObject loadReferenceVectors()
{
    QFile f(fixturePath());
    if (!f.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    return QJsonDocument::fromJson(f.readAll()).object();
}

Core::MOS::MosRunwayParams defaultParams()
{
    Core::MOS::MosRunwayParams p;
    p.L = 3000.0; p.W = 50.0; p.K = 1.5; p.expand = 1.5;
    p.step = 1.0; p.minLength = 460.0; p.minWidth = 15.0;
    p.backfill = 50.0; p.uxoHours = 8.0; p.tiers = 3;
    return p;
}

Core::MOS::MosGeneratorParams defaultGenParams()
{
    Core::MOS::MosGeneratorParams g;
    g.craterCount = 2; g.craterRMin = 3.0; g.craterRMax = 6.0;
    g.uxoCount = 2; g.uxoYMin = 10.0; g.uxoYMax = 50.0;
    return g;
}

bool fuzzyEq(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) <= eps;
}

} // namespace

class MosContractClosureTest : public QObject
{
    Q_OBJECT

private slots:
    void seedVectorsMatchCanonicalBytes();
    void craterRadiusFractionalInputValidates();
    void craterRadiusBoundaryLowIsValid();
    void craterRadiusBoundaryHighIsValid();
    void craterRadiusBelowLowIsRejected();
    void craterRadiusAboveHighIsRejected();
    void craterRadiusNonFiniteIsRejected();
    void craterRadiusMinAboveMaxIsRejected();
    void estimatorCubesVisibleRadiusNotInfluenceRadius();
};

// === 5 个种子向量逐字节比对 ===
void MosContractClosureTest::seedVectorsMatchCanonicalBytes()
{
    const auto vectors = loadReferenceVectors();
    QVERIFY2(!vectors.isEmpty(), "无法加载 mos_rng_vectors.json");
    const auto seedVectors = vectors.value("seed_vectors").toArray();
    QCOMPARE(seedVectors.size(), 5);

    const auto p = defaultParams();
    const auto g = defaultGenParams();
    const qint32 seeds[] = {0, 42, -1, -2147483647 - 1, 2147483647};

    for (int i = 0; i < 5; ++i) {
        const auto sv = seedVectors[i].toObject();
        const QByteArray expected = sv.value("canonical_bytes").toString().toUtf8();
        QVERIFY2(!expected.isEmpty(), "canonical_bytes 缺失");
        const auto obs = Core::MOS::MosFixtureGenerator::generate(p, g, seeds[i]);
        const QByteArray actual = Core::MOS::serializeObstacleSetBytes(obs);
        const QByteArray msg = "seed=" + QByteArray::number(seeds[i]) +
                               " expected=" + expected + " actual=" + actual;
        QVERIFY2(actual == expected, msg.constData());
    }
}

// === craterRMin/RMax: double 0.1..100 校验 ===
void MosContractClosureTest::craterRadiusFractionalInputValidates()
{
    auto g = defaultGenParams();
    g.craterRMin = 3.5;
    g.craterRMax = 5.5;
    const auto result = Core::MOS::validateGeneratorParams(g);
    QVERIFY(result.valid);
    // 小数输入下生成器仍产出整数 visibleRadius（jsRound 半数向正无穷）
    const auto obs = Core::MOS::MosFixtureGenerator::generate(defaultParams(), g, 42);
    QCOMPARE(obs.craters.size(), 2);
    for (const auto &c : obs.craters) {
        QVERIFY(c.visibleRadius == std::floor(c.visibleRadius));
    }
}

void MosContractClosureTest::craterRadiusBoundaryLowIsValid()
{
    auto g = defaultGenParams();
    g.craterRMin = 0.1;
    g.craterRMax = 0.1;
    QVERIFY(Core::MOS::validateGeneratorParams(g).valid);
}

void MosContractClosureTest::craterRadiusBoundaryHighIsValid()
{
    auto g = defaultGenParams();
    g.craterRMin = 100.0;
    g.craterRMax = 100.0;
    QVERIFY(Core::MOS::validateGeneratorParams(g).valid);
}

void MosContractClosureTest::craterRadiusBelowLowIsRejected()
{
    auto g = defaultGenParams();
    g.craterRMin = 0.09;
    const auto result = Core::MOS::validateGeneratorParams(g);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::CraterRadius);
}

void MosContractClosureTest::craterRadiusAboveHighIsRejected()
{
    auto g = defaultGenParams();
    g.craterRMax = 100.1;
    const auto result = Core::MOS::validateGeneratorParams(g);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::CraterRadius);
}

void MosContractClosureTest::craterRadiusNonFiniteIsRejected()
{
    auto g = defaultGenParams();
    g.craterRMin = std::numeric_limits<double>::quiet_NaN();
    const auto result = Core::MOS::validateGeneratorParams(g);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::CraterRadius);

    g.craterRMin = 3.0;
    g.craterRMax = std::numeric_limits<double>::infinity();
    const auto result2 = Core::MOS::validateGeneratorParams(g);
    QVERIFY(!result2.valid);
    QCOMPARE(result2.reason, Core::MOS::MosValidationReason::CraterRadius);
}

void MosContractClosureTest::craterRadiusMinAboveMaxIsRejected()
{
    auto g = defaultGenParams();
    g.craterRMin = 6.5;
    g.craterRMax = 6.0;
    const auto result = Core::MOS::validateGeneratorParams(g);
    QVERIFY(!result.valid);
    QCOMPARE(result.reason, Core::MOS::MosValidationReason::CraterRadius);
}

// === 估算器使用 visibleRadius 而非 influenceRadius ===
void MosContractClosureTest::estimatorCubesVisibleRadiusNotInfluenceRadius()
{
    Core::MOS::MosObstacleSet obs;
    Core::MOS::MosCrater c;
    c.id = QStringLiteral("c1");
    c.visibleRadius = 3.0;
    c.influenceRadius = 9.0; // 故意与 visibleRadius 不同
    obs.craters.append(c);

    auto p = defaultParams();
    p.backfill = 50.0;
    p.uxoHours = 8.0;

    // 测试前置：visibleRadius 须 != influenceRadius，否则无法区分
    QVERIFY2(c.visibleRadius != c.influenceRadius,
             "visibleRadius 须 != influenceRadius");

    const auto est = Core::MOS::MosEstimator::estimate(obs, p, 0, 3);
    // 估算器须立方 visibleRadius(=3)，而非 influenceRadius(=9)
    const double expectedVolume = 4.0 / 3.0 * M_PI * 27.0; // 3³ = 27
    QVERIFY(fuzzyEq(est.totalBackfillVolume, expectedVolume));
    QVERIFY(fuzzyEq(est.backfillHours, expectedVolume / 50.0));
    QVERIFY(fuzzyEq(est.uxoHours, 0.0));
    QVERIFY(fuzzyEq(est.totalHours, expectedVolume / 50.0));
}

QTEST_APPLESS_MAIN(MosContractClosureTest)

#include "mos_contract_closure_test.moc"
