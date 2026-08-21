#include "MainWindow/NavigationWidget.h"

#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "Common/GlobalStyle.h"
#include "MainWindow/UiIcons.h"

NavigationWidget::NavigationWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentIndex(0)
{
    m_navItems = {
        {"situation", "态势"},
        {"detect",    "探测"},
        {"decision",  "决策"},
        {"device",    "设备"},
        {"stats",     "统计"},
        {"config",    "配置"}
    };

    setupUi();
}

NavigationWidget::~NavigationWidget()
{
}

void NavigationWidget::setupUi()
{
    setFixedWidth(80);
    // 属性化全局 QSS：主窗体底色（替代内联 setStyleSheet）。
    // 基线像素取证更正：基线裸样式表（背景+border-right）自身因缺 WA_StyledBackground 未绘制，
    // 但其 border-right 级联到无自身样式表的 LOGO 标签被原生绘制（x79,y46-85 的 #3C3C3C 竖线）；
    // 该线现由 LOGO 标签的 edgeBorder="right" 显式恢复（见下）。
    setProperty("containerBg", "main");
    setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 16, 0, 16);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignTop);

    QLabel *logoLabel = new QLabel("UXO", this);
    logoLabel->setObjectName(QStringLiteral("DEC-NAV-LOGO"));
    logoLabel->setFixedHeight(40);
    logoLabel->setAlignment(Qt::AlignCenter);
    // LOGO 样式由全局 QSS labelRole="logo" 提供（原内联样式迁移至 GlobalStyle）
    logoLabel->setProperty("labelRole", "logo");
    // 像素回归修复（批次3门禁）：恢复基线裸样式表级联到 LOGO 的右边线（x79,y46-85 #3C3C3C）；
    // 按钮因基线即有自身样式表未受级联影响，无需处理
    logoLabel->setProperty("edgeBorder", "right");
    layout->addWidget(logoLabel);

    layout->addSpacing(16);

    // 导航按钮样式由全局 QSS navBtn/selected 属性提供（原两套内联样式已迁移至 GlobalStyle）；
    // 图标采用 FA 字形 + QToolButton TextUnderIcon，替代原“◎+\n+文字”文本拼接近似
    for (int i = 0; i < m_navItems.size(); ++i) {
        QToolButton *btn = new QToolButton(this);
        btn->setText(m_navItems[i].label);
        // 图标在上、文字在下的双行布局（批次9 图标体系接入）
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setIconSize(QSize(16, 16));
        // 稳定对象名 DEC-NAV-01..06 供集成 UI 测试与可访问性工具定位
        btn->setObjectName(QStringLiteral("DEC-NAV-%1").arg(i + 1, 2, 10, QLatin1Char('0')));
        btn->setFixedHeight(56);
        btn->setProperty("navIndex", i);
        btn->setProperty("navBtn", true);
        // 构造期设置属性，首次 polish 前生效，无需 repolish
        btn->setProperty("selected", i == 0);
        applyNavIcon(i);

        connect(btn, &QToolButton::clicked, this, [this, i]() {
            setCurrentIndex(i);
            emit navigationChanged(i);
        });

        layout->addWidget(btn);
        m_navButtons.append(btn);
    }

    layout->addStretch();
}

void NavigationWidget::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_navItems.size()) return;
    m_currentIndex = index;
    updateSelection();
}

int NavigationWidget::currentIndex() const
{
    return m_currentIndex;
}

void NavigationWidget::updateSelection()
{
    for (int i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setProperty("selected", i == m_currentIndex);
        // 运行期属性变化需 repolish，触发全局 QSS 按新属性重算样式
        m_navButtons[i]->style()->unpolish(m_navButtons[i]);
        m_navButtons[i]->style()->polish(m_navButtons[i]);
        // 图标状态色无法由 QSS 驱动，随选中态一并重建
        applyNavIcon(i);
    }
}

void NavigationWidget::applyNavIcon(int index)
{
    if (index < 0 || index >= m_navButtons.size()) return;
    // 状态色与 navBtn QSS 文本色对齐：未选=TextSecondary(%18)、选中/悬停=TextPrimary(%3)
    const QColor base = (index == m_currentIndex)
                            ? GlobalStyle::Colors::TextPrimary
                            : GlobalStyle::Colors::TextSecondary;
    m_navButtons[index]->setIcon(
        UiIcons::icon(UiIcons::navGlyph(index), base, GlobalStyle::Colors::TextPrimary));
}
