// DecisionView 档位按钮重建实现：从 DecisionViewSnapshot.cpp 拆分以满足单文件纯代码行门禁。
// 与 DecisionView.cpp / DecisionViewSnapshot.cpp 等共同实现 DecisionView 类，按职责拆分。
// 仅按传入的 MosPlanningSnapshot 副本渲染档位按钮，被动发出信号；不持有会话状态、不发起规划、不联网。
// 所有数据均为本地合成 fixture 语义，非真实跑道、真实弹坑或真实安全结论。

#include "MainWindow/DecisionView.h"
#include "Common/GlobalStyle.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QStringList>

void DecisionView::rebuildTierButtons(int tierCount)
{
    // 清空旧按钮
    qDeleteAll(m_tierButtons);
    m_tierButtons.clear();
    auto *layout = qobject_cast<QHBoxLayout *>(m_tierButtonContainer->layout());
    if (!layout) return;
    const QStringList labels = {
        QStringLiteral("不含处理假设"), QStringLiteral("部分处理假设"),
        QStringLiteral("更多处理假设"), QStringLiteral("高处理假设"),
        QStringLiteral("最高处理假设")
    };
    // 档位按钮本地令牌样式：正常态使用中性 ToolbarBackground/Border，
    // 仅 checked 使用 SelectionBackground/SelectionBorder 选中令牌；
    // 禁用态降级为 PanelBackground/TextDisabled。避免全局 QPushButton 默认军绿背景
    // 令未选中档位看起来已激活，确保无可行档位与未选中档位都不会被误读为选中，
    // 与候选卡片选中态保持一致
    const QString tierButtonStyle = QStringLiteral(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "  text-align: left;"
        "}"
        "QPushButton:hover {"
        "  background-color: %3;"
        "}"
        "QPushButton:checked {"
        "  background-color: %4;"
        "  color: %2;"
        "  border: 2px solid %5;"
        "  padding: 5px 11px;"
        "}"
        "QPushButton:disabled {"
        "  background-color: %6;"
        "  color: %7;"
        "  border: 1px solid %3;"
        "}"
    )
        .arg(GlobalStyle::Colors::ToolbarBackground)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Colors::SelectionBackground)
        .arg(GlobalStyle::Colors::SelectionBorder)
        .arg(GlobalStyle::Colors::PanelBackground)
        .arg(GlobalStyle::Colors::TextDisabled);
    for (int i = 0; i < tierCount && i < labels.size(); ++i) {
        auto *btn = new QPushButton(QStringLiteral("档位%1·%2").arg(i + 1).arg(labels.at(i)), m_tierButtonContainer);
        btn->setObjectName(QStringLiteral("DEC-TB-PLAN-%1").arg(i + 1));
        btn->setCheckable(true);
        btn->setStyleSheet(tierButtonStyle);
        // 该档位无可行矩形时禁用按钮，避免用户选中无法生成起降带的档位
        if (m_snapshot.hasResult && i < m_snapshot.result.tiers.size()
            && !m_snapshot.result.tiers.at(i).rectangle.valid) {
            btn->setEnabled(false);
        }
        const int idx = i;
        connect(btn, &QPushButton::clicked, this, [this, idx](){ emit tierSelected(idx); });
        layout->addWidget(btn);
        m_tierButtons.append(btn);
    }
}
