// GPR .rad 头解析测试（REQ-013）：合成 fixture 覆盖实测格式合法头、缺必需键、
// 数值非法、未知键、空白/行尾/分隔符变体，并校验 dtNs/depthForSample 两程换算数学；
// 末尾真实数据 smoke 由 GPR_SAMPLE_DATA_DIR 编译宏驱动，目录/文件缺失时自动跳过。

#include "GPR/GprRadarHeader.h"

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace {

// 实测样例 .rad 完整头（泰安测试 2025-11-13 H_T1R1 原文照录）：36 行、35 个唯一键，
// 含空格键（DISTANCE INTERVAL）、空格值（ANTENNA ORIENTATION:NOT VALID FIELD）
// 与重复键（START POSITION 出现两次）
QStringList validRadLines()
{
    return {
        QStringLiteral("SAMPLES:512"),
        QStringLiteral("FREQUENCY:160"),
        QStringLiteral("FREQUENCY STEPS:1"),
        QStringLiteral("SIGNAL POSITION:0"),
        QStringLiteral("DISTANCE FLAG:1"),
        QStringLiteral("TIME FLAG:0"),
        QStringLiteral("PROGRAM FLAG:0"),
        QStringLiteral("EXTERNAL FLAG:100"),
        QStringLiteral("TIME INTERVAL:0.21"),
        QStringLiteral("DISTANCE INTERVAL:0.02"),
        QStringLiteral("OPERATOR:0"),
        QStringLiteral("CUSTOMER:0"),
        QStringLiteral("SITE:0"),
        QStringLiteral("ANTENNAS:800"),
        QStringLiteral("ANTENNA ORIENTATION:NOT VALID FIELD"),
        QStringLiteral("ANTENNA SEPARATION:0.012"),
        QStringLiteral("COMMENT:None"),
        QStringLiteral("TIMEWINDOW:110.000"),
        QStringLiteral("STACKS:1"),
        QStringLiteral("STACK EXPONENT:0"),
        QStringLiteral("STACKING TIME:0.015"),
        QStringLiteral("LAST TRACE:0"),
        QStringLiteral("STOP POSITION:0"),
        QStringLiteral("SYSTEM CALIBRATION:0.001"),
        QStringLiteral("START POSITION:0"),
        QStringLiteral("SHORT FLAG:1"),
        QStringLiteral("INTERMEDIATE FLAG:0"),
        QStringLiteral("START POSITION:0"),
        QStringLiteral("LONG FLAG:0"),
        QStringLiteral("PREPROCESSING:0"),
        QStringLiteral("HIGH:18"),
        QStringLiteral("LOW:16"),
        QStringLiteral("FIXED INCREMENT:0.3"),
        QStringLiteral("FIXED MOVES UP:0"),
        QStringLiteral("FIXED MOVES DOWN:1"),
        QStringLiteral("FIXED POSITION:0"),
    };
}

// 在临时目录写入原始字节内容，返回绝对路径；失败返回空串
QString writeRadFile(const QTemporaryDir &dir, const QString &fileName, const QByteArray &content)
{
    const QString path = QDir(dir.path()).absoluteFilePath(fileName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return QString();
    }
    file.write(content);
    return path;
}

// 在临时目录写入 .rad 文件（UTF-8、'\n' 行尾），返回绝对路径；失败返回空串
QString writeRadFile(const QTemporaryDir &dir, const QString &fileName, const QStringList &lines)
{
    QByteArray content;
    for (const QString &line : lines) {
        content += line.toUtf8();
        content += '\n';
    }
    return writeRadFile(dir, fileName, content);
}

} // namespace

class GprHeaderTest : public QObject
{
    Q_OBJECT

private slots:
    // === 实测格式完整头：解析成功 + 全字段正确 ===
    void validRealShapedHeaderParses();
    // === 缺任一必需键或数值非法 → isValid()==false ===
    void missingRequiredKeyOrBadValueIsInvalid();
    // === 未知键仅保留在 rawPairs；重复键后值覆盖前值 ===
    void unknownKeysAndDuplicateKeyHandling();
    // === 空白/注释/无分隔符行与 CRLF 行尾容忍解析 ===
    void whitespaceAndNoiseLinesTolerated();
    // === '=' 分隔符容忍（设计文档速记记法）===
    void equalsSeparatorTolerated();
    // === dtNs / depthForSample 两程换算数学（§5.4：dt≈0.2148 ns，v=0.1 全时窗约 5.5 m）===
    void derivedMathMatchesDesign();
    // === 文件不存在：加载失败且保持无效 ===
    void missingFileFailsToLoad();
    // === 真实数据 smoke：GPR_SAMPLE_DATA_DIR 首个 .rad（未定义宏或目录/文件缺失自动跳过）===
    void realDataSmoke();
};

