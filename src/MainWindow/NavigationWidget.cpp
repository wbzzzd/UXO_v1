#include "MainWindow/NavigationWidget.h"
#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>

NavigationWidget::NavigationWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentIndex(0)
{
    m_navItems = {
        {"situation", "态势", "◎"},
        {"detect",    "探测", "◎"},
        {"decision",  "决策", "◎"},
        {"device",    "设备", "◎"},
        {"stats",     "统计", "◎"},
        {"config",    "配置", "◎"}
    };

    setupUi();
}

NavigationWidget::~NavigationWidget()
{
}

void NavigationWidget::setupUi()
{
    setFixedWidth(80);
    setStyleSheet(QString("background-color: %1; border-right: 1px solid %2;")
        .arg(GlobalStyle::Colors::Background)
        .arg(GlobalStyle::Colors::Border));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 16, 0, 16);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignTop);

    QLabel *logoLabel = new QLabel("UXO", this);
    logoLabel->setFixedHeight(40);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setStyleSheet(QString(
        "color: %1; font-size: 18px; font-weight: bold; letter-spacing: 2px;")
        .arg(GlobalStyle::Colors::PrimaryGreen));
    layout->addWidget(logoLabel);

    layout->addSpacing(16);

    QString normalStyle = QString(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: %1;"
        "   border: none;"
        "   border-left: 3px solid transparent;"
        "   padding: 12px 0px;"
        "   font-size: 12px;"
        "   text-align: center;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "   color: %3;"
        "}")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg("#2A2A2A")
        .arg(GlobalStyle::Colors::TextPrimary);

    QString selectedStyle = QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: none;"
        "   border-left: 3px solid %3;"
        "   padding: 12px 0px;"
        "   font-size: 12px;"
        "   text-align: center;"
        "   font-weight: bold;"
        "}")
        .arg("#2A3F54")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::PrimaryGreen);

    for (int i = 0; i < m_navItems.size(); ++i) {
        QPushButton *btn = new QPushButton(
            m_navItems[i].icon + "\n" + m_navItems[i].label, this);
        btn->setFixedHeight(56);
        btn->setProperty("navIndex", i);
        btn->setStyleSheet(i == 0 ? selectedStyle : normalStyle);
        btn->setProperty("selected", i == 0);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
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
    QString normalStyle = QString(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: %1;"
        "   border: none;"
        "   border-left: 3px solid transparent;"
        "   padding: 12px 0px;"
        "   font-size: 12px;"
        "   text-align: center;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "   color: %3;"
        "}")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg("#2A2A2A")
        .arg(GlobalStyle::Colors::TextPrimary);

    QString selectedStyle = QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: none;"
        "   border-left: 3px solid %3;"
        "   padding: 12px 0px;"
        "   font-size: 12px;"
        "   text-align: center;"
        "   font-weight: bold;"
        "}")
        .arg("#2A3F54")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::PrimaryGreen);

    for (int i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setStyleSheet(i == m_currentIndex ? selectedStyle : normalStyle);
        m_navButtons[i]->setProperty("selected", i == m_currentIndex);
    }
}
