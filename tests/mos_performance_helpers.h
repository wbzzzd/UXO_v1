#ifndef MOS_PERFORMANCE_HELPERS_H
#define MOS_PERFORMANCE_HELPERS_H
// MOS P0 性能/确定性门禁共享助手。全部为合成本地 fixture 语义，非真实跑道/弹坑/作业参数。
#include "Core/MOS/MosTypes.h"
#include "Core/MOS/MosPlanner.h"
#include "Core/MOS/MosFixtureGenerator.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QSysInfo>
#include <QTextStream>
#include <QVector>
#include <algorithm>

namespace MosPerf {

// P0 Approved 最大包络参数（边界值：L=6000, W=100, step=0.5, tiers=5）
inline Core::MOS::MosRunwayParams envelopeParams()
{
    Core::MOS::MosRunwayParams p;
    p.L = 6000.0; p.W = 100.0; p.K = 1.5; p.expand = 1.5;
    p.step = 0.5; p.minLength = 460.0; p.minWidth = 15.0;
    p.backfill = 50.0; p.uxoHours = 8.0; p.tiers = 5;
    return p;
}

// 最大包络生成器：8 弹坑 + 5 UXO = 13 障碍物（N 上界）
inline Core::MOS::MosGeneratorParams envelopeGenParams()
{
    Core::MOS::MosGeneratorParams g;
    g.craterCount = 8; g.craterRMin = 3; g.craterRMax = 6;
    g.uxoCount = 5; g.uxoYMin = 10.0; g.uxoYMax = 50.0;
    return g;
}

// no-solution 场景专用小跑道包络：L=500,W=100,expand=10,minLength=500,minWidth=100
// 其余参数与最大包络保持一致；用于合法无解断言（tier 0 无可行矩形）。
inline Core::MOS::MosRunwayParams noSolutionParams()
{
    Core::MOS::MosRunwayParams p;
    p.L = 500.0; p.W = 100.0; p.K = 1.5; p.expand = 10.0;
    p.step = 0.5; p.minLength = 500.0; p.minWidth = 100.0;
    p.backfill = 50.0; p.uxoHours = 8.0; p.tiers = 5;
    return p;
}

// 规范哈希：覆盖 fixture 字节 + 全档位矩形/估算/已修复 ID（非仅 fixture）
inline QByteArray canonicalHash(const Core::MOS::MosObstacleSet &obs,
                                const Core::MOS::MosProgressiveResult &result)
{
    QJsonObject doc;
    doc["fixture"] = Core::MOS::serializeObstacleSet(obs);
    QJsonArray tiers;
    for (const auto &t : result.tiers) {
        QJsonObject to;
        QJsonArray ids;
        for (const auto &id : t.repairedIds) ids.append(id);
        to["repairedIds"] = ids;
        to["valid"] = t.rectangle.valid;
        to["reason"] = static_cast<int>(t.rectangle.reason);
        to["xStart"] = t.rectangle.xStart; to["xEnd"] = t.rectangle.xEnd;
        to["yStart"] = t.rectangle.yStart; to["yEnd"] = t.rectangle.yEnd;
        to["length"] = t.rectangle.length; to["width"] = t.rectangle.width;
        to["area"] = t.rectangle.area;
        to["bv"] = t.estimate.totalBackfillVolume; to["bh"] = t.estimate.backfillHours;
        to["uh"] = t.estimate.uxoHours; to["th"] = t.estimate.totalHours;
        to["diff"] = static_cast<int>(t.estimate.difficulty);
        tiers.append(to);
    }
    doc["tiers"] = tiers;
    doc["accepted"] = result.accepted;
    doc["reason"] = static_cast<int>(result.reason);
    return QCryptographicHash::hash(
        QJsonDocument(doc).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex();
}

// 百分位（输入须已升序）
inline qint64 percentile(const QVector<qint64> &sorted, double pct)
{
    if (sorted.isEmpty()) return 0;
    int idx = int(double(sorted.size()) * pct / 100.0);
    if (idx < 0) idx = 0;
    if (idx >= sorted.size()) idx = sorted.size() - 1;
    return sorted[idx];
}

// 当前进程 RSS（Linux /proc/self/status VmRSS，字节）
inline qint64 rssBytes()
{
    QFile f(QStringLiteral("/proc/self/status"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;
    for (const auto &line : QString::fromUtf8(f.readAll()).split('\n')) {
        if (line.startsWith("VmRSS:")) {
            const auto parts = line.split(QChar(' '), Qt::SkipEmptyParts);
            if (parts.size() >= 2) return parts.at(1).toLongLong() * 1024;
            break;
        }
    }
    return -1;
}

// 系统总内存（Linux /proc/meminfo MemTotal，字节）
inline qint64 totalRamBytes()
{
    QFile f(QStringLiteral("/proc/meminfo"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;
    for (const auto &line : QString::fromUtf8(f.readAll()).split('\n')) {
        if (line.startsWith("MemTotal:")) {
            const auto parts = line.split(QChar(' '), Qt::SkipEmptyParts);
            if (parts.size() >= 2) return parts.at(1).toLongLong() * 1024;
            break;
        }
    }
    return -1;
}

// 环境元数据：编译器/Qt/CPU/平台/显示后端/构建类型/git 提交/总内存
inline QJsonObject envMetadata()
{
    QJsonObject e;
    e["qtVersion"] = QString::fromLatin1(QT_VERSION_STR);
#ifdef __GNUC__
    e["compiler"] = QStringLiteral("gcc %1.%2.%3")
                        .arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__);
#else
    e["compiler"] = QStringLiteral("unknown");
#endif
    e["cpuArch"] = QSysInfo::currentCpuArchitecture();
    e["prettyName"] = QSysInfo::prettyProductName();
    e["displayBackend"] = QString::fromUtf8(qgetenv("QT_QPA_PLATFORM"));
#ifdef MOS_BUILD_TYPE
    e["buildType"] = QStringLiteral(MOS_BUILD_TYPE);
#else
    e["buildType"] = QStringLiteral("unknown");
#endif
#ifdef MOS_GIT_COMMIT
    e["gitCommit"] = QStringLiteral(MOS_GIT_COMMIT);
#else
    e["gitCommit"] = QStringLiteral("unknown");
#endif
    e["totalRamBytes"] = totalRamBytes();
    return e;
}

// 写 JSON 文件
inline bool writeJson(const QString &path, const QJsonObject &obj)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    f.close();
    return f.error() == QFile::NoError;
}

// 失败断言 JSON（用于 threshold-regression/seed-mismatch 退出非零）
inline QJsonObject failJson(const QString &assertion, const QString &message,
                            const QJsonObject &extra = {})
{
    QJsonObject o;
    o["status"] = QStringLiteral("failedAssertion");
    o["assertion"] = assertion;
    o["message"] = message;
    for (auto it = extra.begin(); it != extra.end(); ++it) o[it.key()] = it.value();
    return o;
}

// 场景函数声明（实现在 mos_performance_scenarios.cpp）
int runChildMeasure(int warmup, int samples);
int runChildHash(int seed);
int runHappyParent(const QString &outputPath, qint64 maxP99Ms);
int runStaleRevision(const QString &outputPath);
int runNoSolution(const QString &outputPath);
int runThresholdRegression(const QString &outputPath, qint64 maxP99Ms);
int runSeedMismatch(const QString &outputPath);

} // namespace MosPerf

#endif // MOS_PERFORMANCE_HELPERS_H
