// GPR .rd3 B-Scan 读取测试（REQ-012）：合成 fixture 覆盖 round-trip、极值、
// LRU 容量外访问、尺寸/路径校验拒绝与逐通道 t0 初至估计（含平道回退）；
// 全通道预载覆盖内存常驻（删源文件后仍可访问）、幂等与重载复位；
// 末尾真实数据 smoke/预载性能由 GPR_SAMPLE_DATA_DIR 编译宏驱动，目录/文件缺失时自动跳过。

#include "GPR/GprBScanData.h"

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace {

// 生成 traces×samples 数据（按道顺序平铺），值由 generator(trace, sample) 给出
template <typename Generator>
QVector<qint16> makeValues(int traces, int samples, Generator generator)
{
    QVector<qint16> values;
    values.reserve(traces * samples);
    for (int t = 0; t < traces; ++t) {
        for (int s = 0; s < samples; ++s) {
            values.append(generator(t, s));
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

} // namespace

class GprBScanTest : public QObject
{
    Q_OBJECT

private slots:
    // === 加载与逐点/整道访问 round-trip（含越界映射）===
    void loadAndAccessRoundTrip();
    // === 负值与 int16 极值 round-trip ===
    void negativeAndExtremeValuesRoundTrip();
    // === 访问道数超过 LRU 容量（8）时数据仍正确（强制淘汰与重读）===
    void lazyAccessBeyondLruCapacity();
    // === 文件大小与 expectedSamples 不匹配 → 拒绝并保持无效 ===
    void mismatchedSizeIsRejected();
    // === 不存在路径 / 非正样本数 / 目录路径 → 拒绝 ===
    void invalidInputsAreRejected();
    // === 文件尾部多 2 字节（不能整除）→ 拒绝 ===
    void oversizeFileIsRejected();
    // === 文件尾部少 1 字节（不能整除）→ 拒绝 ===
    void undersizeFileIsRejected();
    // === 已知阶跃位置（第 40 样本）被初至估计命中；重复调用走缓存 ===
    void firstBreakDetectsKnownStep();
    // === 平道（全 0）：主判据无命中，回退 |一阶差分| argmax → 0 ===
    void firstBreakFallbackOnFlatTrace();
    // === 无效数据初至返回哨兵 -1 ===
    void firstBreakInvalidReturnsSentinel();
    // === 全通道预载：内存常驻（预载后删除源文件仍可访问）、幂等、重载复位 ===
    void preloadAllTracesMemoryResident();
    // === 真实数据 smoke：GPR_SAMPLE_DATA_DIR 首个 *_H_*.rd3（缺失自动跳过）===
    void realDataSmoke();
    // === 真实数据预载性能 smoke：预载后全道访问为内存速度（缺失自动跳过）===
    void preloadAllTracesRealDataPerf();
};

// === 加载与逐点/整道访问 round-trip（含越界映射）===
void GprBScanTest::loadAndAccessRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // 20 道 × 64 样本，值 = 道索引（所有样本同道同值）
    const QVector<qint16> values = makeValues(20, 64, [](int t, int) { return qint16(t); });
    const QString path = writeRd3File(dir, QStringLiteral("roundtrip.rd3"), values);
    QVERIFY(!path.isEmpty());

    GPR::GprBScanData bscan;
    QVERIFY(bscan.loadFromFile(path, 64));
    QVERIFY(bscan.isValid());
    QCOMPARE(bscan.traceCount(), 20);
    QCOMPARE(bscan.sampleCount(), 64);

    // 逐点抽查 + 整道校验
    QCOMPARE(int(bscan.sampleAt(0, 0)), 0);
    QCOMPARE(int(bscan.sampleAt(7, 33)), 7);
    QCOMPARE(int(bscan.sampleAt(19, 63)), 19);
    const QVector<qint16> trace5 = bscan.trace(5);
    QCOMPARE(trace5.size(), 64);
    for (const qint16 v : trace5) {
        QCOMPARE(int(v), 5);
    }

    // 越界访问：sampleAt 映射 0，trace 映射空向量
    QCOMPARE(int(bscan.sampleAt(-1, 0)), 0);
    QCOMPARE(int(bscan.sampleAt(0, -1)), 0);
    QCOMPARE(int(bscan.sampleAt(20, 0)), 0);
    QCOMPARE(int(bscan.sampleAt(0, 64)), 0);
    QVERIFY(bscan.trace(-1).isEmpty());
    QVERIFY(bscan.trace(20).isEmpty());
}

// === 负值与 int16 极值 round-trip ===
void GprBScanTest::negativeAndExtremeValuesRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // 1 道 × 8 样本：覆盖 0、±1、int16 上下限与常规正负值
    const QVector<qint16> values = {
        qint16(0), qint16(-1), qint16(1), qint16(32767),
        qint16(-32768), qint16(12345), qint16(-12345), qint16(42),
    };
    const QString path = writeRd3File(dir, QStringLiteral("extreme.rd3"), values);
    QVERIFY(!path.isEmpty());

    GPR::GprBScanData bscan;
    QVERIFY(bscan.loadFromFile(path, 8));
    QVERIFY(bscan.isValid());
    const QVector<qint16> back = bscan.trace(0);
    QCOMPARE(back.size(), values.size());
    for (int s = 0; s < values.size(); ++s) {
        QCOMPARE(int(back.at(s)), int(values.at(s)));
        QCOMPARE(int(bscan.sampleAt(0, s)), int(values.at(s)));
    }
}

// === 访问道数超过 LRU 容量（8）时数据仍正确（强制淘汰与重读）===
void GprBScanTest::lazyAccessBeyondLruCapacity()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // 40 道 × 4 样本，值 = 100 + 道索引；容量 8 << 40，顺序访问必然触发淘汰
    const QVector<qint16> values = makeValues(40, 4, [](int t, int) { return qint16(100 + t); });
    const QString path = writeRd3File(dir, QStringLiteral("lru.rd3"), values);
    QVERIFY(!path.isEmpty());

    GPR::GprBScanData bscan;
    QVERIFY(bscan.loadFromFile(path, 4));
    QVERIFY(bscan.isValid());

    // 两轮完整访问：第一轮淘汰后，第二轮须从文件重读且结果一致
    for (int round = 0; round < 2; ++round) {
        for (int t = 0; t < 40; ++t) {
            const QVector<qint16> trace = bscan.trace(t);
            QCOMPARE(trace.size(), 4);
            for (const qint16 v : trace) {
                QCOMPARE(int(v), 100 + t);
            }
        }
    }
}

