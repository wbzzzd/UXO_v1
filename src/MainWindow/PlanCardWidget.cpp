// 右面板方案富卡片实现
// 对齐 HTML 原型 .plan-card：迷你跑道缩略图 + 名称/徽章 + 3×2 信息网格
// 仅渲染本地合成 fixture，非真实方案。

#include "MainWindow/PlanCardWidget.h"
#include "Common/GlobalStyle.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>

// 颜色类名到具体颜色的映射
// green=#4CAF50(安全/低), orange=#FFB74D(中), red=#FF5252(高)
static QString colorForClass(const QString &cls)
{
    if (cls == QStringLiteral("green"))
        return QStringLiteral("#4CAF50");
    if (cls == QStringLiteral("orange"))
        return GlobalStyle::Colors::ThreatMedium;
    if (cls == QStringLiteral("red"))
        return GlobalStyle::Colors::ThreatHigh;
    return GlobalStyle::Colors::TextPrimary;
}

PlanCardWidget::PlanCardWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // 左侧：迷你跑道缩略图 48×24
    m_thumbnail = new QLabel(this);
    m_thumbnail->setFixedSize(48, 24);
    mainLayout->addWidget(m_thumbnail);

    // 右侧：名称/徽章 + 3×2 网格
    auto *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(4);

    // 名称行：名称(bold) + 徽章
    auto *nameRow = new QHBoxLayout();
    nameRow->setSpacing(6);
    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("font-size:12px;font-weight:bold;color:#FFFFFF;");
    nameRow->addWidget(m_nameLabel);
    m_badgeLabel = new QLabel(this);
    m_badgeLabel->setStyleSheet(
        "font-size:10px;padding:1px 6px;border-radius:3px;"
        "background:rgba(136,136,136,0.2);color:#AAAAAA;border:1px solid #666;");
    nameRow->addWidget(m_badgeLabel);
    nameRow->addStretch();
    rightLayout->addLayout(nameRow);

    // 3×2 信息网格
    auto *grid = new QGridLayout();
    grid->setSpacing(4);
    grid->setHorizontalSpacing(8);

    // 6 个网格项：标签 + 值
    const QString labels[6] = {
        QStringLiteral("可用面积"), QStringLiteral("尺寸"), QStringLiteral("处理工时"),
        QStringLiteral("涉及损毁"), QStringLiteral("工程量"), QStringLiteral("几何间距")
    };
    for (int i = 0; i < 6; ++i) {
        auto *lbl = new QLabel(labels[i], this);
        lbl->setStyleSheet("font-size:10px;color:#888888;");
        m_gridLabels[i] = lbl;

        auto *val = new QLabel(this);
        val->setStyleSheet("font-size:11px;color:#FFFFFF;");
        m_gridValues[i] = val;

        const int row = i / 3;
        const int col = i % 3;
        // 每个单元格用垂直布局：标签在上，值在下
        auto *cell = new QVBoxLayout();
        cell->setSpacing(1);
        cell->addWidget(lbl);
        cell->addWidget(val);
        grid->addLayout(cell, row, col);
    }
    rightLayout->addLayout(grid);

    mainLayout->addLayout(rightLayout, 1);

    updateStyle();
}

void PlanCardWidget::setData(int tierIndex, const QString &name, const QString &badge,
                             double area, const QString &sizeText,
                             const QString &time, const QString &timeCls,
                             int count,
                             const QString &effort, const QString &effortCls,
                             const QString &clearance,
                             double thumbLeftPct, double thumbWidthPct, bool valid)
{
    m_tierIndex = tierIndex;
    m_valid = valid;
    m_thumbLeftPct = thumbLeftPct;
    m_thumbWidthPct = thumbWidthPct;

    m_nameLabel->setText(valid ? name : QStringLiteral("无可行方案"));
    m_badgeLabel->setText(badge);

    if (valid) {
        m_gridValues[0]->setText(QStringLiteral("%1m²").arg(area, 0, 'f', 0));
        m_gridValues[1]->setText(sizeText);
        m_gridValues[2]->setText(time);
        m_gridValues[2]->setStyleSheet(
            QStringLiteral("font-size:11px;color:%1;").arg(colorForClass(timeCls)));
        m_gridValues[3]->setText(QStringLiteral("%1个").arg(count));
        m_gridValues[4]->setText(effort);
        m_gridValues[4]->setStyleSheet(
            QStringLiteral("font-size:11px;color:%1;").arg(colorForClass(effortCls)));
        m_gridValues[5]->setText(clearance);
    } else {
        // 无有效矩形：所有值显示 "--"
        for (int i = 0; i < 6; ++i) {
            m_gridValues[i]->setText(QStringLiteral("--"));
            m_gridValues[i]->setStyleSheet("font-size:11px;color:#666666;");
        }
    }

    updateThumbnail();
    updateStyle();
}

void PlanCardWidget::setSelected(bool selected)
{
    m_selected = selected;
    updateThumbnail();
    updateStyle();
}

void PlanCardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void PlanCardWidget::updateStyle()
{
    // 选中态：蓝色边框 + 蓝色半透明背景，匹配 HTML .plan-card.active
    // 无效态：降低不透明度
    if (m_selected) {
        setStyleSheet(QStringLiteral("PlanCardWidget{background:rgba(91,155,213,0.1);"
                                     "border:2px solid #5B9BD5;border-radius:4px;}"));
    } else {
        setStyleSheet(QStringLiteral("PlanCardWidget{background:%1;border:1px solid %2;"
                                     "border-radius:4px;}")
                          .arg(GlobalStyle::Colors::ToolbarBackground,
                               GlobalStyle::Colors::Border));
    }
    // 无效方案降低整体不透明度
    setEnabled(m_valid);
}

void PlanCardWidget::updateThumbnail()
{
    // 迷你跑道缩略图 48×24px
    // 背景：暗绿 #1a2a1a，跑道条：居中 8px 高 #3D3D3D，MOS 矩形：按百分比定位
    QPixmap pix(48, 24);
    pix.fill(QColor("#1a2a1a"));

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 跑道条：x=2..46 (宽44), y=8..16 (高8)
    const QRect runwayRect(2, 8, 44, 8);
    painter.fillRect(runwayRect, QColor(GlobalStyle::Colors::Runway));

    if (m_valid) {
        // MOS 矩形：按百分比计算位置和宽度
        const int mosX = 2 + static_cast<int>(m_thumbLeftPct / 100.0 * 44);
        const int mosW = qMax(2, static_cast<int>(m_thumbWidthPct / 100.0 * 44));
        // 选中时用蓝色，未选中时用橙色
        const QColor mosColor = m_selected ? QColor("#5B9BD5") : QColor(GlobalStyle::Colors::ThreatMedium);
        const QColor mosFill = m_selected ? QColor(91, 155, 213, 60) : QColor(255, 183, 77, 50);
        painter.setPen(QPen(mosColor, 1));
        painter.setBrush(mosFill);
        painter.drawRect(mosX, 8, mosW, 8);
    }

    painter.end();
    m_thumbnail->setPixmap(pix);
}
