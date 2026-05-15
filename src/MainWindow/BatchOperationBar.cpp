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
    setFixedHeight(48);
    setStyleSheet(QString("background-color: %1; border-top: 1px solid %2;")
        .arg("#333333")
        .arg(GlobalStyle::Colors::Border));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(12);

    m_countLabel = new QLabel("已选择: 0", this);
    m_countLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(GlobalStyle::Colors::TextPrimary));
    layout->addWidget(m_countLabel);

    layout->addStretch();

    m_assignBtn = new QPushButton("分配任务", this);
    m_assignBtn->setFixedHeight(32);
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
