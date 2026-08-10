// 模拟工作流测试：验证纯内存状态流转、操作日志和重置行为。

#include "Core/Simulation/DemoScenarioProvider.h"
#include "Core/Simulation/SimulationWorkflow.h"

#include <QtTest>

namespace {

constexpr auto kTargetId = "target-demo-001";

// 测试不依赖 DemoScenarioProvider 的初始目标（空起步），
// 手动构造 1 个测试目标用于验证 SimulationWorkflow 状态流转。
Core::Simulation::SimulationWorkflow createWorkflow()
{
    Core::Simulation::SimulationWorkflow workflow;
    Core::TargetInfo target;
    target.id = QString::fromLatin1(kTargetId);
    target.status = Core::TargetStatus::Detected;
    workflow.reset(QVector<Core::TargetInfo>{target});
    return workflow;
}

}

class SimulationWorkflowTest : public QObject
{
    Q_OBJECT

private slots:
    void providerStartsAtDetected();
    void validTransitionsFollowRequiredSequence();
    void transitionWithoutSelectionIsRejected();
    void invalidSkipIsRejected();
    void backwardAndTerminalTransitionsAreRejected();
    void unknownTargetSelectionIsRejected();
    void logOrderingAndContentAreStable();
    void resetDoesNotPersistState();
};

void SimulationWorkflowTest::providerStartsAtDetected()
{
    // DemoScenarioProvider 已改为空起步（0 目标），目标由 DetectionSimulator 动态注入。
    const auto scenario = Core::Simulation::DemoScenarioProvider::create();

    QCOMPARE(scenario.targets.size(), 0);
}

void SimulationWorkflowTest::validTransitionsFollowRequiredSequence()
{
    auto workflow = createWorkflow();

    QVERIFY(workflow.selectTarget(QString::fromLatin1(kTargetId)));
    QVERIFY(workflow.hasSelectedTarget());
    QCOMPARE(workflow.selectedTargetId(), QString::fromLatin1(kTargetId));

    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Confirmed));
    QCOMPARE(workflow.selectedTarget()->status, Core::TargetStatus::Confirmed);
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposing));
    QCOMPARE(workflow.selectedTarget()->status, Core::TargetStatus::Disposing);
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposed));
    QCOMPARE(workflow.selectedTarget()->status, Core::TargetStatus::Disposed);
}

void SimulationWorkflowTest::transitionWithoutSelectionIsRejected()
{
    auto workflow = createWorkflow();

    QVERIFY(!workflow.requestSelectedTargetStatus(Core::TargetStatus::Confirmed));
    QCOMPARE(workflow.targets().first().status, Core::TargetStatus::Detected);
    QCOMPARE(workflow.logEntries().size(), 1);

    const auto &entry = workflow.logEntries().first();
    QCOMPARE(entry.sequence, quint64(1));
    QCOMPARE(entry.type, Core::Simulation::SimulationOperationType::ActionRejected);
    QCOMPARE(entry.message, QStringLiteral("[模拟] 操作被拒绝：未选择模拟目标"));
}

void SimulationWorkflowTest::invalidSkipIsRejected()
{
    auto workflow = createWorkflow();

    QVERIFY(workflow.selectTarget(QString::fromLatin1(kTargetId)));
    QVERIFY(!workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposing));
    QCOMPARE(workflow.selectedTarget()->status, Core::TargetStatus::Detected);
    QCOMPARE(workflow.logEntries().size(), 2);
    QCOMPARE(workflow.logEntries().last().message,
             QStringLiteral("[模拟] 操作被拒绝：目标 target-demo-001 不能从已发现变更为处置中"));

    // 重复选择不产生第二条选择日志。
    QVERIFY(workflow.selectTarget(QString::fromLatin1(kTargetId)));
    QCOMPARE(workflow.logEntries().size(), 2);
}

void SimulationWorkflowTest::backwardAndTerminalTransitionsAreRejected()
{
    auto workflow = createWorkflow();
    QVERIFY(workflow.selectTarget(QString::fromLatin1(kTargetId)));
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Confirmed));
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposing));
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposed));

    QVERIFY(!workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposing));
    QCOMPARE(workflow.selectedTarget()->status, Core::TargetStatus::Disposed);
    QCOMPARE(workflow.logEntries().last().message,
             QStringLiteral("[模拟] 操作被拒绝：目标 target-demo-001 不能从已完成变更为处置中"));

    QVERIFY(!workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposed));
    QCOMPARE(workflow.selectedTarget()->status, Core::TargetStatus::Disposed);
    QCOMPARE(workflow.logEntries().last().message,
             QStringLiteral("[模拟] 操作被拒绝：目标 target-demo-001 不能从已完成变更为已完成"));
}

