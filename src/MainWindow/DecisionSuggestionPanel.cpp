#include "MainWindow/DecisionSuggestionPanel.h"
#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QFrame>

DecisionSuggestionPanel::DecisionSuggestionPanel(QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(nullptr)
    , m_methodLabel(nullptr)
    , m_riskLabel(nullptr)
    , m_confidenceLabel(nullptr)
    , m_confidenceBar(nullptr)
    , m_detailLabel(nullptr)
    , m_saveDraftBtn(nullptr)
    , m_submitApprovalBtn(nullptr)
    , m_directStartBtn(nullptr)
{
    setupUi();
}

DecisionSuggestionPanel::~DecisionSuggestionPanel()
{
}

void DecisionSuggestionPanel::setupUi()
{
    setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::PanelBackground));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    QWidget *header = new QWidget(this);
    header->setFixedHeight(28);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 0);

    m_titleLabel = new QLabel("决策建议", header);
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
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(10);

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
        "根据目标特征分析，建议采用聚能引爆方式处置。\n"
        "预计处置时间: 15分钟\n"
        "所需物料: 聚能切割器×1, 引爆装置×1",
        contentWidget);
    m_detailLabel->setWordWrap(true);
    m_detailLabel->setStyleSheet(QString("color: %1; font-size: %2px; line-height: 1.4;")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Fonts::CaptionSize));
    contentLayout->addWidget(m_detailLabel);

    mainLayout->addWidget(contentWidget);

    QWidget *btnRow = new QWidget(this);
    QHBoxLayout *btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 4, 0, 0);
    btnLayout->setSpacing(6);
    btnLayout->addStretch();

    m_saveDraftBtn = new QPushButton("保存草稿", btnRow);
    m_saveDraftBtn->setFixedHeight(28);
    m_saveDraftBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg("transparent")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Colors::Border));
    connect(m_saveDraftBtn, &QPushButton::clicked, this, &DecisionSuggestionPanel::saveDraftRequested);
    btnLayout->addWidget(m_saveDraftBtn);

    m_submitApprovalBtn = new QPushButton("提交审批", btnRow);
    m_submitApprovalBtn->setFixedHeight(28);
    m_submitApprovalBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 4px; padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(GlobalStyle::Colors::PrimaryGreen)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::PrimaryGreenHover));
    connect(m_submitApprovalBtn, &QPushButton::clicked, this, &DecisionSuggestionPanel::submitApprovalRequested);
    btnLayout->addWidget(m_submitApprovalBtn);

    m_directStartBtn = new QPushButton("直接启动", btnRow);
    m_directStartBtn->setFixedHeight(28);
    m_directStartBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 4px; padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(GlobalStyle::Colors::DangerRed)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::DangerRedHover));
    connect(m_directStartBtn, &QPushButton::clicked, this, &DecisionSuggestionPanel::directStartRequested);
    btnLayout->addWidget(m_directStartBtn);

    mainLayout->addWidget(btnRow);
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

void DecisionSuggestionPanel::clear()
{
    m_methodLabel->setText("待评估");
    m_riskLabel->setText("● 未知");
    m_riskLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;")
        .arg(GlobalStyle::Colors::TextDisabled));
    m_confidenceBar->setValue(0);
    m_confidenceLabel->setText("0%");
    m_detailLabel->setText("请选择目标以获取决策建议");
}
