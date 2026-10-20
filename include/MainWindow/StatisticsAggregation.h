#ifndef MAINWINDOW_STATISTICSAGGREGATION_H
#define MAINWINDOW_STATISTICSAGGREGATION_H

// REQ-014 统计页聚合层（纯函数）：
// worker 线程跨页拉齐五实体全量行后，与词表副本、基准时刻一起送入本函数，
// 产出 StatisticsSnapshot 的全部统计内容（卡片/分布/趋势/告警汇总）。
// 口径依据 docs/requirements/REQ-014「统计项清单」与
// docs/features/statistics-data-platform.md §5（聚合语义）/§9（图型规则）：
//   - 纯函数：不访问网络/时钟/全局状态；基准时刻 nowUtc 由调用方传入（可测）；
//   - 快照元数据（fetchedAtUtc 与词表副本）不在此填充，由 worker 补全；
//   - 时间口径统一 UTC 日分桶；无效时间戳只计入卡片总数与分布，
//     不进「今日」窗口与趋势（REQ-014 清单口径）。

#include "Core/Data/Types.h"            // Core::TargetInfo/DeviceInfo/MissionInfo/AlarmInfo
#include "Integration/EntityMapping.h"  // Integration::DetectionRecordData
#include "Integration/VocabularyCache.h"  // Integration::Vocabulary
#include "MainWindow/StatisticsSnapshot.h"  // StatisticsSnapshot 及统计值类型

#include <QDateTime>
#include <QVector>

// 聚合输入：五实体全量行（worker 已跨页拉齐；行数即计数口径，不再看 meta.total）。
struct StatisticsFullEntityDump {
    QVector<Core::TargetInfo> targets;
    QVector<Core::DeviceInfo> devices;
    QVector<Core::MissionInfo> missions;
    QVector<Core::AlarmInfo> alarms;
    QVector<Integration::DetectionRecordData> detectionRecords;
};

// 纯聚合：由全量实体转储 + 词表副本 + 基准时刻产出快照统计内容。
// nowUtc 须为有效时刻（两个调用方——worker 与单测——均结构性保证）。
StatisticsSnapshot aggregateStatistics(const StatisticsFullEntityDump &dump,
                                       const QVector<Integration::Vocabulary> &vocabularies,
                                       const QDateTime &nowUtc);

#endif // MAINWINDOW_STATISTICSAGGREGATION_H
