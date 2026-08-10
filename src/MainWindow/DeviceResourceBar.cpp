#include "MainWindow/DeviceResourceBar.h"

#include "Common/GlobalStyle.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QEvent>
#include <QMouseEvent>

namespace {

// 卡片 objectName 前缀，便于测试通过 findChild 定位单张卡片。
// 选中态用动态属性 selected=true 标记，不改 objectName，保证 ID 可解析。
const char *kCardObjectNamePrefix = "deviceCard_";

}  // namespace

DeviceResourceBar::DeviceResourceBar(QWidget *parent)
    : QWidget(parent)
    , m_label(nullptr)
    , m_cardContainer(nullptr)
{
    setupUi();
}

DeviceResourceBar::~DeviceResourceBar() = default;

void DeviceResourceBar::setupUi()
{
    // 固定 36px 高，深色背景，下边框分隔，对齐原型 .device-bar
    setFixedHeight(36);
    setObjectName(QStringLiteral("deviceResourceBar"));
    setStyleSheet(QString(
        "DeviceResourceBar { background-color: %1; border-bottom: 1px solid %2; }")
        .arg(GlobalStyle::Colors::ToolbarBackground)
        .arg(GlobalStyle::Colors::Border));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(8);

    // 左侧标题"设备资源"
    m_label = new QLabel(tr("设备资源"), this);
    m_label->setStyleSheet(QString("color: %1; font-size: %2px;")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Fonts::CaptionSize));
    layout->addWidget(m_label);

    // 卡片容器：水平排列，左侧对齐，超出可滚动（QHBoxLayout 不滚动，超出由父布局裁剪）
    m_cardContainer = new QWidget(this);
    m_cardContainer->setObjectName(QStringLiteral("deviceCardContainer"));
    auto *cardLayout = new QHBoxLayout(m_cardContainer);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(8);
    cardLayout->addStretch();  // 卡片左对齐，右侧留白
    layout->addWidget(m_cardContainer, 1);
}

void DeviceResourceBar::setDevices(const QVector<Core::DeviceInfo> &devices)
{
    m_devices = devices;
    // 若当前选中设备不在新列表中，清除选中
    bool selectedStillPresent = false;
    for (const auto &dev : m_devices) {
        if (dev.id == m_selectedId) {
            selectedStillPresent = true;
            break;
        }
    }
    if (!selectedStillPresent) {
        m_selectedId.clear();
    }
    rebuildCards();
}

void DeviceResourceBar::selectDevice(const QString &id)
{
    if (m_selectedId == id) {
        return;  // 已选中，无变化
    }
    m_selectedId = id;
    rebuildCards();  // 重建以刷新选中态样式
}

QString DeviceResourceBar::selectedDeviceId() const
{
    return m_selectedId;
}

void DeviceResourceBar::rebuildCards()
{
    // 清空旧卡片（保留末尾的 stretch）
    auto *cardLayout = qobject_cast<QHBoxLayout *>(m_cardContainer->layout());
    if (cardLayout == nullptr) {
        return;
    }
    // 移除并删除除 stretch 外的所有 item
    while (cardLayout->count() > 1) {
        QLayoutItem *item = cardLayout->takeAt(0);
        if (item->widget() != nullptr) {
            delete item->widget();
        }
        delete item;
    }

    // 按顺序在 stretch 之前插入新卡片
    for (const auto &dev : m_devices) {
        QWidget *card = createCard(dev);
        cardLayout->insertWidget(cardLayout->count() - 1, card);
    }
}

