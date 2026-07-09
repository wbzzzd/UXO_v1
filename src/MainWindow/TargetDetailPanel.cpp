/**
 * @file MainWindow/TargetDetailPanel.cpp
 * @brief 目标详情面板实现
 * @details 实现目标详情面板的UI布局和数据展示
 * @author 开发团队
 * @date 2024-01-01
 * @version 1.0.0
 */

#include "MainWindow/TargetDetailPanel.h"
#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QDebug>

TargetDetailPanel::TargetDetailPanel(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_scrollArea(nullptr)
    , m_contentWidget(nullptr)
    , m_contentLayout(nullptr)
    , m_imageLabel(nullptr)
    , m_prevBtn(nullptr)
    , m_nextBtn(nullptr)
    , m_imageIndicator(nullptr)
    , m_currentImageIndex(0)
    , m_totalImages(3)
    , m_typeLabel(nullptr)
    , m_modelLabel(nullptr)
    , m_confidenceLabel(nullptr)
    , m_positionLabel(nullptr)
    , m_depthLabel(nullptr)
    , m_threatLabel(nullptr)
    , m_aiRecommendationLabel(nullptr)
    , m_hasTarget(false)
{
    setupUi();
}

TargetDetailPanel::~TargetDetailPanel()
{
}

void TargetDetailPanel::setupUi()
{
    // 设置面板样式
    setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::PanelBackground));
    setMinimumWidth(GlobalStyle::Sizes::RightPanelWidth);
    setMaximumWidth(GlobalStyle::Sizes::RightPanelWidth);

    // 主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 面板头部
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::ToolbarBackground));
    headerWidget->setFixedHeight(40);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(12, 0, 12, 0);

    QLabel *titleLabel = new QLabel("目标详情", headerWidget);
    titleLabel->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::TitleSize));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_mainLayout->addWidget(headerWidget);

    // 滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setStyleSheet(QString("border: none; background-color: %1;").arg(GlobalStyle::Colors::PanelBackground));
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidgetResizable(true);

    // 内容widget
    m_contentWidget = new QWidget(m_scrollArea);
    m_contentWidget->setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::PanelBackground));
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(12, 12, 12, 12);
    m_contentLayout->setSpacing(16);

    // 添加各个部分
    m_contentLayout->addWidget(createImageCarousel());
    m_contentLayout->addWidget(createInfoSection());
    m_contentLayout->addWidget(createAIRecommendation());
    m_contentLayout->addWidget(createActionButtons());
    m_contentLayout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    m_mainLayout->addWidget(m_scrollArea);

    // 初始隐藏所有内容
    clear();
}

