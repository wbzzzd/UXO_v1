// 决策页快照与状态刷新实现：setSnapshot / setPlanning / 档位与目标选择 / 候选卡片重建。
// 与 DecisionView.cpp 共同实现 DecisionView 类，按职责拆分以控制单文件纯代码行数。
// 仅按传入的 MosPlanningSnapshot 副本渲染，被动发出信号；不持有会话状态、不发起规划、不联网。
// 所有数据均为本地合成 fixture 语义，非真实跑道、真实弹坑或真实安全结论。

#include "MainWindow/DecisionView.h"
#include "Common/GlobalStyle.h"
#include "MainWindow/MosParamsPanel.h"
#include "MainWindow/MosRunwayWidget.h"
#include "MainWindow/TargetCardWidget.h"
#include "MainWindow/PlanCardWidget.h"

#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

#include <cmath>

namespace {

// 合成工时格式化：非有限值或超过 1.0e6h 的合成极端值用占位文案，避免在 UI 上渲染
// 误导性的巨大数字（合法无解档位已由 rectangle.valid=false 走占位路径，此处仅兜底）
QString formatHoursWithUnit(double hours)
{
    if (!std::isfinite(hours) || hours > 1.0e6) {
        return QStringLiteral("超出模拟范围");
    }
    return QStringLiteral("%1h").arg(hours, 0, 'f', 1);
}

} // namespace

void DecisionView::selectTier(int tierIndex)
{
    // 计算有效选中档位：负值/越界/无可行矩形均视为无有效选中，
    // 避免在禁用档位的卡片与按钮上残留选中态
    int effectiveTier = tierIndex;
    if (!m_snapshot.hasResult || tierIndex < 0
        || tierIndex >= m_snapshot.result.tiers.size()
        || !m_snapshot.result.tiers.at(tierIndex).rectangle.valid) {
        effectiveTier = -1;
    }
    for (int i = 0; i < m_tierButtons.size(); ++i) {
        m_tierButtons.at(i)->setChecked(i == effectiveTier);
    }
    for (int i = 0; i < m_planCards.size(); ++i) {
        PlanCardWidget *card = m_planCards.at(i);
        if (card == nullptr) continue;
        card->setSelected(i == effectiveTier);
    }
    m_runway->setSelectedTier(effectiveTier);
    // 刷新当前模拟选择摘要：合法档位显示真实几何/估算，无可行矩形档位显示占位文案
    // 避免在合法无解场景下展示 0×0m 与极端工时等误导性合成数值
    if (m_snapshot.hasResult && tierIndex >= 0 && tierIndex < m_snapshot.result.tiers.size()) {
        const auto &tier = m_snapshot.result.tiers.at(tierIndex);
        m_detailTier->setText(QStringLiteral("当前模拟选择：档位%1").arg(tierIndex + 1));
        if (tier.rectangle.valid) {
            const double area = tier.rectangle.area;
            m_detailSize->setText(QStringLiteral("起降带：%1×%2m · %3m²")
                                      .arg(tier.rectangle.length, 0, 'f', 0)
                                      .arg(tier.rectangle.width, 0, 'f', 0)
                                      .arg(area, 0, 'f', 0));
            m_detailArea->setText(QStringLiteral("可用面积：%1 m²").arg(area, 0, 'f', 0));
            m_detailHours->setText(QStringLiteral("模拟处理耗时：%1").arg(formatHoursWithUnit(tier.estimate.totalHours)));
            m_detailDamage->setText(QStringLiteral("涉及损毁点：%1").arg(tier.repairedIds.size()));
            // 合法档位按 result 矩形 yStart..yEnd 派生模拟 Y 区间，不再硬编码 23m
            m_detailArea2->setText(QStringLiteral("模拟 Y 区间：%1..%2m")
                                       .arg(tier.rectangle.yStart, 0, 'f', 0)
                                       .arg(tier.rectangle.yEnd, 0, 'f', 0));
        } else {
            m_detailSize->setText(QStringLiteral("起降带：无可行方案"));
            m_detailArea->setText(QStringLiteral("可用面积：- m²"));
            m_detailHours->setText(QStringLiteral("模拟处理耗时：-h"));
            m_detailDamage->setText(QStringLiteral("涉及损毁点：%1").arg(tier.repairedIds.size()));
            // 无可行矩形档位清空 Y 区间，避免残留上次合法档位的数值
            m_detailArea2->setText(QStringLiteral("模拟 Y 区间：-"));
        }
    } else {
        // 负值或越界档位：清空摘要，避免残留上次选中档位的几何/工时数据
        m_detailTier->setText(QStringLiteral("当前模拟选择：未选择"));
        m_detailSize->setText(QString());
        m_detailArea->setText(QString());
        m_detailHours->setText(QString());
        m_detailDamage->setText(QString());
        // 未选档位同样清空 Y 区间，与无效档位占位一致
        m_detailArea2->setText(QStringLiteral("模拟 Y 区间：-"));
    }
}

