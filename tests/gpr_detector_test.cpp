// GPR 阈值候选检测测试（REQ-012）：合成 fixture 为 200 道 × 512 样本高斯噪声
// （σ=20，固定种子 42，Box-Muller），可选直达波平台（s∈[15,41) +800）与零均值
// 交变强 blob（20 道 × 24 样本，±1000，中心参数化）。
// 检测语义要点（与 src/GPR/GprThresholdDetector.cpp 对齐，测试断言依据）：
// - 初至 f0 在含直达波数据上呈双峰（≈14 或 ≈40），但嫌疑样本坐标满足
//   sampleIndex = f0 + centerJ，f0 相互抵消，blob 中心断言与 f0 取值无关；
// - 时窗 windowEnd 满足 dt*(f0+windowEnd) ≤ maxTimeNs，候选中心结构性外溢
//   不超过膨胀半径 9 样本（测试按 maxTimeNs + 10*dt 容忍）；
// - 分位断言采用端点式而非计数比较：q=1.0 时阈值=时窗内最大值、严格大于
//   必无候选（确定性）；q=0.999 时强 blob 仍存活（裕度约 10×）。计数比较
//   （如 0.999 候选数 ≤ 默认候选数）在渗流区概率性不稳，不采用。

#include "GPR/GprBScanData.h"
#include "GPR/GprRadarHeader.h"
#include "GPR/GprThresholdDetector.h"

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QRandomGenerator>
#include <cmath>

namespace {

// 合成数据固定尺寸（与样例数据 512 样本一致；道数取 200 保证背景窗 ±50 道生效）
constexpr int kTraces = 200;
constexpr int kSamples = 512;
// 噪声标准差（远小于 blob 幅值 1000 与直达波 800）
constexpr double kSigma = 20.0;
// 2π（避免 M_PI 移植性问题）
constexpr double kTwoPi = 6.28318530717958647692;

// Box-Muller 高斯噪声；u1 = 1.0 - generateDouble() ∈ (0,1]，避免 log(0)
double gaussNoise(QRandomGenerator &rng, double sigma)
{
    const double u1 = 1.0 - rng.generateDouble();
    const double u2 = rng.generateDouble();
    return sigma * std::sqrt(-2.0 * std::log(u1)) * std::cos(kTwoPi * u2);
}

// 生成 traces×kSamples 合成数据（按道顺序平铺）：全体高斯噪声（固定种子 42），
// withDirectWave 时 s∈[15,41) 加 +800 直达波平台；blobTraceCenter/blobSampleCenter
// 非负时以中心取 [c-10,c+10) 道 × [c-12,c+12) 样本叠加零均值交变 ±1000
// （按样本奇偶交替，模拟双曲线绕射的高频振荡），最后截断到 ±30000
QVector<qint16> makeSyntheticValues(int traces, bool withDirectWave,
                                    int blobTraceCenter, int blobSampleCenter)
{
    QVector<qint16> values;
    values.reserve(traces * kSamples);
    QRandomGenerator rng(42);
    for (int t = 0; t < traces; ++t) {
        for (int s = 0; s < kSamples; ++s) {
            double v = gaussNoise(rng, kSigma);
            if (withDirectWave && s >= 15 && s < 41) {
                v += 800.0;
            }
            if (blobTraceCenter >= 0 && blobSampleCenter >= 0
                && t >= blobTraceCenter - 10 && t < blobTraceCenter + 10
                && s >= blobSampleCenter - 12 && s < blobSampleCenter + 12) {
                v += ((s - (blobSampleCenter - 12)) % 2 == 0) ? 1000.0 : -1000.0;
            }
            values.append(static_cast<qint16>(qBound(-30000.0, v, 30000.0)));
        }
    }
    return values;
}

// 将样本值按 int16 小端显式写字节（与平台字节序无关），返回绝对路径；失败返回空串
QString writeRd3File(const QTemporaryDir &dir, const QString &fileName, const QVector<qint16> &values)
{
    const QString path = QDir(dir.path()).absoluteFilePath(fileName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return QString();
    }
    QByteArray raw;
    raw.reserve(values.size() * 2);
    for (const qint16 v : values) {
        const quint16 u = static_cast<quint16>(v);
        raw.append(static_cast<char>(u & 0xFF));
        raw.append(static_cast<char>((u >> 8) & 0xFF));
    }
    file.write(raw);
    return path;
}

// 两候选全字段相等（7 字段）：确定性验证用
bool sameSuspect(const GPR::GprSuspect &a, const GPR::GprSuspect &b)
{
    return a.id == b.id && a.traceIndex == b.traceIndex && a.sampleIndex == b.sampleIndex
        && a.depthM == b.depthM && a.amplitude == b.amplitude
        && a.traceSpan == b.traceSpan && a.sampleSpan == b.sampleSpan;
}

} // namespace

