#include "MainWindow/DetectionControlPanel.h"
#include "Common/GlobalStyle.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QTextEdit>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int kPanelMargin = 8;
constexpr int kPanelSpacing = 8;
constexpr int kHeaderHeight = 28;
constexpr int kInfoRowHeight = 40;
constexpr int kButtonHeight = 32;
constexpr int kButtonWidth = 68;
constexpr int kButtonRadius = 4;
constexpr int kLogMinimumHeight = 72;
constexpr int kLogRadius = 4;
constexpr int kLogPadding = 4;
constexpr int kLogFontSize = 10;

const QString kEmptyLogText = QStringLiteral("暂无模拟操作记录（重启后清空）");

}

DetectionControlPanel::DetectionControlPanel(QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(nullptr)
    , m_targetLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_confirmButton(nullptr)
    , m_startButton(nullptr)
    , m_completeButton(nullptr)
    , m_operationLog(nullptr)
{
    setupUi();
}

DetectionControlPanel::~DetectionControlPanel()
{
}

void DetectionControlPanel::setupUi()
{
    setStyleSheet(QStringLiteral("background-color: %1;")
                      .arg(GlobalStyle::Colors::PanelBackground));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(kPanelMargin, kPanelMargin, kPanelMargin, kPanelMargin);
    mainLayout->setSpacing(kPanelSpacing);

    QWidget *header = new QWidget(this);
    header->setFixedHeight(kHeaderHeight);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    m_titleLabel = new QLabel(QStringLiteral("模拟流程与操作日志"), header);
    m_titleLabel->setStyleSheet(
        QStringLiteral("color: %1; font-size: %2px; font-weight: %3;")
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Fonts::BodySize)
            .arg(GlobalStyle::Fonts::TitleWeight));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    mainLayout->addWidget(header);

    QWidget *statusRow = new QWidget(this);
    statusRow->setFixedHeight(kInfoRowHeight);
    QVBoxLayout *statusLayout = new QVBoxLayout(statusRow);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(0);

    const QString primaryLabelStyle =
        QStringLiteral("color: %1; font-size: %2px;")
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Fonts::CaptionSize);
    const QString secondaryLabelStyle =
        QStringLiteral("color: %1; font-size: %2px;")
            .arg(GlobalStyle::Colors::TextSecondary)
            .arg(GlobalStyle::Fonts::CaptionSize);

    m_targetLabel = new QLabel(statusRow);
    m_targetLabel->setObjectName(QStringLiteral("simulationTargetLabel"));
    m_targetLabel->setStyleSheet(primaryLabelStyle);
    statusLayout->addWidget(m_targetLabel);

    m_statusLabel = new QLabel(statusRow);
    m_statusLabel->setObjectName(QStringLiteral("simulationStatusLabel"));
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_statusLabel->setStyleSheet(secondaryLabelStyle);
    statusLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(statusRow);

    QWidget *actionRow = new QWidget(this);
    actionRow->setFixedHeight(kButtonHeight);
    QHBoxLayout *actionLayout = new QHBoxLayout(actionRow);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(kPanelSpacing);

    const QString buttonStyle = QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none; "
        "border-radius: %3px; padding: 0px; font-size: %4px; "
        "min-width: %5px; max-width: %5px; }"
        "QPushButton:hover { background-color: %6; }"
        "QPushButton:disabled { background-color: %7; color: %8; }")
        .arg(GlobalStyle::Colors::PrimaryGreen)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(kButtonRadius)
        .arg(GlobalStyle::Fonts::CaptionSize)
        .arg(kButtonWidth)
        .arg(GlobalStyle::Colors::PrimaryGreenHover)
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Colors::TextDisabled);

    // 三个按钮保持明确的模拟语义，并在 1280px 窗口的半宽信息面板内完整显示。
    m_confirmButton = new QPushButton(QStringLiteral("模拟确认"), actionRow);
    m_confirmButton->setObjectName(QStringLiteral("simulationConfirmButton"));
    m_confirmButton->setFixedSize(kButtonWidth, kButtonHeight);
    m_confirmButton->setStyleSheet(buttonStyle);
    connect(m_confirmButton,
            &QPushButton::clicked,
            this,
            &DetectionControlPanel::confirmSimulationRequested);
    actionLayout->addWidget(m_confirmButton);

    m_startButton = new QPushButton(QStringLiteral("模拟处置"), actionRow);
    m_startButton->setObjectName(QStringLiteral("simulationStartButton"));
    m_startButton->setFixedSize(kButtonWidth, kButtonHeight);
    m_startButton->setStyleSheet(buttonStyle);
    connect(m_startButton,
            &QPushButton::clicked,
            this,
            &DetectionControlPanel::startSimulationDisposalRequested);
    actionLayout->addWidget(m_startButton);

    m_completeButton = new QPushButton(QStringLiteral("模拟完成"), actionRow);
    m_completeButton->setObjectName(QStringLiteral("simulationCompleteButton"));
    m_completeButton->setFixedSize(kButtonWidth, kButtonHeight);
    m_completeButton->setStyleSheet(buttonStyle);
    connect(m_completeButton,
            &QPushButton::clicked,
            this,
            &DetectionControlPanel::completeSimulationDisposalRequested);
    actionLayout->addWidget(m_completeButton);
    actionLayout->addStretch();
    mainLayout->addWidget(actionRow);

    m_operationLog = new QTextEdit(this);
    m_operationLog->setObjectName(QStringLiteral("simulationOperationLog"));
    m_operationLog->setReadOnly(true);
    m_operationLog->setMinimumHeight(kLogMinimumHeight);
    m_operationLog->setStyleSheet(QStringLiteral(
        "QTextEdit { background-color: %1; color: %2; border: 1px solid %3; "
        "border-radius: %4px; font-size: %5px; padding: %6px; }")
        .arg(GlobalStyle::Colors::Background)
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Colors::Border)
        .arg(kLogRadius)
        .arg(kLogFontSize)
        .arg(kLogPadding));
    mainLayout->addWidget(m_operationLog, 1);

    showNoSelection();
    setOperationLog({});
}

