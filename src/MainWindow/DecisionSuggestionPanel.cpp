#include "MainWindow/DecisionSuggestionPanel.h"
#include "Common/GlobalStyle.h"
#include "Core/Simulation/SimulationWorkflow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>

DecisionSuggestionPanel::DecisionSuggestionPanel(QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(nullptr)
    , m_simulationStatusLabel(nullptr)
    , m_methodLabel(nullptr)
    , m_riskLabel(nullptr)
    , m_confidenceLabel(nullptr)
    , m_confidenceBar(nullptr)
    , m_detailLabel(nullptr)
{
    setupUi();
}

DecisionSuggestionPanel::~DecisionSuggestionPanel()
{
}

void DecisionSuggestionPanel::setupUi()
{
    setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::PanelBackground));

    // 1280x720 低分辨率下决策区纵向空间有限，紧凑边距与间距保证模拟声明末行完整显示
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(2);

    QWidget *header = new QWidget(this);
    header->setFixedHeight(22);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 0);

    m_titleLabel = new QLabel("模拟决策建议", header);
    m_titleLabel->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::TitleSize));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    mainLayout->addWidget(header);

    QWidget *contentWidget = new QWidget(this);
    contentWidget->setStyleSheet(QString("background-color: %1; border-radius: 4px;")
        .arg(GlobalStyle::Colors::ToolbarBackground));

    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(6, 6, 6, 6);
    contentLayout->setSpacing(4);

    // 决策区仅展示本地模拟状态，不提供处置操作入口。
    m_simulationStatusLabel = new QLabel("[模拟] 目标状态：未选择", contentWidget);
    m_simulationStatusLabel->setObjectName("decisionSimulationStatusLabel");
    m_simulationStatusLabel->setStyleSheet(QString("color: %1; font-size: %2px;")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Fonts::CaptionSize));
    contentLayout->addWidget(m_simulationStatusLabel);

    QLabel *methodTitle = new QLabel("建议方案", contentWidget);
    methodTitle->setStyleSheet(QString("color: %1; font-size: 12px;").arg(GlobalStyle::Colors::TextSecondary));
    contentLayout->addWidget(methodTitle);

    m_methodLabel = new QLabel("聚能引爆", contentWidget);
    m_methodLabel->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold;")
        .arg(GlobalStyle::Colors::PrimaryGreen)
        .arg(GlobalStyle::Fonts::BodySize));
    contentLayout->addWidget(m_methodLabel);

    QHBoxLayout *riskRow = new QHBoxLayout();
    riskRow->setSpacing(8);

    QLabel *riskTitle = new QLabel("风险等级:", contentWidget);
    riskTitle->setStyleSheet(QString("color: %1; font-size: 12px;").arg(GlobalStyle::Colors::TextSecondary));
    riskRow->addWidget(riskTitle);

    m_riskLabel = new QLabel("● 中", contentWidget);
    m_riskLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;")
        .arg(GlobalStyle::Colors::ThreatMedium));
    riskRow->addWidget(m_riskLabel);
    riskRow->addStretch();
    contentLayout->addLayout(riskRow);

    QHBoxLayout *confRow = new QHBoxLayout();
    confRow->setSpacing(8);

    QLabel *confTitle = new QLabel("置信度:", contentWidget);
    confTitle->setStyleSheet(QString("color: %1; font-size: 12px;").arg(GlobalStyle::Colors::TextSecondary));
    confRow->addWidget(confTitle);

    m_confidenceBar = new QProgressBar(contentWidget);
    m_confidenceBar->setRange(0, 100);
    m_confidenceBar->setValue(78);
    m_confidenceBar->setFixedHeight(8);
    m_confidenceBar->setTextVisible(false);
    m_confidenceBar->setStyleSheet(QString(
        "QProgressBar { background-color: %1; border: none; border-radius: 4px; }"
        "QProgressBar::chunk { background-color: %2; border-radius: 4px; }")
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Colors::PrimaryGreen));
    confRow->addWidget(m_confidenceBar, 1);

    m_confidenceLabel = new QLabel("78%", contentWidget);
    m_confidenceLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(GlobalStyle::Colors::TextPrimary));
    m_confidenceLabel->setFixedWidth(36);
    confRow->addWidget(m_confidenceLabel);
    contentLayout->addLayout(confRow);

    m_detailLabel = new QLabel(
        QStringLiteral("[模拟模式] 请选择目标以获取决策建议。\n（当前数据为模拟，不连接真实设备）"),
        contentWidget);
    m_detailLabel->setWordWrap(true);
    m_detailLabel->setStyleSheet(QString("color: %1; font-size: %2px; line-height: 1.4;")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Fonts::CaptionSize));
    contentLayout->addWidget(m_detailLabel);

    mainLayout->addWidget(contentWidget);
}

