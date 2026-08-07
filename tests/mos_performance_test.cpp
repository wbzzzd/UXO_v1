// MOS P0 性能/确定性门禁 standalone 测试可执行入口。
// CLI：--scenario happy|stale-revision|threshold-regression|seed-mismatch|no-solution
//      --output <json>  --max-p99-ms <value>   （以上均支持 --key value 与 --key=value）
//      --child-measure=W:S  --child-hash=N  （私有子进程模式，仅 = 形式）
// 所有数据为合成本地 fixture 语义，不接真实设备/网络/数据库。
#include "mos_performance_helpers.h"
#include "MainWindow/MosPlanningController.h"
#include <QApplication>
#include <QJsonDocument>
#include <QProcess>
#include <QSignalSpy>
#include <QString>
#include <QStringList>

// 解析 CLI 选项值：支持 --key value 与 --key=value 两种形式；未命中返回 defaultValue
static QString cliOption(const QStringList &args, const QString &key,
                         const QString &defaultValue)
{
    const QString eqForm = key + QStringLiteral("=");
    for (int i = 0; i < args.size(); ++i) {
        const auto &a = args.at(i);
        if (a == key && i + 1 < args.size()) return args.at(i + 1);
        if (a.startsWith(eqForm)) return a.mid(eqForm.length());
    }
    return defaultValue;
}

// 解析 --max-p99-ms 默认 16（P0 Approved 核心 p99 阈值）
static qint64 parseMaxP99(const QStringList &args)
{
    return cliOption(args, QStringLiteral("--max-p99-ms"),
                     QStringLiteral("16")).toLongLong();
}

// 解析 --output 路径
static QString parseOutput(const QStringList &args)
{
    return cliOption(args, QStringLiteral("--output"),
                     QStringLiteral("mos_performance.json"));
}

// 解析 --scenario 默认 happy
static QString parseScenario(const QStringList &args)
{
    return cliOption(args, QStringLiteral("--scenario"),
                     QStringLiteral("happy"));
}