void DetectionControlPanel::showNoSelection()
{
    m_targetLabel->setText(QStringLiteral("模拟目标：未选择"));
    m_statusLabel->setText(QStringLiteral("模拟状态：未选择"));
    updateActionAvailability(Core::TargetStatus::Unknown);
}

void DetectionControlPanel::setSelectedTarget(const Core::TargetInfo &target)
{
    m_targetLabel->setText(QStringLiteral("模拟目标：%1").arg(target.id));
    m_statusLabel->setText(
        QStringLiteral("模拟状态：%1")
            .arg(Core::Simulation::SimulationWorkflow::simulationStatusText(target.status)));
    updateActionAvailability(target.status);
}

void DetectionControlPanel::setOperationLog(
    const QVector<Core::Simulation::SimulationOperationLogEntry> &entries)
{
    if (entries.isEmpty()) {
        m_operationLog->setPlainText(kEmptyLogText);
        return;
    }

    // 日志展示按稳定序号排序，不依赖输入容器顺序或系统时钟精度。
    QVector<Core::Simulation::SimulationOperationLogEntry> orderedEntries = entries;
    std::stable_sort(
        orderedEntries.begin(),
        orderedEntries.end(),
        [](const Core::Simulation::SimulationOperationLogEntry &left,
           const Core::Simulation::SimulationOperationLogEntry &right) {
            return left.sequence < right.sequence;
        });

    QStringList lines;
    lines.reserve(orderedEntries.size());
    for (const Core::Simulation::SimulationOperationLogEntry &entry : orderedEntries) {
        // 使用条目记录的 UTC 时间，保留阶段 4 已确认的时间戳与原始消息格式。
        lines.append(QStringLiteral("[%1] %2")
                         .arg(entry.timestampUtc.toUTC().toString(QStringLiteral("HH:mm:ss")))
                         .arg(entry.message));
    }
    m_operationLog->setPlainText(lines.join(QLatin1Char('\n')));
}

void DetectionControlPanel::updateActionAvailability(Core::TargetStatus status)
{
    m_confirmButton->setEnabled(status == Core::TargetStatus::Detected);
    m_startButton->setEnabled(status == Core::TargetStatus::Confirmed);
    m_completeButton->setEnabled(status == Core::TargetStatus::Disposing);
}
