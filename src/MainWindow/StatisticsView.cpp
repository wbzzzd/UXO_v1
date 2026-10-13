// REQ-013 统计页视图实现（UI 层：数据平台只读消费端，导航 index 4）。
// 纪律：
//   - 本文件只经 StatisticsController 的值类型信号槽交互，不接触任何 Integration/Core
//     对象；全部渲染输入来自 StatisticsSnapshot / StatisticsListPage 值拷贝；
//   - 数据平台不可用（首次拉取失败）时展示显式错误态与重试入口，无任何模拟回退数据；
//   - 快照携带词表副本（9 表 61 项）驱动过滤下拉与词表列显示，UI 不访问 VocabularyCache；
//   - 明细分页请求在途时禁用过滤与翻页控件（worker 串行 FIFO，防重复触发）；
//   - 图表统一暗色主题（QChart 系 QGraphicsWidget，不走全局 QSS，需手动配色）。

#include "MainWindow/StatisticsView.h"
#include "MainWindow/StatisticsController.h"
#include "Common/GlobalStyle.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedLayout>
#include <QStringList>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTime>
#include <QVariant>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include <QtCharts/QAbstractAxis>
#include <QtCharts/QAbstractSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLegend>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QValueAxis>

#include <QtMath>

QT_CHARTS_USE_NAMESPACE