QWidget* TargetDetailPanel::createImageCarousel()
{
    QWidget *carouselWidget = new QWidget(m_contentWidget);
    carouselWidget->setFixedHeight(200);
    carouselWidget->setStyleSheet("background-color: #1A1A1A; border-radius: 4px;");

    QVBoxLayout *layout = new QVBoxLayout(carouselWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    // 图片显示区域
    QWidget *imageArea = new QWidget(carouselWidget);
    imageArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QHBoxLayout *imageLayout = new QHBoxLayout(imageArea);
    imageLayout->setContentsMargins(0, 0, 0, 0);

    m_prevBtn = new QPushButton("<", imageArea);
    m_prevBtn->setFixedSize(30, 60);
    m_prevBtn->setStyleSheet(QString(
        "QPushButton { background-color: rgba(0,0,0,50); color: white; border: none; font-size: 18px; }"
        "QPushButton:hover { background-color: rgba(0,0,0,80); }"));
    connect(m_prevBtn, &QPushButton::clicked, this, &TargetDetailPanel::onPrevImage);
    imageLayout->addWidget(m_prevBtn);

    m_imageLabel = new QLabel(imageArea);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet(QString("color: %1; font-size: 14px;").arg(GlobalStyle::Colors::TextSecondary));
    m_imageLabel->setText("目标图像加载中...");
    imageLayout->addWidget(m_imageLabel, 1);

    m_nextBtn = new QPushButton(">", imageArea);
    m_nextBtn->setFixedSize(30, 60);
    m_nextBtn->setStyleSheet(QString(
        "QPushButton { background-color: rgba(0,0,0,50); color: white; border: none; font-size: 18px; }"
        "QPushButton:hover { background-color: rgba(0,0,0,80); }"));
    connect(m_nextBtn, &QPushButton::clicked, this, &TargetDetailPanel::onNextImage);
    imageLayout->addWidget(m_nextBtn);

    layout->addWidget(imageArea);

    // 图片指示器
    m_imageIndicator = new QLabel(carouselWidget);
    m_imageIndicator->setAlignment(Qt::AlignCenter);
    m_imageIndicator->setStyleSheet(QString("color: %1; font-size: 12px;").arg(GlobalStyle::Colors::TextSecondary));
    m_imageIndicator->setText("1 / 3");
    layout->addWidget(m_imageIndicator);

    return carouselWidget;
}

QWidget* TargetDetailPanel::createInfoSection()
{
    QWidget *infoWidget = new QWidget(m_contentWidget);
    infoWidget->setStyleSheet(QString("background-color: %1; border-radius: 4px; padding: 12px;")
        .arg(GlobalStyle::Colors::Background));

    QVBoxLayout *layout = new QVBoxLayout(infoWidget);
    layout->setSpacing(8);

    // 标题
    QLabel *sectionTitle = new QLabel("基本信息", infoWidget);
    sectionTitle->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold; margin-bottom: 8px;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::BodySize));
    layout->addWidget(sectionTitle);

    // 信息行
    auto addInfoRow = [&](const QString& label, QLabel*& valueLabel) {
        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(8);

        QLabel *labelWidget = new QLabel(label, infoWidget);
        labelWidget->setStyleSheet(QString("color: %1; font-size: %2px;")
            .arg(GlobalStyle::Colors::TextSecondary)
            .arg(GlobalStyle::Fonts::CaptionSize));
        labelWidget->setFixedWidth(60);
        row->addWidget(labelWidget);

        valueLabel = new QLabel("--", infoWidget);
        valueLabel->setStyleSheet(QString("color: %1; font-size: %2px;")
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Fonts::BodySize));
        row->addWidget(valueLabel, 1);

        layout->addLayout(row);
    };

    addInfoRow("类型", m_typeLabel);
    addInfoRow("型号", m_modelLabel);
    addInfoRow("置信度", m_confidenceLabel);
    addInfoRow("位置", m_positionLabel);
    addInfoRow("埋深", m_depthLabel);
    addInfoRow("威胁", m_threatLabel);

    return infoWidget;
}

QWidget* TargetDetailPanel::createAIRecommendation()
{
    QWidget *aiWidget = new QWidget(m_contentWidget);
    aiWidget->setStyleSheet(QString("background-color: %1; border-radius: 4px; padding: 12px;")
        .arg(GlobalStyle::Colors::ToolbarBackground));

    QVBoxLayout *layout = new QVBoxLayout(aiWidget);
    layout->setSpacing(8);

    // 标题
    QLabel *title = new QLabel("AI推荐处置方式", aiWidget);
    title->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold;")
        .arg(GlobalStyle::Colors::PrimaryGreen)
        .arg(GlobalStyle::Fonts::BodySize));
    layout->addWidget(title);

    // 推荐内容
    m_aiRecommendationLabel = new QLabel(
        "根据目标特征分析，建议采用原地引爆方式处置。\n"
        "风险等级：中\n"
        "预计处置时间：15分钟",
        aiWidget);
    m_aiRecommendationLabel->setWordWrap(true);
    m_aiRecommendationLabel->setStyleSheet(QString("color: %1; font-size: %2px; line-height: 1.5;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::CaptionSize));
    layout->addWidget(m_aiRecommendationLabel);

    return aiWidget;
}

QWidget* TargetDetailPanel::createActionButtons()
{
    QWidget *buttonWidget = new QWidget(m_contentWidget);
    QHBoxLayout *layout = new QHBoxLayout(buttonWidget);
    layout->setSpacing(8);

    QPushButton *createTaskBtn = new QPushButton("创建任务", buttonWidget);
    createTaskBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 4px; padding: 8px 16px; font-size: %3px; }"
        "QPushButton:hover { background-color: %4; }")
        .arg(GlobalStyle::Colors::PrimaryGreen)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::BodySize)
        .arg(GlobalStyle::Colors::PrimaryGreenHover));
    connect(createTaskBtn, &QPushButton::clicked, this, &TargetDetailPanel::onCreateTask);
    layout->addWidget(createTaskBtn);

    QPushButton *falseAlarmBtn = new QPushButton("标记误报", buttonWidget);
    falseAlarmBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; padding: 8px 16px; font-size: %4px; }"
        "QPushButton:hover { background-color: %1; }")
        .arg(GlobalStyle::Colors::ToolbarBackground)
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Fonts::BodySize));
    connect(falseAlarmBtn, &QPushButton::clicked, this, &TargetDetailPanel::onMarkAsFalseAlarm);
    layout->addWidget(falseAlarmBtn);

    QPushButton *ignoreBtn = new QPushButton("忽略", buttonWidget);
    ignoreBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; padding: 8px 16px; font-size: %4px; }"
        "QPushButton:hover { background-color: %1; }")
        .arg(GlobalStyle::Colors::ToolbarBackground)
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Fonts::BodySize));
    connect(ignoreBtn, &QPushButton::clicked, this, &TargetDetailPanel::onIgnoreTarget);
    layout->addWidget(ignoreBtn);

    return buttonWidget;
}

