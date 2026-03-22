#include "MainWindow/StatusBarWidget.h"
#include <QMessageBox>

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : QStatusBar(parent)
    , m_deviceStatusLabel(nullptr)
    , m_batteryLabel(nullptr)
    , m_alarmScrollArea(nullptr)
    , m_alarmLabel(nullptr)
    , m_emergencyStopBtn(nullptr)
{
    m_deviceStatusLabel = new QLabel(tr("设备: 0/0 在线"), this);
    addWidget(m_deviceStatusLabel);

    m_batteryLabel = new QLabel(tr("电量: --"), this);
    addWidget(m_batteryLabel);

    m_alarmLabel = new QLabel(this);
    m_alarmScrollArea = new QScrollArea(this);
    m_alarmScrollArea->setWidget(m_alarmLabel);
    m_alarmScrollArea->setMaximumHeight(30);
    addWidget(m_alarmScrollArea);

    m_emergencyStopBtn = new QPushButton(tr("紧急停止"), this);
    m_emergencyStopBtn->setStyleSheet("background-color: red; color: white; font-weight: bold;");
    m_emergencyStopBtn->setFixedWidth(120);
    connect(m_emergencyStopBtn, &QPushButton::clicked, this, &StatusBarWidget::onEmergencyStop);
    addPermanentWidget(m_emergencyStopBtn);
}

StatusBarWidget::~StatusBarWidget()
{
}

void StatusBarWidget::updateDeviceStatus(int online, int total)
{
    m_deviceStatusLabel->setText(tr("设备: %1/%2 在线").arg(online).arg(total));
}

void StatusBarWidget::addAlarm(const QString &message)
{
    QString current = m_alarmLabel->text();
    if (!current.isEmpty()) {
        current += " | ";
    }
    current += message;
    m_alarmLabel->setText(current);
}

void StatusBarWidget::clearAlarms()
{
    m_alarmLabel->clear();
}

void StatusBarWidget::onEmergencyStop()
{
    QMessageBox::StandardButton ret = QMessageBox::warning(this,
        tr("紧急停止"),
        tr("确定要执行紧急停止吗？所有设备将立即停止！"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        emit emergencyStopClicked();
    }
}