namespace {

// 明细页签固定序（与 buildDetailSection 的 addTab 顺序一致）
enum {
    kTabTargets = 0,
    kTabDevices,
    kTabMissions,
    kTabAlarms,
    kTabRecords,
    kTabCount
};

// 各页签对应的列表资源标识（契约 §7 资源名）
const QString kTabResources[kTabCount] = {
    QStringLiteral("targets"),
    QStringLiteral("devices"),
    QStringLiteral("missions"),
    QStringLiteral("alarms"),
    QStringLiteral("detection-records"),
};

// 明细分页大小（契约 §7：page_size 1–100，统计页明细约定 20）
const int kListPageSize = 20;

// 词表名（契约 §4 表 4-1 九表）
const QString kVocabTargetType = QStringLiteral("target_type");
const QString kVocabTargetStatus = QStringLiteral("target_status");
const QString kVocabThreatLevel = QStringLiteral("threat_level");
const QString kVocabDeviceType = QStringLiteral("device_type");
const QString kVocabDeviceStatus = QStringLiteral("device_status");
const QString kVocabMissionType = QStringLiteral("mission_type");
const QString kVocabMissionStatus = QStringLiteral("mission_status");
const QString kVocabAlarmLevel = QStringLiteral("alarm_level");
const QString kVocabDetectionReview = QStringLiteral("detection_review");

// 模型类 9 值域（契约 §5 表 5-1；按字典序展示）
const QString kModelClasses[] = {
    QStringLiteral("aircraft-bombs"),
    QStringLiteral("background"),
    QStringLiteral("fuzes"),
    QStringLiteral("grenades"),
    QStringLiteral("landmines"),
    QStringLiteral("mortars"),
    QStringLiteral("projectiles"),
    QStringLiteral("rockets"),
    QStringLiteral("submunitions"),
};
const int kModelClassCount = sizeof(kModelClasses) / sizeof(kModelClasses[0]);

// 图表配色板：主绿 → 档位蓝 → 威胁橙 → 威胁红 → 在线绿 → 威胁黄，
// 超出后按 HSL 旋转生成可区分色。
// 函数内静态：规避跨编译单元静态初始化顺序问题（GlobalStyle::Colors 为全局 QString）。
QColor chartColor(int index)
{
    static const QVector<QColor> palette = {
        QColor(GlobalStyle::Colors::PrimaryGreen),
        QColor(GlobalStyle::Colors::TierBlue),
        QColor(GlobalStyle::Colors::ThreatMedium),
        QColor(GlobalStyle::Colors::ThreatHigh),
        QColor(GlobalStyle::Colors::StatusOnline),
        QColor(GlobalStyle::Colors::ThreatLow),
    };
    if (index < 0)
        return palette.first();
    if (index < palette.size())
        return palette.at(index);
    return QColor::fromHsl((index * 47) % 360, 130, 150);
}

// 图表主题：背景/边框/标题/图例文字统一暗色系（QChart 不走全局 QSS）
void applyChartTheme(QChart *chart)
{
    chart->setBackgroundBrush(QBrush(QColor(GlobalStyle::Colors::PanelBackground)));
    chart->setBackgroundPen(QPen(QColor(GlobalStyle::Colors::Border)));
    chart->setTitleBrush(QBrush(QColor(GlobalStyle::Colors::TextPrimary)));
    chart->legend()->setLabelColor(QColor(GlobalStyle::Colors::TextSecondary));
}

// 坐标轴主题：刻度文字/轴线/网格线暗色系
void styleAxis(QAbstractAxis *axis)
{
    axis->setLabelsBrush(QBrush(QColor(GlobalStyle::Colors::TextSecondary)));
    axis->setLinePen(QPen(QColor(GlobalStyle::Colors::BorderLight)));
    axis->setGridLinePen(QPen(QColor(GlobalStyle::Colors::Border)));
}

// 图例 marker 透明度：序列隐藏时降为 0.5（Qt Charts legendmarkers 官方示例模式）
void updateMarkerOpacity(QLegendMarker *marker)
{
    const qreal alpha = marker->series()->isVisible() ? 1.0 : 0.5;
    QColor labelColor = marker->labelBrush().color();
    labelColor.setAlphaF(alpha);
    marker->setLabelBrush(QBrush(labelColor));
    QBrush brush = marker->brush();
    QColor brushColor = brush.color();
    brushColor.setAlphaF(alpha);
    brush.setColor(brushColor);
    marker->setBrush(brush);
}

// 时间戳显示：无效时间显示 "—"，有效转本地时间显示到秒
QString formatTimestamp(const QDateTime &utc)
{
    if (!utc.isValid())
        return QStringLiteral("—");
    return utc.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

// 空串兜底 "—"（可空字符串字段）
QString orDash(const QString &text)
{
    return text.isEmpty() ? QStringLiteral("—") : text;
}

// 平均处置时长（秒）人性化显示
QString formatAvgHandleSeconds(double seconds)
{
    if (seconds < 60.0)
        return QString::number(qRound(seconds)) + QStringLiteral(" 秒");
    if (seconds < 3600.0)
        return QString::number(seconds / 60.0, 'f', 1) + QStringLiteral(" 分钟");
    return QString::number(seconds / 3600.0, 'f', 1) + QStringLiteral(" 小时");
}

// 过滤下拉当前整数值（"全部" 与无效 data 均为 -1 = 未设置，与过滤结构体哨兵一致）
int comboIntValue(const QComboBox *combo)
{
    bool ok = false;
    const int value = combo->currentData().toInt(&ok);
    return ok ? value : -1;
}

// 过滤下拉当前字符串值（"全部" 为空串 = 未设置）
QString comboStringValue(const QComboBox *combo)
{
    return combo->currentData().toString();
}

// 过滤后总页数（1 起；无结果时仍为 1 页以保持分页条形态）
int totalPagesFor(qint64 total)
{
    return static_cast<int>(qMax<qint64>(1, (total + kListPageSize - 1) / kListPageSize));
}

// 按单元格文本整行填充（列数以表列数为准）
void setRowItems(QTableWidget *table, int row, const QStringList &cells)
{
    for (int col = 0; col < cells.size() && col < table->columnCount(); ++col)
        table->setItem(row, col, new QTableWidgetItem(cells.at(col)));
}

// 换装 QChartView 的 chart 并删除旧 chart（新 chart 所有权移交 view，旧 chart 由本方释放）
void swapChart(QChartView *view, QChart *chart)
{
    QChart *old = view->chart();
    view->setChart(chart);
    delete old;
}

} // namespace

// ============================================================================
// 构造与页面骨架
// ============================================================================

StatisticsView::StatisticsView(StatisticsController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    // 控制器由 MainWindow 持有并先于本视图创建（契约见头文件注释）
    Q_ASSERT_X(m_controller, "StatisticsView", "controller 不能为空（由 MainWindow 持有并先创建）");

    if (m_controller) {
        // 快照与明细分页结果（值类型，worker 线程 → UI 线程队列连接）
        connect(m_controller, &StatisticsController::snapshotReady,
                this, &StatisticsView::onSnapshotReady);
        connect(m_controller, &StatisticsController::snapshotFetchFailed,
                this, &StatisticsView::onSnapshotFetchFailed);
        connect(m_controller, &StatisticsController::listReady,
                this, &StatisticsView::onListReady);
        connect(m_controller, &StatisticsController::listFetchFailed,
                this, &StatisticsView::onListFetchFailed);
    }

    buildUi();
}

void StatisticsView::buildUi()
{
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 三态栈：0 = 加载中，1 = 错误，2 = 就绪（不加 parent，由 rootLayout 接管）
    m_stackLayout = new QStackedLayout();
    rootLayout->addLayout(m_stackLayout);

    // ---- 状态一：加载中 ----
    m_loadingPage = new QWidget(this);
    QVBoxLayout *loadingLayout = new QVBoxLayout(m_loadingPage);
    QLabel *loadingLabel = new QLabel(QStringLiteral("正在加载数据平台…"), m_loadingPage);
    loadingLabel->setProperty("labelRole", "body2");
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLayout->addWidget(loadingLabel);

    // ---- 状态二：数据平台不可用（显式错误 + 重试，REQ-013 A3）----
    m_errorPage = new QWidget(this);
    QVBoxLayout *errorLayout = new QVBoxLayout(m_errorPage);
    errorLayout->addStretch();
    m_errorLabel = new QLabel(m_errorPage);
    m_errorLabel->setProperty("labelRole", "body2");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet(
        QStringLiteral("color: %1;").arg(GlobalStyle::Colors::DangerRed));
    errorLayout->addWidget(m_errorLabel);
    m_retryBtn = new QPushButton(QStringLiteral("重试"), m_errorPage);
    m_retryBtn->setProperty("btnVariant", "compact");
    errorLayout->addWidget(m_retryBtn, 0, Qt::AlignHCenter);
    errorLayout->addStretch();
    connect(m_retryBtn, &QPushButton::clicked, this, &StatisticsView::triggerLoad);

    // ---- 状态三：就绪（滚动内容页）----
    m_readyScroll = new QScrollArea(this);
    m_readyScroll->setWidgetResizable(true);
    m_readyScroll->setFrameShape(QFrame::NoFrame);
    m_readyScroll->setProperty("containerBg", "main");
    // 视口不自动填充底色（内容 widget 已按 containerBg 着色，避免露出浅色 Base 底）
    m_readyScroll->viewport()->setAutoFillBackground(false);

    m_readyContent = new QWidget();
    m_readyContent->setProperty("containerBg", "main");
    m_readyScroll->setWidget(m_readyContent);
    QVBoxLayout *contentLayout = new QVBoxLayout(m_readyContent);
    contentLayout->setContentsMargins(24, 16, 24, 24);
    contentLayout->setSpacing(16);

    buildStatusRow(contentLayout);
    buildCardsRow(contentLayout);
    buildDistributionSection(contentLayout);
    buildTrendSection(contentLayout);
    buildAlarmSection(contentLayout);
    buildDetailSection(contentLayout);
    contentLayout->addStretch();

    m_stackLayout->addWidget(m_loadingPage);   // index 0：加载中
    m_stackLayout->addWidget(m_errorPage);     // index 1：错误
    m_stackLayout->addWidget(m_readyScroll);   // index 2：就绪
}

// 就绪页状态行：快照时间（本地时间）+ 行内刷新失败提示 + 手动刷新按钮
void StatisticsView::buildStatusRow(QVBoxLayout *contentLayout)
{
    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(8);

    m_snapshotTimeLabel = new QLabel(this);
    m_snapshotTimeLabel->setProperty("labelRole", "caption");
    row->addWidget(m_snapshotTimeLabel);

    m_snapshotErrorLabel = new QLabel(this);
    m_snapshotErrorLabel->setProperty("labelRole", "caption");
    m_snapshotErrorLabel->setStyleSheet(
        QStringLiteral("color: %1;").arg(GlobalStyle::Colors::DangerRed));
    m_snapshotErrorLabel->hide();
    row->addWidget(m_snapshotErrorLabel);

    row->addStretch();

    m_refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    m_refreshBtn->setProperty("btnVariant", "compact");
    row->addWidget(m_refreshBtn);
    connect(m_refreshBtn, &QPushButton::clicked, this, &StatisticsView::triggerLoad);

    contentLayout->addLayout(row);
}

// A 区：恰好 5 张概览卡片（目标/设备/任务/告警/检测记录），等宽一行
void StatisticsView::buildCardsRow(QVBoxLayout *contentLayout)
{
    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(12);

    for (int i = 0; i < 5; ++i) {
        QFrame *card = new QFrame(this);
        card->setProperty("containerBg", "panel");
        card->setAttribute(Qt::WA_StyledBackground, true); // QWidget 家族需显式启用 QSS 背景
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 12, 16, 12);
        cardLayout->setSpacing(4);

        CardWidgets widgets;
        widgets.title = new QLabel(card);
        widgets.title->setProperty("labelRole", "caption");
        cardLayout->addWidget(widgets.title);

        widgets.total = new QLabel(card);
        widgets.total->setProperty("labelRole", "h1");
        cardLayout->addWidget(widgets.total);

        widgets.secondary = new QLabel(card);
        widgets.secondary->setProperty("labelRole", "caption");
        widgets.secondary->hide(); // 无副指标时隐藏（快照到达后按内容显隐）
        cardLayout->addWidget(widgets.secondary);

        m_cards.append(widgets);
        row->addWidget(card, 1);
    }

