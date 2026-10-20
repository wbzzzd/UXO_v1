// GprTrack 单元测试：.cor RTK 轨迹解析的道数/1:1 道对齐、间隙插值、UTC 时间戳、
// 高斯平滑与越界行为（设计基线 docs/features/gpr-display.md §5.2/§5.4）。
// 主用例使用 QTemporaryDir 内的合成 .cor（14 字段布局与真实样例一致，
// 记录号刻意取 2×道号，确保测试能区分"按道号对齐"与"按记录号对齐"）；
// 真实样例数据冒烟由 GPR_SAMPLE_DATA_DIR 编译宏控制，未定义或目录缺失时 QSKIP。

#include "GPR/GprTrack.h"

#include <QtTest>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTime>

#include <cmath>

namespace {

// 合成 .cor 的固定日期/时间（#12/#13，与真实样例同格式；.cor 时间判定为 UTC）
const QString kSyntheticDate = QStringLiteral("2025-11-13");
const QString kSyntheticTime = QStringLiteral("01:38:51.835");

// 期望的合成 UTC 时间戳（全文件固定）
QDateTime expectedTimestamp()
{
    return QDateTime(QDate(2025, 11, 13), QTime(1, 38, 51, 835), Qt::UTC);
}

// 数值比较（度）：1e-9 度 ≈ 0.1 mm，远小于合成数据 1e-5 度步长
void verifyNear(double actual, double expected)
{
    QVERIFY2(std::abs(actual - expected) < 1e-9,
             qPrintable(QStringLiteral("actual=%1 expected=%2 差值=%3")
                            .arg(actual, 0, 'g', 15)
                            .arg(expected, 0, 'g', 15)
                            .arg(actual - expected, 0, 'g', 15)));
}

// 生成一行合成 .cor 记录：14 个空格分隔字段，布局与真实样例一致
//（#1 记录号 #2 道号 #3 纬度 #4 经度 #5..#11 未解码占位 #12 日期 #13 时间 #14 未解码占位）
QString makeCorLine(int recordNumber, int traceNumber, double latitude, double longitude)
{
    return QStringLiteral(
               "%1 %2 %3 %4 0.000000000 0.000000000 0.000000000 0.000000000 0.000000000 0.000000000 4.000000000 %5 %6 1234500")
        .arg(recordNumber)
        .arg(traceNumber)
        .arg(latitude, 0, 'f', 9)
        .arg(longitude, 0, 'f', 9)
        .arg(kSyntheticDate, kSyntheticTime);
}

// 生成 count 行线性斜坡轨迹：道号 k=1..N，纬度 = 36 + k*1e-5，经度 = 117 + k*2e-5
//（记录号取 2k，与道号不同，锁定"按道号对齐"行为）
QStringList makeLinearRampLines(int count)
{
    QStringList lines;
    lines.reserve(count);
    for (int k = 1; k <= count; ++k) {
        lines.append(makeCorLine(2 * k, k, 36.0 + k * 1e-5, 117.0 + k * 2e-5));
    }
    return lines;
}

// 在临时目录内写出合成 .cor 文件（每行换行符结尾，与真实样例一致），返回绝对路径
QString writeCorFile(const QTemporaryDir &dir, const QString &fileName, const QStringList &lines)
{
    const QString path = dir.path() + QStringLiteral("/") + fileName;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return QString();
    }
    QTextStream stream(&file);
    for (const QString &line : lines) {
        stream << line << '\n';
    }
    return path;
}

// 样本方差（总体方差，n 为分母）
double variance(const QVector<double> &values)
{
    if (values.isEmpty()) {
        return 0.0;
    }
    double mean = 0.0;
    for (double value : values) {
        mean += value;
    }
    mean /= values.size();
    double sumSquared = 0.0;
    for (double value : values) {
        sumSquared += (value - mean) * (value - mean);
    }
    return sumSquared / values.size();
}

} // namespace

class TestGprTrack : public QObject
{
    Q_OBJECT

private slots:
    // (a) 200 行线性斜坡：pointCount、道号 1:1 精确坐标、UTC 时间戳
    void loadLinearRamp();
    // (b) 删除中间道号行 → 相邻有效点间线性插值
    void interpolateMissingTrace();
    // (c) 锯齿抖动轨迹 → σ=10 平滑后方差小于原始方差；σ<=0 关闭平滑
    void smoothedVarianceBelowRaw();
    // (d) 越界道索引与空对象行为
    void outOfRangeTraces();
    // 真实样例数据冒烟（GPR_SAMPLE_DATA_DIR 宏控制，缺失时 QSKIP）
    void smokeRealSampleData();
};