// === 文件大小与 expectedSamples 不匹配 → 拒绝并保持无效 ===
void GprBScanTest::mismatchedSizeIsRejected()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // 实际 20 道 × 64 样本（2560 字节），按 100 样本加载：2560 % 200 != 0
    const QVector<qint16> values = makeValues(20, 64, [](int, int) { return qint16(0); });
    const QString path = writeRd3File(dir, QStringLiteral("mismatch.rd3"), values);
    QVERIFY(!path.isEmpty());

    GPR::GprBScanData bscan;
    QVERIFY(!bscan.loadFromFile(path, 100));
    QVERIFY(!bscan.isValid());
    QCOMPARE(bscan.traceCount(), 0);
    QCOMPARE(bscan.sampleCount(), 0);
    QCOMPARE(int(bscan.sampleAt(0, 0)), 0);
    QVERIFY(bscan.trace(0).isEmpty());
    QCOMPARE(bscan.firstBreakSample(), -1);
}

// === 不存在路径 / 非正样本数 / 目录路径 → 拒绝 ===
void GprBScanTest::invalidInputsAreRejected()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QVector<qint16> values = makeValues(2, 8, [](int, int) { return qint16(0); });
    const QString path = writeRd3File(dir, QStringLiteral("valid.rd3"), values);
    QVERIFY(!path.isEmpty());

    // 不存在的文件
    {
        GPR::GprBScanData bscan;
        QVERIFY(!bscan.loadFromFile(QDir(dir.path()).absoluteFilePath(QStringLiteral("missing.rd3")), 8));
        QVERIFY(!bscan.isValid());
    }
    // expectedSamples 为 0 / 负数
    {
        GPR::GprBScanData bscan;
        QVERIFY(!bscan.loadFromFile(path, 0));
        QVERIFY(!bscan.isValid());
        QVERIFY(!bscan.loadFromFile(path, -5));
        QVERIFY(!bscan.isValid());
    }
    // 目录路径而非文件
    {
        GPR::GprBScanData bscan;
        QVERIFY(!bscan.loadFromFile(dir.path(), 8));
        QVERIFY(!bscan.isValid());
    }
}

