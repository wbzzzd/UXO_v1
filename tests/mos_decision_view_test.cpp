// MOS 决策页离屏 UI 契约测试：验证 DEC-* 稳定对象名、被动信号与三栏布局。
// 仅本地合成 fixture 驱动，不连接设备、网络或真实规划会话。

#include "MainWindow/DecisionView.h"
#include "MainWindow/MosGeneratorDialog.h"
#include "MainWindow/MosParamsPanel.h"
#include "MainWindow/MosRunwayWidget.h"
#include "MainWindow/PlanCardWidget.h"
#include "Common/GlobalStyle.h"
#include "Core/MOS/MosFixtureGenerator.h"
#include "Core/MOS/MosPlanner.h"
#include "Core/MOS/MosPlanningSession.h"

#include <QtTest>

#include <QApplication>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QWheelEvent>
#include <algorithm>

class MosDecisionViewTest : public QObject
{
    Q_OBJECT

private slots:
    void stableObjectNamesExist();
    void setSnapshotRebuildsTierButtonsAndCards();
    void replanButtonEmitsSignalOnlyWhenValid();
    void tierButtonEmitsTierSelected();
    void targetListEmitsTargetSelected();
    void generatorDialogEmitsApplied();
    void emptySnapshotShowsEmptyPlanState();
    void snapshotClearsMissingSelectedTarget();
    void noFeasibleSnapshotDisablesInvalidTierAndSuppressesMetrics();
    void negativeSelectedTierShowsNoFeasibleBanner();
    void globalStyleExposesTokenBasedControls();
    void viewportScalePolicy();
    void invalidTierDetailSpacingShowsPlaceholder();
    void validTierDetailSpacingRestoresFixture();
    void initialSnapshotSelectsFirstCraterWithoutEmitting();
    void globalStyleCheckedButtonUsesSelectionTokens();
    void tierSelectionCheckedStateIsUnambiguous();

private:
    Core::MOS::MosPlanningSnapshot makeSeedSnapshot();
};

// 构造一个合法有解快照：3 档、2 弹坑 + 2 UXO
Core::MOS::MosPlanningSnapshot MosDecisionViewTest::makeSeedSnapshot()
{
    Core::MOS::MosRunwayParams params;
    Core::MOS::MosGeneratorParams gen;
    const qint32 seed = 42;
    const auto obstacles = Core::MOS::MosFixtureGenerator::generate(params, gen, seed);
    const auto result = Core::MOS::MosPlanner::planProgressive(obstacles, params);

    Core::MOS::MosPlanningSnapshot snap;
    snap.obstacles = obstacles;
    snap.params = params;
    snap.result = result;
    snap.hasResult = true;
    snap.selectedTier = 1;
    snap.committedRevision = 1;
    return snap;
}