    contentLayout->addLayout(row);
}

// B 区：10 张分布图（3 列网格，快照到达后重建）
void StatisticsView::buildDistributionSection(QVBoxLayout *contentLayout)
{
    QLabel *sectionTitle = new QLabel(QStringLiteral("分布统计"), this);
    sectionTitle->setProperty("labelRole", "h2");
    contentLayout->addWidget(sectionTitle);

    m_distGrid = new QGridLayout();
    m_distGrid->setContentsMargins(0, 0, 0, 0);
    m_distGrid->setSpacing(12);
    for (int col = 0; col < 3; ++col)
        m_distGrid->setColumnStretch(col, 1);
    contentLayout->addLayout(m_distGrid);
}

// C 区：近 30 天日趋势（多序列折线 + 图例切换）
void StatisticsView::buildTrendSection(QVBoxLayout *contentLayout)
{
    QLabel *sectionTitle = new QLabel(QStringLiteral("近 30 天趋势"), this);
    sectionTitle->setProperty("labelRole", "h2");
    contentLayout->addWidget(sectionTitle);

    m_trendChartView = new QChartView(this);
    m_trendChartView->setRenderHint(QPainter::Antialiasing);
    m_trendChartView->setMinimumHeight(280);
    contentLayout->addWidget(m_trendChartView);
}

// D 区：告警处理统计（级别柱状图 + 处置占比饼图 + 未处置数/平均处置时长）
void StatisticsView::buildAlarmSection(QVBoxLayout *contentLayout)
{
    QLabel *sectionTitle = new QLabel(QStringLiteral("告警处理统计"), this);
    sectionTitle->setProperty("labelRole", "h2");
    contentLayout->addWidget(sectionTitle);

    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(12);

    m_alarmLevelChartView = new QChartView(this);
    m_alarmLevelChartView->setRenderHint(QPainter::Antialiasing);
    m_alarmLevelChartView->setMinimumHeight(240);
    row->addWidget(m_alarmLevelChartView, 1);

    m_alarmHandledChartView = new QChartView(this);
    m_alarmHandledChartView->setRenderHint(QPainter::Antialiasing);
    m_alarmHandledChartView->setMinimumHeight(240);
    row->addWidget(m_alarmHandledChartView, 1);

    // 统计文字列（清单 D 区：未处置数 + 平均处置时长）
    QVBoxLayout *statsLayout = new QVBoxLayout();
    statsLayout->setSpacing(8);
    QLabel *statsTitle = new QLabel(QStringLiteral("处置概况"), this);
    statsTitle->setProperty("labelRole", "body2");
    statsLayout->addWidget(statsTitle);
    m_alarmUnhandledLabel = new QLabel(this);
    m_alarmUnhandledLabel->setProperty("labelRole", "body2");
    statsLayout->addWidget(m_alarmUnhandledLabel);
    m_alarmAvgHandleLabel = new QLabel(this);
    m_alarmAvgHandleLabel->setProperty("labelRole", "body2");
    statsLayout->addWidget(m_alarmAvgHandleLabel);
    statsLayout->addStretch();
    row->addLayout(statsLayout);

    contentLayout->addLayout(row);
}

// E 区：明细数据（5 页签分页表：目标/设备/任务/告警/检测记录）
void StatisticsView::buildDetailSection(QVBoxLayout *contentLayout)
{
    QLabel *sectionTitle = new QLabel(QStringLiteral("明细数据"), this);
    sectionTitle->setProperty("labelRole", "h2");
    contentLayout->addWidget(sectionTitle);

    m_detailTabs = new QTabWidget(this);
    // 快照就绪前禁用（过滤下拉与词表列依赖快照词表副本）
    m_detailTabs->setEnabled(false);
    const QString tabTitles[kTabCount] = {
        QStringLiteral("目标"),
        QStringLiteral("设备"),
        QStringLiteral("任务"),
        QStringLiteral("告警"),
        QStringLiteral("检测记录"),
    };
    for (int i = 0; i < kTabCount; ++i)
        m_detailTabs->addTab(buildDetailTab(i, m_detailTabs), tabTitles[i]);

    // 页签首次可见时懒加载第一页（快照就绪后生效）
    connect(m_detailTabs, &QTabWidget::currentChanged, this, [this](int index) {
        maybeRequestInitialList(index);
    });

    contentLayout->addWidget(m_detailTabs);
}