void DecisionView::selectTarget(const QString &targetId)
{
    m_selectedTargetId = targetId;
    // 左面板列表选中态
    for (int i = 0; i < m_targetList->count(); ++i) {
        QListWidgetItem *item = m_targetList->item(i);
        const bool match = (item->data(Qt::UserRole).toString() == targetId);
        item->setSelected(match);
        auto *card = qobject_cast<TargetCardWidget*>(m_targetList->itemWidget(item));
        if (card) card->setSelected(match);
    }
    m_runway->setSelectedTargetId(targetId);
    m_sbTarget->setText(QStringLiteral("当前分析目标：%1").arg(targetId.isEmpty() ? QStringLiteral("未选择") : targetId));
}

void DecisionView::setSnapshot(const Core::MOS::MosPlanningSnapshot &snapshot)
{
    m_snapshot = snapshot;
    m_runway->setSnapshot(snapshot);
    m_paramsPanel->setParams(snapshot.params);
    // 跑道标题按快照 params.L/W 派生，不再硬编码 3000m × 50m
    m_rwTitle->setText(QStringLiteral("跑道 %1m × %2m [模拟]")
                           .arg(snapshot.params.L, 0, 'f', 0)
                           .arg(snapshot.params.W, 0, 'f', 0));

    // 重建左面板目标列表，同时检测上次选中目标是否仍存在于新快照
    m_targetList->clear();
    // 首次刷新或已选目标为空时，若新快照含目标则自动选首个弹坑；无弹坑则选首个 UXO，
    // 避免初始无选中导致列表/状态栏/跑道不一致；仅本地赋值，不发出 targetSelected
    if (m_selectedTargetId.isEmpty()
        && (!snapshot.obstacles.craters.isEmpty() || !snapshot.obstacles.uxo.isEmpty())) {
        if (!snapshot.obstacles.craters.isEmpty()) {
            m_selectedTargetId = snapshot.obstacles.craters.first().id;
        } else {
            m_selectedTargetId = snapshot.obstacles.uxo.first().id;
        }
    }
    bool targetPresent = false;
    auto addItem = [&](const QString &id, const QString &type, bool threatHigh,
                       const QString &status, const QString &coord, const QString &size) {
        auto *item = new QListWidgetItem(m_targetList);
        item->setData(Qt::UserRole, id);
        auto *card = new TargetCardWidget(m_targetList);
        card->setData(id, type, threatHigh, status, coord, size);
        item->setSizeHint(card->sizeHint());
        m_targetList->addItem(item);
        m_targetList->setItemWidget(item, card);
        if (id == m_selectedTargetId) {
            targetPresent = true;
        }
    };
    for (const auto &c : snapshot.obstacles.craters) {
        addItem(c.id, QStringLiteral("弹坑"),
                c.threat == Core::MOS::MosThreatLevel::High,
                QStringLiteral("待处理"),
                QStringLiteral("%1,%2").arg(c.x).arg(c.y),
                QStringLiteral("直径 %1m · 影响 %2m").arg(c.visibleRadius).arg(c.influenceRadius));
    }
    for (const auto &u : snapshot.obstacles.uxo) {
        addItem(u.id, QStringLiteral("未爆弹"),
                u.threat == Core::MOS::MosThreatLevel::High,
                QStringLiteral("待处理"),
                QStringLiteral("%1,%2").arg(u.x).arg(u.y),
                QStringLiteral("当量 %1kg · 影响 %2m").arg(u.syntheticYield).arg(u.influenceRadius));
    }
    const int total = snapshot.obstacles.craters.size() + snapshot.obstacles.uxo.size();
    m_paramsPanel->setDerivedCounts(total, snapshot.selectedTier >= 0 && snapshot.hasResult
        ? snapshot.result.tiers.value(snapshot.selectedTier).repairedIds.size() : 0);

    // 重建档位按钮与候选方案卡片
    int tierCount = snapshot.params.tiers;
    if (snapshot.hasResult && snapshot.result.accepted) {
        tierCount = snapshot.result.tiers.size();
    }
    rebuildTierButtons(tierCount);

    // 重建右面板候选方案卡片
    QLayoutItem *child;
    while ((child = m_plansLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    m_planCards.clear();
    if (snapshot.hasResult && snapshot.result.accepted) {
        const auto &tiers = snapshot.result.tiers;
        const int tierTotal = tiers.size();
        for (int i = 0; i < tierTotal; ++i) {
            const auto &tier = tiers.at(i);
            auto *card = new PlanCardWidget(m_plansContainer);
            card->setObjectName(QStringLiteral("DEC-RP-PLAN-%1").arg(i + 1));
            const QString name = QStringLiteral("档位%1·%2").arg(i + 1).arg(
                i == 0 ? QStringLiteral("不含处理假设")
                       : (i == tierTotal - 1 ? QStringLiteral("更多处理假设")
                                              : QStringLiteral("部分处理假设")));
            const QString badge = i == 0 ? QStringLiteral("最小面积")
                              : (i == tierTotal - 1 ? QStringLiteral("最大面积")
                                                     : QStringLiteral("中间档位"));
            const QString cls = i == 0 ? QStringLiteral("green")
                                 : (i == 1 ? QStringLiteral("orange")
                                            : QStringLiteral("red"));
            const QString effort = Core::MOS::difficultyToString(tier.estimate.difficulty);
            const double thumbLeftPct = (tier.rectangle.xStart / snapshot.params.L) * 100.0;
            const double thumbWidthPct = ((tier.rectangle.xEnd - tier.rectangle.xStart) / snapshot.params.L) * 100.0;
            card->setData(i, name, badge,
                          tier.rectangle.area,
                          QStringLiteral("%1×%2m").arg(tier.rectangle.length, 0, 'f', 0)
                                                   .arg(tier.rectangle.width, 0, 'f', 0),
                          formatHoursWithUnit(tier.estimate.totalHours), cls,
                          tier.repairedIds.size(),
                          effort, cls,
                          QStringLiteral("%1m").arg(tier.rectangle.width, 0, 'f', 0),
                          thumbLeftPct, thumbWidthPct, tier.rectangle.valid);
            const int idx = i;
            connect(card, &PlanCardWidget::clicked, this, [this, idx](){ emit tierSelected(idx); });
            m_plansLayout->addWidget(card);
            m_planCards.append(card);
        }
    }
    m_plansLayout->addStretch();

    selectTier(snapshot.selectedTier);
    // 派生处理假设数刷新
    if (snapshot.hasResult && snapshot.selectedTier >= 0
        && snapshot.selectedTier < snapshot.result.tiers.size()) {
        const int repaired = snapshot.result.tiers.at(snapshot.selectedTier).repairedIds.size();
        m_paramsPanel->setDerivedCounts(total, repaired);
    }
    // 列表重建后复用上次选中目标；若该 ID 已不在新快照中则清空选中态，
    // 避免状态栏与列表/跑道显示指向不存在的目标
    selectTarget(targetPresent ? m_selectedTargetId : QString());
    // 规划状态横幅
    if (m_planning) {
        m_paramsPanel->setPlanState(MosParamsPanel::PlanState::Planning, tierCount);
    } else if (snapshot.hasResult && snapshot.result.accepted) {
        const bool selectedValid = snapshot.selectedTier >= 0
            && snapshot.selectedTier < snapshot.result.tiers.size()
            && snapshot.result.tiers.at(snapshot.selectedTier).rectangle.valid;
        if (selectedValid) {
            m_paramsPanel->setPlanState(MosParamsPanel::PlanState::Result, tierCount);
        } else {
            m_paramsPanel->setPlanState(MosParamsPanel::PlanState::NoFeasible, 0);
        }
    } else if (snapshot.hasResult) {
        // 规划被拒绝（accepted=false）
        m_paramsPanel->setPlanState(MosParamsPanel::PlanState::Error, 0);
    } else if (total == 0) {
        m_paramsPanel->setPlanState(MosParamsPanel::PlanState::Empty, 0);
    } else {
        // 障碍物已加载但尚未规划：等待用户点击重新规划
        m_paramsPanel->setPlanState(MosParamsPanel::PlanState::Idle, 0);
    }
}

void DecisionView::setPlanning(bool planning)
{
    m_planning = planning;
    if (planning) {
        m_paramsPanel->setPlanState(MosParamsPanel::PlanState::Planning, 0);
    }
}

Core::MOS::MosRunwayParams DecisionView::currentParams() const
{
    return m_paramsPanel->currentParams();
}

Core::MOS::MosObstacleSet DecisionView::currentObstacles() const
{
    return m_snapshot.obstacles;
}