void MosDecisionViewTest::stableObjectNamesExist()
{
    DecisionView view;
    view.resize(1280, 720);

    const QStringList requiredObjects = {
        QStringLiteral("DEC-TB-GEN"), QStringLiteral("DEC-TB-PARAMS"),
        QStringLiteral("DEC-LP-TARGET-LIST"),
        QStringLiteral("DEC-CE-RUNWAY"), QStringLiteral("DEC-CE-ZOOM-IN"),
        QStringLiteral("DEC-CE-ZOOM-LEVEL"), QStringLiteral("DEC-CE-ZOOM-OUT"),
        QStringLiteral("DEC-CE-ZOOM-RESET"), QStringLiteral("DEC-CE-PARAMS"),
        QStringLiteral("DEC-CE-PARAM-RESET"), QStringLiteral("DEC-CE-PARAM-LENGTH"),
        QStringLiteral("DEC-CE-PARAM-WIDTH"), QStringLiteral("DEC-CE-PARAM-MINLENGTH"),
        QStringLiteral("DEC-CE-PARAM-MINWIDTH"), QStringLiteral("DEC-CE-PARAM-K"),
        QStringLiteral("DEC-CE-PARAM-STEP"), QStringLiteral("DEC-CE-PARAM-BACKFILL"),
        QStringLiteral("DEC-CE-PARAM-UXOHOURS"), QStringLiteral("DEC-CE-PARAM-EXPAND"),
        QStringLiteral("DEC-CE-PARAM-TIERS"), QStringLiteral("DEC-CE-PARAM-DMGCOUNT"),
        QStringLiteral("DEC-CE-PARAM-REPAIRED"), QStringLiteral("DEC-CE-VALIDATION"),
        QStringLiteral("DEC-CE-PLAN-STATE"), QStringLiteral("DEC-CE-PARAM-REPLAN"),
        QStringLiteral("DEC-RP-PLANS"), QStringLiteral("DEC-RP-DETAIL"),
        QStringLiteral("DEC-RP-DETAIL-NOTE"), QStringLiteral("DEC-RP-P1-SLOT"),
        QStringLiteral("DEC-GEN-MODAL"), QStringLiteral("DEC-GEN-CLOSE"),
        QStringLiteral("DEC-GEN-CRATER-COUNT"), QStringLiteral("DEC-GEN-CRATER-RMIN"),
        QStringLiteral("DEC-GEN-CRATER-RMAX"), QStringLiteral("DEC-GEN-UXO-COUNT"),
        QStringLiteral("DEC-GEN-UXO-YMIN"), QStringLiteral("DEC-GEN-UXO-YMAX"),
        QStringLiteral("DEC-GEN-SEED"), QStringLiteral("DEC-GEN-BANNER"),
        QStringLiteral("DEC-GEN-JSON"), QStringLiteral("DEC-GEN-CANCEL"),
        QStringLiteral("DEC-GEN-APPLY"), QStringLiteral("DEC-SB-DEVICE"),
        QStringLiteral("DEC-SB-SIM"), QStringLiteral("DEC-SB-ALARM"),
        QStringLiteral("DEC-SB-TARGET")
    };

    QStringList missing;
    for (const auto &name : requiredObjects) {
        if (view.findChild<QObject *>(name) == nullptr) {
            missing.append(name);
        }
    }
    QVERIFY2(missing.isEmpty(),
             qPrintable(QStringLiteral("缺少稳定对象名：%1").arg(missing.join(QStringLiteral(", ")))));
}

void MosDecisionViewTest::setSnapshotRebuildsTierButtonsAndCards()
{
    DecisionView view;
    view.resize(1280, 720);

    const auto snap = makeSeedSnapshot();
    view.setSnapshot(snap);

    // 档位数由 params.tiers=3 控制，应生成 DEC-TB-PLAN-1/2/3
    QVERIFY2(view.findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-1")) != nullptr,
             "档位1 按钮缺失");
    QVERIFY2(view.findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-2")) != nullptr,
             "档位2 按钮缺失");
    QVERIFY2(view.findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-3")) != nullptr,
             "档位3 按钮缺失");

    // 右面板候选方案卡片：DEC-RP-PLANS 容器内应至少有 1 个 DEC-RP-PLAN-* 子控件
    auto *plansContainer = view.findChild<QWidget *>(QStringLiteral("DEC-RP-PLANS"));
    QVERIFY2(plansContainer != nullptr, "DEC-RP-PLANS 容器缺失");
    auto cards = plansContainer->findChildren<PlanCardWidget *>(QRegularExpression(QStringLiteral("^DEC-RP-PLAN-")));
    QVERIFY2(!cards.isEmpty(), "setSnapshot 后右面板应重建候选方案卡片");

    // 左面板目标列表应填充弹坑 + UXO 条目
    auto *targetList = view.findChild<QListWidget *>(QStringLiteral("DEC-LP-TARGET-LIST"));
    QVERIFY2(targetList != nullptr, "DEC-LP-TARGET-LIST 缺失");
    QVERIFY2(targetList->count() > 0, "setSnapshot 后左面板目标列表应填充合成障碍物");

    // 损毁点总数派生字段应反映障碍物总数
    auto *dmgCount = view.findChild<QLabel *>(QStringLiteral("DEC-CE-PARAM-DMGCOUNT"));
    QVERIFY2(dmgCount != nullptr, "DEC-CE-PARAM-DMGCOUNT 缺失");
    const int expected = snap.obstacles.craters.size() + snap.obstacles.uxo.size();
    QCOMPARE(dmgCount->text().toInt(), expected);
}

