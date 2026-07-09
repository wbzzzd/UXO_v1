#include "MainWindow/AlertPanel.h"
#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QMouseEvent>

AlertPanel::AlertPanel(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_listContainer(nullptr)
    , m_listLayout(nullptr)
{
    setupUi();
}

AlertPanel::~AlertPanel()
{
}

void AlertPanel::setupUi()
{
    setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::PanelBackground));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(4);

    QWidget *header = new QWidget(this);
    header->setFixedHeight(28);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 0);

    m_titleLabel = new QLabel("告警信息", header);
    m_titleLabel->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::BodySize));
    headerLayout->addWidget(m_titleLabel);

    QLabel *countLabel = new QLabel("0", header);
    countLabel->setObjectName("alertCount");
    countLabel->setFixedSize(20, 20);
    countLabel->setAlignment(Qt::AlignCenter);
    countLabel->setStyleSheet(QString(
        "background-color: %1; color: white; border-radius: 10px; font-size: 10px; font-weight: bold;")
        .arg(GlobalStyle::Colors::DangerRed));
    headerLayout->addWidget(countLabel);
    headerLayout->addStretch();

    m_mainLayout->addWidget(header);

    m_listContainer = new QWidget(this);
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(2);

    m_mainLayout->addWidget(m_listContainer);
    m_mainLayout->addStretch();
}

void AlertPanel::addAlert(const Core::AlarmInfo& alarm)
{
    m_alerts.append(alarm);
    refreshList();
}

void AlertPanel::clearAlerts()
{
    m_alerts.clear();
    refreshList();
}

void AlertPanel::refreshList()
{
    QLayoutItem *child;
    while ((child = m_listLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QLabel *countLabel = findChild<QLabel*>("alertCount");
    if (countLabel) {
        countLabel->setText(QString::number(m_alerts.size()));
    }

    int maxDisplay = qMin(m_alerts.size(), 5);
    for (int i = 0; i < maxDisplay; ++i) {
        const Core::AlarmInfo& alarm = m_alerts[i];

        QWidget *alertItem = new QWidget(m_listContainer);
        alertItem->setFixedHeight(36);
        alertItem->setStyleSheet(QString(
            "QWidget { background-color: %1; border-radius: 3px; }"
            "QWidget:hover { background-color: #363636; }")
            .arg(GlobalStyle::Colors::ToolbarBackground));
        alertItem->setCursor(Qt::PointingHandCursor);

        QHBoxLayout *itemLayout = new QHBoxLayout(alertItem);
        itemLayout->setContentsMargins(8, 4, 8, 4);
        itemLayout->setSpacing(6);

        QString levelColor;
        QString levelIcon;
        switch (alarm.level) {
            case Core::AlarmLevel::Critical:
                levelColor = GlobalStyle::Colors::DangerRed;
                levelIcon = "🔴";
                break;
            case Core::AlarmLevel::Error:
                levelColor = GlobalStyle::Colors::ThreatHigh;
                levelIcon = "🔴";
                break;
            case Core::AlarmLevel::Warning:
                levelColor = GlobalStyle::Colors::ThreatMedium;
                levelIcon = "🟡";
                break;
            default:
                levelColor = GlobalStyle::Colors::TextSecondary;
                levelIcon = "🔵";
        }

        QLabel *iconLabel = new QLabel(levelIcon, alertItem);
        iconLabel->setFixedWidth(16);
        iconLabel->setStyleSheet("font-size: 10px;");
        itemLayout->addWidget(iconLabel);

        QString timeStr = alarm.createTime.toString("HH:mm");
        QLabel *timeLabel = new QLabel(timeStr, alertItem);
        timeLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(GlobalStyle::Colors::TextDisabled));
        timeLabel->setFixedWidth(40);
        itemLayout->addWidget(timeLabel);

        QLabel *msgLabel = new QLabel(alarm.message, alertItem);
        msgLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(levelColor));
        msgLabel->setWordWrap(false);
        itemLayout->addWidget(msgLabel, 1);

        // TODO: mousePressEvent 是 protected 虚函数，不能直接 connect
        // 后续应通过事件过滤器或自定义 widget 实现告警条目点击
        // connect(alertItem, &QWidget::mousePressEvent, this, [this, alarm]() {
        //     emit alertClicked(alarm.id);
        // });

        m_listLayout->addWidget(alertItem);
    }

    if (m_alerts.size() > 5) {
        QLabel *moreLabel = new QLabel(
            QString("还有 %1 条告警...").arg(m_alerts.size() - 5), m_listContainer);
        moreLabel->setAlignment(Qt::AlignCenter);
        moreLabel->setStyleSheet(QString("color: %1; font-size: 11px; padding: 4px;")
            .arg(GlobalStyle::Colors::TextDisabled));
        m_listLayout->addWidget(moreLabel);
    }

    if (m_alerts.isEmpty()) {
        QLabel *emptyLabel = new QLabel("暂无告警", m_listContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(QString("color: %1; font-size: 12px; padding: 16px;")
            .arg(GlobalStyle::Colors::TextDisabled));
        m_listLayout->addWidget(emptyLabel);
    }
}
