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
    setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::PanelBackground));
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
    mapHeader->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(GlobalStyle::Colors::ToolbarBackground)
        .arg(GlobalStyle::Colors::Border));
    QHBoxLayout *mapHeaderLayout = new QHBoxLayout(mapHeader);
    mapHeaderLayout->setContentsMargins(8, 0, 8, 0);

    QLabel *mapTitle = new QLabel("三维态势地图", mapHeader);
    mapTitle->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::BodySize));
    mapHeaderLayout->addWidget(mapTitle);
    mapHeaderLayout->addStretch();

    QPushButton *mapFullscreenBtn = new QPushButton("⛶", mapHeader);
    mapFullscreenBtn->setFixedSize(24, 24);
    mapFullscreenBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #AAA; border: none; font-size: 16px; }"
        "QPushButton:hover { color: white; }");
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

    m_splitter->setStretchFactor(0, 6);
    m_splitter->setStretchFactor(1, 2);
    m_splitter->setStretchFactor(2, 2);

    mainLayout->addWidget(m_splitter);

    connect(m_deviceStatusPanel, &DeviceStatusPanel::deviceClicked,
            this, &RightPanelWidget::deviceClicked);
    connect(m_deviceStatusPanel, &DeviceStatusPanel::openConsoleRequested,
            this, &RightPanelWidget::openConsoleRequested);
    connect(m_situationView, &SituationView::targetClicked,
            this, &RightPanelWidget::targetClicked);
    connect(m_decisionPanel, &DecisionSuggestionPanel::saveDraftRequested,
            this, &RightPanelWidget::saveDraftRequested);
    connect(m_decisionPanel, &DecisionSuggestionPanel::submitApprovalRequested,
            this, &RightPanelWidget::submitApprovalRequested);
    connect(m_decisionPanel, &DecisionSuggestionPanel::directStartRequested,
            this, &RightPanelWidget::directStartRequested);
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
