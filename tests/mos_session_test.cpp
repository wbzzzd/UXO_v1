// MOS P0 会话与 replan 控制器测试：覆盖 revision-safe 提交、拒绝日志、
// 陈旧/重复完成忽略与单向 fixture 导出契约。
// 本测试在 controller/session 实现前编写，期望先失败再通过（failing-first TDD）。
// 全部断言同步完成，不依赖 sleep/QSignalSpy::wait/QTRY/定时器/GUI 控件。

#include "MainWindow/MosPlanningController.h"
#include "Core/MOS/MosPlanner.h"

#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QVariant>

namespace {

// 默认合法跑道参数（合同常量，与 mos_planner_test 保持一致）
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

// 构造合成弹坑（简化构造，合成语义）
Core::MOS::MosCrater makeCrater(const QString &id, double visibleR, int x, int y, double influenceR)
{
    Core::MOS::MosCrater c;
    c.id = id;
    c.visibleRadius = visibleR;
    c.x = x;
    c.y = y;
    c.threat = Core::MOS::MosThreatLevel::High;
    c.influenceRadius = influenceR;
    return c;
}

// 合法有解障碍物集合：两个分离弹坑，递进规划可接受（取自 mos_planner_test）
Core::MOS::MosObstacleSet solvableObstacles()
{
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("c1"), 5.0, 1000, 0, 50.0));
    obs.craters.append(makeCrater(QStringLiteral("c2"), 5.0, 2000, 0, 50.0));
    return obs;
}

// 合法无解障碍物集合：单个超大影响半径圆盘覆盖全跑道（取自 mos_planner_test）
Core::MOS::MosObstacleSet blockedObstacles()
{
    Core::MOS::MosObstacleSet obs;
    obs.craters.append(makeCrater(QStringLiteral("blocker"), 5.0, 1500, 0, 2000.0));
    return obs;
}

// 非法跑道参数：L < 100（取自 mos_planner_test 的拒绝用例）
Core::MOS::MosRunwayParams invalidParams()
{
    auto p = defaultRunwayParams();
    p.L = 50.0;
    return p;
}

// === 快照聚焦比较助手：只读契约相关字段，不依赖整体相等 ===
bool snapshotHasResult(const Core::MOS::MosPlanningSnapshot &s) { return s.hasResult; }
quint64 snapshotCommittedRevision(const Core::MOS::MosPlanningSnapshot &s) { return s.committedRevision; }
int snapshotSelectedTier(const Core::MOS::MosPlanningSnapshot &s) { return s.selectedTier; }
bool snapshotResultAccepted(const Core::MOS::MosPlanningSnapshot &s) { return s.result.accepted; }
Core::MOS::MosPlannerReason snapshotResultReason(const Core::MOS::MosPlanningSnapshot &s) { return s.result.reason; }
int snapshotTierCount(const Core::MOS::MosPlanningSnapshot &s) { return s.result.tiers.size(); }
int snapshotObstacleCount(const Core::MOS::MosPlanningSnapshot &s) { return s.obstacles.craters.size() + s.obstacles.uxo.size(); }
int snapshotLogCount(const Core::MOS::MosPlanningSnapshot &s) { return s.logEntries.size(); }

// 单条日志聚焦比较：sequence/type/revision（message 为实现定义，单独断言非空）
bool logEntryMatches(const Core::MOS::MosSessionLogEntry &e,
                     quint64 sequence,
                     Core::MOS::MosSessionLogType type,
                     quint64 revision)
{
    return e.sequence == sequence
        && e.type == type
        && e.revision == revision;
}

// 业务状态聚焦相等：用于陈旧/重复/导出测试断言"业务不变"（含日志全字段）
bool snapshotsBusinessEqual(const Core::MOS::MosPlanningSnapshot &a,
                            const Core::MOS::MosPlanningSnapshot &b)
{
    if (a.hasResult != b.hasResult) return false;
    if (a.committedRevision != b.committedRevision) return false;
    if (a.selectedTier != b.selectedTier) return false;
    if (a.result.accepted != b.result.accepted) return false;
    if (a.result.reason != b.result.reason) return false;
    if (a.result.tiers.size() != b.result.tiers.size()) return false;
    if (snapshotObstacleCount(a) != snapshotObstacleCount(b)) return false;
    if (a.logEntries.size() != b.logEntries.size()) return false;
    for (int i = 0; i < a.logEntries.size(); ++i) {
        const auto &la = a.logEntries.at(i);
        const auto &lb = b.logEntries.at(i);
        if (la.sequence != lb.sequence) return false;
        if (la.type != lb.type) return false;
        if (la.revision != lb.revision) return false;
        if (la.message != lb.message) return false;
    }
    return true;
}

} // namespace

class MosSessionTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void acceptedSolutionCommitsSnapshotAndEmitsOneStateSignal();
    void acceptedNoSolutionEmitsOneAndStoresInvalidTier();
    void invalidPlannerResultRejectsAndAppendsOneLog();
    void validTierSelectionEmitsOneAndInvalidLeavesUnchanged();
    void workerDirectCallCopiesRevisionAndReturnsCompletion();
    void reverseCompletionStaleRevisionIgnored();
    void duplicateCompletionIsIgnored();
    void exportFixtureObservesWithoutMutation();
    void exportFixtureAfterReplaceObstaclesLeavesStateUnchanged();
};

// 注册元类型，确保 QSignalSpy/QVariant 在直连下也能识别值类型
void MosSessionTest::initTestCase()
{
    qRegisterMetaType<Core::MOS::MosReplanRequest>("Core::MOS::MosReplanRequest");
    qRegisterMetaType<Core::MOS::MosReplanCompletion>("Core::MOS::MosReplanCompletion");
}

// === 接受有解：恰好一次 mosStateChanged，提交快照与接受日志 ===
void MosSessionTest::acceptedSolutionCommitsSnapshotAndEmitsOneStateSignal()
{
    Core::MOS::MosPlanningController controller;
    QSignalSpy stateSpy(&controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QSignalSpy requestedSpy(&controller, &Core::MOS::MosPlanningController::replanRequested);
    QVERIFY(stateSpy.isValid());
    QVERIFY(requestedSpy.isValid());

    const auto params = defaultRunwayParams();
    const auto obs = solvableObstacles();
    const quint64 revision = controller.requestReplan(obs, params);

    QCOMPARE(revision, quint64(1));
    QCOMPARE(stateSpy.count(), 1);       // 接受提交恰好一次通知
    QCOMPARE(requestedSpy.count(), 1);   // 发起恰好一次请求通知

    const auto snap = controller.snapshot();
    QVERIFY(snapshotHasResult(snap));
    QCOMPARE(snapshotCommittedRevision(snap), quint64(1));
    QCOMPARE(snapshotSelectedTier(snap), 0);
    QVERIFY(snapshotResultAccepted(snap));
    QCOMPARE(snapshotResultReason(snap), Core::MOS::MosPlannerReason::Accepted);
    QCOMPARE(snapshotTierCount(snap), params.tiers);
    QCOMPARE(snapshotObstacleCount(snap), 2);

    QCOMPARE(snapshotLogCount(snap), 1);
    QVERIFY(logEntryMatches(snap.logEntries.at(0), quint64(1),
                            Core::MOS::MosSessionLogType::ReplanAccepted, quint64(1)));
    QVERIFY(!snap.logEntries.at(0).message.isEmpty());
    QVERIFY(snap.logEntries.at(0).timestampUtc.isValid());
}

// === 接受无解：恰好一次通知，存储含无效档位的结果 ===
void MosSessionTest::acceptedNoSolutionEmitsOneAndStoresInvalidTier()
{
    Core::MOS::MosPlanningController controller;
    QSignalSpy stateSpy(&controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QVERIFY(stateSpy.isValid());

    const auto params = defaultRunwayParams();
    const auto obs = blockedObstacles();
    const quint64 revision = controller.requestReplan(obs, params);

    QCOMPARE(revision, quint64(1));
    QCOMPARE(stateSpy.count(), 1);

    const auto snap = controller.snapshot();
    QVERIFY(snapshotHasResult(snap));
    QCOMPARE(snapshotCommittedRevision(snap), quint64(1));
    QVERIFY(snapshotResultAccepted(snap));   // 合法无解仍接受
    QCOMPARE(snapshotResultReason(snap), Core::MOS::MosPlannerReason::Accepted);
    QCOMPARE(snapshotTierCount(snap), params.tiers);
    QVERIFY(!snap.result.tiers.isEmpty());
    // tier 0 未修复 -> 无解档位（valid=false, reason=NoFeasibleRectangle）
    QVERIFY(!snap.result.tiers.at(0).rectangle.valid);
    QCOMPARE(snap.result.tiers.at(0).rectangle.reason,
             Core::MOS::MosPlannerReason::NoFeasibleRectangle);
    QCOMPARE(snapshotObstacleCount(snap), 1);

    QCOMPARE(snapshotLogCount(snap), 1);
    QVERIFY(logEntryMatches(snap.logEntries.at(0), quint64(1),
                            Core::MOS::MosSessionLogType::ReplanAccepted, quint64(1)));
}

// === 非法规划结果：恰好一次拒绝通知，业务字段不变，追加一条拒绝日志 ===
void MosSessionTest::invalidPlannerResultRejectsAndAppendsOneLog()
{
    Core::MOS::MosPlanningController controller;
    QSignalSpy stateSpy(&controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QVERIFY(stateSpy.isValid());

    const auto before = controller.snapshot();
    QVERIFY(!snapshotHasResult(before));
    QCOMPARE(snapshotCommittedRevision(before), quint64(0));
    QCOMPARE(snapshotLogCount(before), 0);

    const auto params = invalidParams();
    const auto obs = solvableObstacles();
    const quint64 revision = controller.requestReplan(obs, params);

    QCOMPARE(revision, quint64(1));
    QCOMPARE(stateSpy.count(), 1);   // 拒绝日志追加恰好一次通知

    const auto snap = controller.snapshot();
    // 业务字段保持不变
    QVERIFY(!snapshotHasResult(snap));
    QCOMPARE(snapshotCommittedRevision(snap), quint64(0));
    QCOMPARE(snapshotSelectedTier(snap), 0);
    QCOMPARE(snapshotObstacleCount(snap), 0);   // 拒绝不写入障碍物
    QCOMPARE(snapshotTierCount(snap), 0);       // 拒绝不写入结果
    // 仅追加一条拒绝日志
    QCOMPARE(snapshotLogCount(snap), 1);
    QCOMPARE(snap.logEntries.at(0).sequence, quint64(1));
    QCOMPARE(snap.logEntries.at(0).type, Core::MOS::MosSessionLogType::ReplanRejected);
    QCOMPARE(snap.logEntries.at(0).revision, quint64(1));
    QCOMPARE(snap.logEntries.at(0).reason, Core::MOS::MosPlannerReason::InvalidParams);
    QVERIFY(!snap.logEntries.at(0).message.isEmpty());
}

// === 合法档位切换发一次通知；越界切换不改业务、不发通知、不追加日志 ===
void MosSessionTest::validTierSelectionEmitsOneAndInvalidLeavesUnchanged()
{
    Core::MOS::MosPlanningController controller;
    const auto params = defaultRunwayParams();
    controller.requestReplan(solvableObstacles(), params);

    QSignalSpy stateSpy(&controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QVERIFY(stateSpy.isValid());

    const auto afterCommit = controller.snapshot();
    const int logCountBefore = snapshotLogCount(afterCommit);

    // 合法切换到 tier 1
    QVERIFY(controller.selectTier(1));
    QCOMPARE(stateSpy.count(), 1);
    const auto afterValid = controller.snapshot();
    QCOMPARE(snapshotSelectedTier(afterValid), 1);
    QCOMPARE(snapshotLogCount(afterValid), logCountBefore + 1);
    QCOMPARE(afterValid.logEntries.last().type, Core::MOS::MosSessionLogType::TierSelected);
    QCOMPARE(afterValid.logEntries.last().sequence, quint64(logCountBefore + 1));

    // 越界切换：返回 false，无新通知、无新日志、业务字段不变（拒绝日志语义：无日志即无通知）
    QVERIFY(!controller.selectTier(99));
    QCOMPARE(stateSpy.count(), 1);   // 仍只有合法切换那一次
    const auto afterInvalid = controller.snapshot();
    QCOMPARE(snapshotSelectedTier(afterInvalid), 1);
    QCOMPARE(snapshotCommittedRevision(afterInvalid), snapshotCommittedRevision(afterValid));
    QVERIFY(snapshotHasResult(afterInvalid) == snapshotHasResult(afterValid));
    QCOMPARE(snapshotLogCount(afterInvalid), logCountBefore + 1);   // 未追加
    QVERIFY(snapshotsBusinessEqual(afterInvalid, afterValid));
}

// === worker 直接调用：拷贝 revision 并返回完成结果 ===
void MosSessionTest::workerDirectCallCopiesRevisionAndReturnsCompletion()
{
    Core::MOS::MosReplanWorker worker;
    QSignalSpy completedSpy(&worker, &Core::MOS::MosReplanWorker::replanCompleted);
    QVERIFY(completedSpy.isValid());

    Core::MOS::MosReplanRequest request;
    request.revision = 42;
    request.obstacles = solvableObstacles();
    request.params = defaultRunwayParams();

    worker.replan(request);   // 同步执行

    QCOMPARE(completedSpy.count(), 1);
    const auto completion = completedSpy.takeFirst().at(0).value<Core::MOS::MosReplanCompletion>();
    QCOMPARE(completion.revision, quint64(42));   // revision 被值拷贝
    QVERIFY(completion.result.accepted);           // 值拷贝的规划结果
    QCOMPARE(completion.result.reason, Core::MOS::MosPlannerReason::Accepted);
    QCOMPARE(completion.result.tiers.size(), request.params.tiers);
}

// === 反向完成：先提交 revision 2，再提交 revision 1 应被忽略 ===
void MosSessionTest::reverseCompletionStaleRevisionIgnored()
{
    Core::MOS::MosPlanningController controller;
    // 断开控制器与 worker 的同步完成连接，手动驱动完成（保留 worker->spy 直连）
    controller.worker()->disconnect(&controller);

    QSignalSpy stateSpy(&controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QSignalSpy completedSpy(controller.worker(), &Core::MOS::MosReplanWorker::replanCompleted);
    QVERIFY(stateSpy.isValid());
    QVERIFY(completedSpy.isValid());

    const auto params = defaultRunwayParams();
    // revision 1：合法有解
    const auto obs1 = solvableObstacles();
    const quint64 rev1 = controller.requestReplan(obs1, params);
    // revision 2：合法无解（结果与 rev1 可区分）
    const auto obs2 = blockedObstacles();
    const quint64 rev2 = controller.requestReplan(obs2, params);

    QCOMPARE(rev1, quint64(1));
    QCOMPARE(rev2, quint64(2));
    QCOMPARE(completedSpy.count(), 2);   // 两次请求各同步触发一次 worker 完成
    QCOMPARE(stateSpy.count(), 0);       // 未手动完成，业务未变

    // 取出 worker 发出的两次完成（顺序与请求一致）
    const auto completion1 = completedSpy.at(0).at(0).value<Core::MOS::MosReplanCompletion>();
    const auto completion2 = completedSpy.at(1).at(0).value<Core::MOS::MosReplanCompletion>();
    QCOMPARE(completion1.revision, quint64(1));
    QCOMPARE(completion2.revision, quint64(2));

    // 先提交 revision 2：Committed
    QCOMPARE(controller.completeReplan(completion2), Core::MOS::MosCompletionDisposition::Committed);
    QCOMPARE(stateSpy.count(), 1);
    const auto snapAfterCommit2 = controller.snapshot();
    QVERIFY(snapshotHasResult(snapAfterCommit2));
    QCOMPARE(snapshotCommittedRevision(snapAfterCommit2), quint64(2));

    // 再提交 revision 1：陈旧，应被忽略
    QCOMPARE(controller.completeReplan(completion1), Core::MOS::MosCompletionDisposition::IgnoredStale);
    QCOMPARE(stateSpy.count(), 1);   // 零额外通知
    const auto snapAfterStale = controller.snapshot();
    QCOMPARE(snapshotLogCount(snapAfterStale), snapshotLogCount(snapAfterCommit2));   // 无新日志
    QVERIFY(snapshotsBusinessEqual(snapAfterStale, snapAfterCommit2));                // 业务不变
}

// === 重复完成：同一 revision 二次提交应被忽略 ===
void MosSessionTest::duplicateCompletionIsIgnored()
{
    Core::MOS::MosPlanningController controller;
    QSignalSpy completedSpy(controller.worker(), &Core::MOS::MosReplanWorker::replanCompleted);
    QVERIFY(completedSpy.isValid());

    const auto params = defaultRunwayParams();
    controller.requestReplan(solvableObstacles(), params);
    QCOMPARE(completedSpy.count(), 1);
    const auto completion = completedSpy.at(0).at(0).value<Core::MOS::MosReplanCompletion>();
    QCOMPARE(completion.revision, quint64(1));

    // 控制器已在 requestReplan 同步路径中完成一次（Committed）
    const auto snapAfterFirst = controller.snapshot();
    QVERIFY(snapshotHasResult(snapAfterFirst));
    QCOMPARE(snapshotCommittedRevision(snapAfterFirst), quint64(1));
    const int logCountAfterFirst = snapshotLogCount(snapAfterFirst);

    QSignalSpy stateSpy(&controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QVERIFY(stateSpy.isValid());

    // 同一 revision 二次提交：IgnoredStale
    QCOMPARE(controller.completeReplan(completion), Core::MOS::MosCompletionDisposition::IgnoredStale);
    QCOMPARE(stateSpy.count(), 0);    // 不发通知
    const auto snapAfterDuplicate = controller.snapshot();
    QCOMPARE(snapshotLogCount(snapAfterDuplicate), logCountAfterFirst);   // 不追加日志
    QVERIFY(snapshotsBusinessEqual(snapAfterDuplicate, snapAfterFirst));  // 业务不变
}

// === 单向导出：写入临时工件，不改变业务/日志/revision/通知计数 ===
void MosSessionTest::exportFixtureObservesWithoutMutation()
{
    Core::MOS::MosPlanningController controller;
    controller.requestReplan(solvableObstacles(), defaultRunwayParams());

    const auto before = controller.snapshot();
    QVERIFY(snapshotHasResult(before));
    const int logCountBefore = snapshotLogCount(before);
    const quint64 revisionBefore = snapshotCommittedRevision(before);

    QSignalSpy stateSpy(&controller, &Core::MOS::MosPlanningController::mosStateChanged);
    QVERIFY(stateSpy.isValid());

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString exportPath = dir.path() + QStringLiteral("/mos_fixture.json");
    Core::MOS::MosGeneratorParams generatorParams;
    const auto result = controller.exportFixture(
        exportPath, defaultRunwayParams(), generatorParams, 42);

    QVERIFY(result.success);
    QVERIFY(QFile::exists(exportPath));

    QCOMPARE(stateSpy.count(), 0);   // 导出是观测，不发通知
    const auto after = controller.snapshot();
    QVERIFY(snapshotsBusinessEqual(after, before));
    QCOMPARE(snapshotLogCount(after), logCountBefore);
    QCOMPARE(snapshotCommittedRevision(after), revisionBefore);
}

// === replaceObstacles 清除结果后实时导出成功且状态不变 ===
void MosSessionTest::exportFixtureAfterReplaceObstaclesLeavesStateUnchanged()
{
    // Given: 已提交结果的控制器，通过 replaceObstacles 清除 hasResult。
    Core::MOS::MosPlanningController controller;
    controller.requestReplan(solvableObstacles(), defaultRunwayParams());
    QVERIFY(snapshotHasResult(controller.snapshot()));

    Core::MOS::MosObstacleSet emptyObstacles;
    controller.replaceObstacles(emptyObstacles, defaultRunwayParams());
    const auto before = controller.snapshot();
    QVERIFY(!snapshotHasResult(before));

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString target = QDir(directory.path()).absoluteFilePath(
        QStringLiteral("after-replace.json"));

    // When: 在无已提交结果状态下请求导出。
    Core::MOS::MosGeneratorParams generatorParams;
    const auto result = controller.exportFixture(
        target, defaultRunwayParams(), generatorParams, 42);

    // Then: 实时导出成功，会话状态完全不变。
    QVERIFY(result.success);
    QVERIFY(QFile::exists(target));
    const auto after = controller.snapshot();
    QVERIFY(snapshotsBusinessEqual(after, before));
}

QTEST_GUILESS_MAIN(MosSessionTest)

#include "mos_session_test.moc"
