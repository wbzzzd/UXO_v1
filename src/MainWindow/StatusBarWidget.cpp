#include "MainWindow/StatusBarWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QMessageBox>
#include <QPainter>

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : QWidget(parent)
    , m_deviceStatusLabel(nullptr)
    , m_batteryLabel(nullptr)
    , m_alarmScrollArea(nullptr)
    , m_alarmContainer(nullptr)
    , m_alarmLayout(nullptr)
    , m_emergencyStopBtn(nullptr)
{
    setupUi();
}

StatusBarWidget::~StatusBarWidget()
{
}

void StatusBarWidget::setupUi()
{
    setFixedHeight(60);
    setStyleSheet("background-color: #1E1E1E; border-top: 1px solid #3C3C3C;");

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, 8, 16, 8);
    mainLayout->setSpacing(20);

    m_deviceStatusLabel = new QLabel("设备: 3/5 在线", this);
    m_deviceStatusLabel->setStyleSheet("color: #FFFFFF; font-size: 14px;");
    m_deviceStatusLabel->setCursor(Qt::PointingHandCursor);
    mainLayout->addWidget(m_deviceStatusLabel);

    mainLayout->addWidget(createSeparator());

    m_batteryLabel = new QLabel("最低电量: 85%", this);
    m_batteryLabel->setStyleSheet("color: #4CAF50; font-size: 14px;");
    mainLayout->addWidget(m_batteryLabel);

    mainLayout->addWidget(createSeparator());

    m_alarmScrollArea = new QScrollArea(this);
    m_alarmScrollArea->setMaximumHeight(30);
    m_alarmScrollArea->setMinimumWidth(400);
    m_alarmScrollArea->setStyleSheet("background: transparent; border: none;");

    m_alarmContainer = new QWidget(this);
    m_alarmLayout = new QHBoxLayout(m_alarmContainer);
    m_alarmLayout->setContentsMargins(0, 0, 0, 0);
    m_alarmLayout->setSpacing(10);
    m_alarmContainer->setLayout(m_alarmLayout);

    m_alarmScrollArea->setWidget(m_alarmContainer);
    m_alarmScrollArea->setWidgetResizable(true);
    mainLayout->addWidget(m_alarmScrollArea, 1);

    mainLayout->addStretch();

    m_emergencyStopBtn = new QPushButton("  紧 急 停 止  ", this);
    m_emergencyStopBtn->setFixedSize(120, 40);
    m_emergencyStopBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #D32F2F;
            color: #FFFFFF;
            font-size: 14px;
            font-weight: bold;
            border: none;
            border-radius: 4px;
        }
        QPushButton:hover {
            background-color: #B71C1C;
        }
        QPushButton:pressed {
            background-color: #C62828;
        }
    )");
    connect(m_emergencyStopBtn, &QPushButton::clicked, this, &StatusBarWidget::onEmergencyStop);
    mainLayout->addWidget(m_emergencyStopBtn);
}

QWidget* StatusBarWidget::createSeparator()
{
    QFrame *sep = new QFrame(this);
    sep->setFixedSize(1, 24);
    sep->setStyleSheet("background-color: #3C3C3C;");
    return sep;
}

void StatusBarWidget::updateDeviceStatus(int onlineCount, int totalCount)
{
    m_deviceStatusLabel->setText(QString("设备: %1/%2 在线").arg(onlineCount).arg(totalCount));
}

void StatusBarWidget::addAlarm(const QString& message)
{
    QLabel *alarmLabel = new QLabel(message, this);
    alarmLabel->setStyleSheet(R"(
        color: #FFB74D;
        font-size: 12px;
        background-color: #2D2D2D;
        padding: 4px 8px;
        border-radius: 4px;
    )");
    m_alarmLayout->addWidget(alarmLabel);
}

void StatusBarWidget::setMinBatteryLevel(int level)
{
    QString color = (level > 60) ? "#4CAF50" : (level > 20 ? "#FFB74D" : "#FF5252");
    m_batteryLabel->setText(QString("最低电量: %1%").arg(level));
    m_batteryLabel->setStyleSheet(QString("color: %1; font-size: 14px;").arg(color));
}

void StatusBarWidget::onEmergencyStop()
{
    QMessageBox::StandardButton ret = QMessageBox::warning(this,
        "紧急停止",
        "确定要执行紧急停止吗？所有设备将立即停止！",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        emit emergencyStopClicked();
    }
}