// REQ-013 统计页聚合层实现：纯函数，输入全量实体转储 + 词表副本 + 基准时刻，
// 输出快照全部统计内容（卡片/分布/趋势/告警汇总）。
// 口径见 docs/requirements/REQ-013「统计项清单」与
// docs/features/statistics-data-platform.md §5（聚合语义）/§9（图型规则）。

#include "MainWindow/StatisticsAggregation.h"

#include <QDate>
#include <QMap>
#include <QPair>
#include <QtNumeric>

namespace {

// 趋势窗口长度：近 30 天（含今日），首日 = 今日 - 29。
constexpr int kTrendWindowDays = 30;

// ts 的 UTC 日是否落在 today；无效时间戳恒为 false（清单口径：不进「今日」窗口）。
bool isSameUtcDay(const QDateTime &ts, const QDate &today)
{
    return ts.isValid() && ts.toUTC().date() == today;
}

// 词表标签：按表名 + code 线性扫描（9 表 61 项量级无需索引）；
// 未命中（词表缺失或 code 越界）回退 "未知#<code>"，保证桶不丢行。
QString vocabularyLabel(const QVector<Integration::Vocabulary> &vocabularies,
                        const QString &vocabularyName, int code)
{
    for (const Integration::Vocabulary &vocabulary : vocabularies) {
        if (vocabulary.name != vocabularyName)
            continue;
        for (const Integration::VocabularyItem &item : vocabulary.items) {
            if (item.code == code)
                return item.nameCn;
        }
        break; // 命中表名后不再扫其他表（正典无同名表）
    }
    return QStringLiteral("未知#%1").arg(code);
}

// 副指标拼接："前缀 N"；置信度样本数 > 0 时追加 " · 平均置信度 X.XX"（两位小数）。
// 计数型副指标（今日新建/未处置等）以 0 样本调用，自然退化为纯计数。
QString secondaryText(const QString &prefix, qint64 count, double confidenceSum, qint64 samples)
{
    QString text = QStringLiteral("%1 %2").arg(prefix).arg(count);
    if (samples > 0)
        text += QStringLiteral(" · 平均置信度 %1").arg(confidenceSum / samples, 0, 'f', 2);
    return text;
}

// 词表维度分布：counts 按 code 存于 QMap（键升序即桶序），标签取词表 name_cn；
// 图型规则（§9）：等级序维度（威胁等级/告警级别）恒柱状，其余桶数 >5 用柱状、≤5 用饼图。
void appendVocabDistribution(QVector<StatisticsDistribution> *distributions, const QString &title,
                             const QMap<int, qint64> &counts,
                             const QVector<Integration::Vocabulary> &vocabularies,
                             const QString &vocabularyName, bool ordinal)
{
    StatisticsDistribution distribution;
    distribution.title = title;
    distribution.useBar = ordinal || counts.size() > 5;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        distribution.buckets.append(
            qMakePair(vocabularyLabel(vocabularies, vocabularyName, it.key()), it.value()));
    }
    distributions->append(distribution);
}

// 近 30 天零基趋势序列：首日 windowStart（今日 - 29），按日升序恰好 30 点。
StatisticsTrendSeries makeTrendSeries(const QString &name, bool defaultVisible, const QDate &windowStart)
{
    StatisticsTrendSeries series;
    series.name = name;
    series.defaultVisible = defaultVisible;
    for (int i = 0; i < kTrendWindowDays; ++i)
        series.points.append(qMakePair(windowStart.addDays(i), qint64(0)));
    return series;
}

// 趋势点累加：按 ts 的 UTC 日落桶；无效时间戳与窗口外日期跳过（清单口径）。
void addTrendPoint(StatisticsTrendSeries *series, const QDateTime &ts, const QDate &windowStart)
{
    if (!ts.isValid())
        return;
    const qint64 day = windowStart.daysTo(ts.toUTC().date());
    if (day < 0 || day >= kTrendWindowDays)
        return;
    series->points[static_cast<int>(day)].second += 1;
}