void DecisionSuggestionPanel::setSuggestion(const QString& method, const QString& riskLevel, double confidence)
{
    m_methodLabel->setText(method);

    QString riskColor;
    if (riskLevel == "高") riskColor = GlobalStyle::Colors::ThreatHigh;
    else if (riskLevel == "中") riskColor = GlobalStyle::Colors::ThreatMedium;
    else riskColor = GlobalStyle::Colors::ThreatLow;

    m_riskLabel->setText(QString("● %1").arg(riskLevel));
    m_riskLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;").arg(riskColor));

    int confPercent = static_cast<int>(confidence * 100);
    m_confidenceBar->setValue(confPercent);
    m_confidenceLabel->setText(QString("%1%").arg(confPercent));
}

void DecisionSuggestionPanel::setTarget(const Core::TargetInfo& target)
{
    m_currentTarget = target;
    m_simulationStatusLabel->setText(
        QStringLiteral("[模拟] 目标状态：%1")
            .arg(Core::Simulation::SimulationWorkflow::simulationStatusText(target.status)));

    QString method;
    QString riskLevel;
    double confidence = target.confidence;

    switch (target.threatLevel) {
        case Core::ThreatLevel::High:
            method = "聚能引爆";
            riskLevel = "高";
            break;
        case Core::ThreatLevel::Medium:
            method = "转移处置";
            riskLevel = "中";
            break;
        case Core::ThreatLevel::Low:
            method = "人工排除";
            riskLevel = "低";
            break;
        default:
            method = "待评估";
            riskLevel = "未知";
    }

    setSuggestion(method, riskLevel, confidence);
}

// 展示模拟任务信息，更新决策详情文案
void DecisionSuggestionPanel::setMission(const Core::MissionInfo& mission)
{
    m_currentMission = mission;

    QString statusStr;
    switch (mission.status) {
        case Core::MissionStatus::Planned: statusStr = "规划中"; break;
        case Core::MissionStatus::PendingApproval: statusStr = "待审批"; break;
        case Core::MissionStatus::Approved: statusStr = "已批准"; break;
        case Core::MissionStatus::Executing: statusStr = "执行中"; break;
        case Core::MissionStatus::Completed: statusStr = "已完成"; break;
        default: statusStr = "未知";
    }

    m_detailLabel->setText(
        QString("模拟任务编号: %1\n"
                "任务状态: %2\n"
                "执行单位: %3\n"
                "指派设备: %4\n"
                "（模拟数据，不连接真实设备）")
        .arg(mission.id)
        .arg(statusStr)
        .arg(mission.assignee)
        .arg(mission.deviceId));
}

void DecisionSuggestionPanel::clear()
{
    m_simulationStatusLabel->setText(QStringLiteral("[模拟] 目标状态：未选择模拟目标"));
    m_methodLabel->setText("待评估");
    m_riskLabel->setText("● 未知");
    m_riskLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;")
        .arg(GlobalStyle::Colors::TextDisabled));
    m_confidenceBar->setValue(0);
    m_confidenceLabel->setText("0%");
    m_detailLabel->setText("请选择目标以获取决策建议");
}