// 单个明细页签：过滤行（程序填充下拉 + ID 输入框）→ 表格 → 行内错误 → 分页条
QWidget *StatisticsView::buildDetailTab(int tabIndex, QWidget *parent)
{
    QWidget *page = new QWidget(parent);
    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 8, 0, 0);
    pageLayout->setSpacing(8);

    DetailTab &tab = m_tabs[tabIndex];

    // ---- 过滤行（维度按实体契约 §7 起步过滤集）----
    tab.filterRow = new QWidget(page);
    QHBoxLayout *filterLayout = new QHBoxLayout(tab.filterRow);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(8);

    switch (tabIndex) {
    case kTabTargets:
        tab.filterCombo1 = addComboFilter(filterLayout, QStringLiteral("类型"));
        tab.filterCombo2 = addComboFilter(filterLayout, QStringLiteral("状态"));
        tab.filterCombo3 = addComboFilter(filterLayout, QStringLiteral("威胁等级"));
        break;
    case kTabDevices:
        tab.filterCombo1 = addComboFilter(filterLayout, QStringLiteral("类型"));
        tab.filterCombo2 = addComboFilter(filterLayout, QStringLiteral("状态"));
        break;
    case kTabMissions:
        tab.filterCombo1 = addComboFilter(filterLayout, QStringLiteral("类型"));
        tab.filterCombo2 = addComboFilter(filterLayout, QStringLiteral("状态"));
        tab.targetIdEdit = addIdFilter(filterLayout, QStringLiteral("目标 ID"));
        tab.deviceIdEdit = addIdFilter(filterLayout, QStringLiteral("设备 ID"));
        break;
    case kTabAlarms:
        tab.filterCombo1 = addComboFilter(filterLayout, QStringLiteral("级别"));
        tab.filterCombo2 = addComboFilter(filterLayout, QStringLiteral("是否处置"));
        tab.targetIdEdit = addIdFilter(filterLayout, QStringLiteral("目标 ID"));
        tab.deviceIdEdit = addIdFilter(filterLayout, QStringLiteral("设备 ID"));
        break;
    case kTabRecords:
        tab.filterCombo1 = addComboFilter(filterLayout, QStringLiteral("复核状态"));
        tab.filterCombo2 = addComboFilter(filterLayout, QStringLiteral("模型类"));
        tab.filterCombo3 = addComboFilter(filterLayout, QStringLiteral("是否异常"));
        tab.targetIdEdit = addIdFilter(filterLayout, QStringLiteral("目标 ID"));
        break;
    default:
        break;
    }
    filterLayout->addStretch();
    pageLayout->addWidget(tab.filterRow);

    // ---- 过滤变更接线：回到第 1 页重新查询
    // （下拉在程序填充时 blockSignals，仅用户选择触发；textEdited 仅用户输入触发）----
    const auto connectCombo = [this, tabIndex](QComboBox *combo) {
        if (combo) {
            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, tabIndex](int) { resetAndRequestList(tabIndex); });
        }
    };
    connectCombo(tab.filterCombo1);
    connectCombo(tab.filterCombo2);
    connectCombo(tab.filterCombo3);
    if (tab.targetIdEdit) {
        connect(tab.targetIdEdit, &QLineEdit::textEdited, this,
                [this, tabIndex](const QString &) { resetAndRequestList(tabIndex); });
    }
    if (tab.deviceIdEdit) {
        connect(tab.deviceIdEdit, &QLineEdit::textEdited, this,
                [this, tabIndex](const QString &) { resetAndRequestList(tabIndex); });
    }

    // ---- 表格（只读行选择；列宽按内容自适应，末列拉伸）----
    tab.table = new QTableWidget(page);
    tab.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab.table->setSelectionMode(QAbstractItemView::SingleSelection);
    tab.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tab.table->verticalHeader()->hide();
    tab.table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tab.table->horizontalHeader()->setStretchLastSection(true);
    tab.table->setMinimumHeight(320);
    switch (tabIndex) {
    case kTabTargets:
        tab.table->setColumnCount(7);
        tab.table->setHorizontalHeaderLabels({
            QStringLiteral("ID"), QStringLiteral("类型"), QStringLiteral("状态"),
            QStringLiteral("威胁等级"), QStringLiteral("置信度"),
            QStringLiteral("探测时间"), QStringLiteral("更新时间") });
        break;
    case kTabDevices:
        tab.table->setColumnCount(6);
        tab.table->setHorizontalHeaderLabels({
            QStringLiteral("ID"), QStringLiteral("名称"), QStringLiteral("类型"),
            QStringLiteral("状态"), QStringLiteral("电量"), QStringLiteral("最近在线") });
        break;
    case kTabMissions:
        tab.table->setColumnCount(7);
        tab.table->setHorizontalHeaderLabels({
            QStringLiteral("ID"), QStringLiteral("类型"), QStringLiteral("状态"),
            QStringLiteral("目标"), QStringLiteral("设备"), QStringLiteral("优先级"),
            QStringLiteral("创建时间") });
        break;
    case kTabAlarms:
        tab.table->setColumnCount(7);
        tab.table->setHorizontalHeaderLabels({
            QStringLiteral("ID"), QStringLiteral("类型"), QStringLiteral("级别"),
            QStringLiteral("目标"), QStringLiteral("设备"), QStringLiteral("是否处置"),
            QStringLiteral("创建时间") });
        break;
    case kTabRecords:
        tab.table->setColumnCount(7);
        tab.table->setHorizontalHeaderLabels({
            QStringLiteral("ID"), QStringLiteral("模型类"), QStringLiteral("是否异常"),
            QStringLiteral("复核状态"), QStringLiteral("置信度"), QStringLiteral("目标"),
            QStringLiteral("分析时间") });
        break;
    default:
        break;
    }
    pageLayout->addWidget(tab.table, 1);

    // ---- 行内错误（默认隐藏，无弹窗）----
    tab.errorLabel = new QLabel(page);
    tab.errorLabel->setProperty("labelRole", "caption");
    tab.errorLabel->setStyleSheet(
        QStringLiteral("color: %1;").arg(GlobalStyle::Colors::DangerRed));
    tab.errorLabel->setWordWrap(true);
    tab.errorLabel->hide();
    pageLayout->addWidget(tab.errorLabel);

    // ---- 分页条 ----
    QHBoxLayout *pagerLayout = new QHBoxLayout();
    pagerLayout->setSpacing(8);
    tab.prevPageBtn = new QPushButton(QStringLiteral("上一页"), page);
    tab.prevPageBtn->setProperty("btnVariant", "compact");
    tab.prevPageBtn->setEnabled(false);
    connect(tab.prevPageBtn, &QPushButton::clicked, this, [this, tabIndex]() {
        DetailTab &t = m_tabs[tabIndex];
        if (t.currentPage > 1) {
            t.currentPage -= 1;
            requestTabList(tabIndex);
        }
    });
    tab.pageLabel = new QLabel(page);
    tab.pageLabel->setProperty("labelRole", "caption");
    tab.nextPageBtn = new QPushButton(QStringLiteral("下一页"), page);
    tab.nextPageBtn->setProperty("btnVariant", "compact");
    tab.nextPageBtn->setEnabled(false);
    connect(tab.nextPageBtn, &QPushButton::clicked, this, [this, tabIndex]() {
        DetailTab &t = m_tabs[tabIndex];
        if (t.currentPage < totalPagesFor(t.total)) {
            t.currentPage += 1;
            requestTabList(tabIndex);
        }
    });
    pagerLayout->addWidget(tab.prevPageBtn);
    pagerLayout->addWidget(tab.pageLabel);
    pagerLayout->addWidget(tab.nextPageBtn);
    pagerLayout->addStretch();
    pageLayout->addLayout(pagerLayout);

    return page;
}