class GprDetectorTest : public QObject
{
    Q_OBJECT

private slots:
    // === 强 blob（道 100、样本 210）被检出；id 连续且结果按 (trace,sample) 字典序 ===
    void detectsStrongBlob();
    // === 纯噪声候选数有界（≤80）且两次检测逐字段一致（管线确定性）===
    void pureNoiseIsBoundedAndDeterministic();
    // === 深部 blob（样本 460，超出默认时窗 91.3 ns）不产生候选 ===
    void deepBlobOutsideTimeWindow();
    // === 无效 bscan/header 任意组合 → 空结果 ===
    void emptyAndInvalidInputsReturnNothing();
    // === 参数端点效应：maxTimeNs 收缩排除 blob；minComponentPx/q=1.0 置空；
    //     q=0.999 强 blob 仍存活 ===
    void parameterEffects();
    // === 窄扫描（50 道 < 背景窗 101 道）不崩溃且候选数有界 ===
    void narrowScanDoesNotCrash();

private:
    // 在成员临时目录写入 <caseName>.rad（样例同款键值）与 <caseName>.rd3 并加载；
    // blobTraceCenter/blobSampleCenter 传 -1 表示纯噪声。成功返回 true
    bool makeScan(const QString &caseName, int traces, bool withDirectWave,
                  int blobTraceCenter, int blobSampleCenter,
                  GPR::GprBScanData &bscan, GPR::GprRadarHeader &header);

    QTemporaryDir m_dir;  // 全部用例共享，按 caseName 区分文件
};

