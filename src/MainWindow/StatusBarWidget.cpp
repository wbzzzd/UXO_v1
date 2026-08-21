#include "MainWindow/StatusBarWidget.h"
#include "Common/GlobalStyle.h"
#include "MainWindow/UiIcons.h"

// FA 图标枚举码点（vendored third_party/QtAwesome）
#include "QtAwesome.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QMessageBox>
#include <QPainter>
#include <QVariant>  // 显式包含 qvariant.h：本文件的包含链未传递引入该头，QVariant 为不完整类型时 setProperty 的实参无法完成转换

namespace {

constexpr int kStatusContentHeight = 22;
constexpr int kStatusVerticalMargin = 1;
constexpr int kSeparatorHeight = 18;
constexpr int kAlarmHeight = 18;
// 紧急停止占位按钮宽度需容纳完整中文「紧急停止（模拟占位）」文本，
// 在 1280/1920/4K 下均可完整显示，且保持禁用占位语义不变。
constexpr int kEmergencyButtonWidth = 128;
constexpr int kEmergencyButtonHeight = 20;

}

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : QWidget(parent)
    , m_deviceStatusLabel(nullptr)
    , m_batteryLabel(nullptr)
    , m_simulationLabel(nullptr)
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
    // 像素回归修复（批次3门禁）：原内联样式因缺 WA_StyledBackground 在基线中从未绘制，
    // 基线状态栏实由宿主 QStatusBar 全局规则（#1E1E1E+border-top #3C3C3C）单独呈现；
    // 子部件再加 edgeBorder="top" 会在宿主分隔线旁画出第二条线（基线 #1E1E1E 行变 #3C3C3C，约1646px）。
    // 保基线像素等价：仅保留 containerBg="main"（与宿主底色一致，不可见），不加 edgeBorder。
    setProperty("containerBg", QStringLiteral("main"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(kStatusContentHeight);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, kStatusVerticalMargin, 16, kStatusVerticalMargin);
    mainLayout->setSpacing(16);

    m_deviceStatusLabel = new QLabel("设备: 3/5 在线", this);
    // (a) 主文本白色走全局 QSS textColor="white"（=%3 TextPrimary=#FFFFFF，等价基线内联 color:#FFFFFF）
    m_deviceStatusLabel->setProperty("textColor", QStringLiteral("white"));
    // 像素回归修复（批次3门禁）：基线本部件裸样式表的 background/border-top 级联到本标签被原生绘制
    //（y1056 顶边框分段线 + 不透明底）；属性化后级联消失，按标签显式恢复
    m_deviceStatusLabel->setProperty("labelBg", QStringLiteral("main"));
    m_deviceStatusLabel->setProperty("edgeBorder", QStringLiteral("top"));
    // 12px 字号走 fontSize 覆盖属性（批次5 词汇，避免 caption 角色强制的 TextSecondary 变色）
    m_deviceStatusLabel->setProperty("fontSize", QStringLiteral("12"));
    m_deviceStatusLabel->setCursor(Qt::PointingHandCursor);
    mainLayout->addWidget(m_deviceStatusLabel);

    mainLayout->addWidget(createSeparator());

    m_batteryLabel = new QLabel("最低电量: 85%", this);
    // (a) 在线绿色文本走全局 QSS textColor="online"（=%21 StatusOnline=#4CAF50），构造期静态属性先于 addWidget
    m_batteryLabel->setProperty("textColor", QStringLiteral("online"));
    // 像素回归修复（批次3门禁）：恢复基线裸样式表级联到本标签的不透明底色与顶边框（y1056 分段线）
    m_batteryLabel->setProperty("labelBg", QStringLiteral("main"));
    m_batteryLabel->setProperty("edgeBorder", QStringLiteral("top"));
    // 12px 字号走 fontSize 覆盖属性（批次5 词汇），构造期静态属性先于 addWidget
    m_batteryLabel->setProperty("fontSize", QStringLiteral("12"));
    mainLayout->addWidget(m_batteryLabel);

    mainLayout->addWidget(createSeparator());

    // 模拟模式标识
    m_simulationLabel = new QLabel("[模拟模式]", this);
    // (a) 模拟模式橙色文本走全局 QSS textColor="busy"（=%22 StatusBusy=#FFB74D=ThreatMedium），构造期静态属性先于 addWidget
    m_simulationLabel->setProperty("textColor", QStringLiteral("busy"));
    // 像素回归修复（批次3门禁）：恢复基线裸样式表级联到本标签的不透明底色与顶边框（y1056 分段线）
    m_simulationLabel->setProperty("labelBg", QStringLiteral("main"));
    m_simulationLabel->setProperty("edgeBorder", QStringLiteral("top"));
    // 12px 字号+加粗走 fontSize/fontWeight 覆盖属性（批次5 词汇）
    m_simulationLabel->setProperty("fontSize", QStringLiteral("12"));
    m_simulationLabel->setProperty("fontWeight", QStringLiteral("bold"));
    m_simulationLabel->setVisible(false);
    mainLayout->addWidget(m_simulationLabel);

    mainLayout->addWidget(createSeparator());

    m_alarmScrollArea = new QScrollArea(this);
    m_alarmScrollArea->setMaximumHeight(kAlarmHeight);
    m_alarmScrollArea->setMinimumWidth(400);
    // (c) QScrollArea 视口的 QSS 级联无法用 containerBg 属性选择器覆盖（属性仅作用于 QScrollArea 本身，不传播到视口子部件），保留内联
    m_alarmScrollArea->setStyleSheet("background: transparent; border: none;");

    m_alarmContainer = new QWidget(this);
    // 内容高度与滚动视口一致，避免尺寸提示造成底部 1px 溢出。
    m_alarmContainer->setFixedHeight(kAlarmHeight);
    m_alarmLayout = new QHBoxLayout(m_alarmContainer);
    m_alarmLayout->setContentsMargins(0, 0, 0, 0);
    m_alarmLayout->setSpacing(10);
    m_alarmContainer->setLayout(m_alarmLayout);

    m_alarmScrollArea->setWidget(m_alarmContainer);
    m_alarmScrollArea->setWidgetResizable(true);
    mainLayout->addWidget(m_alarmScrollArea, 1);

    mainLayout->addStretch();

    m_emergencyStopBtn = new QPushButton("紧急停止（模拟占位）", this);
    m_emergencyStopBtn->setFixedSize(kEmergencyButtonWidth, kEmergencyButtonHeight);
    // 模拟占位按钮：禁用且不可点击，明确告知用户无实际效果
    m_emergencyStopBtn->setEnabled(false);
    m_emergencyStopBtn->setToolTip(QStringLiteral("模拟占位，无实际效果"));
    // (c) 实心红色禁用占位按钮无对应 btnVariant 词表（subtle/flat 均为透明底），border-radius=3px≠cardRadius 4px，保留内联；颜色已用 Colors:: token
    m_emergencyStopBtn->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   font-size: 11px;"
        "   font-weight: bold;"
        "   border: 1px solid %3;"
        "   border-radius: 3px;"
        "   min-width: 0px;"
        "   max-width: 128px;"
        "   padding: 0px;"
        "}"
        "QPushButton:disabled {"
        "   background-color: %4;"
        "   color: %2;"
        "   border: 1px solid %3;"
        "}")
        .arg(GlobalStyle::Colors::DangerRed)
        .arg(GlobalStyle::Colors::TextDisabled)
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Colors::ToolbarBackground));
    // 禁用态图标色与占位按钮灰字一致（TextDisabled）
    m_emergencyStopBtn->setIconSize(QSize(12, 12));
    m_emergencyStopBtn->setIcon(UiIcons::icon(fa::fa_hand,
                                              GlobalStyle::Colors::TextPrimary,
                                              QColor(),
                                              GlobalStyle::Colors::TextDisabled));
    connect(m_emergencyStopBtn, &QPushButton::clicked, this, &StatusBarWidget::onEmergencyStop);
    mainLayout->addWidget(m_emergencyStopBtn);
}