QWidget *DeviceResourceBar::createCard(const Core::DeviceInfo &device)
{
    const bool selected = (device.id == m_selectedId);

    auto *card = new QFrame(m_cardContainer);
    card->setObjectName(QString::fromLatin1(kCardObjectNamePrefix) + device.id);
    // 选中态用动态属性标记，便于测试断言且不破坏 ID 解析
    card->setProperty("selected", selected);

    // 卡片样式：对齐原型 .device-card
    // - 默认: 深色面板背景 + 边框
    // - 选中: 军绿色边框 + 选中背景
    const QString baseStyle = QString(
        "QFrame#%1 { background-color: %2; border: 1px solid %3; border-radius: 3px; }"
        "QFrame#%1:hover { background-color: #2A2D2E; }")
        .arg(card->objectName())
        .arg(GlobalStyle::Colors::PanelBackground)
        .arg(GlobalStyle::Colors::Border);
    const QString selectedStyle = QString(
        "QFrame#%1 { background-color: #2F3D2F; border: 1px solid %2; border-radius: 3px; }")
        .arg(card->objectName())
        .arg(GlobalStyle::Colors::PrimaryGreen);
    card->setStyleSheet(selected ? selectedStyle : baseStyle);

    auto *cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(10, 4, 10, 4);
    cardLayout->setSpacing(6);

    // 状态点（颜色随 DeviceStatus）
    auto *dot = new QLabel(card);
    const int dotSize = 8;
    dot->setFixedSize(dotSize, dotSize);
    dot->setStyleSheet(QString("background-color: %1; border-radius: 4px; border: none;")
        .arg(statusDotColor(device.status)));
    cardLayout->addWidget(dot);

    // 设备 ID（加粗）
    auto *idLabel = new QLabel(device.id, card);
    idLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: %2px; border: none;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::CaptionSize));
    cardLayout->addWidget(idLabel);

    // 电量百分比
    auto *battLabel = new QLabel(QString::number(static_cast<int>(device.batteryLevel)) + QStringLiteral("%"), card);
    battLabel->setStyleSheet(QString("color: %1; font-size: %2px; border: none;")
        .arg(GlobalStyle::Colors::StatusOnline)
        .arg(GlobalStyle::Fonts::CaptionSize));
    cardLayout->addWidget(battLabel);

    // 任务状态文案
    auto *taskLabel = new QLabel(taskStatusText(device.status), card);
    taskLabel->setStyleSheet(QString("color: %1; font-size: %2px; border: none;")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Fonts::CaptionSize));
    cardLayout->addWidget(taskLabel);

    // 点击卡片选中设备并发信号
    card->installEventFilter(this);

    return card;
}

QString DeviceResourceBar::taskStatusText(Core::DeviceStatus status) const
{
    switch (status) {
    case Core::DeviceStatus::Online:    return tr("在线");
    case Core::DeviceStatus::Idle:      return tr("待命");
    case Core::DeviceStatus::Busy:      return tr("执行中");
    case Core::DeviceStatus::Offline:   return tr("离线");
    case Core::DeviceStatus::Error:     return tr("故障");
    case Core::DeviceStatus::Maintenance: return tr("维护中");
    default:                            return tr("未知");
    }
}

QString DeviceResourceBar::statusDotColor(Core::DeviceStatus status) const
{
    switch (status) {
    case Core::DeviceStatus::Online:
    case Core::DeviceStatus::Idle:      return GlobalStyle::Colors::StatusOnline;
    case Core::DeviceStatus::Busy:      return GlobalStyle::Colors::StatusBusy;
    case Core::DeviceStatus::Error:     return GlobalStyle::Colors::StatusError;
    default:                            return GlobalStyle::Colors::StatusOffline;
    }
}

QString DeviceResourceBar::deviceIdFromCard(const QWidget *card) const
{
    if (card == nullptr) {
        return QString();
    }
    const QString name = card->objectName();
    const QString prefix = QString::fromLatin1(kCardObjectNamePrefix);
    if (!name.startsWith(prefix)) {
        return QString();
    }
    return name.mid(prefix.size());
}

const Core::DeviceInfo *DeviceResourceBar::findDevice(const QString &id) const
{
    for (const auto &dev : m_devices) {
        if (dev.id == id) {
            return &dev;
        }
    }
    return nullptr;
}

bool DeviceResourceBar::eventFilter(QObject *watched, QEvent *event)
{
    // 仅处理卡片上的鼠标释放（点击），避免拖动误触发
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *card = qobject_cast<QWidget *>(watched);
        if (card != nullptr) {
            const QString id = deviceIdFromCard(card);
            if (!id.isEmpty()) {
                const Core::DeviceInfo *dev = findDevice(id);
                if (dev != nullptr) {
                    m_selectedId = id;
                    rebuildCards();
                    emit deviceSelected(*dev);
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
