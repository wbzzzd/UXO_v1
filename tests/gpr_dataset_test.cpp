// GPR 数据集目录扫描测试（REQ-013）：合成目录树覆盖 H/L 配对聚合、孤儿文件剔除、
// 命名容忍（大小写/分隔符变体）、重复条目名称序取首个、.cor 选择与缺失通道报告；
// 末尾真实数据 smoke 由 GPR_SAMPLE_DATA_DIR 编译宏驱动，目录缺失时自动跳过。

#include "GPR/GprDataset.h"

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace {

// 在临时目录写占位文件（扫描只关心存在性与文件名），返回绝对路径；失败返回空串
QString touchFile(const QTemporaryDir &dir, const QString &fileName)
{
    const QString path = QDir(dir.path()).absoluteFilePath(fileName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return QString();
    }
    file.write("x", 1);
    return path;
}

} // namespace

class GprDatasetTest : public QObject
{
    Q_OBJECT

private slots:
    // === 合成目录树：配对聚合、孤儿剔除、缺失通道报告、.cor 选择 ===
    void scanSyntheticTree();
    // === 命名容忍：小写频段/通道、'-'/'.' 分隔通道、重复条目名称序取首个 ===
    void scanTolerantNaming();
    // === 不存在路径与文件路径 → 拒绝且状态复位 ===
    void scanRejectsInvalidRoot();
    // === 未命中查询返回无效条目（含未扫描对象）===
    void entryForUnknownReturnsInvalid();
    // === 真实数据 smoke：H/L 各 16 通道齐全（缺失自动跳过）===
    void realDataSmoke();
};

// === 合成目录树：配对聚合、孤儿剔除、缺失通道报告、.cor 选择 ===
void GprDatasetTest::scanSyntheticTree()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // H 组 3 对完整文件组
    const QStringList hPairs = {
        QStringLiteral("demo_H_T1R1_001"),
        QStringLiteral("demo_H_T1R2_001"),
        QStringLiteral("demo_H_T2R2_001"),
    };
    for (const QString &base : hPairs) {
        QVERIFY(!touchFile(dir, base + QStringLiteral(".rad")).isEmpty());
        QVERIFY(!touchFile(dir, base + QStringLiteral(".rd3")).isEmpty());
    }
    // 孤儿文件：只有 .rd3 的 T4R4 与只有 .rad 的 T5R5，均不成条目
    QVERIFY(!touchFile(dir, QStringLiteral("demo_H_T4R4_001.rd3")).isEmpty());
    QVERIFY(!touchFile(dir, QStringLiteral("demo_H_T5R5_001.rad")).isEmpty());
    // 非 GPR 文件与无法解析名称的 .rad：整体跳过
    QVERIFY(!touchFile(dir, QStringLiteral("notes.txt")).isEmpty());
    QVERIFY(!touchFile(dir, QStringLiteral("random.rad")).isEmpty());
    // L 组 1 对
    QVERIFY(!touchFile(dir, QStringLiteral("demo_L_T1R1_001.rad")).isEmpty());
    QVERIFY(!touchFile(dir, QStringLiteral("demo_L_T1R1_001.rd3")).isEmpty());
    // RTK 轨迹文件（实测命名 <prefix>_001_RTK.cor）
    QVERIFY(!touchFile(dir, QStringLiteral("demo_001_RTK.cor")).isEmpty());

    GPR::GprDataset dataset;
    QVERIFY(dataset.scan(dir.path()));
    QCOMPARE(dataset.rootDir(), QDir(dir.path()).absolutePath());
    QCOMPARE(dataset.bands(), QStringList({ QStringLiteral("H"), QStringLiteral("L") }));
    QCOMPARE(dataset.channels(QStringLiteral("H")),
             QStringList({ QStringLiteral("T1R1"), QStringLiteral("T1R2"), QStringLiteral("T2R2") }));
    QCOMPARE(dataset.channels(QStringLiteral("L")), QStringList({ QStringLiteral("T1R1") }));

    // 缺失通道：H 缺 13（16-3），L 缺 15（16-1），共 28；孤儿不计入
    const QStringList missing = dataset.missingChannels();
    QCOMPARE(missing.size(), 28);
    QVERIFY(missing.contains(QStringLiteral("H/T3R3")));
    QVERIFY(missing.contains(QStringLiteral("L/T1R2")));
    QVERIFY(!missing.contains(QStringLiteral("H/T1R1")));

    // .cor：唯一候选且为 _rtk 结尾
    QVERIFY(dataset.corPath().endsWith(QStringLiteral("demo_001_RTK.cor")));

    // 条目查询：完整对有效，孤儿无效
    const GPR::GprDatasetEntry entry = dataset.entryFor(QStringLiteral("H"), QStringLiteral("T1R1"));
    QVERIFY(entry.isValid());
    QCOMPARE(entry.band, QStringLiteral("H"));
    QCOMPARE(entry.channel, QStringLiteral("T1R1"));
    QVERIFY(entry.rd3Path.endsWith(QStringLiteral(".rd3")));
    QVERIFY(entry.radPath.endsWith(QStringLiteral(".rad")));
    QVERIFY(!dataset.entryFor(QStringLiteral("H"), QStringLiteral("T4R4")).isValid());
    QVERIFY(!dataset.entryFor(QStringLiteral("H"), QStringLiteral("T5R5")).isValid());
}