void MosDecisionViewTest::replanButtonEmitsSignalOnlyWhenValid()
{
    DecisionView view;
    view.resize(1280, 720);

    QSignalSpy spy(&view, &DecisionView::replanRequested);
    QVERIFY(spy.isValid());

    auto *paramReplan = view.findChild<QPushButton *>(QStringLiteral("DEC-CE-PARAM-REPLAN"));
    QVERIFY(paramReplan != nullptr);

    // 默认参数合法，重新规划按钮应可用
    QVERIFY2(paramReplan->isEnabled(), "合法默认参数下 DEC-CE-PARAM-REPLAN 应可用");

    // 点击参数面板重新规划按钮应发出一次 replanRequested
    QTest::mouseClick(paramReplan, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);

    // 设为非法参数（minLength > L，越界但 minLength 自身 spinbox 范围 [1,6000] 接受）
    auto *minLength = view.findChild<QDoubleSpinBox *>(QStringLiteral("DEC-CE-PARAM-MINLENGTH"));
    QVERIFY(minLength != nullptr);
    minLength->setValue(350.0); // > 默认 L=300，触发 MinLength 拒绝
    QVERIFY2(!paramReplan->isEnabled(), "非法参数下 DEC-CE-PARAM-REPLAN 应禁用");

    spy.clear();
    // 程序化点击禁用按钮不发信号（Qt 行为）；直接验证 isValid=false
    auto *panel = view.findChild<MosParamsPanel *>(QStringLiteral("DEC-CE-PARAMS"));
    QVERIFY(panel != nullptr);
    QVERIFY2(!panel->isValid(), "MosParamsPanel 应在 minLength > L 时判定非法");
    QCOMPARE(spy.count(), 0);
}

void MosDecisionViewTest::tierButtonEmitsTierSelected()
{
    DecisionView view;
    view.resize(1280, 720);
    view.setSnapshot(makeSeedSnapshot());

    QSignalSpy spy(&view, &DecisionView::tierSelected);
    QVERIFY(spy.isValid());

    auto *tier2 = view.findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-2"));
    QVERIFY(tier2 != nullptr);
    QTest::mouseClick(tier2, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toInt(), 1); // 档位序号从 0 起
}

void MosDecisionViewTest::targetListEmitsTargetSelected()
{
    DecisionView view;
    view.resize(1280, 720);
    view.setSnapshot(makeSeedSnapshot());

    QSignalSpy spy(&view, &DecisionView::targetSelected);
    QVERIFY(spy.isValid());

    auto *targetList = view.findChild<QListWidget *>(QStringLiteral("DEC-LP-TARGET-LIST"));
    QVERIFY(targetList != nullptr);
    QVERIFY(targetList->count() > 0);

    // 通过真实鼠标点击触发 itemClicked 信号路径（与 simulation_workflow_ui_test 同模式）
    QListWidgetItem *firstItem = targetList->item(0);
    QVERIFY(firstItem != nullptr);
    targetList->scrollToItem(firstItem);
    const QRect itemRect = targetList->visualItemRect(firstItem);
    if (itemRect.isValid()) {
        QTest::mouseClick(targetList->viewport(), Qt::LeftButton, Qt::NoModifier, itemRect.center());
    }

    QVERIFY2(spy.count() >= 1, "点击目标列表项应发出 targetSelected");
}

