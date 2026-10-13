#ifndef MAINWINDOW_STATISTICSSNAPSHOT_H
#define MAINWINDOW_STATISTICSSNAPSHOT_H

// REQ-013 统计页数据快照（值类型）：worker 线程聚合完成后经排队信号整体回投 UI 线程。
// 依据 docs/requirements/REQ-013「统计项清单」与 docs/features/statistics-data-platform.md §5：
//   - 全部数据来自本地数据平台真实数据（只读）；UI 线程只持有本值类型，不接触任何
//     Integration/Core 对象指针或引用；
//   - 快照携带词表副本（9 表 61 项，加载后只读）：分布桶标签、明细区词表列与过滤下拉
//     均取自此处，UI 不另行访问 VocabularyCache；
//   - 副指标与平均处置时长由聚合层按清单口径拼算完成，UI 不做二次推导。

#include "Core/Data/Types.h"          // Core::TargetInfo/DeviceInfo/MissionInfo/AlarmInfo
#include "Integration/EntityMapping.h"  // Integration::DetectionRecordData
#include "Integration/VocabularyCache.h"  // Integration::Vocabulary/VocabularyItem

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QVector>

// 概览卡片（恰好 5 张，固定序：目标/设备/任务/告警/检测记录）
struct StatisticsCard {
    QString title;           // 卡片标题（"隐患目标"/"设备"/"处置任务"/"告警"/"检测记录"）
    qint64 total = 0;        // 主指标：对应实体列表 meta.total
    QString secondaryText;   // 副指标行（聚合层按口径拼好，如 "今日新探测 3 · 平均置信度 0.82"；无副指标为空串）
};

// 分布图（恰好 10 张，固定序：目标 3（类型/状态/威胁等级）→ 设备 2（类型/状态）→
// 任务 2（类型/状态）→ 告警 1（级别）→ 检测记录 2（模型类/复核状态））
struct StatisticsDistribution {
    QString title;                            // 图题（"目标类型分布" 等，中文）
    bool useBar = false;                      // 图型（批准规则）：桶数 >5 或等级序维度为 true=柱状，否则 false=饼图
    QVector<QPair<QString, qint64>> buckets;  // （词表 name_cn 或维度标签，计数），按词表 code 升序；空实体为空数组
};

// 日趋势序列（固定序：目标探测/检测记录/检测记录-异常/告警/设备登记/任务登记；
// defaultVisible=false 的序列默认隐藏，经图例切换展示）
struct StatisticsTrendSeries {
    QString name;                           // 序列名（"目标探测" 等，中文）
    bool defaultVisible = true;             // false 的序列（设备/任务登记）默认隐藏，经切换展示
    QVector<QPair<QDate, qint64>> points;   // 近 30 天按 UTC 日分桶，空日补 0，按日升序，恰好 30 点
};

// 告警处理统计（清单 D 区）
struct StatisticsAlarmSummary {
    qint64 handled = 0;             // 已处置数（is_handled=true）
    qint64 unhandled = 0;           // 未处置数（is_handled=false）
    double avgHandleSeconds = 0.0;  // 平均处置时长（秒）：已处置且 handleTime 有效告警的 handleTime-createTime 均值
    bool hasAvgHandle = false;      // 无可计入样本时为 false（UI 显示 "—"）
};

// 全量快照：一次"进页加载/手动刷新"的完整产物
struct StatisticsSnapshot {
    QDateTime fetchedAtUtc;                        // 拉取完成时刻（UTC；UI 负责转本地时区显示）
    QVector<StatisticsCard> cards;                 // 恰好 5
    QVector<StatisticsDistribution> distributions; // 恰好 10
    QVector<StatisticsTrendSeries> trends;         // 恰好 6：目标探测/检测记录/检测记录-异常/告警/设备登记/任务登记
    StatisticsAlarmSummary alarmSummary;
    QVector<Integration::Vocabulary> vocabularies; // 9 词表副本（明细区词表列与过滤下拉的数据源）
};

// 明细区一页结果（直接消费分页 API，不进全量快照）：
// resource 标识实体，仅对应实体行成员有值，其余为空。
struct StatisticsListPage {
    QString resource;   // targets / devices / missions / alarms / detection-records
    int page = 1;       // 当前页（1 起）
    qint64 total = 0;   // meta.total（过滤后跨页总数）
    QVector<Core::TargetInfo> targets;
    QVector<Core::DeviceInfo> devices;
    QVector<Core::MissionInfo> missions;
    QVector<Core::AlarmInfo> alarms;
    QVector<Integration::DetectionRecordData> detectionRecords;
};

#endif // MAINWINDOW_STATISTICSSNAPSHOT_H
