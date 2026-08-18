#include "MainWindow/LeftPanelWidget.h"
#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableWidgetItem>
#include <QEvent>
#include <QMouseEvent>

namespace {

// 目标表格列定义（去掉任务/设备 tab 后仅保留此一张表）
constexpr int kTargetTypeColumn = 0;
constexpr int kTargetConfidenceColumn = 1;
constexpr int kTargetPositionColumn = 2;
constexpr int kTargetStatusColumn = 3;

// 列宽基于 320px 面板与 4px item padding 实测：类型列 4 个 CJK（"反跑道雷"），位置列 "X:123 Y:0"，
// 置信度列 56px 容纳 "88%" 与表头"置信度"；位置列 88（原 100）补偿置信度增宽，状态列 stretch
constexpr int kTargetTypeColumnWidth = 92;
constexpr int kTargetConfidenceColumnWidth = 56;
constexpr int kTargetPositionColumnWidth = 88;

// 折叠态宽度（窄条）；展开态使用 GlobalStyle::Sizes::LeftPanelWidth（320px）
constexpr int kCollapsedWidth = 40;

QString simulationTargetStatusText(Core::TargetStatus status)
{
    switch (status) {
    case Core::TargetStatus::Detected:
        return QStringLiteral("已发现");
    case Core::TargetStatus::Confirmed:
        return QStringLiteral("已确认");
    case Core::TargetStatus::Disposing:
        return QStringLiteral("处置中");
    case Core::TargetStatus::Disposed:
        return QStringLiteral("已完成");
    case Core::TargetStatus::FalseAlarm:
        return QStringLiteral("误报");
    default:
        return QStringLiteral("状态未知");
    }
}

} // namespace

LeftPanelWidget::LeftPanelWidget(QWidget *parent)
    : QWidget(parent)
    , m_targetTable(nullptr)
    , m_searchEdit(nullptr)
    , m_statusTabPending(nullptr)
    , m_statusTabExecuting(nullptr)
    , m_statusTabCompleted(nullptr)
    , m_collapseBtn(nullptr)
    , m_collapsedLabel(nullptr)
    , m_contentWidget(nullptr)
    , m_collapsed(false) // 默认展开：探测阶段需可见目标列表
{
    setupUi();
    applyCollapseState();
}

LeftPanelWidget::~LeftPanelWidget()
{
}