void MosDecisionViewTest::generatorDialogEmitsApplied()
{
    DecisionView view;
    view.resize(1280, 720);

    auto *dialog = view.findChild<MosGeneratorDialog *>(QStringLiteral("DEC-GEN-MODAL"));
    QVERIFY(dialog != nullptr);

    QSignalSpy spy(dialog, &MosGeneratorDialog::applied);
    QVERIFY(spy.isValid());

    auto *applyBtn = dialog->findChild<QPushButton *>(QStringLiteral("DEC-GEN-APPLY"));
    QVERIFY(applyBtn != nullptr);
    QVERIFY2(applyBtn->isEnabled(), "默认合法参数下应用按钮应可用");

    QTest::mouseClick(applyBtn, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);

    // 通过 dialog 公共接口验证值（避免对自定义类型的 Q_DECLARE_METATYPE 依赖）
    const auto params = dialog->currentParams();
    const qint32 seed = dialog->currentSeed();
    QCOMPARE(params.craterCount, 2);
    QCOMPARE(seed, 42);
}

void MosDecisionViewTest::emptySnapshotShowsEmptyPlanState()
{
    DecisionView view;
    view.resize(1280, 720);

    // 空快照：hasResult=false，obstacles 为空
    Core::MOS::MosPlanningSnapshot empty;
    empty.hasResult = false;
    empty.selectedTier = -1;
    view.setSnapshot(empty);

    auto *planState = view.findChild<QLabel *>(QStringLiteral("DEC-CE-PLAN-STATE"));
    QVERIFY(planState != nullptr);
    QVERIFY2(planState->text().contains(QStringLiteral("空")),
             qPrintable(QStringLiteral("空快照应显示 empty 状态横幅，实际：%1").arg(planState->text())));
}

void MosDecisionViewTest::snapshotClearsMissingSelectedTarget()
{
    // 契约：新快照移除已选目标 ID 时，状态栏显示"未选择"、列表无选中、跑道选择清空。
    // 复用 makeSeedSnapshot 提供合法有解基线，所有数据为本地合成 fixture。
    DecisionView view;
    view.resize(1280, 720);

    const auto snap1 = makeSeedSnapshot();
    view.setSnapshot(snap1);

    auto *targetList = view.findChild<QListWidget *>(QStringLiteral("DEC-LP-TARGET-LIST"));
    QVERIFY(targetList != nullptr);
    QVERIFY2(targetList->count() > 0, "基线快照应填充目标列表");

    // 点击列表首项触发 itemClicked 路径，确立初始选中目标
    QListWidgetItem *firstItem = targetList->item(0);
    QVERIFY(firstItem != nullptr);
    targetList->scrollToItem(firstItem);
    const QRect itemRect = targetList->visualItemRect(firstItem);
    if (itemRect.isValid()) {
        QTest::mouseClick(targetList->viewport(), Qt::LeftButton, Qt::NoModifier, itemRect.center());
    }
    const QString selectedId = firstItem->data(Qt::UserRole).toString();
    QVERIFY2(!selectedId.isEmpty(), "点击应确立非空目标 ID");
    QCOMPARE(view.findChild<QLabel *>(QStringLiteral("DEC-SB-TARGET"))->text(),
             QStringLiteral("当前分析目标：%1").arg(selectedId));

    // 构造第二个快照：移除已选目标 ID，保留其余障碍物以避免触发空态分支
    Core::MOS::MosPlanningSnapshot snap2 = snap1;
    auto &craters = snap2.obstacles.craters;
    auto &uxo = snap2.obstacles.uxo;
    craters.erase(std::remove_if(craters.begin(), craters.end(),
                                 [&selectedId](const Core::MOS::MosCrater &c){ return c.id == selectedId; }),
                  craters.end());
    uxo.erase(std::remove_if(uxo.begin(), uxo.end(),
                             [&selectedId](const Core::MOS::MosUxo &u){ return u.id == selectedId; }),
              uxo.end());
    QVERIFY2(craters.size() + uxo.size() > 0, "应保留至少一个障碍物以区别于空态");

    view.setSnapshot(snap2);

    auto *sbTarget = view.findChild<QLabel *>(QStringLiteral("DEC-SB-TARGET"));
    QVERIFY(sbTarget != nullptr);
    QVERIFY2(sbTarget->text() == QStringLiteral("当前分析目标：未选择"),
             qPrintable(QStringLiteral("已选目标被移除后应显示未选择，实际：%1").arg(sbTarget->text())));
    QVERIFY2(targetList->selectedItems().isEmpty(),
             "已选目标被移除后列表不应残留选中项");
    // 跑道选中清空由 selectTarget(QString()) 内部调用 m_runway->setSelectedTargetId 保证，
    // 状态栏显示"未选择"即证明该代码路径已执行
}

