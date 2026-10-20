#include "MainWindow/EventLogWidget.h"
#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QDateTime>

namespace {

// 「告警与日志」条带内的面板头部高度与 AlertPanel 头部一致（28px）
constexpr int kHeaderHeight = 28;
// 日志条目正文/时间戳字号：与 AlertPanel 条目 fontSize=11 对齐
constexpr int kLogFontSize = 11;
// QTextEdit 内联样式圆角/内边距（沿袭 DetectionControlPanel 日志先例值）
constexpr int kLogRadius = 4;
constexpr int kLogPadding = 4;
// 容量上限：超出滚动丢弃最旧（每条一 block，见 setupUi 的 maximumBlockCount）
constexpr int kMaxEntries = 200;

}  // namespace

EventLogWidget::EventLogWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_logView(nullptr)
{
    setupUi();
}

EventLogWidget::~EventLogWidget()
{
}

void EventLogWidget::setupUi()
{
    // 面板背景走 containerBg="panel"（与 AlertPanel 同词表，构造期静态属性先于 addWidget）
    setProperty("containerBg", "panel");
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName(QStringLiteral("eventLogWidget"));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(4);

    // 头部：标题「事件日志」（labelRole=h2 与 AlertPanel 头部同款排版）
    QWidget *header = new QWidget(this);
    header->setFixedHeight(kHeaderHeight);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 0);

    m_titleLabel = new QLabel(QStringLiteral("事件日志"), header);
    m_titleLabel->setProperty("labelRole", "h2");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    m_mainLayout->addWidget(header);

    // 只读追加式日志视图：QTextEdit 只读 + 占位符空态文案
    m_logView = new QTextEdit(this);
    m_logView->setObjectName(QStringLiteral("eventLogView"));
    m_logView->setReadOnly(true);
    m_logView->setPlaceholderText(QStringLiteral("暂无事件记录"));
    // 容量上限 200 条：每条 append 恰好一个 block，超出由 QTextDocument
    // 自动从顶部丢弃最旧 block（滚动丢弃语义，无需手工裁剪）
    m_logView->document()->setMaximumBlockCount(kMaxEntries);
    // QTextEdit 的底色/边框/字号无 QSS 词表（fieldVariant 仅 QLineEdit、
    // textColor 仅 QLabel），保留内联样式；颜色均已用 Colors:: 令牌
    //（与未纳入编译的 DetectionControlPanel 日志先例同模式）
    m_logView->setStyleSheet(QStringLiteral(
        "QTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: %4px;"
        "  font-size: %5px;"
        "  padding: %6px;"
        "}")
        .arg(GlobalStyle::Colors::Background)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::Border)
        .arg(kLogRadius)
        .arg(kLogFontSize)
        .arg(kLogPadding));

    m_mainLayout->addWidget(m_logView, 1);
}

void EventLogWidget::appendEvent(const Core::AlarmLevel level, const QString &text)
{
    // 级别 -> 语义色（与 AlertPanel.refreshList 的 textColor 角色映射一致：
    // Critical=error->DangerRed, Error=high->ThreatHigh, Warning=medium->ThreatMedium,
    // 其余=secondary->TextSecondary），保证同一事件在两区颜色一致
    QString levelColor;
    switch (level) {
        case Core::AlarmLevel::Critical:
            levelColor = GlobalStyle::Colors::DangerRed;
            break;
        case Core::AlarmLevel::Error:
            levelColor = GlobalStyle::Colors::ThreatHigh;
            break;
        case Core::AlarmLevel::Warning:
            levelColor = GlobalStyle::Colors::ThreatMedium;
            break;
        default:
            levelColor = GlobalStyle::Colors::TextSecondary;
    }

    // 条目格式：HH:mm:ss 时间戳（禁用色）+ 级别语义色正文；
    // 正文来自外部链路，先转义避免破坏富文本结构
    const QString timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    m_logView->append(QStringLiteral(
        "<span style=\"color:%1;\">%2</span> <span style=\"color:%3;\">%4</span>")
        .arg(GlobalStyle::Colors::TextDisabled, timestamp,
             levelColor, text.toHtmlEscaped()));
}