void TargetDetailPanel::setTarget(const Core::TargetInfo& target)
{
    m_currentTarget = target;
    m_hasTarget = true;

    // 更新基本信息
    m_typeLabel->setText(target.typeName);
    m_modelLabel->setText("型号-" + target.id);
    m_confidenceLabel->setText(QString::number(target.confidence * 100, 'f', 0) + "%");
    m_positionLabel->setText(QString("X:%1 Y:%2").arg(int(target.position.x())).arg(int(target.position.z())));
    m_depthLabel->setText(QString::number(target.depth, 'f', 1) + "m");

    // 威胁等级
    QString threatText;
    QString threatColor;
    switch (target.threatLevel) {
        case Core::ThreatLevel::High:
            threatText = "高";
            threatColor = GlobalStyle::Colors::ThreatHigh;
            break;
        case Core::ThreatLevel::Medium:
            threatText = "中";
            threatColor = GlobalStyle::Colors::ThreatMedium;
            break;
        case Core::ThreatLevel::Low:
            threatText = "低";
            threatColor = GlobalStyle::Colors::ThreatLow;
            break;
        default:
            threatText = "未知";
            threatColor = GlobalStyle::Colors::TextDisabled;
    }
    m_threatLabel->setText(QString("<span style='color: %1;'>●</span> %2").arg(threatColor).arg(threatText));

    // 更新图片
    updateImageDisplay();

    // 显示内容
    m_scrollArea->setVisible(true);
}

void TargetDetailPanel::clear()
{
    m_hasTarget = false;
    m_typeLabel->setText("--");
    m_modelLabel->setText("--");
    m_confidenceLabel->setText("--");
    m_positionLabel->setText("--");
    m_depthLabel->setText("--");
    m_threatLabel->setText("--");
    m_imageLabel->setText("请选择目标");
}

void TargetDetailPanel::onCreateTask()
{
    if (m_hasTarget) {
        emit createTaskRequested(m_currentTarget.id);
    }
}

void TargetDetailPanel::onMarkAsFalseAlarm()
{
    if (m_hasTarget) {
        emit markAsFalseAlarmRequested(m_currentTarget.id);
    }
}

void TargetDetailPanel::onIgnoreTarget()
{
    if (m_hasTarget) {
        emit ignoreTargetRequested(m_currentTarget.id);
    }
}

void TargetDetailPanel::onPrevImage()
{
    if (m_currentImageIndex > 0) {
        m_currentImageIndex--;
        updateImageDisplay();
    }
}

void TargetDetailPanel::onNextImage()
{
    if (m_currentImageIndex < m_totalImages - 1) {
        m_currentImageIndex++;
        updateImageDisplay();
    }
}

void TargetDetailPanel::updateImageDisplay()
{
    QStringList imageTypes = {"可见光图像", "红外图像", "雷达图像"};
    m_imageLabel->setText(QString("%1\n(模拟图像)").arg(imageTypes[m_currentImageIndex]));
    m_imageIndicator->setText(QString("%1 / %2").arg(m_currentImageIndex + 1).arg(m_totalImages));

    // 更新按钮状态
    m_prevBtn->setEnabled(m_currentImageIndex > 0);
    m_nextBtn->setEnabled(m_currentImageIndex < m_totalImages - 1);
}