// 合法无解快照：单个超大影响半径弹坑覆盖全跑道，tier 0/1 矩形无效，tier 2 修复后有效
namespace {
Core::MOS::MosPlanningSnapshot makeNoFeasibleSnapshot()
{
    Core::MOS::MosRunwayParams params;
    Core::MOS::MosCrater blocker;
    blocker.id = QStringLiteral("blocker");
    blocker.visibleRadius = 5.0;
    blocker.x = 1500;
    blocker.y = 0;
    blocker.threat = Core::MOS::MosThreatLevel::High;
    blocker.influenceRadius = 2000.0;
    Core::MOS::MosObstacleSet obstacles;
    obstacles.craters.append(blocker);
    const auto result = Core::MOS::MosPlanner::planProgressive(obstacles, params);

    Core::MOS::MosPlanningSnapshot snap;
    snap.obstacles = obstacles;
    snap.params = params;
    snap.result = result;
    snap.hasResult = true;
    snap.selectedTier = 0;  // 选中无可行矩形的档位
    snap.committedRevision = 1;
    return snap;
}
} // namespace

void MosDecisionViewTest::noFeasibleSnapshotDisablesInvalidTierAndSuppressesMetrics()
{
    DecisionView view;
    view.resize(1280, 720);

    const auto snap = makeNoFeasibleSnapshot();
    QVERIFY2(snap.hasResult && snap.result.accepted, "合法无解快照必须被接受");
    QVERIFY2(!snap.result.tiers.at(0).rectangle.valid, "tier 0 矩形必须无效");

    view.setSnapshot(snap);

    // 无可行档位的工具栏按钮必须禁用，避免用户选中无法生成起降带的档位
    auto *tier1 = view.findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-1"));
    QVERIFY2(tier1 != nullptr, "DEC-TB-PLAN-1 缺失");
    QVERIFY2(!tier1->isEnabled(), "无可行档位的 DEC-TB-PLAN-1 必须禁用");

    // 无可行档位的候选卡片必须禁用且显示无可行方案占位文案，而非 0×0m 等合成假数据
    auto *plansContainer = view.findChild<QWidget *>(QStringLiteral("DEC-RP-PLANS"));
    QVERIFY2(plansContainer != nullptr, "DEC-RP-PLANS 容器缺失");
    auto cards = plansContainer->findChildren<PlanCardWidget *>(QRegularExpression(QStringLiteral("^DEC-RP-PLAN-")));
    QVERIFY2(!cards.isEmpty(), "setSnapshot 后右面板应重建候选方案卡片");
    QVERIFY2(!cards.at(0)->isEnabled(), "无可行档位的 DEC-RP-PLAN-1 卡片必须禁用");
    {
        const auto labels = cards.at(0)->findChildren<QLabel *>();
        bool foundNoFeasible = false;
        for (const auto *lbl : labels) {
            if (lbl->text().contains(QStringLiteral("无可行方案"))) {
                foundNoFeasible = true;
                break;
            }
        }
        QVERIFY2(foundNoFeasible, "无可行卡片应显示无可行方案占位文案");
    }

    // 当前模拟选择摘要必须显示无可行占位，不得展示 0×0m 或极端工时
    auto *detailBox = view.findChild<QWidget *>(QStringLiteral("DEC-RP-DETAIL"));
    QVERIFY2(detailBox != nullptr, "DEC-RP-DETAIL 缺失");
    // 详情区标签不得含 0×0m 与极端工时合成数值，必须含无可行方案占位
    const auto detailLabels = detailBox->findChildren<QLabel *>();
    bool foundNoFeasibleDetail = false;
    for (const auto *lbl : detailLabels) {
        if (lbl->text().contains(QStringLiteral("无可行方案"))) {
            foundNoFeasibleDetail = true;
            break;
        }
    }
    QVERIFY2(foundNoFeasibleDetail, "选中无可行档位时详情区必须显示无可行方案");
    // 不得有标签展示 0×0m 起降带
    bool foundFakeSize = false;
    for (const auto *lbl : detailLabels) {
        if (lbl->text().contains(QStringLiteral("0×0m"))) {
            foundFakeSize = true;
            break;
        }
    }
    QVERIFY2(!foundFakeSize, "无可行档位不得展示 0×0m 起降带");

    // 规划状态横幅必须显示无可行
    auto *planState = view.findChild<QLabel *>(QStringLiteral("DEC-CE-PLAN-STATE"));
    QVERIFY2(planState != nullptr, "DEC-CE-PLAN-STATE 缺失");
    QVERIFY2(planState->text().contains(QStringLiteral("无可行")),
             qPrintable(QStringLiteral("选中无可行档位应显示无可行横幅，实际：%1").arg(planState->text())));
}