// 过滤行组合框：标签 + 下拉（选项由快照词表副本在 populateFilterCombos 填充）
QComboBox *StatisticsView::addComboFilter(QHBoxLayout *layout, const QString &label)
{
    QLabel *labelWidget = new QLabel(label, this);
    labelWidget->setProperty("labelRole", "caption");
    layout->addWidget(labelWidget);
    QComboBox *combo = new QComboBox(this);
    layout->addWidget(combo);
    return combo;
}

// 过滤行 ID 输入框：占位提示 + 一键清空
QLineEdit *StatisticsView::addIdFilter(QHBoxLayout *layout, const QString &placeholder)
{
    QLineEdit *edit = new QLineEdit(this);
    edit->setPlaceholderText(placeholder);
    edit->setClearButtonEnabled(true);
    edit->setMaximumWidth(220);
    layout->addWidget(edit);
    return edit;
}

// ============================================================================
// 进页加载 / 手动刷新与三态切换
// ============================================================================

// 进页加载与手动刷新同一入口：刷新期间旧内容整体置灰不清空（REQ-013 A3 交互）
void StatisticsView::triggerLoad()
{
    m_readyContent->setEnabled(false);
    m_snapshotErrorLabel->hide();
    if (!m_snapshotLoaded)
        m_stackLayout->setCurrentWidget(m_loadingPage);
    m_controller->requestSnapshot();
}

