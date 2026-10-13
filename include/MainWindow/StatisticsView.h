#ifndef MAINWINDOW_STATISTICSVIEW_H
#define MAINWINDOW_STATISTICSVIEW_H

// REQ-013 统计页视图（数据平台只读消费端，导航 index 4）。
// 依据 docs/requirements/REQ-013「统计项清单」与 docs/features/statistics-data-platform.md §2/§3：
//   - 全部数据经 StatisticsController 值类型信号到达（快照/明细分页），无任何模拟回退；
//     数据平台不可用时展示显式不可用态与重试入口（REQ-013 A3），不显示编造数据；
//   - 快照携带词表副本（9 表 61 项）驱动过滤下拉与明细词表列显示，UI 不访问 VocabularyCache；
//   - 三态页面：加载中 / 错误（消息+重试）/ 就绪（快照时间行 + 5 概览卡片 + 10 分布图 +
//     近 30 天趋势 + 告警处理统计 + 5 实体明细分页表）；
//   - 进页加载与手动刷新同一入口 triggerLoad()：刷新期间旧内容整体置灰不清空；
//   - 控制器由 MainWindow 持有并先于本视图创建（ctor 参数非空，非拥有指针）。

#include "MainWindow/StatisticsSnapshot.h"

#include <QWidget>

class QStackedLayout;
class QLabel;
class QPushButton;
class QScrollArea;
class QGridLayout;
class QVBoxLayout;
class QHBoxLayout;
class QTabWidget;
class QComboBox;
class QLineEdit;
class QTableWidget;

namespace QtCharts {
class QChart;
class QChartView;
}

class StatisticsController;

class StatisticsView : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsView(StatisticsController *controller, QWidget *parent = nullptr);

public slots:
    // 进页加载 / 手动刷新入口（MainWindow 在导航切到统计页时调用）
    void triggerLoad();

private slots:
    void onSnapshotReady(const StatisticsSnapshot &snapshot);
    void onSnapshotFetchFailed(const QString &message);
    void onListReady(const StatisticsListPage &page);
    void onListFetchFailed(const QString &resource, const QString &message);

private:
    // 概览卡片控件组（恰好 5 张：标题/主指标/副指标）
    struct CardWidgets {
        QLabel *title = nullptr;
        QLabel *total = nullptr;
        QLabel *secondary = nullptr;
    };

    // 明细页签控件组（纯控件聚合，无 Q_OBJECT；信号在 buildDetailTab 内以 lambda 接线）
    struct DetailTab {
        QWidget *filterRow = nullptr;       // 过滤控件行（本页签请求在途时禁用）
        QComboBox *filterCombo1 = nullptr;  // 维度一：目标/设备/任务=类型，告警=级别，检测记录=复核状态
        QComboBox *filterCombo2 = nullptr;  // 维度二：目标/设备/任务=状态，告警=是否处置(三态)，检测记录=模型类
        QComboBox *filterCombo3 = nullptr;  // 维度三：目标=威胁等级，检测记录=是否异常(三态)；设备/任务/告警页无
        QLineEdit *targetIdEdit = nullptr;  // 目标 ID 过滤（任务/告警/检测记录）
        QLineEdit *deviceIdEdit = nullptr;  // 设备 ID 过滤（任务/告警）
        QTableWidget *table = nullptr;
        QPushButton *prevPageBtn = nullptr;
        QPushButton *nextPageBtn = nullptr;
        QLabel *pageLabel = nullptr;
        QLabel *errorLabel = nullptr;       // 行内错误提示（默认隐藏，无弹窗）
        int currentPage = 1;
        qint64 total = 0;
        bool pending = false;               // 分页请求在途（worker 串行 FIFO，防重复触发）
    };

    // 页面骨架构建
    void buildUi();
    void buildStatusRow(QVBoxLayout *contentLayout);
    void buildCardsRow(QVBoxLayout *contentLayout);
    void buildDistributionSection(QVBoxLayout *contentLayout);
    void buildTrendSection(QVBoxLayout *contentLayout);
    void buildAlarmSection(QVBoxLayout *contentLayout);
    void buildDetailSection(QVBoxLayout *contentLayout);
    QWidget *buildDetailTab(int tabIndex, QWidget *parent);
    QComboBox *addComboFilter(QHBoxLayout *layout, const QString &label);
    QLineEdit *addIdFilter(QHBoxLayout *layout, const QString &placeholder);

    // 快照驱动更新
    void updateCards(const StatisticsSnapshot &snapshot);
    void updateDistributions(const StatisticsSnapshot &snapshot);
    void updateTrendChart(const StatisticsSnapshot &snapshot);
    void updateAlarmSection(const StatisticsSnapshot &snapshot);
    void populateFilterCombos();
    QtCharts::QChart *buildBarChart(const StatisticsDistribution &dist);
    QtCharts::QChart *buildPieChart(const StatisticsDistribution &dist);
    QWidget *buildEmptyChartPlaceholder(const QString &title);

    // 明细查询与回填
    void maybeRequestInitialList(int tabIndex);
    void resetAndRequestList(int tabIndex);
    void requestTabList(int tabIndex);
    void fillDetailTable(int tabIndex, const StatisticsListPage &page);
    void updateTabPager(int tabIndex);
    void fillVocabCombo(QComboBox *combo, const QString &vocabName);
    void fillTriStateCombo(QComboBox *combo);
    void fillModelClassCombo(QComboBox *combo);
    QString vocabNameCn(const QString &vocabName, int code) const;

    StatisticsController *m_controller = nullptr;  // 非拥有（MainWindow 持有）

    // 三态栈与各态页面
    QStackedLayout *m_stackLayout = nullptr;
    QWidget *m_loadingPage = nullptr;
    QWidget *m_errorPage = nullptr;
    QLabel *m_errorLabel = nullptr;
    QPushButton *m_retryBtn = nullptr;
    QScrollArea *m_readyScroll = nullptr;
    QWidget *m_readyContent = nullptr;

    // 就绪页：状态行（快照时间 + 刷新）
    QLabel *m_snapshotTimeLabel = nullptr;
    QLabel *m_snapshotErrorLabel = nullptr;
    QPushButton *m_refreshBtn = nullptr;

    // 就绪页：A 区卡片 / B 区分布网格
    QVector<CardWidgets> m_cards;
    QGridLayout *m_distGrid = nullptr;

    // 就绪页：C 区趋势 / D 区告警处理统计
    QtCharts::QChartView *m_trendChartView = nullptr;
    QtCharts::QChartView *m_alarmLevelChartView = nullptr;
    QtCharts::QChartView *m_alarmHandledChartView = nullptr;
    QLabel *m_alarmUnhandledLabel = nullptr;
    QLabel *m_alarmAvgHandleLabel = nullptr;

    // 就绪页：E 区明细（5 页签，固定序 目标/设备/任务/告警/检测记录）
    QTabWidget *m_detailTabs = nullptr;
    DetailTab m_tabs[5];

    StatisticsSnapshot m_snapshot;  // 最近一次成功快照（词表副本数据源）
    bool m_snapshotLoaded = false;
};

#endif // MAINWINDOW_STATISTICSVIEW_H
