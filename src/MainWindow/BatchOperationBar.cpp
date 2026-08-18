#include "MainWindow/BatchOperationBar.h"
#include "Common/GlobalStyle.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

BatchOperationBar::BatchOperationBar(QWidget *parent)
    : QWidget(parent)
    , m_countLabel(nullptr)
    , m_assignBtn(nullptr)
    , m_ignoreBtn(nullptr)
    , m_selectedCount(0)
{
    setupUi();
    hide();
}

BatchOperationBar::~BatchOperationBar()
{
}

void BatchOperationBar::setupUi()
{
    // 顶部分隔线走全局 QSS edgeBorder="top"（构造期静态属性，先于 setFixedHeight，无需 repolish）
    setProperty("edgeBorder", "top");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(48);
    // #333333 不在 Colors:: token 与 containerBg 词表内，背景色保留内联
    setStyleSheet(QString("background-color: %1;").arg("#333333"));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(12);

    m_countLabel = new QLabel("已选择: 0", this);
    // 文本主色走全局 QSS textColor="white"（=%3 TextPrimary=#FFFFFF），构造期静态属性先于 addWidget
    m_countLabel->setProperty("textColor", "white");
    // 12px 字号无对应 labelRole 词表（caption/body2 强制 TextSecondary 会变色），保留内联
    m_countLabel->setStyleSheet("font-size: 12px;");
    layout->addWidget(m_countLabel);

    layout->addStretch();

    m_assignBtn = new QPushButton("分配任务", this);
    m_assignBtn->setFixedHeight(32);
    // 实心主色按钮无对应 btnVariant 词表（subtle/flat 均为透明底），保留内联；颜色均已用 Colors:: token
    m_assignBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 4px; padding: 4px 16px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(GlobalStyle::Colors::PrimaryGreen)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::PrimaryGreenHover));
    connect(m_assignBtn, &QPushButton::clicked, this, [this]() {
        emit assignTaskRequested(m_selectedIds);
    });
    layout->addWidget(m_assignBtn);

    m_ignoreBtn = new QPushButton("标记忽略", this);
    m_ignoreBtn->setFixedHeight(32);
    // 描边次级按钮无对应 btnVariant 词表（flat=无边框、icon=无圆角、tab=下划线），保留内联；颜色均已用 Colors:: token
    m_ignoreBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; padding: 4px 16px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg("transparent")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Colors::Border));
    connect(m_ignoreBtn, &QPushButton::clicked, this, [this]() {
        emit markIgnoreRequested(m_selectedIds);
    });
    layout->addWidget(m_ignoreBtn);
}

void BatchOperationBar::setSelectedCount(int count)
{
    m_selectedCount = count;
    m_countLabel->setText(QString("已选择: %1").arg(count));

    if (count > 0) {
        show();
    } else {
        hide();
    }
}

int BatchOperationBar::selectedCount() const
{
    return m_selectedCount;
}

void BatchOperationBar::show()
{
    QWidget::show();
}

void BatchOperationBar::hide()
{
    QWidget::hide();
}
