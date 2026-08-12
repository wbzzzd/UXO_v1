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
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    // Row 1: ID(monospace) + 类型(bold, stretch) + 威胁徽章
    auto *row1 = new QHBoxLayout();
    row1->setSpacing(8);
    m_idLabel = new QLabel(this);
    m_idLabel->setStyleSheet(
        "font-family:'Consolas','Courier New',monospace;"
        "font-size:12px;color:#AAAAAA;");
    row1->addWidget(m_idLabel);
    m_typeLabel = new QLabel(this);
    m_typeLabel->setStyleSheet("font-weight:bold;font-size:13px;color:#FFFFFF;");
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
    m_statusLabel->setStyleSheet("font-size:11px;color:#AAAAAA;");
    row2->addWidget(m_statusLabel);
    row2->addStretch();
    m_coordLabel = new QLabel(this);
    m_coordLabel->setStyleSheet(
        "font-family:'Consolas','Courier New',monospace;"
        "font-size:11px;color:#AAAAAA;");
    row2->addWidget(m_coordLabel);
    layout->addLayout(row2);

    // Row 3: 尺寸(monospace)
    m_sizeLabel = new QLabel(this);
    m_sizeLabel->setStyleSheet(
        "font-family:'Consolas','Courier New',monospace;"
        "font-size:11px;color:#AAAAAA;");
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
    if (m_selected) {
        setStyleSheet(QStringLiteral("TargetCardWidget{background:%1;border:2px solid #5B9BD5;"
                                     "border-radius:4px;}")
                          .arg(GlobalStyle::Colors::SelectionBackground));
    } else {
        setStyleSheet(QStringLiteral("TargetCardWidget{background:%1;border:1px solid %2;"
                                     "border-radius:4px;}")
                          .arg(GlobalStyle::Colors::ToolbarBackground,
                               GlobalStyle::Colors::Border));
    }
}