QWidget* StatusBarWidget::createSeparator()
{
    QFrame *sep = new QFrame(this);
    sep->setFixedSize(1, kSeparatorHeight);
    // (c) Border(#3C3C3C) 不在 containerBg 词表（main/panel/toolbar/transparent）内，保留内联 setStyleSheet
    sep->setStyleSheet(QStringLiteral("background-color: %1;").arg(GlobalStyle::Colors::Border));
    return sep;
}

void StatusBarWidget::updateDeviceStatus(int onlineCount, int totalCount)
{
    m_deviceStatusLabel->setText(QString("设备: %1/%2 在线").arg(onlineCount).arg(totalCount));
}

void StatusBarWidget::addAlarm(const QString& message)
{
    QLabel *alarmLabel = new QLabel(message, this);
    // (a) 告警橙色文本走全局 QSS textColor="busy"（=%22 StatusBusy=#FFB74D=ThreatMedium），新建标签先于 setFixedHeight/addWidget
    alarmLabel->setProperty("textColor", QStringLiteral("busy"));
    alarmLabel->setFixedHeight(kAlarmHeight);
    // 告警标签=chip 徽标底（%2 底+2px/8px 内边距）+cardRadius 圆角+fontSize 12px（批次5 词汇，与原内联四属性逐值等价）
    alarmLabel->setProperty("labelBg", QStringLiteral("chip"));
    alarmLabel->setProperty("cardRadius", QStringLiteral("true"));
    alarmLabel->setProperty("fontSize", QStringLiteral("12"));
    m_alarmLayout->addWidget(alarmLabel);
}

void StatusBarWidget::setMinBatteryLevel(int level)
{
    // (b) 动态电量等级颜色（运行时计算值），保留 setStyleSheet；颜色已用 Colors:: token：充足=StatusOnline，中等=ThreatMedium，低=ThreatHigh
    QString color = (level > 60) ? GlobalStyle::Colors::StatusOnline
                                 : (level > 20 ? GlobalStyle::Colors::ThreatMedium
                                               : GlobalStyle::Colors::ThreatHigh);
    m_batteryLabel->setText(QString("最低电量: %1%").arg(level));
    m_batteryLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(color));
}

void StatusBarWidget::setSimulationMode(bool enabled)
{
    m_simulationLabel->setVisible(enabled);
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