// === 实测格式完整头：解析成功 + 全字段正确 ===
void GprHeaderTest::validRealShapedHeaderParses()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeRadFile(dir, QStringLiteral("valid.rad"), validRadLines());
    QVERIFY(!path.isEmpty());

    GPR::GprRadarHeader header;
    QVERIFY(header.loadFromFile(path));
    QVERIFY(header.isValid());
    QCOMPARE(header.samples(), 512);
    QVERIFY(qFuzzyCompare(header.timeWindowNs(), 110.0));
    QVERIFY(qFuzzyCompare(header.distanceIntervalM(), 0.02));
    // ANTENNAS 与实测主频矛盾，仅原样保留（gpr-display.md §5.4）
    QCOMPARE(header.antennaDescription(), QStringLiteral("800"));
    // 36 行含 1 个重复键（START POSITION）→ 35 个唯一键
    QCOMPARE(header.rawPairs().size(), 35);
    // 空格值按原文保留
    QCOMPARE(header.rawPairs().value(QStringLiteral("ANTENNA ORIENTATION")).toString(),
             QStringLiteral("NOT VALID FIELD"));
}

// === 缺任一必需键或数值非法 → isValid()==false ===
void GprHeaderTest::missingRequiredKeyOrBadValueIsInvalid()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // --- 缺键：按 "KEY:" 前缀过滤移除该键的全部行；文件仍可解析但内容无效 ---
    const QStringList requiredKeys = {
        QStringLiteral("SAMPLES"),
        QStringLiteral("TIMEWINDOW"),
        QStringLiteral("DISTANCE INTERVAL"),
    };
    for (const QString &key : requiredKeys) {
        const QString prefix = key + QLatin1Char(':');
        QStringList lines;
        for (const QString &line : validRadLines()) {
            if (!line.startsWith(prefix)) {
                lines.append(line);
            }
        }
        const QString path = writeRadFile(dir, QStringLiteral("missing-") + key + QStringLiteral(".rad"), lines);
        QVERIFY(!path.isEmpty());
        GPR::GprRadarHeader header;
        QVERIFY(header.loadFromFile(path));
        QVERIFY(!header.isValid());
        // 其余键仍保留在 rawPairs
        QVERIFY(header.rawPairs().contains(QStringLiteral("FREQUENCY")));
    }

    // --- 数值非法：必需键不可解析或退化（samples<=0 / timewindow<=0 / dx<0）→ 无效 ---
    struct BadValueCase {
        QString fileName;
        QString oldLine;
        QString newLine;
    };
    const BadValueCase badValueCases[] = {
        { QStringLiteral("bad-samples-text.rad"),
          QStringLiteral("SAMPLES:512"), QStringLiteral("SAMPLES:abc") },
        { QStringLiteral("bad-samples-zero.rad"),
          QStringLiteral("SAMPLES:512"), QStringLiteral("SAMPLES:0") },
        { QStringLiteral("bad-timewindow-zero.rad"),
          QStringLiteral("TIMEWINDOW:110.000"), QStringLiteral("TIMEWINDOW:0") },
        { QStringLiteral("bad-dx-negative.rad"),
          QStringLiteral("DISTANCE INTERVAL:0.02"), QStringLiteral("DISTANCE INTERVAL:-0.01") },
    };
    for (const BadValueCase &bad : badValueCases) {
        QStringList lines = validRadLines();
        lines.replaceInStrings(bad.oldLine, bad.newLine);
        const QString path = writeRadFile(dir, bad.fileName, lines);
        QVERIFY(!path.isEmpty());
        GPR::GprRadarHeader header;
        QVERIFY(header.loadFromFile(path));
        QVERIFY(!header.isValid());
    }

    // --- DISTANCE INTERVAL:0（时间触发采集，无道间距）按有效处理 ---
    {
        QStringList lines = validRadLines();
        lines.replaceInStrings(QStringLiteral("DISTANCE INTERVAL:0.02"),
                               QStringLiteral("DISTANCE INTERVAL:0"));
        const QString path = writeRadFile(dir, QStringLiteral("dx-zero.rad"), lines);
        QVERIFY(!path.isEmpty());
        GPR::GprRadarHeader header;
        QVERIFY(header.loadFromFile(path));
        QVERIFY(header.isValid());
        QCOMPARE(header.distanceIntervalM(), 0.0);
    }
}

