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
    // 面板背景转 containerBg="panel" 属性（词汇表完整覆盖，移除内联）
    setProperty("containerBg", "panel");
    setAttribute(Qt::WA_StyledBackground, true);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(4);

    QWidget *header = new QWidget(this);
    header->setFixedHeight(28);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 0);

    m_titleLabel = new QLabel("告警信息", header);
    // 标题转 labelRole="h2"（color/14px/bold 与词汇表一致，移除内联）
    m_titleLabel->setProperty("labelRole", "h2");
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
        // 正常态背景转 containerBg="toolbar"；border-radius:3px（cardRadius 为 4px 不匹配）与 :hover 背景保留内联
        alertItem->setProperty("containerBg", "toolbar");
        alertItem->setAttribute(Qt::WA_StyledBackground, true);
        alertItem->setFixedHeight(36);
        alertItem->setStyleSheet(QString(
            "QWidget { border-radius: 3px; }"
            "QWidget:hover { background-color: %1; }")
            .arg(GlobalStyle::Colors::HoverBackground));
        alertItem->setCursor(Qt::PointingHandCursor);

        QHBoxLayout *itemLayout = new QHBoxLayout(alertItem);
        itemLayout->setContentsMargins(8, 4, 8, 4);
        itemLayout->setSpacing(6);

        // 告警等级颜色映射到 textColor 语义角色（error/high/medium/secondary）
        QString levelTextColor;
        switch (alarm.level) {
            case Core::AlarmLevel::Critical:
                levelTextColor = "error";
                break;
            case Core::AlarmLevel::Error:
                levelTextColor = "high";
                break;
            case Core::AlarmLevel::Warning:
                levelTextColor = "medium";
                break;
            default:
                levelTextColor = "secondary";
        }

        QLabel *iconLabel = new QLabel("●", alertItem);
        // 颜色转 textColor 属性；10px 字号走 fontSize 词汇（批次5，先于 addWidget）
        iconLabel->setProperty("textColor", levelTextColor);
        iconLabel->setFixedWidth(16);
        iconLabel->setProperty("fontSize", "10");
        itemLayout->addWidget(iconLabel);

        QString timeStr = alarm.createTime.toString("HH:mm");
        QLabel *timeLabel = new QLabel(timeStr, alertItem);
        // 属性转换（批次5）：禁用色+11px 逐值等价走 textColor/fontSize 词汇（先于 addWidget）
        timeLabel->setProperty("textColor", "disabled");
        timeLabel->setProperty("fontSize", "11");
        timeLabel->setFixedWidth(40);
        itemLayout->addWidget(timeLabel);

        QLabel *msgLabel = new QLabel(alarm.message, alertItem);
        // 颜色转 textColor 属性；11px 字号走 fontSize 词汇（批次5，先于 addWidget）
        msgLabel->setProperty("textColor", levelTextColor);
        msgLabel->setProperty("fontSize", "11");
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
