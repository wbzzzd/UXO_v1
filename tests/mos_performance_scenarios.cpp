// MOS P0 性能/确定性场景实现：所有数据为合成本地 fixture 语义，非真实跑道/弹坑/作业参数。
// 子进程模式经隐藏 CLI 触发；父进程通过 QProcess 拉起全新子进程收集证据。
#include "mos_performance_helpers.h"
#include "MainWindow/DecisionView.h"
#include "MainWindow/MainWindow.h"
#include "MainWindow/MosPlanningController.h"
#include <QAbstractButton>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QProcess>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <QtTest>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace MosPerf {

// === 子进程：测量核心 replan 时延，输出紧凑 JSON 到 stdout ===
int runChildMeasure(int warmup, int samples)
{
    const auto params = envelopeParams();
    const auto gen = envelopeGenParams();
    const auto obs = Core::MOS::MosFixtureGenerator::generate(params, gen, 42);
    const qint64 rssBaseline = rssBytes();  // 空 harness 基线（QCoreApplication 已初始化）
    for (int i = 0; i < warmup; ++i)
        Core::MOS::MosPlanner::planProgressive(obs, params);
    const qint64 rssAfterWarmup = rssBytes();
    QVector<qint64> times; times.reserve(samples);
    for (int i = 0; i < samples; ++i) {
        QElapsedTimer t; t.start();
        Core::MOS::MosPlanner::planProgressive(obs, params);
        times.append(t.nsecsElapsed());
    }
    const qint64 rssAfterSamples = rssBytes();
    const auto result = Core::MOS::MosPlanner::planProgressive(obs, params);
    std::sort(times.begin(), times.end());
    QJsonObject o;
    o["p50"] = percentile(times, 50);
    o["p95"] = percentile(times, 95);
    o["p99"] = percentile(times, 99);
    o["max"] = times.last();
    o["warmup"] = warmup;
    o["samples"] = samples;
    o["hash"] = QString::fromLatin1(canonicalHash(obs, result));
    o["tempDelta"] = rssAfterSamples - rssBaseline;
    o["postWarmupGrowth"] = rssAfterSamples - rssAfterWarmup;
    o["memoryAvailable"] = (rssBaseline >= 0 && rssAfterSamples >= 0);
    std::fprintf(stdout, "%s\n",
                 QJsonDocument(o).toJson(QJsonDocument::Compact).constData());
    return 0;
}

// === 子进程：计算给定种子的规范哈希，输出至 stdout ===
int runChildHash(int seed)
{
    const auto params = envelopeParams();
    const auto gen = envelopeGenParams();
    const auto obs = Core::MOS::MosFixtureGenerator::generate(params, gen, qint32(seed));
    const auto result = Core::MOS::MosPlanner::planProgressive(obs, params);
    std::fprintf(stdout, "%s\n", canonicalHash(obs, result).constData());
    return 0;
}

