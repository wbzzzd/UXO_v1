#include "MainWindow/RightPanelWidget.h"
#include "MainWindow/SituationView.h"
#include "MainWindow/DeviceStatusPanel.h"
#include "MainWindow/DecisionSuggestionPanel.h"
#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>

RightPanelWidget::RightPanelWidget(QWidget *parent)
    : QWidget(parent)
    , m_situationView(nullptr)
    , m_deviceStatusPanel(nullptr)
    , m_decisionPanel(nullptr)
    , m_splitter(nullptr)
{
    setupUi();
}

RightPanelWidget::~RightPanelWidget()
{
}

void RightPanelWidget::setupUi()
{
    // 属性化全局 QSS：面板底色（替代内联 setStyleSheet）
    setProperty("containerBg", "panel");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(360);
    setMaximumWidth(420);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->setStyleSheet(QString(
        "QSplitter::handle { background-color: %1; height: 1px; }")
        .arg(GlobalStyle::Colors::Border));

    QWidget *mapSection = new QWidget(m_splitter);
    QVBoxLayout *mapLayout = new QVBoxLayout(mapSection);
    mapLayout->setContentsMargins(0, 0, 0, 0);
    mapLayout->setSpacing(0);

    QWidget *mapHeader = new QWidget(mapSection);
    mapHeader->setFixedHeight(32);
    // 属性化全局 QSS：工具栏底色 + 底部描边
    mapHeader->setProperty("containerBg", "toolbar");
    mapHeader->setProperty("edgeBorder", "bottom");
    mapHeader->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout *mapHeaderLayout = new QHBoxLayout(mapHeader);
    mapHeaderLayout->setContentsMargins(8, 0, 8, 0);

    QLabel *mapTitle = new QLabel("三维态势地图", mapHeader);
    // 区块标题样式由全局 QSS labelRole="h2" 提供（BodySize=15px；转换时与原内联 14px 等值，批次7 token 提升至 15px 后随全局值）
    mapTitle->setProperty("labelRole", "h2");
    mapHeaderLayout->addWidget(mapTitle);
    mapHeaderLayout->addStretch();

    QPushButton *mapFullscreenBtn = new QPushButton("全", mapHeader);
    mapFullscreenBtn->setToolTip("全屏查看");
    mapFullscreenBtn->setFixedSize(24, 24);
    // 透明图标按钮：全局 QSS btnVariant="icon"（16px 与原内联字号一致）
    mapFullscreenBtn->setProperty("btnVariant", "icon");
    mapHeaderLayout->addWidget(mapFullscreenBtn);

    mapLayout->addWidget(mapHeader);

    m_situationView = new SituationView(mapSection);
    mapLayout->addWidget(m_situationView, 1);

    m_splitter->addWidget(mapSection);

    QWidget *deviceSection = new QWidget(m_splitter);
    QVBoxLayout *deviceLayout = new QVBoxLayout(deviceSection);
    deviceLayout->setContentsMargins(0, 0, 0, 0);
    deviceLayout->setSpacing(0);

    m_deviceStatusPanel = new DeviceStatusPanel(deviceSection);
    deviceLayout->addWidget(m_deviceStatusPanel);

    m_splitter->addWidget(deviceSection);

    QWidget *decisionSection = new QWidget(m_splitter);
    QVBoxLayout *decisionLayout = new QVBoxLayout(decisionSection);
    decisionLayout->setContentsMargins(0, 0, 0, 0);
    decisionLayout->setSpacing(0);

    m_decisionPanel = new DecisionSuggestionPanel(decisionSection);
    decisionLayout->addWidget(m_decisionPanel);

    m_splitter->addWidget(decisionSection);

    // 紧凑高度分配：1280x720 下决策面板需完整显示任务详情（含指派设备与模拟声明末两行），
    // 地图/设备/决策拉伸因子采用 4/1/5：设备区收缩至 1 份释放纵向空间，
    // 4/1/5 下决策区在 1280x720 实际分得约 247px，决策区最小高度设为 240px 作为不溢出 QSplitter 父容器的安全下限，末两行仍完整显示；
    // 1920 下地图仍占主导，设备区比例相应收窄。
    decisionSection->setMinimumHeight(240);
    m_splitter->setStretchFactor(0, 4);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 5);

    mainLayout->addWidget(m_splitter);

    connect(m_deviceStatusPanel, &DeviceStatusPanel::deviceClicked,
            this, &RightPanelWidget::deviceClicked);
    connect(m_situationView, &SituationView::targetClicked,
            this, &RightPanelWidget::targetClicked);
}

SituationView* RightPanelWidget::situationView() const
{
    return m_situationView;
}

DeviceStatusPanel* RightPanelWidget::deviceStatusPanel() const
{
    return m_deviceStatusPanel;
}

DecisionSuggestionPanel* RightPanelWidget::decisionPanel() const
{
    return m_decisionPanel;
}

void RightPanelWidget::setDevices(const QVector<Core::DeviceInfo>& devices)
{
    m_deviceStatusPanel->setDevices(devices);
}

void RightPanelWidget::setTarget(const Core::TargetInfo& target)
{
    m_decisionPanel->setTarget(target);
    m_situationView->highlightTarget(target.id, true);
    m_situationView->focusOnTarget(target.position);
}

void RightPanelWidget::clearTarget()
{
    m_decisionPanel->clear();
}
