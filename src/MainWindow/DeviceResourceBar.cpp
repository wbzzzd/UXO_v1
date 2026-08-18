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
    // 像素回归修复（批次3门禁）：原内联 DeviceResourceBar{background:ToolbarBackground;border-bottom}
    // 因缺 WA_StyledBackground 在基线中从未绘制，基线顶条实显 centralWidget 级联 #1E1E1E；
    // 若按原型词表用 toolbar+edgeBorder 会激活基线不存在的 #2D2D2D 底色+下边线（约49k px 回归）。
    // 为保基线像素等价改用 containerBg="main"（#1E1E1E）且不加 edgeBorder；
    // 后续如需按原型 .device-bar 呈现 #2D2D2D，须作为显式视觉变更单独评审过门禁。
    setProperty("containerBg", "main");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(36);
    setObjectName(QStringLiteral("deviceResourceBar"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(8);

    // 左侧标题"设备资源"
    m_label = new QLabel(tr("设备资源"), this);
    // (a) 次级文本色+12px 字号走全局 QSS labelRole="caption"（=%18 TextSecondary=#AAAAAA + %20px CaptionSize=12），构造期静态属性先于 addWidget
    m_label->setProperty("labelRole", "caption");
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

    // (c) 卡片样式使用 per-instance objectName QSS（hover/选中态），border-radius=3px≠cardRadius 4px，无词表可表达；颜色已用 Colors:: token
    // - 默认: 深色面板背景 + 边框
    // - 选中: 军绿色边框 + 选中背景
    // hover 与选中背景使用 RowHover / RowSelected token（原 hex 为近似值）
    const QString baseStyle = QString(
        "QFrame#%1 { background-color: %2; border: 1px solid %3; border-radius: 3px; }"
        "QFrame#%1:hover { background-color: %4; }")
        .arg(card->objectName())
        .arg(GlobalStyle::Colors::PanelBackground)
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Colors::RowHover);
    const QString selectedStyle = QString(
        "QFrame#%1 { background-color: %2; border: 1px solid %3; border-radius: 3px; }")
        .arg(card->objectName())
        .arg(GlobalStyle::Colors::RowSelected)
        .arg(GlobalStyle::Colors::PrimaryGreen);
    card->setStyleSheet(selected ? selectedStyle : baseStyle);

    auto *cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(10, 4, 10, 4);
    cardLayout->setSpacing(6);

    // 状态点（颜色随 DeviceStatus）
    auto *dot = new QLabel(card);
    const int dotSize = 8;
    // (a) 状态点圆角走全局 QSS cardRadius="true"（border-radius:4px），新建标签先于 setFixedSize/addWidget
    dot->setProperty("cardRadius", "true");
    dot->setAttribute(Qt::WA_StyledBackground, true);
    dot->setFixedSize(dotSize, dotSize);
    // (b) 状态点背景色随 DeviceStatus 动态计算，保留 setStyleSheet；border:none 覆盖可能的全局 QLabel 边框
    dot->setStyleSheet(QString("background-color: %1; border: none;")
        .arg(statusDotColor(device.status)));
    cardLayout->addWidget(dot);

    // 设备 ID（加粗）
    auto *idLabel = new QLabel(device.id, card);
    // (a) 设备 ID 主文本色走全局 QSS textColor="white"（=%3 TextPrimary=#FFFFFF），新建标签先于 addWidget
    idLabel->setProperty("textColor", "white");
    // 像素回归修复（批次3门禁）：基线 centralWidget 裸样式表 #1E1E1E 级联到卡内标签（卡片底 #252526 上可见），
    // 属性化后级联消失，labelBg 按标签恢复不透明底
    idLabel->setProperty("labelBg", "main");
    // 属性转换（批次5）：12px+bold 逐值等价走 fontSize/fontWeight 词汇；border:none 为无操作声明
    // （基线 QLabel 规则无边框，各祖先样式表也无 QLabel 边框级联），删除后像素不变
    idLabel->setProperty("fontSize", "12");
    idLabel->setProperty("fontWeight", "bold");
    cardLayout->addWidget(idLabel);

    // 电量百分比
    auto *battLabel = new QLabel(QString::number(static_cast<int>(device.batteryLevel)) + QStringLiteral("%"), card);
    // (a) 电量在线绿色文本走全局 QSS textColor="online"（=%21 StatusOnline=#4CAF50），新建标签先于 addWidget
    battLabel->setProperty("textColor", "online");
    // 像素回归修复（批次3门禁）：恢复基线裸样式表级联的 #1E1E1E 不透明标签底（见 idLabel 注释）
    battLabel->setProperty("labelBg", "main");
    // 属性转换（批次5）：12px 逐值等价走 fontSize 词汇；border:none 为无操作声明（见 idLabel 注释）
    battLabel->setProperty("fontSize", "12");
    cardLayout->addWidget(battLabel);

    // 任务状态文案
    auto *taskLabel = new QLabel(taskStatusText(device.status), card);
    // (a) 任务状态次级文本色走全局 QSS textColor="secondary"（=%18 TextSecondary=#AAAAAA），新建标签先于 addWidget
    taskLabel->setProperty("textColor", "secondary");
    // 像素回归修复（批次3门禁）：恢复基线裸样式表级联的 #1E1E1E 不透明标签底（见 idLabel 注释）
    taskLabel->setProperty("labelBg", "main");
    // 属性转换（批次5）：12px 逐值等价走 fontSize 词汇；border:none 为无操作声明（见 idLabel 注释）
    taskLabel->setProperty("fontSize", "12");
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