// 告警处理统计（清单 D 区）：均值只计入「已处置且两时刻有效且时长非负」的样本，
// 负时长（handleTime 早于 createTime）按脏数据剔除；无样本时 hasAvgHandle=false。
StatisticsAlarmSummary makeAlarmSummary(const QVector<Core::AlarmInfo> &alarms)
{
    StatisticsAlarmSummary summary;
    double durationSumSeconds = 0.0;
    qint64 samples = 0;
    for (const Core::AlarmInfo &alarm : alarms) {
        if (!alarm.isHandled) {
            ++summary.unhandled;
            continue;
        }
        ++summary.handled;
        if (alarm.handleTime.isValid() && alarm.createTime.isValid()
            && alarm.handleTime.toUTC() >= alarm.createTime.toUTC()) {
            durationSumSeconds +=
                (alarm.handleTime.toMSecsSinceEpoch() - alarm.createTime.toMSecsSinceEpoch()) / 1000.0;
            ++samples;
        }
    }
    if (samples > 0) {
        summary.hasAvgHandle = true;
        summary.avgHandleSeconds = durationSumSeconds / samples;
    }
    return summary;
}

} // namespace

StatisticsSnapshot aggregateStatistics(const StatisticsFullEntityDump &dump,
                                       const QVector<Integration::Vocabulary> &vocabularies,
                                       const QDateTime &nowUtc)
{
    // 时间基准一次性归一化到 UTC（nowUtc 由调用方保证有效），此后全部按 UTC 日比较。
    const QDateTime now = nowUtc.toUTC();
    const QDate today = now.date();
    const QDate windowStart = today.addDays(-(kTrendWindowDays - 1));

    StatisticsSnapshot snapshot;

    // ---- 概览卡片（恰好 5，固定序：目标/设备/任务/告警/检测记录）----
    // 隐患目标：总数；今日新探测（detectTime UTC 日=今日）；
    // 平均置信度取全量均值（非有限值跳过，不参与均值）。
    {
        qint64 todayDetected = 0;
        double confidenceSum = 0.0;
        qint64 confidenceSamples = 0;
        for (const Core::TargetInfo &target : dump.targets) {
            if (isSameUtcDay(target.detectTime, today))
                ++todayDetected;
            if (qIsFinite(target.confidence)) {
                confidenceSum += target.confidence;
                ++confidenceSamples;
            }
        }
        StatisticsCard card;
        card.title = QStringLiteral("隐患目标");
        card.total = dump.targets.size();
        card.secondaryText = secondaryText(QStringLiteral("今日新探测"), todayDetected,
                                           confidenceSum, confidenceSamples);
        snapshot.cards.append(card);
    }
    // 设备：总数；近 24 小时在线（lastOnlineTime 落在闭区间 [now-24h, now]）。
    {
        qint64 onlineRecently = 0;
        const QDateTime windowBegin = now.addSecs(-24 * 3600);
        for (const Core::DeviceInfo &device : dump.devices) {
            if (device.lastOnlineTime.isValid() && device.lastOnlineTime.toUTC() >= windowBegin
                && device.lastOnlineTime.toUTC() <= now)
                ++onlineRecently;
        }
        StatisticsCard card;
        card.title = QStringLiteral("设备");
        card.total = dump.devices.size();
        card.secondaryText = secondaryText(QStringLiteral("近24小时在线"), onlineRecently, 0.0, 0);
        snapshot.cards.append(card);
    }
    // 处置任务：总数；今日新建（createTime UTC 日=今日）。
    {
        qint64 todayCreated = 0;
        for (const Core::MissionInfo &mission : dump.missions) {
            if (isSameUtcDay(mission.createTime, today))
                ++todayCreated;
        }
        StatisticsCard card;
        card.title = QStringLiteral("处置任务");
        card.total = dump.missions.size();
        card.secondaryText = secondaryText(QStringLiteral("今日新建"), todayCreated, 0.0, 0);
        snapshot.cards.append(card);
    }
    // 告警：总数；未处置数。
    {
        qint64 unhandled = 0;
        for (const Core::AlarmInfo &alarm : dump.alarms) {
            if (!alarm.isHandled)
                ++unhandled;
        }
        StatisticsCard card;
        card.title = QStringLiteral("告警");
        card.total = dump.alarms.size();
        card.secondaryText = secondaryText(QStringLiteral("未处置"), unhandled, 0.0, 0);
        snapshot.cards.append(card);
    }
    // 检测记录：总数；异常数；平均置信度取全量均值（与目标卡片同口径）。
    {
        qint64 anomalyCount = 0;
        double confidenceSum = 0.0;
        qint64 confidenceSamples = 0;
        for (const Integration::DetectionRecordData &record : dump.detectionRecords) {
            if (record.hasAnomaly)
                ++anomalyCount;
            if (qIsFinite(record.confidence)) {
                confidenceSum += record.confidence;
                ++confidenceSamples;
            }
        }
        StatisticsCard card;
        card.title = QStringLiteral("检测记录");
        card.total = dump.detectionRecords.size();
        card.secondaryText = secondaryText(QStringLiteral("异常"), anomalyCount,
                                           confidenceSum, confidenceSamples);
        snapshot.cards.append(card);
    }

    // ---- 分布图（恰好 10，固定序：目标 3 → 设备 2 → 任务 2 → 告警 1 → 检测记录 2）----
    // 目标三图：类型/状态/威胁等级（等级序 → 恒柱状）。
    {
        QMap<int, qint64> typeCounts;
        QMap<int, qint64> statusCounts;
        QMap<int, qint64> threatCounts;
        for (const Core::TargetInfo &target : dump.targets) {
            ++typeCounts[static_cast<int>(target.type)];
            ++statusCounts[static_cast<int>(target.status)];
            ++threatCounts[static_cast<int>(target.threatLevel)];
        }
        appendVocabDistribution(&snapshot.distributions, QStringLiteral("目标类型分布"),
                                typeCounts, vocabularies, QStringLiteral("target_type"), false);
        appendVocabDistribution(&snapshot.distributions, QStringLiteral("目标状态分布"),
                                statusCounts, vocabularies, QStringLiteral("target_status"), false);
        appendVocabDistribution(&snapshot.distributions, QStringLiteral("目标威胁等级分布"),
                                threatCounts, vocabularies, QStringLiteral("threat_level"), true);
    }
    // 设备两图：类型/状态。
    {
        QMap<int, qint64> typeCounts;
        QMap<int, qint64> statusCounts;
        for (const Core::DeviceInfo &device : dump.devices) {
            ++typeCounts[static_cast<int>(device.type)];
            ++statusCounts[static_cast<int>(device.status)];
        }
        appendVocabDistribution(&snapshot.distributions, QStringLiteral("设备类型分布"),
                                typeCounts, vocabularies, QStringLiteral("device_type"), false);
        appendVocabDistribution(&snapshot.distributions, QStringLiteral("设备状态分布"),
                                statusCounts, vocabularies, QStringLiteral("device_status"), false);
    }
    // 任务两图：类型/状态。
    {
        QMap<int, qint64> typeCounts;
        QMap<int, qint64> statusCounts;
        for (const Core::MissionInfo &mission : dump.missions) {
            ++typeCounts[static_cast<int>(mission.type)];
            ++statusCounts[static_cast<int>(mission.status)];
        }
        appendVocabDistribution(&snapshot.distributions, QStringLiteral("任务类型分布"),
                                typeCounts, vocabularies, QStringLiteral("mission_type"), false);
        appendVocabDistribution(&snapshot.distributions, QStringLiteral("任务状态分布"),
                                statusCounts, vocabularies, QStringLiteral("mission_status"), false);
    }
    // 告警一图：级别（等级序 → 恒柱状）。
    {
        QMap<int, qint64> levelCounts;
        for (const Core::AlarmInfo &alarm : dump.alarms)
            ++levelCounts[static_cast<int>(alarm.level)];
        appendVocabDistribution(&snapshot.distributions, QStringLiteral("告警级别分布"),
                                levelCounts, vocabularies, QStringLiteral("alarm_level"), true);
    }
    // 检测记录两图：模型类（按原始 class_name 升序计数，空串输出为 "未知"）/ 复核状态。
    {
        QMap<QString, qint64> classNameCounts;
        QMap<int, qint64> reviewCounts;
        for (const Integration::DetectionRecordData &record : dump.detectionRecords) {
            ++classNameCounts[record.className];
            ++reviewCounts[record.review];
        }
        StatisticsDistribution classNameDistribution;
        classNameDistribution.title = QStringLiteral("检测记录模型类分布");
        classNameDistribution.useBar = classNameCounts.size() > 5;
        for (auto it = classNameCounts.constBegin(); it != classNameCounts.constEnd(); ++it) {
            const QString label = it.key().isEmpty() ? QStringLiteral("未知") : it.key();
            classNameDistribution.buckets.append(qMakePair(label, it.value()));
        }
        snapshot.distributions.append(classNameDistribution);
        appendVocabDistribution(&snapshot.distributions, QStringLiteral("检测记录复核状态分布"),
                                reviewCounts, vocabularies, QStringLiteral("detection_review"), false);
    }

    // ---- 日趋势（恰好 6，固定序：目标探测/检测记录/检测记录-异常/告警/设备登记/任务登记）----
    {
        StatisticsTrendSeries targetSeries =
            makeTrendSeries(QStringLiteral("目标探测"), true, windowStart);
        StatisticsTrendSeries recordSeries =
            makeTrendSeries(QStringLiteral("检测记录"), true, windowStart);
        StatisticsTrendSeries anomalySeries =
            makeTrendSeries(QStringLiteral("检测记录-异常"), true, windowStart);
        StatisticsTrendSeries alarmSeries =
            makeTrendSeries(QStringLiteral("告警"), true, windowStart);
        // 设备/任务登记默认隐藏（登记流水量大，避免稀释主趋势），经图例切换展示。
        StatisticsTrendSeries deviceSeries =
            makeTrendSeries(QStringLiteral("设备登记"), false, windowStart);
        StatisticsTrendSeries missionSeries =
            makeTrendSeries(QStringLiteral("任务登记"), false, windowStart);

        for (const Core::TargetInfo &target : dump.targets)
            addTrendPoint(&targetSeries, target.detectTime, windowStart);
        for (const Integration::DetectionRecordData &record : dump.detectionRecords) {
            addTrendPoint(&recordSeries, record.analyzedAt, windowStart);
            if (record.hasAnomaly)
                addTrendPoint(&anomalySeries, record.analyzedAt, windowStart);
        }
        for (const Core::AlarmInfo &alarm : dump.alarms)
            addTrendPoint(&alarmSeries, alarm.createTime, windowStart);
        for (const Core::DeviceInfo &device : dump.devices)
            addTrendPoint(&deviceSeries, device.createTime, windowStart);
        for (const Core::MissionInfo &mission : dump.missions)
            addTrendPoint(&missionSeries, mission.createTime, windowStart);

        snapshot.trends.append(targetSeries);
        snapshot.trends.append(recordSeries);
        snapshot.trends.append(anomalySeries);
        snapshot.trends.append(alarmSeries);
        snapshot.trends.append(deviceSeries);
        snapshot.trends.append(missionSeries);
    }

    // ---- 告警处理统计（清单 D 区）----
    snapshot.alarmSummary = makeAlarmSummary(dump.alarms);

    // 快照元数据（fetchedAtUtc/词表副本）由 worker 补全，本层不填。
    return snapshot;
}