void StatisticsView::onSnapshotReady(const StatisticsSnapshot &snapshot)
{
    m_snapshot = snapshot;
    m_snapshotLoaded = true;

    m_snapshotTimeLabel->setText(
        QStringLiteral("数据快照：%1（本地时间，手动刷新）")
            .arg(snapshot.fetchedAtUtc.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
    m_snapshotErrorLabel->hide();

    updateCards(snapshot);
    updateDistributions(snapshot);
    updateTrendChart(snapshot);
    updateAlarmSection(snapshot);
    populateFilterCombos();

    m_detailTabs->setEnabled(true);
    m_readyContent->setEnabled(true);
    m_stackLayout->setCurrentWidget(m_readyScroll);

    // 首个可见页签懒加载第一页（其余页签在 currentChanged 时加载）
    maybeRequestInitialList(m_detailTabs->currentIndex());
}

void StatisticsView::onSnapshotFetchFailed(const QString &message)
{
    m_readyContent->setEnabled(true);
    if (!m_snapshotLoaded) {
        // 从未成功过：整页错误态 + 重试（不显示任何编造数据）
        m_errorLabel->setText(QStringLiteral("数据平台不可用：%1").arg(message));
        m_stackLayout->setCurrentWidget(m_errorPage);
        return;
    }
    // 已有旧数据：行内提示刷新失败，保留旧内容
    m_snapshotErrorLabel->setText(QStringLiteral("刷新失败：%1").arg(message));
    m_snapshotErrorLabel->show();
}

// ============================================================================
// 快照驱动更新（A/B/C/D 区）
// ============================================================================

void StatisticsView::updateCards(const StatisticsSnapshot &snapshot)
{
    const int count = qMin(m_cards.size(), snapshot.cards.size());
    for (int i = 0; i < count; ++i) {
        const StatisticsCard &card = snapshot.cards.at(i);
        m_cards[i].title->setText(card.title);
        m_cards[i].total->setText(QString::number(card.total));
        m_cards[i].secondary->setText(card.secondaryText);
        m_cards[i].secondary->setVisible(!card.secondaryText.isEmpty());
    }
}

void StatisticsView::updateDistributions(const StatisticsSnapshot &snapshot)
{
    // 清空旧图（旧 QChartView 连同其 QChart 一并销毁，防止刷新累积）
    while (QLayoutItem *item = m_distGrid->takeAt(0)) {
        if (QWidget *widget = item->widget())
            delete widget;
        delete item;
    }

    for (int i = 0; i < snapshot.distributions.size(); ++i) {
        const StatisticsDistribution &dist = snapshot.distributions.at(i);
        QWidget *chartWidget = nullptr;
        if (dist.buckets.isEmpty()) {
            chartWidget = buildEmptyChartPlaceholder(dist.title);
        } else if (dist.useBar) {
            QChartView *view = new QChartView(buildBarChart(dist), this);
            view->setRenderHint(QPainter::Antialiasing);
            chartWidget = view;
        } else {
            QChartView *view = new QChartView(buildPieChart(dist), this);
            view->setRenderHint(QPainter::Antialiasing);
            chartWidget = view;
        }
        chartWidget->setMinimumHeight(240);
        m_distGrid->addWidget(chartWidget, i / 3, i % 3);
    }
}

void StatisticsView::updateTrendChart(const StatisticsSnapshot &snapshot)
{
    QChart *chart = new QChart();
    chart->setTitle(QStringLiteral("日增量趋势（UTC 日分桶）"));
    applyChartTheme(chart);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat(QStringLiteral("MM-dd"));
    axisX->setTickCount(6); // 30 天区间约每 5~6 天一个刻度
    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat(QStringLiteral("%d"));

    // 序列按快照固定序着色；defaultVisible=false 的序列（设备/任务登记）默认隐藏
    QVector<QLineSeries *> seriesList;
    QDate minDate;
    QDate maxDate;
    qint64 maxValue = 0;
    for (int i = 0; i < snapshot.trends.size(); ++i) {
        const StatisticsTrendSeries &trend = snapshot.trends.at(i);
        QLineSeries *series = new QLineSeries();
        series->setName(trend.name);
        series->setColor(chartColor(i));
        series->setVisible(trend.defaultVisible);
        for (const QPair<QDate, qint64> &point : trend.points) {
            // UTC 日分桶：统一以 UTC 零点换算毫秒，序列与坐标轴同基准
            series->append(QDateTime(point.first, QTime(0, 0), Qt::UTC).toMSecsSinceEpoch(),
                           qreal(point.second));
            if (!minDate.isValid() || point.first < minDate)
                minDate = point.first;
            if (!maxDate.isValid() || point.first > maxDate)
                maxDate = point.first;
            maxValue = qMax(maxValue, point.second);
        }
        seriesList.append(series);
    }

    for (QLineSeries *series : seriesList)
        chart->addSeries(series);
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    for (QLineSeries *series : seriesList) {
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }
    styleAxis(axisX);
    styleAxis(axisY);

    // 显式轴范围：图例切换序列不改变坐标尺度（空日补 0、恰好 30 点由聚合层保证）
    if (minDate.isValid() && maxDate.isValid())
        axisX->setRange(QDateTime(minDate, QTime(0, 0), Qt::UTC),
                        QDateTime(maxDate, QTime(0, 0), Qt::UTC));
    axisY->setRange(0.0, qreal(qCeil(qMax<qreal>(4.0, qreal(maxValue)) * 1.05)));

    // 图例点击切换序列可见性（Qt Charts legendmarkers 官方示例模式：
    // marker 自身保持可见，仅降低不透明度表示序列隐藏，以便再次点击恢复）
    const QList<QLegendMarker *> markers = chart->legend()->markers();
    for (QLegendMarker *marker : markers) {
        connect(marker, &QLegendMarker::clicked, this, [marker]() {
            marker->series()->setVisible(!marker->series()->isVisible());
            marker->setVisible(true);
            updateMarkerOpacity(marker);
        });
        updateMarkerOpacity(marker); // 初始隐藏序列的 marker 同步降不透明度
    }

    swapChart(m_trendChartView, chart);
}

void StatisticsView::updateAlarmSection(const StatisticsSnapshot &snapshot)
{
    // D 区级别分布柱状图与 B 区告警级别图同源（固定序第 8 张分布图，index 7）
    if (snapshot.distributions.size() > 7) {
        const StatisticsDistribution &levelDist = snapshot.distributions.at(7);
        QChart *levelChart = nullptr;
        if (levelDist.buckets.isEmpty()) {
            // 无告警数据：仅保留图题的空图（与 B 区空图占位一致的语义）
            levelChart = new QChart();
            levelChart->setTitle(levelDist.title);
            applyChartTheme(levelChart);
        } else {
            levelChart = buildBarChart(levelDist);
        }
        swapChart(m_alarmLevelChartView, levelChart);
    }

    // 处置占比饼图：已处置（在线绿）/ 未处置（警示红）；无告警时仅保留图题
    QChart *handledChart = new QChart();
    handledChart->setTitle(QStringLiteral("处置占比"));
    applyChartTheme(handledChart);
    handledChart->legend()->hide();
    const StatisticsAlarmSummary &summary = snapshot.alarmSummary;
    if (summary.handled + summary.unhandled > 0) {
        QPieSeries *series = new QPieSeries();
        QPieSlice *handledSlice =
            series->append(QStringLiteral("已处置"), qreal(summary.handled));
        handledSlice->setColor(QColor(GlobalStyle::Colors::StatusOnline));
        handledSlice->setLabelVisible(true);
        handledSlice->setLabelColor(QColor(GlobalStyle::Colors::TextPrimary));
        handledSlice->setLabel(QStringLiteral("已处置 %1").arg(summary.handled));
        QPieSlice *unhandledSlice =
            series->append(QStringLiteral("未处置"), qreal(summary.unhandled));
        unhandledSlice->setColor(QColor(GlobalStyle::Colors::DangerRed));
        unhandledSlice->setLabelVisible(true);
        unhandledSlice->setLabelColor(QColor(GlobalStyle::Colors::TextPrimary));
        unhandledSlice->setLabel(QStringLiteral("未处置 %1").arg(summary.unhandled));
        handledChart->addSeries(series);
    }
    swapChart(m_alarmHandledChartView, handledChart);

    // 文字指标：未处置数 + 平均处置时长（无可计入样本时显示 "—"，口径见快照结构体）
    m_alarmUnhandledLabel->setText(QStringLiteral("未处置：%1").arg(summary.unhandled));
    m_alarmAvgHandleLabel->setText(
        summary.hasAvgHandle
            ? QStringLiteral("平均处置时长：%1").arg(formatAvgHandleSeconds(summary.avgHandleSeconds))
            : QStringLiteral("平均处置时长：—"));
}

// ============================================================================
// 图表构建（B 区通用）
// ============================================================================

// 柱状图：单序列单色（主绿），类目轴 + 数值轴（0 起，整数刻度标签），隐藏图例
QChart *StatisticsView::buildBarChart(const StatisticsDistribution &dist)
{
    QChart *chart = new QChart();
    chart->setTitle(dist.title);
    applyChartTheme(chart);
    chart->legend()->hide();

    QBarSeries *series = new QBarSeries();
    QBarSet *barSet = new QBarSet(dist.title);
    barSet->setColor(QColor(GlobalStyle::Colors::PrimaryGreen));
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    qint64 maxValue = 0;
    for (const QPair<QString, qint64> &bucket : dist.buckets) {
        *barSet << qreal(bucket.second);
        axisX->append(bucket.first);
        maxValue = qMax(maxValue, bucket.second);
    }
    series->append(barSet);

    chart->addSeries(series);
    chart->addAxis(axisX, Qt::AlignBottom);
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0.0, maxValue > 0 ? qreal(maxValue) : 1.0);
    axisY->setLabelFormat(QStringLiteral("%d"));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    styleAxis(axisX);
    styleAxis(axisY);
    return chart;
}

// 饼图：≤5 桶的分布（批准规则），分片带 "名称 计数" 标签，隐藏图例
QChart *StatisticsView::buildPieChart(const StatisticsDistribution &dist)
{
    QChart *chart = new QChart();
    chart->setTitle(dist.title);
    applyChartTheme(chart);
    chart->legend()->hide();

    QPieSeries *series = new QPieSeries();
    for (int i = 0; i < dist.buckets.size(); ++i) {
        const QPair<QString, qint64> &bucket = dist.buckets.at(i);
        QPieSlice *slice = series->append(bucket.first, qreal(bucket.second));
        slice->setColor(chartColor(i));
        slice->setLabelColor(QColor(GlobalStyle::Colors::TextPrimary));
        // 计数为 0 的桶不画标签（避免多个零值标签在起角堆叠）
        slice->setLabelVisible(bucket.second > 0);
        slice->setLabel(QStringLiteral("%1 %2").arg(bucket.first).arg(bucket.second));
    }
    chart->addSeries(series);
    return chart;
}

// 空数据占位（面板底 + 图题 + "暂无数据"，与分布图网格同最小高度）
QWidget *StatisticsView::buildEmptyChartPlaceholder(const QString &title)
{
    QFrame *placeholder = new QFrame(this);
    placeholder->setProperty("containerBg", "panel");
    placeholder->setAttribute(Qt::WA_StyledBackground, true);
    placeholder->setMinimumHeight(240);
    QVBoxLayout *layout = new QVBoxLayout(placeholder);
    QLabel *titleLabel = new QLabel(title, placeholder);
    titleLabel->setProperty("labelRole", "body2");
    layout->addWidget(titleLabel);
    layout->addStretch();
    QLabel *emptyLabel = new QLabel(QStringLiteral("暂无数据"), placeholder);
    emptyLabel->setProperty("labelRole", "caption");
    emptyLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(emptyLabel);
    layout->addStretch();
    return placeholder;
}

// ============================================================================
// 明细查询与回填（E 区）
// ============================================================================

// 页签首次可见时懒加载第一页（快照就绪前不动作；已有数据/在途请求不重复拉取）
void StatisticsView::maybeRequestInitialList(int tabIndex)
{
    if (!m_snapshotLoaded || tabIndex < 0 || tabIndex >= kTabCount)
        return;
    DetailTab &tab = m_tabs[tabIndex];
    if (!tab.pending && tab.table->rowCount() == 0) {
        tab.currentPage = 1;
        requestTabList(tabIndex);
    }
}

// 过滤变更：回到第 1 页重新查询
void StatisticsView::resetAndRequestList(int tabIndex)
{
    if (!m_snapshotLoaded || tabIndex < 0 || tabIndex >= kTabCount)
        return;
    m_tabs[tabIndex].currentPage = 1;
    requestTabList(tabIndex);
}

// 按页签当前过滤条件与页码发起分页查询（在途时忽略重复触发）
void StatisticsView::requestTabList(int tabIndex)
{
    if (!m_snapshotLoaded || tabIndex < 0 || tabIndex >= kTabCount)
        return;
    DetailTab &tab = m_tabs[tabIndex];
    if (tab.pending)
        return;

    StatisticsController::ListRequest request;
    request.resource = kTabResources[tabIndex];
    switch (tabIndex) {
    case kTabTargets:
        request.targetFilter.type = comboIntValue(tab.filterCombo1);
        request.targetFilter.status = comboIntValue(tab.filterCombo2);
        request.targetFilter.threatLevel = comboIntValue(tab.filterCombo3);
        break;
    case kTabDevices:
        request.deviceFilter.type = comboIntValue(tab.filterCombo1);
        request.deviceFilter.status = comboIntValue(tab.filterCombo2);
        break;
    case kTabMissions:
        request.missionFilter.type = comboIntValue(tab.filterCombo1);
        request.missionFilter.status = comboIntValue(tab.filterCombo2);
        request.missionFilter.targetId = tab.targetIdEdit->text().trimmed();
        request.missionFilter.deviceId = tab.deviceIdEdit->text().trimmed();
        break;
    case kTabAlarms:
        request.alarmFilter.level = comboIntValue(tab.filterCombo1);
        request.alarmFilter.isHandled = comboIntValue(tab.filterCombo2);
        request.alarmFilter.targetId = tab.targetIdEdit->text().trimmed();
        request.alarmFilter.deviceId = tab.deviceIdEdit->text().trimmed();
        break;
    case kTabRecords:
        request.detectionRecordFilter.review = comboIntValue(tab.filterCombo1);
        request.detectionRecordFilter.className = comboStringValue(tab.filterCombo2);
        request.detectionRecordFilter.hasAnomaly = comboIntValue(tab.filterCombo3);
        request.detectionRecordFilter.targetId = tab.targetIdEdit->text().trimmed();
        break;
    default:
        return;
    }
    request.page.page = tab.currentPage;
    request.page.pageSize = kListPageSize;

    tab.pending = true;
    tab.errorLabel->hide();
    updateTabPager(tabIndex);
    m_controller->requestList(request);
}

// 分页结果回填：按 resource 匹配在途页签（陈旧或非本页结果忽略）
void StatisticsView::onListReady(const StatisticsListPage &page)
{
    for (int i = 0; i < kTabCount; ++i) {
        DetailTab &tab = m_tabs[i];
        if (!tab.pending || kTabResources[i] != page.resource)
            continue;
        tab.pending = false;
        tab.total = page.total;
        tab.currentPage = qMax(1, page.page);
        fillDetailTable(i, page);
        updateTabPager(i);
        return;
    }
}

// 分页查询失败：行内提示（无弹窗），恢复控件可用
void StatisticsView::onListFetchFailed(const QString &resource, const QString &message)
{
    for (int i = 0; i < kTabCount; ++i) {
        DetailTab &tab = m_tabs[i];
        if (!tab.pending || kTabResources[i] != resource)
            continue;
        tab.pending = false;
        tab.errorLabel->setText(QStringLiteral("查询失败：%1").arg(message));
        tab.errorLabel->show();
        updateTabPager(i);
        return;
    }
}

// 表格回填：词表列经 vocabNameCn（快照词表副本），时间为本地时间，可空字段兜底 "—"
void StatisticsView::fillDetailTable(int tabIndex, const StatisticsListPage &page)
{
    QTableWidget *table = m_tabs[tabIndex].table;
    switch (tabIndex) {
    case kTabTargets: {
        table->setRowCount(page.targets.size());
        for (int row = 0; row < page.targets.size(); ++row) {
            const Core::TargetInfo &t = page.targets.at(row);
            setRowItems(table, row, {
                t.id,
                vocabNameCn(kVocabTargetType, static_cast<int>(t.type)),
                vocabNameCn(kVocabTargetStatus, static_cast<int>(t.status)),
                vocabNameCn(kVocabThreatLevel, static_cast<int>(t.threatLevel)),
                QString::number(t.confidence, 'f', 2),
                formatTimestamp(t.detectTime),
                formatTimestamp(t.updateTime) });
        }
        break;
    }
    case kTabDevices: {
        table->setRowCount(page.devices.size());
        for (int row = 0; row < page.devices.size(); ++row) {
            const Core::DeviceInfo &d = page.devices.at(row);
            setRowItems(table, row, {
                d.id,
                orDash(d.name),
                vocabNameCn(kVocabDeviceType, static_cast<int>(d.type)),
                vocabNameCn(kVocabDeviceStatus, static_cast<int>(d.status)),
                QStringLiteral("%1%").arg(QString::number(d.batteryLevel, 'f', 0)),
                formatTimestamp(d.lastOnlineTime) });
        }
        break;
    }
    case kTabMissions: {
        table->setRowCount(page.missions.size());
        for (int row = 0; row < page.missions.size(); ++row) {
            const Core::MissionInfo &m = page.missions.at(row);
            setRowItems(table, row, {
                m.id,
                vocabNameCn(kVocabMissionType, static_cast<int>(m.type)),
                vocabNameCn(kVocabMissionStatus, static_cast<int>(m.status)),
                orDash(m.targetId),
                orDash(m.deviceId),
                QString::number(m.priority),
                formatTimestamp(m.createTime) });
        }
        break;
    }
    case kTabAlarms: {
        table->setRowCount(page.alarms.size());
        for (int row = 0; row < page.alarms.size(); ++row) {
            const Core::AlarmInfo &a = page.alarms.at(row);
            setRowItems(table, row, {
                a.id,
                orDash(a.type),
                vocabNameCn(kVocabAlarmLevel, static_cast<int>(a.level)),
                orDash(a.targetId),
                orDash(a.deviceId),
                a.isHandled ? QStringLiteral("已处置") : QStringLiteral("未处置"),
                formatTimestamp(a.createTime) });
        }
        break;
    }
    case kTabRecords: {
        table->setRowCount(page.detectionRecords.size());
        for (int row = 0; row < page.detectionRecords.size(); ++row) {
            const Integration::DetectionRecordData &r = page.detectionRecords.at(row);
            setRowItems(table, row, {
                r.id,
                orDash(r.className),
                r.hasAnomaly ? QStringLiteral("是") : QStringLiteral("否"),
                vocabNameCn(kVocabDetectionReview, r.review),
                QString::number(r.confidence, 'f', 2),
                orDash(r.targetId),
                formatTimestamp(r.analyzedAt) });
        }
        break;
    }
    default:
        break;
    }
}

// 分页条刷新：页码文案 + 上下页/过滤行可用性（在途请求期间禁用）
void StatisticsView::updateTabPager(int tabIndex)
{
    const DetailTab &tab = m_tabs[tabIndex];
    const int totalPages = totalPagesFor(tab.total);
    tab.pageLabel->setText(QStringLiteral("第 %1 / %2 页 · 共 %3 条")
                               .arg(tab.currentPage)
                               .arg(totalPages)
                               .arg(tab.total));
    tab.prevPageBtn->setEnabled(!tab.pending && tab.currentPage > 1);
    tab.nextPageBtn->setEnabled(!tab.pending && tab.currentPage < totalPages);
    tab.filterRow->setEnabled(!tab.pending);
}

// ============================================================================
// 过滤下拉填充（快照词表副本驱动）
// ============================================================================

// 快照到达后填充全部过滤下拉（已填充则跳过：词表为追加式正典，刷新保留用户选择）
void StatisticsView::populateFilterCombos()
{
    fillVocabCombo(m_tabs[kTabTargets].filterCombo1, kVocabTargetType);
    fillVocabCombo(m_tabs[kTabTargets].filterCombo2, kVocabTargetStatus);
    fillVocabCombo(m_tabs[kTabTargets].filterCombo3, kVocabThreatLevel);
    fillVocabCombo(m_tabs[kTabDevices].filterCombo1, kVocabDeviceType);
    fillVocabCombo(m_tabs[kTabDevices].filterCombo2, kVocabDeviceStatus);
    fillVocabCombo(m_tabs[kTabMissions].filterCombo1, kVocabMissionType);
    fillVocabCombo(m_tabs[kTabMissions].filterCombo2, kVocabMissionStatus);
    fillVocabCombo(m_tabs[kTabAlarms].filterCombo1, kVocabAlarmLevel);
    fillTriStateCombo(m_tabs[kTabAlarms].filterCombo2);
    fillVocabCombo(m_tabs[kTabRecords].filterCombo1, kVocabDetectionReview);
    fillModelClassCombo(m_tabs[kTabRecords].filterCombo2);
    fillTriStateCombo(m_tabs[kTabRecords].filterCombo3);
}

// 词表下拉："全部"(-1) + 词表项（name_cn，data=code）
void StatisticsView::fillVocabCombo(QComboBox *combo, const QString &vocabName)
{
    if (!combo || combo->count() > 0)
        return;

    const bool blocked = combo->blockSignals(true); // 程序填充不触发过滤查询
    combo->addItem(QStringLiteral("全部"), -1);
    for (const Integration::Vocabulary &vocab : m_snapshot.vocabularies) {
        if (vocab.name != vocabName)
            continue;
        for (const Integration::VocabularyItem &item : vocab.items)
            combo->addItem(item.nameCn, item.code);
        break;
    }
    combo->setCurrentIndex(0);
    combo->blockSignals(blocked);
}

// 三态布尔下拉："全部"(-1) / "是"(1) / "否"(0)，与契约布尔过滤编码一致
void StatisticsView::fillTriStateCombo(QComboBox *combo)
{
    if (!combo || combo->count() > 0)
        return;

    const bool blocked = combo->blockSignals(true);
    combo->addItem(QStringLiteral("全部"), -1);
    combo->addItem(QStringLiteral("是"), 1);
    combo->addItem(QStringLiteral("否"), 0);
    combo->setCurrentIndex(0);
    combo->blockSignals(blocked);
}

// 模型类下拉："全部"(空串) + 9 值域（字典序）
void StatisticsView::fillModelClassCombo(QComboBox *combo)
{
    if (!combo || combo->count() > 0)
        return;

    const bool blocked = combo->blockSignals(true);
    combo->addItem(QStringLiteral("全部"), QString());
    for (int i = 0; i < kModelClassCount; ++i)
        combo->addItem(kModelClasses[i], kModelClasses[i]);
    combo->setCurrentIndex(0);
    combo->blockSignals(blocked);
}

// 词表 code → 中文名（快照词表副本线性扫描；缺项时显示数值本身，不显示空白）
QString StatisticsView::vocabNameCn(const QString &vocabName, int code) const
{
    for (const Integration::Vocabulary &vocab : m_snapshot.vocabularies) {
        if (vocab.name != vocabName)
            continue;
        for (const Integration::VocabularyItem &item : vocab.items) {
            if (item.code == code)
                return item.nameCn;
        }
        break;
    }
    return QString::number(code);
}