void MosDecisionViewTest::negativeSelectedTierShowsNoFeasibleBanner()
{
    // 契约：selectedTier=-1（未选中任何档位）在有解结果下应显示无可行横幅，
    // 避免负值选中落入 Result 分支展示绿色有解状态
    DecisionView view;
    view.resize(1280, 720);

    auto snap = makeSeedSnapshot();
    snap.selectedTier = -1;  // 未选中任何档位
    view.setSnapshot(snap);

    auto *planState = view.findChild<QLabel *>(QStringLiteral("DEC-CE-PLAN-STATE"));
    QVERIFY(planState != nullptr);
    QVERIFY2(planState->text().contains(QStringLiteral("无可行")),
             qPrintable(QStringLiteral("selectedTier=-1 应显示无可行横幅，实际：%1").arg(planState->text())));

    // 摘要区不应残留起降带几何数据
    auto *detailBox = view.findChild<QWidget *>(QStringLiteral("DEC-RP-DETAIL"));
    QVERIFY(detailBox != nullptr);
    const auto detailLabels = detailBox->findChildren<QLabel *>();
    for (const auto *lbl : detailLabels) {
        QVERIFY2(!lbl->text().contains(QStringLiteral("起降带：")),
                 qPrintable(QStringLiteral("selectedTier=-1 摘要不应残留起降带数据，实际：%1").arg(lbl->text())));
    }
}

void MosDecisionViewTest::globalStyleExposesTokenBasedControls()
{
    // 契约：全局样式必须覆盖 QSpinBox/QDoubleSpinBox/QLabel，且禁用态与聚焦态使用令牌色
    // 避免控件在暗色背景上回退到原生样式或对比不足
    const QString style = GlobalStyle::getMainWindowStyle();
    QVERIFY2(style.contains(QStringLiteral("QSpinBox")),
             "全局样式必须覆盖 QSpinBox");
    QVERIFY2(style.contains(QStringLiteral("QDoubleSpinBox")),
             "全局样式必须覆盖 QDoubleSpinBox");
    QVERIFY2(style.contains(QStringLiteral("QLabel")),
             "全局样式必须覆盖 QLabel");
    QVERIFY2(style.contains(QStringLiteral("QSpinBox::up-button")),
             "全局样式必须定制步进框上下箭头按钮");
    QVERIFY2(style.contains(QStringLiteral(":disabled")),
             "全局样式必须定义禁用态");
    QVERIFY2(style.contains(QStringLiteral(":focus")),
             "全局样式必须定义聚焦态");
}

void MosDecisionViewTest::viewportScalePolicy()
{
    // 契约：clamp(min(w/1920, h/1080), 1.0, 2.0)
    // 1280x720 与 1920x1080 均为 1.0；3840x2160 为 2.0
    DecisionView view;

    view.resize(1280, 720);
    QCOMPARE(view.viewportScale(), 1.0);

    view.resize(1920, 1080);
    QCOMPARE(view.viewportScale(), 1.0);

    view.resize(3840, 2160);
    QCOMPARE(view.viewportScale(), 2.0);

    // 非对称：宽高比 16:9 但高度更低时取较小约束
    view.resize(2560, 1080);
    QCOMPARE(view.viewportScale(), 1.0);

    // 4K 超宽但高度不足时仍受高度约束
    view.resize(7680, 1080);
    QCOMPARE(view.viewportScale(), 1.0);
}