// === 合成扫描对（.rad + .rd3）写入并加载 ===
bool GprDetectorTest::makeScan(const QString &caseName, int traces, bool withDirectWave,
                               int blobTraceCenter, int blobSampleCenter,
                               GPR::GprBScanData &bscan, GPR::GprRadarHeader &header)
{
    if (!m_dir.isValid()) {
        return false;
    }
    // .rad 头：MALA 键值对（':' 分隔），数值与样例数据一致（§5.4）
    const QString radPath = m_dir.filePath(caseName + QStringLiteral(".rad"));
    QFile radFile(radPath);
    if (!radFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    radFile.write("SAMPLES:512\nTIMEWINDOW:110.000\nDISTANCE INTERVAL:0.020\nANTENNAS:800\n");
    radFile.close();

    // .rd3 数据：int16 小端
    const QVector<qint16> values = makeSyntheticValues(
        traces, withDirectWave, blobTraceCenter, blobSampleCenter);
    const QString rd3Path = writeRd3File(m_dir, caseName + QStringLiteral(".rd3"), values);
    if (rd3Path.isEmpty()) {
        return false;
    }

    if (!header.loadFromFile(radPath) || !header.isValid()) {
        return false;
    }
    if (!bscan.loadFromFile(rd3Path, header.samples()) || !bscan.isValid()) {
        return false;
    }
    return bscan.traceCount() == traces && bscan.sampleCount() == kSamples;
}

// === 强 blob（道 100、样本 210）被检出；id 连续且结果按 (trace,sample) 字典序 ===
void GprDetectorTest::detectsStrongBlob()
{
    GPR::GprBScanData bscan;
    GPR::GprRadarHeader header;
    QVERIFY(makeScan(QStringLiteral("blob"), kTraces, true, 100, 210, bscan, header));
    QCOMPARE(bscan.traceCount(), kTraces);
    QCOMPARE(bscan.sampleCount(), kSamples);
    // 初至双峰 ≈14/≈40 均可接受；blob 中心断言经 f0 抵消与其取值无关
    const int f0 = bscan.firstBreakSample();
    QVERIFY(f0 >= 0 && f0 < 150);

    const GPR::GprThresholdDetector detector;
    const QList<GPR::GprSuspect> suspects = detector.detect(bscan, header);

    // 命中强 blob：外接框中心容差 道 ±10 / 样本 ±15（膨胀半径内结构性偏移）
    bool found = false;
    for (const GPR::GprSuspect &s : suspects) {
        if (qAbs(s.traceIndex - 100) <= 10 && qAbs(s.sampleIndex - 210) <= 15) {
            found = true;
            QCOMPARE(s.depthM, header.depthForSample(s.sampleIndex));
        }
    }
    QVERIFY(found);

    // id 从 1 连续编号，且按 (traceIndex, sampleIndex) 非严格字典序
    for (int i = 0; i < suspects.size(); ++i) {
        QCOMPARE(suspects[i].id, i + 1);
        if (i > 0) {
            QVERIFY(suspects[i].traceIndex > suspects[i - 1].traceIndex
                || (suspects[i].traceIndex == suspects[i - 1].traceIndex
                    && suspects[i].sampleIndex >= suspects[i - 1].sampleIndex));
        }
    }
}

// === 纯噪声候选数有界（≤80）且两次检测逐字段一致（管线确定性）===
void GprDetectorTest::pureNoiseIsBoundedAndDeterministic()
{
    GPR::GprBScanData bscan;
    GPR::GprRadarHeader header;
    QVERIFY(makeScan(QStringLiteral("noise"), kTraces, false, -1, -1, bscan, header));
    const int f0 = bscan.firstBreakSample();
    QVERIFY(f0 >= 0 && f0 < 150);

    const GPR::GprThresholdDetector detector;
    const QList<GPR::GprSuspect> first = detector.detect(bscan, header);
    const QList<GPR::GprSuspect> second = detector.detect(bscan, header);

    // 有界性：渗流合并后纯噪声候选估算为个位数，80 留有充分裕度
    QVERIFY(first.size() <= 80);
    // 确定性：同一数据两次检测逐字段一致
    QCOMPARE(first.size(), second.size());
    for (int i = 0; i < first.size(); ++i) {
        QVERIFY(sameSuspect(first.at(i), second.at(i)));
    }
    // 候选字段 sanity：坐标在数据范围内、跨度非负、幅值非负、深度换算一致
    for (const GPR::GprSuspect &s : first) {
        QVERIFY(s.traceIndex >= 0 && s.traceIndex < kTraces);
        QVERIFY(s.sampleIndex >= f0 && s.sampleIndex < kSamples);
        QVERIFY(s.traceSpan >= 1);
        QVERIFY(s.sampleSpan >= 1);
        QVERIFY(s.amplitude >= 0.0);
        QCOMPARE(s.depthM, header.depthForSample(s.sampleIndex));
    }
}

// === 深部 blob（样本 460，超出默认时窗 91.3 ns）不产生候选 ===
void GprDetectorTest::deepBlobOutsideTimeWindow()
{
    GPR::GprBScanData bscan;
    GPR::GprRadarHeader header;
    QVERIFY(makeScan(QStringLiteral("deep"), kTraces, true, 100, 460, bscan, header));
    const double dt = header.dtNs();
    QVERIFY(dt > 0.0);
    // fixture 前提：blob 中心时间在默认时窗外（460*dt ≈ 98.8 ns > 91.3 ns）
    QVERIFY(460 * dt > 91.3);

    const GPR::GprThresholdDetector detector;
    const QList<GPR::GprSuspect> suspects = detector.detect(bscan, header);
    for (const GPR::GprSuspect &s : suspects) {
        // blob 道向邻域内不得出现深部候选
        QVERIFY(!(s.traceIndex >= 70 && s.traceIndex <= 130 && s.sampleIndex >= 440));
        // 全部候选满足时窗 + 膨胀外溢容忍（9 样本膨胀 + 取整，按 10*dt 放宽）
        QVERIFY(s.sampleIndex * dt <= 91.3 + 10.0 * dt);
    }
}

// === 无效 bscan/header 任意组合 → 空结果 ===
void GprDetectorTest::emptyAndInvalidInputsReturnNothing()
{
    const GPR::GprBScanData invalidBscan;
    const GPR::GprRadarHeader invalidHeader;
    const GPR::GprThresholdDetector detector;
    QVERIFY(detector.detect(invalidBscan, invalidHeader).isEmpty());

    GPR::GprBScanData bscan;
    GPR::GprRadarHeader header;
    QVERIFY(makeScan(QStringLiteral("valid4"), 20, false, -1, -1, bscan, header));
    QVERIFY(detector.detect(invalidBscan, header).isEmpty());
    QVERIFY(detector.detect(bscan, invalidHeader).isEmpty());
}

// === 参数端点效应：maxTimeNs 收缩排除 blob；minComponentPx/q=1.0 置空；
//     q=0.999 强 blob 仍存活 ===
void GprDetectorTest::parameterEffects()
{
    GPR::GprBScanData bscan;
    GPR::GprRadarHeader header;
    QVERIFY(makeScan(QStringLiteral("params"), kTraces, true, 100, 210, bscan, header));

    const auto hitsBlob = [](const QList<GPR::GprSuspect> &list) {
        for (const GPR::GprSuspect &s : list) {
            if (qAbs(s.traceIndex - 100) <= 10 && qAbs(s.sampleIndex - 210) <= 15) {
                return true;
            }
        }
        return false;
    };

    // 时窗收缩到 40 ns：blob 中心样本 210（≈45 ns）结构性排除
    // （候选样本坐标 ≤ f0+windowEnd ≈ 186 < 195，确定性）
    {
        GPR::GprThresholdDetector detector;
        detector.setMaxTimeNs(40.0);
        QVERIFY(!hitsBlob(detector.detect(bscan, header)));
    }
    // 成分面积下限远超任何可能成分 → 全部过滤为空
    {
        GPR::GprThresholdDetector detector;
        detector.setMinComponentPx(100000);
        QVERIFY(detector.detect(bscan, header).isEmpty());
    }
    // q=1.0：阈值 = 时窗内最大幅值，严格大于 → 掩膜必空（确定性端点）
    {
        GPR::GprThresholdDetector detector;
        detector.setQuantile(1.0);
        QVERIFY(detector.detect(bscan, header).isEmpty());
    }
    // q=0.999：阈值落在 blob 像素内部（top ~78 像素），子簇 ≥15px 存活，
    // 中心偏移 ≤10 在命中容差内（blob 包络约为噪声包络 10×，确定性裕度充分）
    {
        GPR::GprThresholdDetector detector;
        detector.setQuantile(0.999);
        QVERIFY(hitsBlob(detector.detect(bscan, header)));
    }
}

// === 窄扫描（50 道 < 背景窗 101 道）不崩溃且候选数有界 ===
void GprDetectorTest::narrowScanDoesNotCrash()
{
    GPR::GprBScanData bscan;
    GPR::GprRadarHeader header;
    QVERIFY(makeScan(QStringLiteral("narrow"), 50, false, -1, -1, bscan, header));
    QCOMPARE(bscan.traceCount(), 50);

    const GPR::GprThresholdDetector detector;
    const QList<GPR::GprSuspect> suspects = detector.detect(bscan, header);
    QVERIFY(suspects.size() <= 80);
}

QTEST_GUILESS_MAIN(GprDetectorTest)
#include "gpr_detector_test.moc"