void LeftPanelWidget::setupUi()
{
    // 折叠/展开宽度均固定，避免布局挤压主地图区
    // 面板底色由属性化全局 QSS 提供（containerBg="panel"）
    setProperty("containerBg", "panel");
    setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // === 展开态内容容器 ===
    m_contentWidget = new QWidget(this);
    m_contentWidget->setFixedWidth(GlobalStyle::Sizes::LeftPanelWidth);
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->setSpacing(8);

    // 顶部标题栏 + 折叠按钮
    QWidget *header = new QWidget(m_contentWidget);
    // 属性化全局 QSS：面板底色
    header->setProperty("containerBg", "panel");
    header->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    QLabel *title = new QLabel(QStringLiteral("目标列表"), header);
    // 区块标题样式由全局 QSS labelRole="h2" 提供（14px 与原内联字号一致）
    title->setProperty("labelRole", "h2");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    m_collapseBtn = new QPushButton(QStringLiteral("◀"), header);
    m_collapseBtn->setFixedSize(24, 24);
    m_collapseBtn->setToolTip(QStringLiteral("收起面板"));
    // 弱化按钮：全局 QSS btnVariant="subtle"
    m_collapseBtn->setProperty("btnVariant", "subtle");
    connect(m_collapseBtn, &QPushButton::clicked, this, [this]() { setCollapsed(true); });
    headerLayout->addWidget(m_collapseBtn);

    contentLayout->addWidget(header);

    // 搜索框
    m_searchEdit = new QLineEdit(m_contentWidget);
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索目标..."));
    // 紧凑输入框：全局 QSS fieldVariant="compact"
    //（原 ::placeholder 规则不被 Qt5 QSS 支持，属无效声明，未迁移）
    m_searchEdit->setProperty("fieldVariant", "compact");
    m_searchEdit->setFixedHeight(28);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &LeftPanelWidget::onSearchTextChanged);
    contentLayout->addWidget(m_searchEdit);

    // 状态子标签（按目标计数：待检测/处置中/已完成）
    QWidget *statusTabs = new QWidget(m_contentWidget);
    // 属性化全局 QSS：工具栏底色 + 卡片圆角
    statusTabs->setProperty("containerBg", "toolbar");
    statusTabs->setProperty("cardRadius", "true");
    statusTabs->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusTabs);
    statusLayout->setContentsMargins(4, 4, 4, 4);
    statusLayout->setSpacing(0);

    // 下划线选项卡：全局 QSS btnVariant="tab"，选中态由 selected 属性驱动；
    // updateStatusTabs() 仅更新文本、不切换属性，因此无需 repolish
    m_statusTabPending = new QPushButton(QStringLiteral("待检测 0"), statusTabs);
    m_statusTabPending->setProperty("btnVariant", "tab");
    m_statusTabPending->setProperty("selected", true);
    statusLayout->addWidget(m_statusTabPending, 1);

    m_statusTabExecuting = new QPushButton(QStringLiteral("处置中 0"), statusTabs);
    m_statusTabExecuting->setProperty("btnVariant", "tab");
    statusLayout->addWidget(m_statusTabExecuting, 1);

    m_statusTabCompleted = new QPushButton(QStringLiteral("已完成 0"), statusTabs);
    m_statusTabCompleted->setProperty("btnVariant", "tab");
    statusLayout->addWidget(m_statusTabCompleted, 1);

    contentLayout->addWidget(statusTabs);

    // 目标表格
    m_targetTable = new QTableWidget(m_contentWidget);
    m_targetTable->setObjectName(QStringLiteral("targetTable"));
    setupTargetList();
    contentLayout->addWidget(m_targetTable, 1);

    // 刷新按钮（底部）
    QPushButton *refreshBtn = new QPushButton(QStringLiteral("刷新"), m_contentWidget);
    refreshBtn->setFixedHeight(28);
    // 主操作紧凑按钮：全局 QSS primary + btnVariant="compact"（字号 12、解除最小宽度）
    refreshBtn->setProperty("primary", true);
    refreshBtn->setProperty("btnVariant", "compact");
    connect(refreshBtn, &QPushButton::clicked, this, &LeftPanelWidget::onRefreshTargets);
    contentLayout->addWidget(refreshBtn);

    mainLayout->addWidget(m_contentWidget);

    // === 折叠态窄条容器 ===
    m_collapsedLabel = new QWidget(this);
    m_collapsedLabel->setFixedWidth(kCollapsedWidth);
    // 属性化全局 QSS：面板底色
    m_collapsedLabel->setProperty("containerBg", "panel");
    m_collapsedLabel->setAttribute(Qt::WA_StyledBackground, true);
    QVBoxLayout *collapsedLayout = new QVBoxLayout(m_collapsedLabel);
    collapsedLayout->setContentsMargins(0, 8, 0, 8);
    collapsedLayout->setSpacing(8);
    collapsedLayout->setAlignment(Qt::AlignHCenter);

    QPushButton *expandBtn = new QPushButton(QStringLiteral("▶"), m_collapsedLabel);
    expandBtn->setFixedSize(24, 24);
    expandBtn->setToolTip(QStringLiteral("展开目标列表"));
    // 弱化按钮：全局 QSS btnVariant="subtle"
    expandBtn->setProperty("btnVariant", "subtle");
    connect(expandBtn, &QPushButton::clicked, this, [this]() { setCollapsed(false); });
    collapsedLayout->addWidget(expandBtn, 0, Qt::AlignHCenter);

    // 纵向文字 "目标列表"：逐字换行实现，避免旋转绘制的复杂度
    QLabel *vertLabel = new QLabel(QStringLiteral("目\n标\n列\n表"), m_collapsedLabel);
    vertLabel->setAlignment(Qt::AlignCenter);
    // 次级纵向文字：全局 QSS labelRole="body2"
    vertLabel->setProperty("labelRole", "body2");
    collapsedLayout->addWidget(vertLabel, 1, Qt::AlignHCenter);

    mainLayout->addWidget(m_collapsedLabel);

    // 折叠态窄条整体可点击：容器和纵向文字均安装事件过滤器
    m_collapsedLabel->installEventFilter(this);
    vertLabel->installEventFilter(this);
}