void SimulationWorkflowTest::unknownTargetSelectionIsRejected()
{
    auto workflow = createWorkflow();

    QVERIFY(!workflow.selectTarget(QStringLiteral("target-missing")));
    QVERIFY(!workflow.hasSelectedTarget());
    QCOMPARE(workflow.logEntries().size(), 1);
    QCOMPARE(workflow.logEntries().first().message,
             QStringLiteral("[模拟] 操作被拒绝：模拟目标 target-missing 不存在"));
}

void SimulationWorkflowTest::logOrderingAndContentAreStable()
{
    auto workflow = createWorkflow();
    QVERIFY(workflow.selectTarget(QString::fromLatin1(kTargetId)));
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Confirmed));
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposing));
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposed));

    const auto &entries = workflow.logEntries();
    QCOMPARE(entries.size(), 4);

    const QVector<QString> expectedMessages = {
        QStringLiteral("[模拟] 已选择目标 target-demo-001"),
        QStringLiteral("[模拟] 目标 target-demo-001：已发现 -> 已确认"),
        QStringLiteral("[模拟] 目标 target-demo-001：已确认 -> 处置中"),
        QStringLiteral("[模拟] 目标 target-demo-001：处置中 -> 已完成")
    };

    for (int index = 0; index < entries.size(); ++index) {
        QCOMPARE(entries.at(index).sequence, quint64(index + 1));
        QVERIFY(entries.at(index).timestampUtc.isValid());
        QCOMPARE(entries.at(index).timestampUtc.timeSpec(), Qt::UTC);
        QCOMPARE(entries.at(index).targetId, QString::fromLatin1(kTargetId));
        QCOMPARE(entries.at(index).message, expectedMessages.at(index));
        if (index > 0) {
            QVERIFY(entries.at(index - 1).timestampUtc <= entries.at(index).timestampUtc);
        }
    }

    QCOMPARE(entries.at(0).type, Core::Simulation::SimulationOperationType::TargetSelected);
    QCOMPARE(entries.at(0).beforeStatus, Core::TargetStatus::Detected);
    QCOMPARE(entries.at(0).afterStatus, Core::TargetStatus::Detected);
    for (int index = 1; index < entries.size(); ++index) {
        QCOMPARE(entries.at(index).type, Core::Simulation::SimulationOperationType::StatusChanged);
    }
    QCOMPARE(entries.at(1).beforeStatus, Core::TargetStatus::Detected);
    QCOMPARE(entries.at(1).afterStatus, Core::TargetStatus::Confirmed);
    QCOMPARE(entries.at(2).beforeStatus, Core::TargetStatus::Confirmed);
    QCOMPARE(entries.at(2).afterStatus, Core::TargetStatus::Disposing);
    QCOMPARE(entries.at(3).beforeStatus, Core::TargetStatus::Disposing);
    QCOMPARE(entries.at(3).afterStatus, Core::TargetStatus::Disposed);

    QCOMPARE(Core::Simulation::SimulationWorkflow::simulationStatusText(Core::TargetStatus::Detected),
             QStringLiteral("已发现"));
    QCOMPARE(Core::Simulation::SimulationWorkflow::simulationStatusText(Core::TargetStatus::Confirmed),
             QStringLiteral("已确认"));
    QCOMPARE(Core::Simulation::SimulationWorkflow::simulationStatusText(Core::TargetStatus::Disposing),
             QStringLiteral("处置中"));
    QCOMPARE(Core::Simulation::SimulationWorkflow::simulationStatusText(Core::TargetStatus::Disposed),
             QStringLiteral("已完成"));
    QCOMPARE(Core::Simulation::SimulationWorkflow::simulationStatusText(Core::TargetStatus::Pending),
             QStringLiteral("不支持的模拟状态"));
}

void SimulationWorkflowTest::resetDoesNotPersistState()
{
    auto workflow = createWorkflow();
    QVERIFY(workflow.selectTarget(QString::fromLatin1(kTargetId)));
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Confirmed));
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposing));
    QVERIFY(workflow.requestSelectedTargetStatus(Core::TargetStatus::Disposed));

    // 重置时手动注入新的测试目标（与 createWorkflow 相同），验证 reset 清除状态
    Core::TargetInfo freshTarget;
    freshTarget.id = QString::fromLatin1(kTargetId);
    freshTarget.status = Core::TargetStatus::Detected;
    workflow.reset(QVector<Core::TargetInfo>{freshTarget});

    QVERIFY(!workflow.hasSelectedTarget());
    QVERIFY(workflow.selectedTarget() == nullptr);
    QVERIFY(workflow.selectedTargetId().isEmpty());
    QCOMPARE(workflow.targets().first().status, Core::TargetStatus::Detected);
    QVERIFY(workflow.logEntries().isEmpty());

    QVERIFY(workflow.selectTarget(QString::fromLatin1(kTargetId)));
    QCOMPARE(workflow.logEntries().first().sequence, quint64(1));
}

QTEST_APPLESS_MAIN(SimulationWorkflowTest)

#include "simulation_workflow_test.moc"