void MosDecisionViewTest::invalidTierDetailSpacingShowsPlaceholder()
{
    // 契约：选中无可行矩形档位时，模拟 Y 区间清空为 "-"，不得残留上次合法档位的数值
    DecisionView view;
    view.resize(1280, 720);
    view.setSnapshot(makeNoFeasibleSnapshot());

    auto *detailBox = view.findChild<QWidget *>(QStringLiteral("DEC-RP-DETAIL"));
    QVERIFY(detailBox != nullptr);
    const auto labels = detailBox->findChildren<QLabel *>();
    bool found = false;
    for (const auto *lbl : labels) {
        if (lbl->text().contains(QStringLiteral("模拟 Y 区间：-"))) {
            found = true;
            break;
        }
    }
    QVERIFY2(found, "无可行档位应显示模拟 Y 区间：- 而非残留上次合法档位的数值");
}

void MosDecisionViewTest::validTierDetailSpacingRestoresFixture()
{
    // 契约：合法档位按 result 矩形 yStart..yEnd 派生模拟 Y 区间，不再硬编码 23m
    DecisionView view;
    view.resize(1280, 720);
    const auto snap = makeSeedSnapshot();
    view.setSnapshot(snap);

    // 从快照选中档位（tier 1）矩形派生期望文案，避免与 fixture 数值脱钩
    QVERIFY2(snap.hasResult && snap.selectedTier >= 0
             && snap.selectedTier < snap.result.tiers.size(),
             "seed 快照必须有选中档位");
    const auto &rect = snap.result.tiers.at(snap.selectedTier).rectangle;
    QVERIFY2(rect.valid, "seed 快照选中档位必须有可行矩形");
    const QString expected = QStringLiteral("模拟 Y 区间：%1..%2m")
                                  .arg(rect.yStart, 0, 'f', 0)
                                  .arg(rect.yEnd, 0, 'f', 0);

    auto *detailBox = view.findChild<QWidget *>(QStringLiteral("DEC-RP-DETAIL"));
    QVERIFY(detailBox != nullptr);
    const auto labels = detailBox->findChildren<QLabel *>();
    bool found = false;
    for (const auto *lbl : labels) {
        if (lbl->text().contains(expected)) {
            found = true;
            break;
        }
    }
    QVERIFY2(found, qPrintable(QStringLiteral("合法档位应显示 %1，实际未找到").arg(expected)));
}

void MosDecisionViewTest::initialSnapshotSelectsFirstCraterWithoutEmitting()
{
    // 契约：首次 setSnapshot 且无已选目标时，自动选首个弹坑；状态栏/列表一致；
    // 刷新期间不发出 targetSelected（被动语义）
    DecisionView view;
    view.resize(1280, 720);

    QSignalSpy spy(&view, &DecisionView::targetSelected);
    QVERIFY(spy.isValid());

    const auto snap = makeSeedSnapshot();
    QVERIFY2(!snap.obstacles.craters.isEmpty(), "seed 快照必须含弹坑");
    const QString firstCraterId = snap.obstacles.craters.first().id;

    view.setSnapshot(snap);

    QVERIFY2(spy.count() == 0,
             qPrintable(QStringLiteral("setSnapshot 刷新期间不得发出 targetSelected，实际：%1").arg(spy.count())));

    auto *sbTarget = view.findChild<QLabel *>(QStringLiteral("DEC-SB-TARGET"));
    QVERIFY(sbTarget != nullptr);
    QCOMPARE(sbTarget->text(), QStringLiteral("当前分析目标：%1").arg(firstCraterId));

    auto *targetList = view.findChild<QListWidget *>(QStringLiteral("DEC-LP-TARGET-LIST"));
    QVERIFY(targetList != nullptr);
    QVERIFY2(!targetList->selectedItems().isEmpty(), "首项应自动选中");
    QCOMPARE(targetList->selectedItems().first()->data(Qt::UserRole).toString(), firstCraterId);
}