void LeftPanelWidget::setupTargetList()
{
    m_targetTable->setColumnCount(4);
    m_targetTable->setHorizontalHeaderLabels({QStringLiteral("类型"),
                                              QStringLiteral("置信度"), QStringLiteral("位置"),
                                              QStringLiteral("状态")});
    QHeaderView *targetHeader = m_targetTable->horizontalHeader();
    // 表头背景/文字色与全局 QSS 一致，仅 padding 4px 不同于全局 8px，故保留调用并用 token 替换 hex
    targetHeader->setStyleSheet(QString("QHeaderView::section { background-color: %1; color: %2; padding: 4px; }")
        .arg(GlobalStyle::Colors::ToolbarBackground)
        .arg(GlobalStyle::Colors::TextPrimary));
    targetHeader->setSectionResizeMode(kTargetTypeColumn, QHeaderView::Fixed);
    targetHeader->setSectionResizeMode(kTargetConfidenceColumn, QHeaderView::Fixed);
    targetHeader->setSectionResizeMode(kTargetPositionColumn, QHeaderView::Fixed);
    targetHeader->setSectionResizeMode(kTargetStatusColumn, QHeaderView::Stretch);
    m_targetTable->setColumnWidth(kTargetTypeColumn, kTargetTypeColumnWidth);
    m_targetTable->setColumnWidth(kTargetConfidenceColumn, kTargetConfidenceColumnWidth);
    m_targetTable->setColumnWidth(kTargetPositionColumn, kTargetPositionColumnWidth);
    m_targetTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 关闭自动折行，确保"反跑道雷"和"X:108 Y:0"等作为完整单元格显示，不产生孤字。
    m_targetTable->setWordWrap(false);
    m_targetTable->verticalHeader()->setVisible(false);
    m_targetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_targetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_targetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 关闭交替行：未选中行统一 PanelBackground 单一底色，避免奇偶行两种未选中底
    m_targetTable->setAlternatingRowColors(false);
    m_targetTable->setStyleSheet(QString(
        "QTableWidget {"
        "  background-color: %1;"          // 表格整体底色 = PanelBackground
        "  color: %2;"
        "  gridline-color: %3;"
        "  border: none;"
        "}"
        "QTableWidget::item {"
        "  padding: 4px;"
        "  background-color: %1;"          // 未选中行统一底色，配合 alternatingRowColors=false
        "}"
        "QTableWidget::item:selected {"
        "  background-color: %4;"          // 唯一绿色选中态（RowSelected）
        "  color: %2;"
        "}"
        // 覆盖全局 getMainWindowStyle 的 ::item:hover（RowHover #2A2A2A）：
        // 未选中 hover 仍用 PanelBackground，不引入第二种未选中底色
        "QTableWidget::item:hover {"
        "  background-color: %1;"
        "}"
        // 选中行 hover 保持绿色，避免被上一条 :hover 拉回 PanelBackground
        "QTableWidget::item:selected:hover {"
        "  background-color: %4;"
        "}"
        "QScrollBar:vertical {"
        "  background: %5;"
        "  width: 8px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %6;"             // BorderLight（近似 #555555）
        "  border-radius: 4px;"
        "}"
        )
        .arg(GlobalStyle::Colors::PanelBackground)    // %1
        .arg(GlobalStyle::Colors::TextPrimary)         // %2
        .arg(GlobalStyle::Colors::Border)              // %3
        .arg(GlobalStyle::Colors::RowSelected)         // %4
        .arg(GlobalStyle::Colors::ToolbarBackground)   // %5
        .arg(GlobalStyle::Colors::BorderLight));       // %6

    connect(m_targetTable, &QTableWidget::itemClicked, this, [this](QTableWidgetItem *item) {
        if (item && item->row() >= 0 && item->row() < m_targets.size()) {
            emit targetSelected(m_targets[item->row()]);
        }
    });
}

void LeftPanelWidget::populateTargetList()
{
    m_targetTable->setRowCount(m_targets.size());

    for (int i = 0; i < m_targets.size(); ++i) {
        appendTargetRow(i, m_targets[i]);
    }

    updateStatusTabs();
}

// 按目标状态计数（替代旧版按任务计数）：Detected/Confirmed=待检测，Disposing=处置中，Disposed=已完成
void LeftPanelWidget::updateStatusTabs()
{
    int pending = 0, executing = 0, completed = 0;
    for (const Core::TargetInfo &t : m_targets) {
        switch (t.status) {
        case Core::TargetStatus::Detected:
        case Core::TargetStatus::Confirmed:
            pending++;
            break;
        case Core::TargetStatus::Disposing:
            executing++;
            break;
        case Core::TargetStatus::Disposed:
            completed++;
            break;
        default:
            pending++;
        }
    }
    m_statusTabPending->setText(QString("待检测 %1").arg(pending));
    m_statusTabExecuting->setText(QString("处置中 %1").arg(executing));
    m_statusTabCompleted->setText(QString("已完成 %1").arg(completed));
}

