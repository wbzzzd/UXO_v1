#include "MainWindow/DeviceStatusPanel.h"
#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

DeviceStatusPanel::DeviceStatusPanel(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_listContainer(nullptr)
    , m_listLayout(nullptr)
{
    setupUi();
}

DeviceStatusPanel::~DeviceStatusPanel()
{
}

void DeviceStatusPanel::setupUi()
{
    setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::PanelBackground));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(6);

    QWidget *header = new QWidget(this);
    header->setFixedHeight(28);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 0);

    // 设备区仅展示本地模拟数据，不提供设备控制入口。
    m_titleLabel = new QLabel("模拟设备状态（只读）", header);
    m_titleLabel->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::TitleSize));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    m_mainLayout->addWidget(header);

    m_listContainer = new QWidget(this);
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(4);

    m_mainLayout->addWidget(m_listContainer);
    m_mainLayout->addStretch();
}

void DeviceStatusPanel::setDevices(const QVector<Core::DeviceInfo>& devices)
{
    m_devices = devices;
    refreshList();
}

void DeviceStatusPanel::refreshList()
{
    QLayoutItem *child;
    while ((child = m_listLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    m_cards.clear();

    for (const Core::DeviceInfo& device : m_devices) {
        DeviceCard card;
        card.info = device;

        card.card = new QWidget(m_listContainer);
        card.card->setFixedHeight(40);
        // QWidget 容器不在全局 QSS 覆盖范围内，需显式设置暗色背景与 hover 态。
        card.card->setStyleSheet(QString(
            "QWidget { background-color: %1; border-radius: 4px; }"
            "QWidget:hover { background-color: %2; }")
            .arg(GlobalStyle::Colors::ToolbarBackground)
            .arg(GlobalStyle::Colors::HoverBackground));
        card.card->setCursor(Qt::PointingHandCursor);

        QHBoxLayout *cardLayout = new QHBoxLayout(card.card);
        cardLayout->setContentsMargins(8, 4, 8, 4);
        cardLayout->setSpacing(8);

        QString statusColor;
        QString statusText;
        switch (device.status) {
            case Core::DeviceStatus::Online:
            case Core::DeviceStatus::Idle:
                statusColor = GlobalStyle::Colors::StatusOnline;
                statusText = "模拟在线";
                break;
            case Core::DeviceStatus::Busy:
                statusColor = GlobalStyle::Colors::StatusBusy;
                statusText = "模拟任务中";
                break;
            case Core::DeviceStatus::Offline:
                statusColor = GlobalStyle::Colors::StatusOffline;
                statusText = "模拟离线";
                break;
            case Core::DeviceStatus::Error:
                statusColor = GlobalStyle::Colors::StatusError;
                statusText = "模拟故障";
                break;
            default:
                statusColor = GlobalStyle::Colors::TextDisabled;
                statusText = "模拟未知";
        }

        card.statusDot = new QLabel("●", card.card);
        card.statusDot->setStyleSheet(QString("color: %1; font-size: 10px;").arg(statusColor));
        card.statusDot->setFixedWidth(14);
        cardLayout->addWidget(card.statusDot);

        card.nameLabel = new QLabel(device.name, card.card);
        card.nameLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(GlobalStyle::Colors::TextPrimary));
        card.nameLabel->setFixedWidth(80);
        cardLayout->addWidget(card.nameLabel);

        card.statusText = new QLabel(statusText, card.card);
        card.statusText->setStyleSheet(QString("color: %1; font-size: 11px;").arg(statusColor));
        cardLayout->addWidget(card.statusText);

        cardLayout->addStretch();

        QString batteryColor;
        if (device.batteryLevel > 60) batteryColor = GlobalStyle::Colors::StatusOnline;
        else if (device.batteryLevel > 20) batteryColor = GlobalStyle::Colors::StatusBusy;
        else batteryColor = GlobalStyle::Colors::StatusError;

        card.batteryLabel = new QLabel(QString("⚡%1%").arg(static_cast<int>(device.batteryLevel)), card.card);
        card.batteryLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(batteryColor));
        card.batteryLabel->setFixedWidth(40);
        cardLayout->addWidget(card.batteryLabel);

        // TODO: mousePressEvent 是 protected 虚函数，不能直接 connect
        // 后续应通过事件过滤器或自定义 widget 实现卡片点击
        // connect(card.card, &QWidget::mousePressEvent, this, [this, device]() {
        //     emit deviceClicked(device.id);
        // });

        m_listLayout->addWidget(card.card);
        m_cards.append(card);
    }
}

void DeviceStatusPanel::updateDeviceStatus(const QString& deviceId, Core::DeviceStatus status, double battery)
{
    for (int i = 0; i < m_cards.size(); ++i) {
        if (m_cards[i].info.id == deviceId) {
            m_cards[i].info.status = status;
            m_cards[i].info.batteryLevel = battery;

            QString statusColor;
            QString statusText;
            switch (status) {
                case Core::DeviceStatus::Online:
                case Core::DeviceStatus::Idle:
                    statusColor = GlobalStyle::Colors::StatusOnline;
                    statusText = "模拟在线";
                    break;
                case Core::DeviceStatus::Busy:
                    statusColor = GlobalStyle::Colors::StatusBusy;
                    statusText = "模拟任务中";
                    break;
                case Core::DeviceStatus::Offline:
                    statusColor = GlobalStyle::Colors::StatusOffline;
                    statusText = "模拟离线";
                    break;
                case Core::DeviceStatus::Error:
                    statusColor = GlobalStyle::Colors::StatusError;
                    statusText = "模拟故障";
                    break;
                default:
                    statusColor = GlobalStyle::Colors::TextDisabled;
                    statusText = "模拟未知";
            }

            m_cards[i].statusDot->setStyleSheet(QString("color: %1; font-size: 10px;").arg(statusColor));
            m_cards[i].statusText->setText(statusText);
            m_cards[i].statusText->setStyleSheet(QString("color: %1; font-size: 11px;").arg(statusColor));

            QString batteryColor;
            if (battery > 60) batteryColor = GlobalStyle::Colors::StatusOnline;
            else if (battery > 20) batteryColor = GlobalStyle::Colors::StatusBusy;
            else batteryColor = GlobalStyle::Colors::StatusError;

            m_cards[i].batteryLabel->setText(QString("⚡%1%").arg(static_cast<int>(battery)));
            m_cards[i].batteryLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(batteryColor));
            break;
        }
    }
}