// (a) 线性斜坡：已知精确经纬度 + 固定 UTC 日期时间
void TestGprTrack::loadLinearRamp()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeCorFile(dir, QStringLiteral("linear.cor"), makeLinearRampLines(200));
    QVERIFY(!path.isEmpty());

    GPR::GprTrack track;
    QVERIFY(track.loadFromFile(path));
    QVERIFY(track.isValid());
    // 覆盖道数 = 最大道号（与 .rd3 道数一致），而非记录号（最大 400）
    QCOMPARE(track.pointCount(), 200);

    // 首道（0 基索引 0 → 道号 1）：精确坐标 + UTC 时间戳
    const GPR::GprTrackPoint first = track.pointForTrace(0);
    QVERIFY(first.valid);
    verifyNear(first.latitude, 36.0 + 1.0 * 1e-5);
    verifyNear(first.longitude, 117.0 + 1.0 * 2e-5);
    QVERIFY(first.timestampUtc.isValid());
    QCOMPARE(first.timestampUtc.timeSpec(), Qt::UTC); // 内部统一 UTC 存储
    QCOMPARE(first.timestampUtc.toMSecsSinceEpoch(), expectedTimestamp().toMSecsSinceEpoch());

    // 中间道（索引 137 → 道号 138）
    const GPR::GprTrackPoint middle = track.pointForTrace(137);
    QVERIFY(middle.valid);
    verifyNear(middle.latitude, 36.0 + 138.0 * 1e-5);
    verifyNear(middle.longitude, 117.0 + 138.0 * 2e-5);
    QCOMPARE(middle.timestampUtc.toMSecsSinceEpoch(), expectedTimestamp().toMSecsSinceEpoch());

    // 末道（索引 199 → 道号 200）
    const GPR::GprTrackPoint last = track.pointForTrace(199);
    QVERIFY(last.valid);
    verifyNear(last.latitude, 36.0 + 200.0 * 1e-5);
    verifyNear(last.longitude, 117.0 + 200.0 * 2e-5);
}

// (b) 删除道号 100（0 基索引 99）的行：该道应在道 99/101 间线性插值
void TestGprTrack::interpolateMissingTrace()
{
    QStringList lines = makeLinearRampLines(200);
    lines.removeAt(99); // 行序与道号序一致：第 100 行（0 基 99）对应道号 100

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeCorFile(dir, QStringLiteral("gap.cor"), lines);
    QVERIFY(!path.isEmpty());

    GPR::GprTrack track;
    QVERIFY(track.loadFromFile(path));
    QVERIFY(track.isValid());
    // 覆盖道数仍为最大道号 200（缺道留作间隙，不缩小覆盖范围）
    QCOMPARE(track.pointCount(), 200);

    // 缺失道：位置 = 相邻道 99/101 的中点；时间戳两侧相同（全文件固定时间）
    const GPR::GprTrackPoint interpolated = track.pointForTrace(99);
    QVERIFY(interpolated.valid);
    verifyNear(interpolated.latitude, 36.0 + 100.0 * 1e-5);
    verifyNear(interpolated.longitude, 117.0 + 100.0 * 2e-5);
    QCOMPARE(interpolated.timestampUtc.toMSecsSinceEpoch(), expectedTimestamp().toMSecsSinceEpoch());

    // 相邻道不受影响（仍为道号直接命中）
    const GPR::GprTrackPoint left = track.pointForTrace(98);
    QVERIFY(left.valid);
    verifyNear(left.latitude, 36.0 + 99.0 * 1e-5);
    const GPR::GprTrackPoint right = track.pointForTrace(100);
    QVERIFY(right.valid);
    verifyNear(right.latitude, 36.0 + 101.0 * 1e-5);
}