void LeftPanelWidget::appendTargetRow(int row, const Core::TargetInfo &target)
{
    QColor threatColor;
    switch (target.threatLevel) {
        case Core::ThreatLevel::High: threatColor = QColor("#FF5252"); break;
        case Core::ThreatLevel::Medium: threatColor = QColor("#FFB74D"); break;
        case Core::ThreatLevel::Low: threatColor = QColor("#FFF176"); break;
        default: threatColor = QColor("#888888");
    }

    // 清除默认 ItemIsUserCheckable，保持其余默认标志不变
    auto clearCheckable = [](QTableWidgetItem *item) {
        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
    };

    QTableWidgetItem *typeItem = new QTableWidgetItem(target.typeName);
    typeItem->setForeground(threatColor);
    clearCheckable(typeItem);
    m_targetTable->setItem(row, kTargetTypeColumn, typeItem);

    QTableWidgetItem *confItem = new QTableWidgetItem(QString::number(target.confidence * 100, 'f', 0) + "%");
    clearCheckable(confItem);
    m_targetTable->setItem(row, kTargetConfidenceColumn, confItem);

    QString posStr = QString("X:%1 Y:%2").arg(int(target.position.x())).arg(int(target.position.z()));
    QTableWidgetItem *posItem = new QTableWidgetItem(posStr);
    clearCheckable(posItem);
    m_targetTable->setItem(row, kTargetPositionColumn, posItem);

    QTableWidgetItem *statusItem = new QTableWidgetItem(simulationTargetStatusText(target.status));
    clearCheckable(statusItem);
    m_targetTable->setItem(row, kTargetStatusColumn, statusItem);

    m_targetTable->setRowHeight(row, 40);
}

void LeftPanelWidget::setTargets(const QVector<Core::TargetInfo> &targets)
{
    m_targets = targets;
    populateTargetList();
}

void LeftPanelWidget::addTargetRow(const Core::TargetInfo &target)
{
    // 视频驱动目标注入：追加单行，避免全表重置闪烁
    m_targets.append(target);
    const int row = m_targetTable->rowCount();
    m_targetTable->insertRow(row);
    appendTargetRow(row, target);
    updateStatusTabs();
}

void LeftPanelWidget::selectTargetRow(const QString &targetId)
{
    if (targetId.isEmpty()) {
        m_targetTable->clearSelection();
        return;
    }
    for (int i = 0; i < m_targets.size(); ++i) {
        if (m_targets[i].id == targetId) {
            m_targetTable->selectRow(i);
            return;
        }
    }
}

void LeftPanelWidget::updateTargetStatus(const QString &targetId, Core::TargetStatus status)
{
    for (int row = 0; row < m_targets.size(); ++row) {
        Core::TargetInfo &target = m_targets[row];
        if (target.id != targetId) {
            continue;
        }

        target.status = status;
        QTableWidgetItem *statusItem = m_targetTable->item(row, kTargetStatusColumn);
        if (statusItem != nullptr) {
            statusItem->setText(simulationTargetStatusText(status));
        }
        updateStatusTabs();
        return;
    }
}

bool LeftPanelWidget::isCollapsed() const
{
    return m_collapsed;
}

void LeftPanelWidget::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;
    applyCollapseState();
    emit collapseChanged(m_collapsed);
}

// 应用折叠/展开视觉态：切换可见容器 + 固定宽度
void LeftPanelWidget::applyCollapseState()
{
    if (m_collapsed) {
        m_contentWidget->setVisible(false);
        m_collapsedLabel->setVisible(true);
        setFixedWidth(kCollapsedWidth);
    } else {
        m_contentWidget->setVisible(true);
        m_collapsedLabel->setVisible(false);
        setFixedWidth(GlobalStyle::Sizes::LeftPanelWidth);
    }
}

bool LeftPanelWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (m_collapsed && event->type() == QEvent::MouseButtonPress) {
        setCollapsed(false);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void LeftPanelWidget::onRefreshTargets()
{
    emit refreshSimulationRequested();
}

void LeftPanelWidget::onSearchTextChanged(const QString &text)
{
    for (int i = 0; i < m_targetTable->rowCount(); ++i) {
        bool match = false;
        for (int j = 0; j < m_targetTable->columnCount(); ++j) {
            QTableWidgetItem *item = m_targetTable->item(i, j);
            if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        m_targetTable->setRowHidden(i, !match && !text.isEmpty());
    }
}