namespace MosPerf {

// === stale-revision：反向完成被忽略，业务不变，退出 0 ===
int runStaleRevision(const QString &outputPath)
{
    using namespace Core::MOS;
    MosPlanningController controller;
    controller.worker()->disconnect(&controller);
    const auto params = envelopeParams();
    const auto obs1 = MosFixtureGenerator::generate(params, envelopeGenParams(), 42);
    const auto obs2 = MosFixtureGenerator::generate(params, envelopeGenParams(), 43);
    QSignalSpy spy(controller.worker(), &MosReplanWorker::replanCompleted);
    controller.requestReplan(obs1, params);
    controller.requestReplan(obs2, params);
    const auto c1 = spy.at(0).at(0).value<MosReplanCompletion>();
    const auto c2 = spy.at(1).at(0).value<MosReplanCompletion>();
    const auto d2 = controller.completeReplan(c2);
    const auto d1 = controller.completeReplan(c1);
    const bool ok = d2 == MosCompletionDisposition::Committed
                    && d1 == MosCompletionDisposition::IgnoredStale;
    QJsonObject o;
    o["status"] = ok ? QStringLiteral("ok") : QStringLiteral("failedAssertion");
    o["staleDisposition"] = static_cast<int>(d1);
    o["committedDisposition"] = static_cast<int>(d2);
    o["assertion"] = QStringLiteral("staleRevisionIgnored");
    writeJson(outputPath, o);
    return ok ? 0 : 1;
}

// === no-solution：合法无解被接受，tier 0 矩形无效，退出 0 ===
int runNoSolution(const QString &outputPath)
{
    using namespace Core::MOS;
    MosPlanningController controller;
    const auto params = noSolutionParams();  // L=500,W=100,expand=10,minLength=500
    MosObstacleSet blocked;
    MosCrater c; c.id = QStringLiteral("blocker"); c.visibleRadius = 100.0;
    c.x = 250; c.y = 0; c.threat = MosThreatLevel::High; c.influenceRadius = 1000.0;
    blocked.craters.append(c);
    controller.requestReplan(blocked, params);
    const auto snap = controller.snapshot();
    const bool ok = snap.hasResult && snap.result.accepted
                    && !snap.result.tiers.at(0).rectangle.valid
                    && snap.result.tiers.at(0).rectangle.reason == MosPlannerReason::NoFeasibleRectangle;
    QJsonObject o;
    o["status"] = ok ? QStringLiteral("ok") : QStringLiteral("failedAssertion");
    o["accepted"] = snap.result.accepted;
    o["tier0Valid"] = snap.result.tiers.at(0).rectangle.valid;
    o["assertion"] = QStringLiteral("legalNoSolutionAccepted");
    writeJson(outputPath, o);
    return ok ? 0 : 1;
}

// === threshold-regression：--max-p99-ms 0 强制 p99 断言失败，退出非零 ===
int runThresholdRegression(const QString &outputPath, qint64 maxP99Ms)
{
    const QString exec = QCoreApplication::applicationFilePath();
    QProcess p;
    p.start(exec, {QStringLiteral("--child-measure=5:20")}, QIODevice::ReadOnly);
    if (!p.waitForFinished(60000) || p.exitCode() != 0) {
        writeJson(outputPath, failJson(QStringLiteral("childMeasure"),
                                       QStringLiteral("子进程失败")));
        return 1;
    }
    const auto o = QJsonDocument::fromJson(p.readAllStandardOutput().trimmed()).object();
    const double p99Ms = o["p99"].toVariant().toLongLong() / 1e6;
    if (maxP99Ms <= 0 && p99Ms > 0.0) {  // 0ms 阈值下任何实测 p99 都违反
        QJsonObject extra; extra["p99Ms"] = p99Ms; extra["maxP99Ms"] = qint64(maxP99Ms);
        writeJson(outputPath, failJson(QStringLiteral("p99Threshold"),
                                       QStringLiteral("p99 超过 0ms 阈值"), extra));
        return 1;
    }
    QJsonObject ok; ok["status"] = QStringLiteral("ok"); ok["p99Ms"] = p99Ms;
    writeJson(outputPath, ok);
    return 0;
}

// === seed-mismatch：不同种子哈希不同被断言为相等，退出非零 ===
int runSeedMismatch(const QString &outputPath)
{
    const QString exec = QCoreApplication::applicationFilePath();
    QProcess p1, p2;
    p1.start(exec, {QStringLiteral("--child-hash=42")}, QIODevice::ReadOnly);
    p2.start(exec, {QStringLiteral("--child-hash=43")}, QIODevice::ReadOnly);
    p1.waitForFinished(30000); p2.waitForFinished(30000);
    const auto h1 = p1.readAllStandardOutput().trimmed();
    const auto h2 = p2.readAllStandardOutput().trimmed();
    if (h1 != h2) {  // 注入失败：断言不同种子应产生相同哈希
        QJsonObject extra;
        extra["seed1"] = 42; extra["seed2"] = 43;
        extra["hash1"] = QString::fromLatin1(h1);
        extra["hash2"] = QString::fromLatin1(h2);
        writeJson(outputPath, failJson(QStringLiteral("seedDeterminism"),
                                       QStringLiteral("不同种子产生不同哈希"), extra));
        return 1;
    }
    QJsonObject ok; ok["status"] = QStringLiteral("ok");
    writeJson(outputPath, ok);
    return 0;
}

} // namespace MosPerf

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QStringList args = app.arguments();

    // 私有子进程模式：--child-measure=W:S
    for (int i = 0; i < args.size(); ++i) {
        const auto &a = args.at(i);
        if (a.startsWith(QStringLiteral("--child-measure="))) {
            const auto parts = a.mid(QStringLiteral("--child-measure=").length())
                                   .split(QChar(':'));
            if (parts.size() != 2) return 2;
            return MosPerf::runChildMeasure(parts.at(0).toInt(), parts.at(1).toInt());
        }
        if (a.startsWith(QStringLiteral("--child-hash="))) {
            return MosPerf::runChildHash(
                a.mid(QStringLiteral("--child-hash=").length()).toInt());
        }
    }

    // 父进程场景分发
    const QString scenario = parseScenario(args);
    const QString output = parseOutput(args);
    const qint64 maxP99 = parseMaxP99(args);

    if (scenario == QStringLiteral("happy")) {
        return MosPerf::runHappyParent(output, maxP99);
    }
    if (scenario == QStringLiteral("stale-revision")) {
        return MosPerf::runStaleRevision(output);
    }
    if (scenario == QStringLiteral("no-solution")) {
        return MosPerf::runNoSolution(output);
    }
    if (scenario == QStringLiteral("threshold-regression")) {
        return MosPerf::runThresholdRegression(output, maxP99);
    }
    if (scenario == QStringLiteral("seed-mismatch")) {
        return MosPerf::runSeedMismatch(output);
    }

    // 未知场景：写失败 JSON 并退出非零
    MosPerf::writeJson(output, MosPerf::failJson(
        QStringLiteral("unknownScenario"),
        QStringLiteral("未知场景：%1").arg(scenario)));
    return 2;
}