// (c) 锯齿抖动：奇数道 +5e-5、偶数道 -5e-5 度；σ=10 道（半径 30 道，
// 覆盖约 30 个抖动周期）平滑后纬度方差必须小于原始方差
void TestGprTrack::smoothedVarianceBelowRaw()
{
    const int count = 200;
    QStringList lines;
    lines.reserve(count);
    for (int k = 1; k <= count; ++k) {
        const double zigzag = (k % 2 == 1) ? 5e-5 : -5e-5; // 模拟 RTK 横向抖动
        lines.append(makeCorLine(2 * k, k, 36.0 + zigzag, 117.0));
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeCorFile(dir, QStringLiteral("zigzag.cor"), lines);
    QVERIFY(!path.isEmpty());

    GPR::GprTrack track;
    QVERIFY(track.loadFromFile(path));
    QVERIFY(track.isValid());

    // 原始纬度序列方差（每道直接命中）
    QVector<double> rawLatitudes;
    rawLatitudes.reserve(count);
    for (int i = 0; i < count; ++i) {
        const GPR::GprTrackPoint point = track.pointForTrace(i);
        QVERIFY(point.valid);
        rawLatitudes.append(point.latitude);
    }
    const double rawVariance = variance(rawLatitudes);
    QVERIFY(rawVariance > 0.0); // 锯齿数据本身方差非零（防止用例退化）

    // σ=10 道平滑后纬度序列方差
    track.setSmoothingSigmaTraces(10.0);
    QVector<double> smoothedLatitudes;
    smoothedLatitudes.reserve(count);
    for (int i = 0; i < count; ++i) {
        const GPR::GprTrackPoint point = track.smoothedPointForTrace(i);
        QVERIFY(point.valid);
        smoothedLatitudes.append(point.latitude);
    }
    const double smoothedVariance = variance(smoothedLatitudes);

    QVERIFY2(smoothedVariance < rawVariance,
             qPrintable(QStringLiteral("平滑后方差=%1 应小于 原始方差=%2")
                            .arg(smoothedVariance, 0, 'g', 15)
                            .arg(rawVariance, 0, 'g', 15)));

    // σ<=0 关闭平滑：平滑点应退化为原始点
    track.setSmoothingSigmaTraces(0.0);
    const GPR::GprTrackPoint rawPoint = track.pointForTrace(50);
    const GPR::GprTrackPoint smoothingOff = track.smoothedPointForTrace(50);
    QVERIFY(smoothingOff.valid);
    verifyNear(smoothingOff.latitude, rawPoint.latitude);
    verifyNear(smoothingOff.longitude, rawPoint.longitude);
}

// (d) 越界道索引 → valid=false；空对象（未载入）与不存在文件
void TestGprTrack::outOfRangeTraces()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeCorFile(dir, QStringLiteral("linear.cor"), makeLinearRampLines(200));
    QVERIFY(!path.isEmpty());

    GPR::GprTrack track;
    QVERIFY(track.loadFromFile(path));

    // 越界：负索引 / pointCount()（末道+1）/ 远超范围；原始与平滑接口一致
    QVERIFY(!track.pointForTrace(-1).valid);
    QVERIFY(!track.pointForTrace(200).valid);
    QVERIFY(!track.pointForTrace(9999).valid);
    QVERIFY(!track.smoothedPointForTrace(-1).valid);
    QVERIFY(!track.smoothedPointForTrace(200).valid);

    // 空对象：未载入时 isValid=false、pointCount=0、任意道索引无效
    GPR::GprTrack empty;
    QVERIFY(!empty.isValid());
    QCOMPARE(empty.pointCount(), 0);
    QVERIFY(!empty.pointForTrace(0).valid);
    QVERIFY(!empty.smoothedPointForTrace(0).valid);

    // 不存在的文件：载入失败且不改变空态语义
    const QString missingPath = dir.path() + QStringLiteral("/no_such_file.cor");
    QVERIFY(!empty.loadFromFile(missingPath));
    QVERIFY(!empty.isValid());
    QCOMPARE(empty.pointCount(), 0);
}

// 真实样例数据冒烟（只读）：载入 GPR_SAMPLE_DATA_DIR 内第一个 .cor，
// 验证规模（>1000 道）与首点可用性；样例数据应为 7297 道。
void TestGprTrack::smokeRealSampleData()
{
#ifdef GPR_SAMPLE_DATA_DIR
    // 宏由构建系统注入带引号的绝对路径（与 DETECTION_ASSETS_DIR 同构）；
    // 路径含中文，统一按 UTF-8 解码
    const QDir sampleDir(QString::fromUtf8(GPR_SAMPLE_DATA_DIR));
    const QStringList corFiles =
        sampleDir.entryList(QStringList() << QStringLiteral("*.cor"), QDir::Files, QDir::Name);
    if (corFiles.isEmpty()) {
        QSKIP("GPR_SAMPLE_DATA_DIR 下未找到 .cor 样例文件（数据目录可能未挂载）");
    }

    const QString path = sampleDir.filePath(corFiles.first());
    GPR::GprTrack track;
    QVERIFY2(track.loadFromFile(path),
             qPrintable(QStringLiteral("真实样例 .cor 载入失败: %1").arg(path)));
    QVERIFY(track.isValid());
    QVERIFY(track.pointCount() > 1000); // 样例应为 7297 道（.rd3 道数一致）
    const GPR::GprTrackPoint first = track.pointForTrace(0);
    QVERIFY(first.valid);                                  // 首道位置可用（道号 1:1 对齐）
    QVERIFY(first.timestampUtc.timeSpec() == Qt::UTC);     // 内部统一 UTC 存储
#else
    QSKIP("未定义 GPR_SAMPLE_DATA_DIR，跳过真实样例数据冒烟测试");
#endif
}

QTEST_GUILESS_MAIN(TestGprTrack)

#include "gpr_track_test.moc"