// === 文件尾部多 2 字节（不能整除）→ 拒绝 ===
void GprBScanTest::oversizeFileIsRejected()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QVector<qint16> values = makeValues(20, 64, [](int, int) { return qint16(0); });
    const QString path = writeRd3File(dir, QStringLiteral("oversize.rd3"), values);
    QVERIFY(!path.isEmpty());

    // 追加 2 字节：2562 % 128 != 0
    QFile file(path);
    QVERIFY(file.open(QIODevice::Append));
    const char extra[2] = { char(1), char(2) };
    QCOMPARE(file.write(extra, 2), qint64(2));
    file.close();

    GPR::GprBScanData bscan;
    QVERIFY(!bscan.loadFromFile(path, 64));
    QVERIFY(!bscan.isValid());
}

// === 文件尾部少 1 字节（不能整除）→ 拒绝 ===
void GprBScanTest::undersizeFileIsRejected()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QVector<qint16> values = makeValues(20, 64, [](int, int) { return qint16(0); });
    const QString path = writeRd3File(dir, QStringLiteral("undersize.rd3"), values);
    QVERIFY(!path.isEmpty());

    // 截掉 1 字节：2559 % 128 != 0
    QFile file(path);
    QVERIFY(file.resize(file.size() - 1));

    GPR::GprBScanData bscan;
    QVERIFY(!bscan.loadFromFile(path, 64));
    QVERIFY(!bscan.isValid());
}

// === 已知阶跃位置（第 40 样本）被初至估计命中；重复调用走缓存 ===
void GprBScanTest::firstBreakDetectsKnownStep()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // 4 道 × 64 样本：s<40 为小噪声（|v|<=3，门限 |mean|+6σ≈12 不可达），
    // s>=40 为大阶跃（4000+t）；初至应稳定命中 40
    const QVector<qint16> values = makeValues(4, 64, [](int t, int s) {
        return qint16(s < 40 ? (t * 7 + s * 3) % 7 - 3 : 4000 + t);
    });
    const QString path = writeRd3File(dir, QStringLiteral("step.rd3"), values);
    QVERIFY(!path.isEmpty());

    GPR::GprBScanData bscan;
    QVERIFY(bscan.loadFromFile(path, 64));
    QVERIFY(bscan.isValid());
    QCOMPARE(bscan.firstBreakSample(), 40);
    // 重复调用命中缓存，结果一致
    QCOMPARE(bscan.firstBreakSample(), 40);
}

// === 平道（全 0）：主判据无命中，回退 |一阶差分| argmax → 0 ===
void GprBScanTest::firstBreakFallbackOnFlatTrace()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QVector<qint16> values = makeValues(3, 64, [](int, int) { return qint16(0); });
    const QString path = writeRd3File(dir, QStringLiteral("flat.rd3"), values);
    QVERIFY(!path.isEmpty());

    GPR::GprBScanData bscan;
    QVERIFY(bscan.loadFromFile(path, 64));
    QVERIFY(bscan.isValid());
    // 平道：门限为 0 且严格大于不成立 → 主判据无命中；差分全 0 → 回退取首个位置 0
    QCOMPARE(bscan.firstBreakSample(), 0);
}

// === 无效数据初至返回哨兵 -1 ===
void GprBScanTest::firstBreakInvalidReturnsSentinel()
{
    GPR::GprBScanData bscan;
    QVERIFY(!bscan.isValid());
    QCOMPARE(bscan.firstBreakSample(), -1);
}

// === 全通道预载：内存常驻（预载后删除源文件仍可访问）、幂等、重载复位 ===
void GprBScanTest::preloadAllTracesMemoryResident()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // 30 道 × 16 样本（道数 > LRU 容量 8），值 = 道*100 + 样本，道间可区分
    const QVector<qint16> values = makeValues(30, 16, [](int t, int s) {
        return qint16(t * 100 + s);
    });
    const QString path = writeRd3File(dir, QStringLiteral("preload.rd3"), values);
    QVERIFY(!path.isEmpty());

    GPR::GprBScanData bscan;
    QVERIFY(bscan.loadFromFile(path, 16));
    QVERIFY(!bscan.isFullyLoaded());

    // 预载：单次顺序读入全部道数据；幂等（重复调用直接成功）
    QVERIFY(bscan.preloadAllTraces());
    QVERIFY(bscan.isFullyLoaded());
    QVERIFY(bscan.preloadAllTraces());

    // 删除源文件：预载后的任何访问不得再触盘
    // （回归锁：GUI 线程经 LRU-8 逐道 9p 小读是雷达页交互冻结根因）
    QVERIFY(QFile::remove(path));
    for (int t = 0; t < 30; ++t) {
        const QVector<qint16> trace = bscan.trace(t);
        QCOMPARE(trace.size(), 16);
        for (int s = 0; s < 16; ++s) {
            QCOMPARE(int(trace.at(s)), t * 100 + s);
        }
    }
    QCOMPARE(int(bscan.sampleAt(7, 9)), 709);
    QVERIFY(bscan.firstBreakSample() >= 0);

    // 重新加载失败 → 预载状态复位
    QVERIFY(!bscan.loadFromFile(QDir(dir.path()).absoluteFilePath(QStringLiteral("missing.rd3")), 16));
    QVERIFY(!bscan.isFullyLoaded());
}