// === 命名容忍：小写频段/通道、'-'/'.' 分隔通道、重复条目名称序取首个 ===
void GprDatasetTest::scanTolerantNaming()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // 小写频段与通道
    QVERIFY(!touchFile(dir, QStringLiteral("demo_h_t1r1_001.rad")).isEmpty());
    QVERIFY(!touchFile(dir, QStringLiteral("demo_h_t1r1_001.rd3")).isEmpty());
    // '-' 分隔通道
    QVERIFY(!touchFile(dir, QStringLiteral("demo_H_T3-R3_002.rad")).isEmpty());
    QVERIFY(!touchFile(dir, QStringLiteral("demo_H_T3-R3_002.rd3")).isEmpty());
    // '.' 分隔通道：与上一对同归一化为 T3R3（重复条目，名称序取首个）
    QVERIFY(!touchFile(dir, QStringLiteral("demo_H_t3.r3_003.rad")).isEmpty());
    QVERIFY(!touchFile(dir, QStringLiteral("demo_H_t3.r3_003.rd3")).isEmpty());
    // 小写 _rtk 结尾 .cor
    QVERIFY(!touchFile(dir, QStringLiteral("demo_002_rtk.cor")).isEmpty());

    GPR::GprDataset dataset;
    QVERIFY(dataset.scan(dir.path()));
    QCOMPARE(dataset.bands(), QStringList({ QStringLiteral("H") }));
    // 通道按期望清单序：T1R1 在 T3R3 之前
    QCOMPARE(dataset.channels(QStringLiteral("H")),
             QStringList({ QStringLiteral("T1R1"), QStringLiteral("T3R3") }));

    // 重复条目取名称序首个（"demo_H_T3-R3_002" 先于 "demo_H_t3.r3_003"）
    const GPR::GprDatasetEntry entry = dataset.entryFor(QStringLiteral("H"), QStringLiteral("T3R3"));
    QVERIFY(entry.isValid());
    QVERIFY(entry.rd3Path.contains(QStringLiteral("T3-R3_002")));

    // 查询侧同样归一化（小写/原样 token）
    QVERIFY(dataset.entryFor(QStringLiteral("h"), QStringLiteral("t1r1")).isValid());
    QVERIFY(dataset.corPath().endsWith(QStringLiteral("demo_002_rtk.cor")));
}

// === 不存在路径与文件路径 → 拒绝且状态复位 ===
void GprDatasetTest::scanRejectsInvalidRoot()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // 不存在目录
    {
        GPR::GprDataset dataset;
        QVERIFY(!dataset.scan(QDir(dir.path()).absoluteFilePath(QStringLiteral("missing_dir"))));
        QVERIFY(dataset.bands().isEmpty());
        QVERIFY(dataset.missingChannels().isEmpty());
        QVERIFY(dataset.corPath().isEmpty());
        QVERIFY(dataset.rootDir().isEmpty());
    }
    // 文件路径而非目录
    {
        const QString filePath = touchFile(dir, QStringLiteral("a_file.txt"));
        QVERIFY(!filePath.isEmpty());
        GPR::GprDataset dataset;
        QVERIFY(!dataset.scan(filePath));
        QVERIFY(dataset.bands().isEmpty());
    }
    // 目录存在但无任何可配对文件 → 同样拒绝
    {
        GPR::GprDataset dataset;
        QVERIFY(!dataset.scan(dir.path()));
        QVERIFY(dataset.bands().isEmpty());
    }
}

// === 未命中查询返回无效条目（含未扫描对象）===
void GprDatasetTest::entryForUnknownReturnsInvalid()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(!touchFile(dir, QStringLiteral("demo_H_T1R1_001.rad")).isEmpty());
    QVERIFY(!touchFile(dir, QStringLiteral("demo_H_T1R1_001.rd3")).isEmpty());

    GPR::GprDataset dataset;
    QVERIFY(dataset.scan(dir.path()));
    // 未出现的通道 / 非法频段 / 空 token
    QVERIFY(!dataset.entryFor(QStringLiteral("H"), QStringLiteral("T9R9")).isValid());
    QVERIFY(!dataset.entryFor(QStringLiteral("X"), QStringLiteral("T1R1")).isValid());
    QVERIFY(!dataset.entryFor(QString(), QString()).isValid());

    // 从未扫描的对象：一切查询均为空/无效
    GPR::GprDataset fresh;
    QVERIFY(!fresh.entryFor(QStringLiteral("H"), QStringLiteral("T1R1")).isValid());
    QVERIFY(fresh.bands().isEmpty());
    QVERIFY(fresh.rootDir().isEmpty());
}

// === 真实数据 smoke：H/L 各 16 通道齐全（缺失自动跳过）===
void GprDatasetTest::realDataSmoke()
{
#ifdef GPR_SAMPLE_DATA_DIR
    const QDir sampleDir(QStringLiteral(GPR_SAMPLE_DATA_DIR));
    if (!sampleDir.exists()) {
        QSKIP("GPR_SAMPLE_DATA_DIR 目录不存在，跳过真实数据 smoke 测试");
    }

    // §5.4 已验证事实：H/L 两组 × 16 通道（TnRn + TnR(n+1)，n=1..8）+ RTK .cor
    GPR::GprDataset dataset;
    QVERIFY(dataset.scan(sampleDir.path()));
    QCOMPARE(dataset.bands(), QStringList({ QStringLiteral("H"), QStringLiteral("L") }));
    QCOMPARE(dataset.channels(QStringLiteral("H")).size(), 16);
    QCOMPARE(dataset.channels(QStringLiteral("L")).size(), 16);
    QVERIFY(dataset.missingChannels().isEmpty());
    QVERIFY(!dataset.corPath().isEmpty());
    QVERIFY(dataset.corPath().endsWith(QStringLiteral(".cor"), Qt::CaseInsensitive));
#else
    QSKIP("未定义 GPR_SAMPLE_DATA_DIR，跳过真实数据 smoke 测试");
#endif
}

QTEST_GUILESS_MAIN(GprDatasetTest)
#include "gpr_dataset_test.moc"
