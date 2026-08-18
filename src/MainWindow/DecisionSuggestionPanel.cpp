#include "MainWindow/DecisionSuggestionPanel.h"
#include "Common/GlobalStyle.h"
#include "Core/Simulation/SimulationWorkflow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QStyle>

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
    // 根容器基线为裸样式表（无 WA_StyledBackground 从不自身绘制，仅级联出标签底色盒），
    // 已属性化：标题 labelBg="panel"，内容区标签 labelBg="toolbar" 显式恢复基线底色。

    // 1280x720 低分辨率下决策区纵向空间有限，紧凑边距与间距保证模拟声明末行完整显示
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(2);

    QWidget *header = new QWidget(this);
    header->setFixedHeight(22);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 0);

    m_titleLabel = new QLabel("模拟决策建议", header);
    // 基线 16px 加粗主色 + Panel 底标签盒 -> labelRole="h1" + labelBg="panel"
    m_titleLabel->setProperty("labelRole", QLatin1String("h1"));
    m_titleLabel->setProperty("labelBg", QLatin1String("panel"));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    mainLayout->addWidget(header);

    QWidget *contentWidget = new QWidget(this);
    // 基线裸样式表（Toolbar 底 + 4px 圆角，自身从不绘制，仅级联内容标签底色）
    // 已属性化：内容区标签统一 labelBg="toolbar"（词汇自带 4px 圆角）恢复。

    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(6, 6, 6, 6);
    contentLayout->setSpacing(4);

    // 决策区仅展示本地模拟状态，不提供处置操作入口。
    m_simulationStatusLabel = new QLabel("[模拟] 目标状态：未选择", contentWidget);
    m_simulationStatusLabel->setObjectName("decisionSimulationStatusLabel");
    // 基线 12px 次级色 + Toolbar 底标签盒 -> labelRole="caption" + labelBg="toolbar"
    m_simulationStatusLabel->setProperty("labelRole", QLatin1String("caption"));
    m_simulationStatusLabel->setProperty("labelBg", QLatin1String("toolbar"));
    contentLayout->addWidget(m_simulationStatusLabel);

    QLabel *methodTitle = new QLabel("建议方案", contentWidget);
    methodTitle->setProperty("labelRole", QLatin1String("caption"));
    methodTitle->setProperty("labelBg", QLatin1String("toolbar"));
    contentLayout->addWidget(methodTitle);

    m_methodLabel = new QLabel("聚能引爆", contentWidget);
    // 基线 14px 加粗主绿 + Toolbar 底 -> labelRole="h2" + textColor="green" + labelBg="toolbar"
    m_methodLabel->setProperty("labelRole", QLatin1String("h2"));
    m_methodLabel->setProperty("textColor", QLatin1String("green"));
    m_methodLabel->setProperty("labelBg", QLatin1String("toolbar"));
    contentLayout->addWidget(m_methodLabel);

    QHBoxLayout *riskRow = new QHBoxLayout();
    riskRow->setSpacing(8);

    QLabel *riskTitle = new QLabel("风险等级:", contentWidget);
    riskTitle->setProperty("labelRole", QLatin1String("caption"));
    riskTitle->setProperty("labelBg", QLatin1String("toolbar"));
    riskRow->addWidget(riskTitle);

    m_riskLabel = new QLabel("● 中", contentWidget);
    // 12px 加粗不在角色词汇内，保留仅字体声明的内联样式（不影响属性化底色/前景色）；
    // 语义色走 textColor 属性（构造期 medium，运行期随风险等级切换并 repolish）。
    m_riskLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    m_riskLabel->setProperty("textColor", QLatin1String("medium"));
    m_riskLabel->setProperty("labelBg", QLatin1String("toolbar"));
    riskRow->addWidget(m_riskLabel);
    riskRow->addStretch();
    contentLayout->addLayout(riskRow);

    QHBoxLayout *confRow = new QHBoxLayout();
    confRow->setSpacing(8);

    QLabel *confTitle = new QLabel("置信度:", contentWidget);
    confTitle->setProperty("labelRole", QLatin1String("caption"));
    confTitle->setProperty("labelBg", QLatin1String("toolbar"));
    confRow->addWidget(confTitle);

    // QProgressBar 为令牌驱动的组件级样式，QLabel 属性词汇不适用，判定保留
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
    // 基线 12px 主文字色 + Toolbar 底 -> caption 排版 + textColor="white" 覆盖次级色
    m_confidenceLabel->setProperty("labelRole", QLatin1String("caption"));
    m_confidenceLabel->setProperty("textColor", QLatin1String("white"));
    m_confidenceLabel->setProperty("labelBg", QLatin1String("toolbar"));
    m_confidenceLabel->setFixedWidth(36);
    confRow->addWidget(m_confidenceLabel);
    contentLayout->addLayout(confRow);

    m_detailLabel = new QLabel(
        QStringLiteral("[模拟模式] 请选择目标以获取决策建议。\n（当前数据为模拟，不连接真实设备）"),
        contentWidget);
    m_detailLabel->setWordWrap(true);
    // 基线 12px 次级色 + Toolbar 底；line-height 声明 Qt5 QSS 不支持（无效），未迁移
    m_detailLabel->setProperty("labelRole", QLatin1String("caption"));
    m_detailLabel->setProperty("labelBg", QLatin1String("toolbar"));
    contentLayout->addWidget(m_detailLabel);

    mainLayout->addWidget(contentWidget);
}

void DecisionSuggestionPanel::setSuggestion(const QString& method, const QString& riskLevel, double confidence)
{
    m_methodLabel->setText(method);

    // 风险语义色映射为 textColor 属性（high/medium/low），切换后 repolish 生效；
    // 其余取值（含“未知”）沿用 ThreatLow，保持基线行为
    const char *riskRole = "low";
    if (riskLevel == "高") riskRole = "high";
    else if (riskLevel == "中") riskRole = "medium";

    m_riskLabel->setText(QString("● %1").arg(riskLevel));
    m_riskLabel->setProperty("textColor", QLatin1String(riskRole));
    m_riskLabel->style()->unpolish(m_riskLabel);
    m_riskLabel->style()->polish(m_riskLabel);

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
    // 未选择态降级为禁用色（textColor 属性 + repolish）
    m_riskLabel->setProperty("textColor", QLatin1String("disabled"));
    m_riskLabel->style()->unpolish(m_riskLabel);
    m_riskLabel->style()->polish(m_riskLabel);
    m_confidenceBar->setValue(0);
    m_confidenceLabel->setText("0%");
    m_detailLabel->setText("请选择目标以获取决策建议");
}