// === 真实数据 smoke：GPR_SAMPLE_DATA_DIR 首个 *_H_*.rd3（缺失自动跳过）===
void GprBScanTest::realDataSmoke()
{
#ifdef GPR_SAMPLE_DATA_DIR
    const QDir sampleDir(QStringLiteral(GPR_SAMPLE_DATA_DIR));
    if (!sampleDir.exists()) {
        QSKIP("GPR_SAMPLE_DATA_DIR 目录不存在，跳过真实数据 smoke 测试");
    }
    const QStringList rd3Files = sampleDir.entryList(
        QStringList() << QStringLiteral("*_H_*.rd3"), QDir::Files, QDir::Name);
    if (rd3Files.isEmpty()) {
        QSKIP("GPR_SAMPLE_DATA_DIR 目录无 H 组 .rd3 文件，跳过真实数据 smoke 测试");
    }

    // 首个 H 组 .rd3（按文件名排序）：§5.4 已验证 SAMPLES=512
    GPR::GprBScanData bscan;
    QVERIFY(bscan.loadFromFile(sampleDir.filePath(rd3Files.first()), 512));
    QVERIFY(bscan.isValid());
    QVERIFY(bscan.traceCount() > 0);
    QCOMPARE(bscan.sampleCount(), 512);
    const int firstBreak = bscan.firstBreakSample();
    QVERIFY(firstBreak >= 0 && firstBreak < 512);
    // 抽点确认数据可读（末道末样本不越界）
    QVERIFY(bscan.trace(bscan.traceCount() - 1).size() == 512);
#else
    QSKIP("未定义 GPR_SAMPLE_DATA_DIR，跳过真实数据 smoke 测试");
#endif
}

// === 真实数据预载性能 smoke：预载后全道访问为内存速度（缺失自动跳过）===
void GprBScanTest::preloadAllTracesRealDataPerf()
{
#ifdef GPR_SAMPLE_DATA_DIR
    const QDir sampleDir(QStringLiteral(GPR_SAMPLE_DATA_DIR));
    if (!sampleDir.exists()) {
        QSKIP("GPR_SAMPLE_DATA_DIR 目录不存在，跳过真实数据预载性能测试");
    }
    const QStringList rd3Files = sampleDir.entryList(
        QStringList() << QStringLiteral("*_H_*.rd3"), QDir::Files, QDir::Name);
    if (rd3Files.isEmpty()) {
        QSKIP("GPR_SAMPLE_DATA_DIR 目录无 H 组 .rd3 文件，跳过真实数据预载性能测试");
    }

    GPR::GprBScanData bscan;
    QVERIFY(bscan.loadFromFile(sampleDir.filePath(rd3Files.first()), 512));

    // 预载 = 单次顺序 bulk read：/mnt/d（9p）实测秒级，宽松上限 30s
    QElapsedTimer timer;
    timer.start();
    QVERIFY(bscan.preloadAllTraces());
    const qint64 preloadMs = timer.elapsed();
    QVERIFY2(preloadMs < 30000,
             qPrintable(QStringLiteral("preloadAllTraces 耗时 %1 ms").arg(preloadMs)));

    // 预载后全道访问必须为内存速度：7297 道全遍历上限 2000ms
    // （修复前同路径经 LRU-8 逐道 9p 小读需数十秒，量级差异保证阈值无 flaky）
    timer.restart();
    qint64 checksum = 0;
    for (int t = 0; t < bscan.traceCount(); ++t) {
        checksum += bscan.trace(t).last();
    }
    const qint64 accessMs = timer.elapsed();
    qDebug() << "preload ms:" << preloadMs << " full-access ms:" << accessMs
             << " checksum:" << checksum;
    QVERIFY2(accessMs < 2000,
             qPrintable(QStringLiteral("预载后全道访问耗时 %1 ms").arg(accessMs)));
#else
    QSKIP("未定义 GPR_SAMPLE_DATA_DIR，跳过真实数据预载性能测试");
#endif
}

QTEST_GUILESS_MAIN(GprBScanTest)
#include "gpr_bscan_test.moc"