// === 父进程 happy：5 测量进程 + 100 哈希进程 + 点击到渲染 + 心跳 ===
int runHappyParent(const QString &outputPath, qint64 maxP99Ms)
{
    const QString exec = QCoreApplication::applicationFilePath();
    // 5 个全新测量进程：每进程 20 预热 + 200 样本 = 100 预热 + 1000 样本
    QJsonArray procs;
    for (int i = 0; i < 5; ++i) {
        QProcess p;
        p.start(exec, {QStringLiteral("--child-measure=20:200")}, QIODevice::ReadOnly);
        if (!p.waitForFinished(120000) || p.exitCode() != 0)
            return 1 | int(!writeJson(outputPath, failJson(
                QStringLiteral("childMeasure"), QStringLiteral("子进程 %1 失败").arg(i))));
        procs.append(QJsonDocument::fromJson(p.readAllStandardOutput().trimmed()).object());
    }
    // 100 个全新哈希进程：种子 42，断言全部相同
    QByteArray firstHash;
    bool allEqual = true;
    for (int i = 0; i < 100; ++i) {
        QProcess p;
        p.start(exec, {QStringLiteral("--child-hash=42")}, QIODevice::ReadOnly);
        if (!p.waitForFinished(30000) || p.exitCode() != 0)
            return 1 | int(!writeJson(outputPath, failJson(
                QStringLiteral("childHash"), QStringLiteral("哈希子进程 %1 失败").arg(i))));
        const auto h = p.readAllStandardOutput().trimmed();
        if (firstHash.isEmpty()) firstHash = h;
        else if (h != firstHash) allEqual = false;
    }
    // 聚合 5 进程时延：取各分位最大值
    qint64 p50 = 0, p95 = 0, p99 = 0, maxT = 0, tempDelta = 0, postGrowth = 0;
    bool memAvailable = true;  // 全部子进程 RSS 可用才算可用
    for (const auto &v : procs) {
        const auto o = v.toObject();
        p50 = std::max(p50, o["p50"].toVariant().toLongLong());
        p95 = std::max(p95, o["p95"].toVariant().toLongLong());
        p99 = std::max(p99, o["p99"].toVariant().toLongLong());
        maxT = std::max(maxT, o["max"].toVariant().toLongLong());
        tempDelta = std::max(tempDelta, o["tempDelta"].toVariant().toLongLong());
        postGrowth = std::max(postGrowth, o["postWarmupGrowth"].toVariant().toLongLong());
        if (!o["memoryAvailable"].toBool()) memAvailable = false;
    }
    const double p50Ms = p50 / 1e6, p95Ms = p95 / 1e6, p99Ms = p99 / 1e6, maxMs = maxT / 1e6;

    // 点击到渲染：真实 MainWindow 离屏路径，DEC-NAV-03 点击 -> grab()
    qint64 clickP95 = 0, clickMax = 0;
    {
        MainWindow win; win.resize(1280, 720); win.show();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        auto *navSit = win.findChild<QAbstractButton *>(QStringLiteral("DEC-NAV-01"));
        auto *navDec = win.findChild<QAbstractButton *>(QStringLiteral("DEC-NAV-03"));
        for (int i = 0; i < 5; ++i) {  // 预热
            QTest::mouseClick(navSit, Qt::LeftButton);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QTest::mouseClick(navDec, Qt::LeftButton);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            win.grab();
        }
        QVector<qint64> clickTimes;
        for (int i = 0; i < 30; ++i) {
            QTest::mouseClick(navSit, Qt::LeftButton);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QElapsedTimer t; t.start();
            QTest::mouseClick(navDec, Qt::LeftButton);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            win.grab();
            clickTimes.append(t.nsecsElapsed());
        }
        std::sort(clickTimes.begin(), clickTimes.end());
        clickP95 = percentile(clickTimes, 95);
        clickMax = clickTimes.last();
    }
    const double clickP95Ms = clickP95 / 1e6, clickMaxMs = clickMax / 1e6;

    // 心跳：10ms QTimer 在 100 次顺序 replan 期间，断言派发延迟/无丢失/无陈旧。
    // lastFire 用纳秒精度计量相对 10ms 周期的延迟（lateness = max(0, elapsed - 10ms)）。
    qint64 hbMaxLatenessNs = 0;
    int hbCount = 0;
    bool hbOk = true;
    QString hbError;
    {
        Core::MOS::MosPlanningController controller;
        QTimer hb; hb.setInterval(10);
        QElapsedTimer lastFire; lastFire.start();
        QEventLoop loop;
        QVector<quint64> seenRevs;
        QObject::connect(&hb, &QTimer::timeout, [&]() {
            const qint64 elapsedNs = lastFire.nsecsElapsed();
            lastFire.restart();
            hbMaxLatenessNs = std::max(hbMaxLatenessNs,
                                       std::max(qint64(0), elapsedNs - qint64(10) * 1000000));
            const int seed = 42 + hbCount;
            const auto obs = Core::MOS::MosFixtureGenerator::generate(
                envelopeParams(), envelopeGenParams(), qint32(seed));
            seenRevs.append(controller.requestReplan(obs, envelopeParams()));
            ++hbCount;
            if (hbCount >= 100) { hb.stop(); loop.quit(); }
        });
        hb.start();
        loop.exec();
        if (seenRevs.size() != 100) { hbOk = false; hbError = QStringLiteral("size=%1").arg(seenRevs.size()); }
        for (int i = 0; hbOk && i < 100; ++i)
            if (seenRevs[i] != quint64(i + 1)) { hbOk = false; hbError = QStringLiteral("rev[%1]=%2").arg(i).arg(seenRevs[i]); }
        if (hbOk) {  // 末次哈希匹配 seed 141（无陈旧覆写）
            const auto expObs = Core::MOS::MosFixtureGenerator::generate(envelopeParams(), envelopeGenParams(), 141);
            const auto expRes = Core::MOS::MosPlanner::planProgressive(expObs, envelopeParams());
            const auto snap = controller.snapshot();
            if (canonicalHash(snap.obstacles, snap.result) != canonicalHash(expObs, expRes)) {
                hbOk = false; hbError = QStringLiteral("staleOverwriteDetected");
            }
        }
    }
    const double hbMaxMs = hbMaxLatenessNs / 1e6;
    if (!hbOk)
        return 1 | int(!writeJson(outputPath, failJson(QStringLiteral("heartbeat"), hbError)));

    // 阈值断言（不可削弱）：RSS 不可用时显式失败，避免负 delta 误判通过
    const double p99Limit = maxP99Ms > 0 ? double(maxP99Ms) : 16.0;
    if (!memAvailable
        || p95Ms > 10.0 || p99Ms > p99Limit || maxMs > 33.0
        || clickP95Ms > 100.0 || clickMaxMs > 200.0
        || hbMaxMs > 50.0
        || tempDelta > 16.0 * 1024 * 1024 || postGrowth > 4.0 * 1024 * 1024
        || !allEqual) {
        QJsonObject extra;
        extra["coreP50Ms"] = p50Ms; extra["coreP95Ms"] = p95Ms;
        extra["coreP99Ms"] = p99Ms; extra["coreMaxMs"] = maxMs;
        extra["clickP95Ms"] = clickP95Ms; extra["clickMaxMs"] = clickMaxMs;
        extra["heartbeatMaxMs"] = hbMaxMs;
        extra["tempDelta"] = qint64(tempDelta); extra["postWarmupGrowth"] = qint64(postGrowth);
        extra["memoryAvailable"] = memAvailable;
        extra["allHashesEqual"] = allEqual;
        writeJson(outputPath, failJson(QStringLiteral("performanceThreshold"),
            QStringLiteral("性能/确定性门禁未通过"), extra));
        return 1;
    }
    // 显式阈值对象：便于机器可读核对，数值与上方断言一致
    QJsonObject thresholds;
    thresholds["coreP95Ms"] = 10.0; thresholds["coreP99Ms"] = p99Limit;
    thresholds["coreMaxMs"] = 33.0;
    thresholds["clickP95Ms"] = 100.0; thresholds["clickMaxMs"] = 200.0;
    thresholds["heartbeatMaxMs"] = 50.0;
    thresholds["tempDeltaBytes"] = qint64(16) * 1024 * 1024;
    thresholds["postWarmupGrowthBytes"] = qint64(4) * 1024 * 1024;
    QJsonObject result;
    result["status"] = QStringLiteral("ok");
    result["coreP50Ms"] = p50Ms; result["coreP95Ms"] = p95Ms;
    result["coreP99Ms"] = p99Ms; result["coreMaxMs"] = maxMs;
    result["clickP95Ms"] = clickP95Ms; result["clickMaxMs"] = clickMaxMs;
    result["heartbeatMaxMs"] = hbMaxMs;
    result["tempDelta"] = qint64(tempDelta); result["postWarmupGrowth"] = qint64(postGrowth);
    result["memoryAvailable"] = memAvailable;
    result["hashesEqual"] = allEqual;
    result["measurementProcesses"] = 5;
    result["warmupIterations"] = 100;  // 5 进程 × 20
    result["sampleIterations"] = 1000;  // 5 进程 × 200
    result["hashProcesses"] = 100;
    result["canonicalHash"] = QString::fromLatin1(firstHash);
    result["thresholds"] = thresholds;
    result["processes"] = procs;
    result["environment"] = envMetadata();
    writeJson(outputPath, result);
    return 0;
}

} // namespace MosPerf