void MosDecisionViewTest::globalStyleCheckedButtonUsesSelectionTokens()
{
    // 契约：全局样式必须包含 QPushButton:checked 选中态，且使用 SelectionBackground/
    // SelectionBorder/TextPrimary 令牌色与显式 border，使档位工具栏按钮 checked 后有可见选中样式
    const QString style = GlobalStyle::getMainWindowStyle();
    QVERIFY2(style.contains(QStringLiteral("QPushButton:checked")),
             "全局样式必须覆盖 QPushButton:checked 选中态");
    QVERIFY2(style.contains(GlobalStyle::Colors::SelectionBackground),
             "QPushButton:checked 必须使用 SelectionBackground 令牌色");
    QVERIFY2(style.contains(GlobalStyle::Colors::SelectionBorder),
             "QPushButton:checked 必须使用 SelectionBorder 令牌色");
    QVERIFY2(style.contains(QStringLiteral("border:")),
             "QPushButton:checked 必须显式声明 border");
}

void MosDecisionViewTest::tierSelectionCheckedStateIsUnambiguous()
{
    // 契约：档位工具栏选中态必须无歧义--合法有解快照下选中档位按钮 checked；
    // 合法无解快照下选中无效档位时，所有档位按钮（含更高位的合法替代档位）均不得 checked，
    // 避免合法替代档位被误读为当前选中
    DecisionView view;
    view.resize(1280, 720);

    // 合法有解快照：selectedTier=1 且档位 1 有可行矩形，DEC-TB-PLAN-2 应 checked
    const auto happy = makeSeedSnapshot();
    QVERIFY2(happy.selectedTier == 1, "seed 快照选中档位应为 1");
    QVERIFY2(happy.result.tiers.at(1).rectangle.valid, "seed 快照档位 1 必须有可行矩形");
    view.setSnapshot(happy);

    auto *happyTier2 = view.findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-2"));
    QVERIFY2(happyTier2 != nullptr, "DEC-TB-PLAN-2 缺失");
    QVERIFY2(happyTier2->isChecked(), "合法有解快照下选中档位 DEC-TB-PLAN-2 必须 checked");
    auto *happyTier1 = view.findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-1"));
    QVERIFY2(happyTier1 != nullptr, "DEC-TB-PLAN-1 缺失");
    QVERIFY2(!happyTier1->isChecked(), "未选中的 DEC-TB-PLAN-1 不得 checked");

    // 合法无解快照：selectedTier=0（无可行矩形），effectiveTier=-1，
    // 所有档位按钮均不得 checked，尤其合法的更高替代档位 DEC-TB-PLAN-3 不得被误判为选中
    view.setSnapshot(makeNoFeasibleSnapshot());

    auto *validAlt = view.findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-3"));
    QVERIFY2(validAlt != nullptr, "DEC-TB-PLAN-3 缺失");
    QVERIFY2(validAlt->isEnabled(), "合法替代档位 DEC-TB-PLAN-3 应可用");
    QVERIFY2(!validAlt->isChecked(),
             "选中无效档位时合法替代档位 DEC-TB-PLAN-3 不得 checked");
    auto *invalidTier1 = view.findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-1"));
    QVERIFY2(invalidTier1 != nullptr, "DEC-TB-PLAN-1 缺失");
    QVERIFY2(!invalidTier1->isChecked(), "无可行档位 DEC-TB-PLAN-1 不得 checked");
    auto *invalidTier2 = view.findChild<QPushButton *>(QStringLiteral("DEC-TB-PLAN-2"));
    QVERIFY2(invalidTier2 != nullptr, "DEC-TB-PLAN-2 缺失");
    QVERIFY2(!invalidTier2->isChecked(), "无可行档位 DEC-TB-PLAN-2 不得 checked");
}

QTEST_MAIN(MosDecisionViewTest)
#include "mos_decision_view_test.moc"