// === 未知键仅保留在 rawPairs；重复键后值覆盖前值 ===
void GprHeaderTest::unknownKeysAndDuplicateKeyHandling()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // 未知键不影响合法性，仅出现在 rawPairs（值保留原文）
    {
        QStringList lines = validRadLines();
        lines.append(QStringLiteral("TOTAL DIMENSION:100"));
        lines.append(QStringLiteral("UNKNOWN KEY WITH SPACES:some value 42"));
        const QString path = writeRadFile(dir, QStringLiteral("unknown.rad"), lines);
        QVERIFY(!path.isEmpty());
        GPR::GprRadarHeader header;
        QVERIFY(header.loadFromFile(path));
        QVERIFY(header.isValid());
        QCOMPARE(header.samples(), 512);
        QCOMPARE(header.rawPairs().value(QStringLiteral("TOTAL DIMENSION")).toString(),
                 QStringLiteral("100"));
        QCOMPARE(header.rawPairs().value(QStringLiteral("UNKNOWN KEY WITH SPACES")).toString(),
                 QStringLiteral("some value 42"));
    }

    // 重复键：后值覆盖前值（实测文件 START POSITION 出现两次）
    {
        QStringList lines = validRadLines();
        lines.append(QStringLiteral("SAMPLES:1024"));
        const QString path = writeRadFile(dir, QStringLiteral("dup-key.rad"), lines);
        QVERIFY(!path.isEmpty());
        GPR::GprRadarHeader header;
        QVERIFY(header.loadFromFile(path));
        QVERIFY(header.isValid());
        QCOMPARE(header.samples(), 1024);
    }
}

// === 空白/注释/无分隔符行与 CRLF 行尾容忍解析 ===
void GprHeaderTest::whitespaceAndNoiseLinesTolerated()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // 行首/行尾空格、制表符、键值两侧空白、注释行、空行、无分隔符行，整体 CRLF 行尾
    const QStringList lines = {
        QStringLiteral("# MALA .rad synthetic fixture"),
        QString(),
        QStringLiteral("  SAMPLES : 512  "),
        QStringLiteral("\tFREQUENCY:\t160\t"),
        QStringLiteral("; semicolon comment"),
        QStringLiteral("TIMEWINDOW:  110.000"),
        QStringLiteral(" DISTANCE INTERVAL :\t0.02"),
        QStringLiteral("GARBAGE LINE WITHOUT SEPARATOR"),
        QStringLiteral("ANTENNAS:800"),
        QString(),
    };
    QByteArray content;
    for (const QString &line : lines) {
        content += line.toUtf8();
        content += "\r\n";
    }
    const QString path = writeRadFile(dir, QStringLiteral("whitespace.rad"), content);
    QVERIFY(!path.isEmpty());

    GPR::GprRadarHeader header;
    QVERIFY(header.loadFromFile(path));
    QVERIFY(header.isValid());
    QCOMPARE(header.samples(), 512);
    QVERIFY(qFuzzyCompare(header.timeWindowNs(), 110.0));
    QVERIFY(qFuzzyCompare(header.distanceIntervalM(), 0.02));
    QCOMPARE(header.antennaDescription(), QStringLiteral("800"));
    // 注释/空行/无分隔符行不进入 rawPairs；有效键共 5 个
    QCOMPARE(header.rawPairs().size(), 5);
    QVERIFY(!header.rawPairs().contains(QStringLiteral("GARBAGE LINE WITHOUT SEPARATOR")));
}

// === '=' 分隔符容忍（设计文档速记记法）===
void GprHeaderTest::equalsSeparatorTolerated()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // gpr-display.md §5.4 速记采用 KEY=VALUE 记法；解析器按首个 ':' 或 '=' 分隔
    const QStringList lines = {
        QStringLiteral("SAMPLES=512"),
        QStringLiteral("TIMEWINDOW=110.000"),
        QStringLiteral("DISTANCE INTERVAL=0.02"),
    };
    const QString path = writeRadFile(dir, QStringLiteral("equals.rad"), lines);
    QVERIFY(!path.isEmpty());

    GPR::GprRadarHeader header;
    QVERIFY(header.loadFromFile(path));
    QVERIFY(header.isValid());
    QCOMPARE(header.samples(), 512);
    QVERIFY(qFuzzyCompare(header.timeWindowNs(), 110.0));
    QVERIFY(qFuzzyCompare(header.distanceIntervalM(), 0.02));
    // ANTENNAS 非必需键：缺失时 antennaDescription() 为空但头仍有效
    QCOMPARE(header.antennaDescription(), QString());
}

