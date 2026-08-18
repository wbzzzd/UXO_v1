// 左面板损毁目标富卡片实现
// 对齐 HTML 原型 .target-card 三行布局：ID/类型/威胁徽章、状态/坐标、尺寸
// 仅渲染本地合成 fixture，非真实目标状态。

#include "MainWindow/TargetCardWidget.h"
#include "Common/GlobalStyle.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>

TargetCardWidget::TargetCardWidget(QWidget *parent)
    : QWidget(parent)
{
    // 像素回归修复（批次3门禁）：updateStyle() 设置的根样式表（卡片底色/边框/圆角）在基线中
    // 因缺 WA_StyledBackground 从未绘制，p3 列表卡片实际透明显列表底色 #252526；
    // 构造期补 WA_StyledBackground+cardRadius 会激活该休眠样式，使卡片变为 #2D2D2D 实底
    // （基线不存在的视觉变更，p3 约27.5k px 回归）。为保基线像素等价，此处不启用 styled
    // background，根样式表维持基线同款休眠状态；后续如需激活卡片底色须显式走像素门禁。
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    // Row 1: ID(monospace) + 类型(bold, stretch) + 威胁徽章
    auto *row1 = new QHBoxLayout();
    row1->setSpacing(8);
    m_idLabel = new QLabel(this);
    // 属性转换：颜色/字号交由 labelRole="caption"（TextSecondary/12px/transparent）；
    // font-family:monospace 为 ID 专属，无对应词汇，保留内联
    m_idLabel->setProperty("labelRole", "caption");
    m_idLabel->setStyleSheet(
        QStringLiteral("font-family:'Consolas','Courier New',monospace;"));
    row1->addWidget(m_idLabel);
    m_typeLabel = new QLabel(this);
    // QLabel 字号/字重覆盖（13px bold）保留，hex 替换为 token
    m_typeLabel->setStyleSheet(
        QStringLiteral("font-weight:bold;font-size:13px;color:%1;")
            .arg(GlobalStyle::Colors::TextPrimary));
    row1->addWidget(m_typeLabel, 1);
    m_threatBadge = new QLabel(this);
    m_threatBadge->setStyleSheet(
        "font-size:10px;padding:1px 6px;border-radius:3px;font-weight:bold;");
    row1->addWidget(m_threatBadge);
    layout->addLayout(row1);

    // Row 2: 状态 + 坐标(monospace)
    auto *row2 = new QHBoxLayout();
    row2->setSpacing(8);
    m_statusLabel = new QLabel(this);
    // QLabel 字号覆盖（11px）保留，hex 替换为 token
    m_statusLabel->setStyleSheet(
        QStringLiteral("font-size:11px;color:%1;")
            .arg(GlobalStyle::Colors::TextSecondary));
    row2->addWidget(m_statusLabel);
    row2->addStretch();
    m_coordLabel = new QLabel(this);
    // QLabel 字号覆盖（11px）保留，hex 替换为 token
    m_coordLabel->setStyleSheet(
        QStringLiteral("font-family:'Consolas','Courier New',monospace;"
                       "font-size:11px;color:%1;")
            .arg(GlobalStyle::Colors::TextSecondary));
    row2->addWidget(m_coordLabel);
    layout->addLayout(row2);

    // Row 3: 尺寸(monospace)
    m_sizeLabel = new QLabel(this);
    // QLabel 字号覆盖（11px）保留，hex 替换为 token
    m_sizeLabel->setStyleSheet(
        QStringLiteral("font-family:'Consolas','Courier New',monospace;"
                       "font-size:11px;color:%1;")
            .arg(GlobalStyle::Colors::TextSecondary));
    layout->addWidget(m_sizeLabel);

    updateStyle();
}

void TargetCardWidget::setData(const QString &id, const QString &type,
                               bool threatHigh,
                               const QString &status, const QString &coord,
                               const QString &size)
{
    m_id = id;
    m_idLabel->setText(id);
    m_typeLabel->setText(type);

    // 威胁徽章颜色：高=红色 threat-high，中=橙色 threat-medium
    const QString threatText = threatHigh ? QStringLiteral("高威胁") : QStringLiteral("中威胁");
    const QString threatColor =
        threatHigh ? GlobalStyle::Colors::ThreatHigh : GlobalStyle::Colors::ThreatMedium;
    const QString threatBg =
        threatHigh ? QStringLiteral("rgba(255,82,82,0.2)")
                   : QStringLiteral("rgba(255,183,77,0.2)");
    m_threatBadge->setText(threatText);
    m_threatBadge->setStyleSheet(
        QStringLiteral("font-size:10px;padding:1px 6px;border-radius:3px;"
                       "font-weight:bold;background:%1;color:%2;border:1px solid %2;")
            .arg(threatBg, threatColor));

    m_statusLabel->setText(status);
    m_coordLabel->setText(QStringLiteral("坐标 %1").arg(coord));
    m_sizeLabel->setText(size);
}

void TargetCardWidget::setSelected(bool selected)
{
    m_selected = selected;
    updateStyle();
}

void TargetCardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void TargetCardWidget::updateStyle()
{
    // 选中态：蓝色边框 + 选中背景，匹配 HTML .target-card.active
    // 默认态：普通边框 + 工具栏背景
    // border-radius:4px 已由构造期 cardRadius="true" 属性规则提供，内联不再重复
    if (m_selected) {
        // 容器背景+选中边框保留，hex 替换为 token
        setStyleSheet(QStringLiteral("TargetCardWidget{background:%1;border:2px solid %2;}")
                          .arg(GlobalStyle::Colors::SelectionBackground,
                               GlobalStyle::Colors::CardSelectedBorder));
    } else {
        setStyleSheet(QStringLiteral("TargetCardWidget{background:%1;border:1px solid %2;}")
                          .arg(GlobalStyle::Colors::ToolbarBackground,
                               GlobalStyle::Colors::Border));
    }
}