// === dtNs / depthForSample 两程换算数学（§5.4：dt≈0.2148 ns，v=0.1 全时窗约 5.5 m）===
void GprHeaderTest::derivedMathMatchesDesign()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeRadFile(dir, QStringLiteral("math.rad"), validRadLines());
    QVERIFY(!path.isEmpty());

    GPR::GprRadarHeader header;
    QVERIFY(header.loadFromFile(path));
    QVERIFY(header.isValid());

    // 采样间隔 = 110 / 512 ≈ 0.2148 ns
    QVERIFY(qFuzzyCompare(header.dtNs(), 110.0 / 512.0));
    // 首样本深度为 0
    QCOMPARE(header.depthForSample(0), 0.0);
    // 半时窗深度：v=0.1 m/ns 两程换算 → 0.1*55/2 = 2.75 m
    QVERIFY(qFuzzyCompare(header.depthForSample(256), 2.75));
    // 全时窗深度 ≈ 5.5 m（§5.4 预研结论）
    QVERIFY(qFuzzyCompare(header.depthForSample(512), 5.5));
    // 自定义波速 v=0.13 m/ns：0.13*110/2 = 7.15 m
    QVERIFY(qFuzzyCompare(header.depthForSample(512, 0.13), 7.15));
    // 负样本索引按 0 处理
    QCOMPARE(header.depthForSample(-3), 0.0);

    // 默认构造（无效头）：dtNs 不做除零，depthForSample 恒为 0
    GPR::GprRadarHeader invalidHeader;
    QCOMPARE(invalidHeader.dtNs(), 0.0);
    QCOMPARE(invalidHeader.depthForSample(100), 0.0);
}

// === 文件不存在：加载失败且保持无效 ===
void GprHeaderTest::missingFileFailsToLoad()
{
    GPR::GprRadarHeader header;
    QVERIFY(!header.loadFromFile(QStringLiteral("/nonexistent/gpr_header_test_missing.rad")));
    QVERIFY(!header.isValid());
    QCOMPARE(header.samples(), 0);
    QVERIFY(header.antennaDescription().isEmpty());
    QVERIFY(header.rawPairs().isEmpty());
}

// === 真实数据 smoke：GPR_SAMPLE_DATA_DIR 首个 .rad（未定义宏或目录/文件缺失自动跳过）===
void GprHeaderTest::realDataSmoke()
{
#ifdef GPR_SAMPLE_DATA_DIR
    const QDir sampleDir(QStringLiteral(GPR_SAMPLE_DATA_DIR));
    if (!sampleDir.exists()) {
        QSKIP("GPR_SAMPLE_DATA_DIR 目录不存在，跳过真实数据 smoke 测试");
    }
    const QStringList radFiles = sampleDir.entryList(
        QStringList() << QStringLiteral("*.rad"), QDir::Files, QDir::Name);
    if (radFiles.isEmpty()) {
        QSKIP("GPR_SAMPLE_DATA_DIR 目录无 .rad 文件，跳过真实数据 smoke 测试");
    }

    // 首个 .rad（按文件名排序）：样例数据为泰安测试 2025-11-13 MALA 格式
    GPR::GprRadarHeader header;
    QVERIFY(header.loadFromFile(sampleDir.filePath(radFiles.first())));
    QVERIFY(header.isValid());
    // §5.4 已验证事实：SAMPLES=512、TIMEWINDOW=110 ns、DISTANCE INTERVAL=0.02 m
    QCOMPARE(header.samples(), 512);
    QVERIFY(qFuzzyCompare(header.timeWindowNs(), 110.0));
    QVERIFY(qFuzzyCompare(header.distanceIntervalM(), 0.02));
    QVERIFY(header.dtNs() > 0.0);
    QVERIFY(!header.rawPairs().isEmpty());
#else
    QSKIP("未定义 GPR_SAMPLE_DATA_DIR，跳过真实数据 smoke 测试");
#endif
}

QTEST_GUILESS_MAIN(GprHeaderTest)
#include "gpr_header_test.moc"
