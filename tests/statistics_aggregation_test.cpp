// REQ-014 统计页聚合层单元测试：验证纯函数 aggregateStatistics 的全部口径
//（清单 A 概览卡片 / B 分布图 / C 日趋势 / D 告警处理统计，见
// docs/requirements/REQ-014 与 docs/features/statistics-data-platform.md
// §5 聚合语义 / §9 图型规则）。
// 全部实体与词表为本地合成数据（不触网、不依赖数据平台服务）；
// 时间基准统一固定为 2026-10-13 12:00 UTC，趋势窗口 = [2026-09-14, 2026-10-13]。
// 快照元数据（fetchedAtUtc/词表副本）由 worker 补全，本测试钉住「聚合层不填」。

#include "MainWindow/StatisticsAggregation.h"

#include <QtNumeric>
#include <QtTest>

#include <QStringList>

namespace {

// 统一基准时刻（UTC）与趋势窗口首日（今日 - 29）。
const QDateTime kNowUtc(QDate(2026, 10, 13), QTime(12, 0, 0), Qt::UTC);
const QDate kWindowStart(2026, 9, 14);

QDateTime utcTime(int year, int month, int day, int hour, int minute, int second)
{
    return QDateTime(QDate(year, month, day), QTime(hour, minute, second), Qt::UTC);
}

// 合成词表：code 0..maxCode（与正典 9 表 61 项码位一致），name_cn = 前缀 + code。
Integration::Vocabulary makeVocabulary(const QString &name, int maxCode, const QString &prefix)
{
    Integration::Vocabulary vocabulary;
    vocabulary.name = name;
    for (int code = 0; code <= maxCode; ++code) {
        Integration::VocabularyItem item;
        item.code = code;
        item.nameCn = QStringLiteral("%1%2").arg(prefix).arg(code);
        vocabulary.items.append(item);
    }
    return vocabulary;
}

// 全部 9 表（合计 61 项），标签覆盖测试用到的全部 code。
QVector<Integration::Vocabulary> makeVocabularies()
{
    QVector<Integration::Vocabulary> vocabularies;
    vocabularies.append(makeVocabulary(QStringLiteral("target_type"), 11, QStringLiteral("类型")));
    vocabularies.append(makeVocabulary(QStringLiteral("threat_level"), 4, QStringLiteral("等级")));
    vocabularies.append(makeVocabulary(QStringLiteral("target_status"), 6, QStringLiteral("状态")));
    vocabularies.append(makeVocabulary(QStringLiteral("mission_type"), 4, QStringLiteral("任务类型")));
    vocabularies.append(makeVocabulary(QStringLiteral("mission_status"), 8, QStringLiteral("任务状态")));
    vocabularies.append(makeVocabulary(QStringLiteral("device_type"), 7, QStringLiteral("设备类型")));
    vocabularies.append(makeVocabulary(QStringLiteral("device_status"), 6, QStringLiteral("设备状态")));
    vocabularies.append(makeVocabulary(QStringLiteral("alarm_level"), 4, QStringLiteral("级别")));
    vocabularies.append(makeVocabulary(QStringLiteral("detection_review"), 2, QStringLiteral("复核")));
    return vocabularies;
}

// ---- 实体构造（只填聚合消费的字段，其余取默认值）----

Core::TargetInfo makeTarget(int type, int threat, int status, const QDateTime &detectTime,
                            double confidence)
{
    Core::TargetInfo target;
    target.type = static_cast<Core::TargetType>(type);
    target.threatLevel = static_cast<Core::ThreatLevel>(threat);
    target.status = static_cast<Core::TargetStatus>(status);
    target.detectTime = detectTime;
    target.confidence = confidence;
    return target;
}

Core::DeviceInfo makeDevice(int type, int status, const QDateTime &lastOnlineTime,
                            const QDateTime &createTime)
{
    Core::DeviceInfo device;
    device.type = static_cast<Core::DeviceType>(type);
    device.status = static_cast<Core::DeviceStatus>(status);
    device.lastOnlineTime = lastOnlineTime;
    device.createTime = createTime;
    return device;
}

Core::MissionInfo makeMission(int type, int status, const QDateTime &createTime)
{
    Core::MissionInfo mission;
    mission.type = static_cast<Core::MissionType>(type);
    mission.status = static_cast<Core::MissionStatus>(status);
    mission.createTime = createTime;
    return mission;
}

Core::AlarmInfo makeAlarm(int level, bool handled, const QDateTime &createTime,
                          const QDateTime &handleTime)
{
    Core::AlarmInfo alarm;
    alarm.level = static_cast<Core::AlarmLevel>(level);
    alarm.isHandled = handled;
    alarm.createTime = createTime;
    alarm.handleTime = handleTime;
    return alarm;
}

Integration::DetectionRecordData makeRecord(const QDateTime &analyzedAt, bool hasAnomaly,
                                            const QString &className, double confidence, int review)
{
    Integration::DetectionRecordData record;
    record.analyzedAt = analyzedAt;
    record.hasAnomaly = hasAnomaly;
    record.className = className;
    record.confidence = confidence;
    record.review = review;
    return record;
}

// ---- 断言辅助 ----

// 序列取数：日期命中返回计数，未命中返回 -1（区分「空日补 0」与「窗口外未命中」）。
qint64 pointFor(const StatisticsTrendSeries &series, const QDate &date)
{
    for (const QPair<QDate, qint64> &point : series.points) {
        if (point.first == date)
            return point.second;
    }
    return -1;
}

// 分布桶计数总和
qint64 bucketSumOf(const StatisticsDistribution &distribution)
{
    qint64 sum = 0;
    for (const QPair<QString, qint64> &bucket : distribution.buckets)
        sum += bucket.second;
    return sum;
}

} // namespace

class StatisticsAggregationTest : public QObject
{
    Q_OBJECT

private slots:
    // 清单 A：五张概览卡片的主指标与副指标口径
    void cardTotalsAndSecondaryTexts();
    // 清单 B：十张分布图的桶标签、桶计数与图型规则（含无词表回退）
    void distributionBucketsLabelsAndChartType();
    // 清单 C：六条近 30 天趋势序列的分桶、补零与窗口边界
    void trendSeriesBucketing();
    // 清单 D：告警处理统计（计数/平均处置时长/脏数据剔除）
    void alarmSummaryMetrics();
    // 边界：今日与 24 小时窗口边界、时区偏移、无效时间戳
    void boundaryWindowsAndInvalidTimestamps();
    // 基线：空转储 → 全零快照（元数据由 worker 补全）
    void emptyDumpBaseline();
};

void StatisticsAggregationTest::cardTotalsAndSecondaryTexts()
{
    StatisticsFullEntityDump dump;
    // 目标 3：今日 2（置信度 0.80/0.90）、昨日 1（置信度 NaN 不计均值）
    dump.targets.append(makeTarget(1, 1, 1, utcTime(2026, 10, 13, 8, 0, 0), 0.80));
    dump.targets.append(makeTarget(2, 3, 2, utcTime(2026, 10, 13, 9, 30, 0), 0.90));
    dump.targets.append(makeTarget(5, 4, 3, utcTime(2026, 10, 12, 10, 0, 0), qQNaN()));
    // 设备 3：lastOnline 距基准 -2h / 恰 -24h（闭区间边界，计入）/ -25h（不计）
    dump.devices.append(makeDevice(1, 2, kNowUtc.addSecs(-2 * 3600), QDateTime()));
    dump.devices.append(makeDevice(2, 3, kNowUtc.addSecs(-24 * 3600), QDateTime()));
    dump.devices.append(makeDevice(3, 4, kNowUtc.addSecs(-25 * 3600), QDateTime()));
    // 任务 2：今日 1、历史 1
    dump.missions.append(makeMission(0, 1, utcTime(2026, 10, 13, 7, 0, 0)));
    dump.missions.append(makeMission(1, 4, utcTime(2026, 10, 10, 7, 0, 0)));
    // 告警 3：已处置 1、未处置 2
    dump.alarms.append(makeAlarm(2, true, utcTime(2026, 10, 12, 6, 0, 0),
                                  utcTime(2026, 10, 12, 7, 0, 0)));
    dump.alarms.append(makeAlarm(3, false, utcTime(2026, 10, 12, 8, 0, 0), QDateTime()));
    dump.alarms.append(makeAlarm(1, false, utcTime(2026, 10, 13, 9, 0, 0), QDateTime()));
    // 检测记录 2：今日异常 1（置信度 0.5）、昨日正常 1（置信度 NaN 不计均值）
    dump.detectionRecords.append(
        makeRecord(utcTime(2026, 10, 13, 10, 0, 0), true, QStringLiteral("landmines"), 0.5, 0));
    dump.detectionRecords.append(
        makeRecord(utcTime(2026, 10, 12, 11, 0, 0), false, QString(), qQNaN(), 1));

    const StatisticsSnapshot snapshot = aggregateStatistics(dump, makeVocabularies(), kNowUtc);

    QCOMPARE(snapshot.cards.size(), 5);
    const QStringList expectedTitles = {
        QStringLiteral("隐患目标"), QStringLiteral("设备"), QStringLiteral("处置任务"),
        QStringLiteral("告警"), QStringLiteral("检测记录")
    };
    const qint64 expectedTotals[] = { 3, 3, 2, 3, 2 };
    const QStringList expectedSecondary = {
        QStringLiteral("今日新探测 2 · 平均置信度 0.85"),  // (0.80+0.90)/2，NaN 剔除
        QStringLiteral("近24小时在线 2"),                  // -2h 与恰 -24h（闭区间）计入
        QStringLiteral("今日新建 1"),
        QStringLiteral("未处置 2"),
        QStringLiteral("异常 1 · 平均置信度 0.50")         // 仅 0.5 一个样本
    };
    for (int i = 0; i < snapshot.cards.size(); ++i) {
        QCOMPARE(snapshot.cards.at(i).title, expectedTitles.at(i));
        QCOMPARE(snapshot.cards.at(i).total, expectedTotals[i]);
        QCOMPARE(snapshot.cards.at(i).secondaryText, expectedSecondary.at(i));
    }
}

void StatisticsAggregationTest::distributionBucketsLabelsAndChartType()
{
    StatisticsFullEntityDump dump;
    // 目标 4：类型 {1×2, 2, 5}；状态 {1×2, 2, 3}；威胁 {1, 3×2, 4}
    dump.targets.append(makeTarget(1, 1, 1, kNowUtc, 0.5));
    dump.targets.append(makeTarget(1, 3, 1, kNowUtc, 0.5));
    dump.targets.append(makeTarget(2, 3, 2, kNowUtc, 0.5));
    dump.targets.append(makeTarget(5, 4, 3, kNowUtc, 0.5));
    // 设备 6：类型 0-5（6 桶 → 柱状）；状态 {2×2, 3, 4, 5, 6}（5 桶 → 饼图边界）
    dump.devices.append(makeDevice(0, 2, QDateTime(), QDateTime()));
    dump.devices.append(makeDevice(1, 2, QDateTime(), QDateTime()));
    dump.devices.append(makeDevice(2, 3, QDateTime(), QDateTime()));
    dump.devices.append(makeDevice(3, 4, QDateTime(), QDateTime()));
    dump.devices.append(makeDevice(4, 5, QDateTime(), QDateTime()));
    dump.devices.append(makeDevice(5, 6, QDateTime(), QDateTime()));
    // 任务 3；告警 2（级别 2、3 → 等级序柱状）
    dump.missions.append(makeMission(0, 1, kNowUtc));
    dump.missions.append(makeMission(1, 4, kNowUtc));
    dump.missions.append(makeMission(2, 6, kNowUtc));
    dump.alarms.append(makeAlarm(2, false, kNowUtc, QDateTime()));
    dump.alarms.append(makeAlarm(3, true, kNowUtc, kNowUtc));
    // 检测记录 3：class_name 空/"landmines"/"submunitions"；review 0/1/1
    dump.detectionRecords.append(makeRecord(kNowUtc, true, QString(), 0.5, 0));
    dump.detectionRecords.append(
        makeRecord(kNowUtc, true, QStringLiteral("landmines"), 0.5, 1));
    dump.detectionRecords.append(
        makeRecord(kNowUtc, false, QStringLiteral("submunitions"), 0.5, 1));

    const StatisticsSnapshot snapshot = aggregateStatistics(dump, makeVocabularies(), kNowUtc);

    QCOMPARE(snapshot.distributions.size(), 10);
    // 固定序图题（目标 3 → 设备 2 → 任务 2 → 告警 1 → 检测记录 2）
    const QStringList expectedTitles = {
        QStringLiteral("目标类型分布"),      QStringLiteral("目标状态分布"),
        QStringLiteral("目标威胁等级分布"),  QStringLiteral("设备类型分布"),
        QStringLiteral("设备状态分布"),      QStringLiteral("任务类型分布"),
        QStringLiteral("任务状态分布"),      QStringLiteral("告警级别分布"),
        QStringLiteral("检测记录模型类分布"), QStringLiteral("检测记录复核状态分布")
    };
    for (int i = 0; i < snapshot.distributions.size(); ++i)
        QCOMPARE(snapshot.distributions.at(i).title, expectedTitles.at(i));

    // 目标类型：3 桶、词表 name_cn 标签、code 升序、桶数 ≤5 → 饼图
    const StatisticsDistribution &targetType = snapshot.distributions.at(0);
    QCOMPARE(targetType.useBar, false);
    QCOMPARE(targetType.buckets.size(), 3);
    QCOMPARE(targetType.buckets.at(0).first, QStringLiteral("类型1"));
    QCOMPARE(targetType.buckets.at(0).second, qint64(2));
    QCOMPARE(targetType.buckets.at(1).first, QStringLiteral("类型2"));
    QCOMPARE(targetType.buckets.at(1).second, qint64(1));
    QCOMPARE(targetType.buckets.at(2).first, QStringLiteral("类型5"));
    QCOMPARE(targetType.buckets.at(2).second, qint64(1));
    QCOMPARE(bucketSumOf(targetType), qint64(4));

    // 目标威胁等级：等级序维度 → 恒柱状
    const StatisticsDistribution &threat = snapshot.distributions.at(2);
    QCOMPARE(threat.useBar, true);
    QCOMPARE(threat.buckets.size(), 3);
    QCOMPARE(threat.buckets.at(0).first, QStringLiteral("等级1"));
    QCOMPARE(threat.buckets.at(0).second, qint64(1));
    QCOMPARE(threat.buckets.at(1).first, QStringLiteral("等级3"));
    QCOMPARE(threat.buckets.at(1).second, qint64(2));
    QCOMPARE(threat.buckets.at(2).first, QStringLiteral("等级4"));
    QCOMPARE(threat.buckets.at(2).second, qint64(1));

    // 设备类型：6 桶 >5 → 柱状；code 0 在正典词表内（设备类型码位 0-7）
    const StatisticsDistribution &deviceType = snapshot.distributions.at(3);
    QCOMPARE(deviceType.useBar, true);
    QCOMPARE(deviceType.buckets.size(), 6);
    QCOMPARE(deviceType.buckets.at(0).first, QStringLiteral("设备类型0"));
    QCOMPARE(deviceType.buckets.at(5).first, QStringLiteral("设备类型5"));

    // 设备状态：恰 5 桶 → 饼图（图型规则边界）
    const StatisticsDistribution &deviceStatus = snapshot.distributions.at(4);
    QCOMPARE(deviceStatus.useBar, false);
    QCOMPARE(deviceStatus.buckets.size(), 5);
    QCOMPARE(deviceStatus.buckets.at(0).first, QStringLiteral("设备状态2"));
    QCOMPARE(deviceStatus.buckets.at(0).second, qint64(2));

    // 任务两图：结构在场（桶和 = 3）
    QCOMPARE(bucketSumOf(snapshot.distributions.at(5)), qint64(3));
    QCOMPARE(bucketSumOf(snapshot.distributions.at(6)), qint64(3));

    // 告警级别：等级序维度 → 恒柱状
    const StatisticsDistribution &alarmLevel = snapshot.distributions.at(7);
    QCOMPARE(alarmLevel.useBar, true);
    QCOMPARE(alarmLevel.buckets.size(), 2);
    QCOMPARE(alarmLevel.buckets.at(0).first, QStringLiteral("级别2"));
    QCOMPARE(alarmLevel.buckets.at(0).second, qint64(1));
    QCOMPARE(alarmLevel.buckets.at(1).first, QStringLiteral("级别3"));
    QCOMPARE(alarmLevel.buckets.at(1).second, qint64(1));

    // 检测记录模型类：空串 → "未知"；按类名升序；3 桶 → 饼图
    const StatisticsDistribution &className = snapshot.distributions.at(8);
    QCOMPARE(className.useBar, false);
    QCOMPARE(className.buckets.size(), 3);
    QCOMPARE(className.buckets.at(0).first, QStringLiteral("未知"));
    QCOMPARE(className.buckets.at(0).second, qint64(1));
    QCOMPARE(className.buckets.at(1).first, QStringLiteral("landmines"));
    QCOMPARE(className.buckets.at(1).second, qint64(1));
    QCOMPARE(className.buckets.at(2).first, QStringLiteral("submunitions"));
    QCOMPARE(className.buckets.at(2).second, qint64(1));
    QCOMPARE(bucketSumOf(className), qint64(3));

    // 检测记录复核状态：review 0/1 → 词表标签
    const StatisticsDistribution &review = snapshot.distributions.at(9);
    QCOMPARE(review.buckets.size(), 2);
    QCOMPARE(review.buckets.at(0).first, QStringLiteral("复核0"));
    QCOMPARE(review.buckets.at(0).second, qint64(1));
    QCOMPARE(review.buckets.at(1).first, QStringLiteral("复核1"));
    QCOMPARE(review.buckets.at(1).second, qint64(2));

    // 无词表回退：标签 "未知#<code>"，桶不丢行
    StatisticsFullEntityDump noVocabDump;
    noVocabDump.targets.append(makeTarget(1, 3, 2, kNowUtc, 0.5));
    const StatisticsSnapshot noVocabSnapshot =
        aggregateStatistics(noVocabDump, QVector<Integration::Vocabulary>(), kNowUtc);
    QCOMPARE(noVocabSnapshot.distributions.at(0).buckets.size(), 1);
    QCOMPARE(noVocabSnapshot.distributions.at(0).buckets.at(0).first, QStringLiteral("未知#1"));
    QCOMPARE(noVocabSnapshot.distributions.at(0).buckets.at(0).second, qint64(1));
    QCOMPARE(noVocabSnapshot.distributions.at(2).buckets.size(), 1);
    QCOMPARE(noVocabSnapshot.distributions.at(2).buckets.at(0).first, QStringLiteral("未知#3"));
}

void StatisticsAggregationTest::trendSeriesBucketing()
{
    StatisticsFullEntityDump dump;
    // 目标 4：窗口首日 / 窗口内 / 今日 / 窗口前一日（出窗不计）
    dump.targets.append(makeTarget(1, 1, 1, utcTime(2026, 9, 14, 0, 0, 0), 0.5));
    dump.targets.append(makeTarget(1, 1, 1, utcTime(2026, 10, 12, 23, 0, 0), 0.5));
    dump.targets.append(makeTarget(1, 1, 1, utcTime(2026, 10, 13, 8, 0, 0), 0.5));
    dump.targets.append(makeTarget(1, 1, 1, utcTime(2026, 9, 13, 23, 59, 59), 0.5));
    // 检测记录 3：今日 2（其中 1 异常）、昨日 1（异常）
    dump.detectionRecords.append(
        makeRecord(utcTime(2026, 10, 13, 10, 0, 0), true, QStringLiteral("landmines"), 0.5, 0));
    dump.detectionRecords.append(
        makeRecord(utcTime(2026, 10, 13, 11, 0, 0), false, QString(), 0.5, 1));
    dump.detectionRecords.append(
        makeRecord(utcTime(2026, 10, 12, 9, 0, 0), true, QStringLiteral("submunitions"), 0.5, 1));
    // 告警/设备/任务各 1：今日 / 09-20 / 今日
    dump.alarms.append(makeAlarm(1, false, utcTime(2026, 10, 13, 9, 0, 0), QDateTime()));
    dump.devices.append(makeDevice(1, 2, QDateTime(), utcTime(2026, 9, 20, 0, 0, 0)));
    dump.missions.append(makeMission(1, 1, utcTime(2026, 10, 13, 7, 0, 0)));

    const StatisticsSnapshot snapshot = aggregateStatistics(dump, makeVocabularies(), kNowUtc);

    QCOMPARE(snapshot.trends.size(), 6);
    // 固定序 + 默认可见性（设备/任务登记默认隐藏，经图例切换展示）
    const QStringList expectedNames = {
        QStringLiteral("目标探测"),     QStringLiteral("检测记录"),
        QStringLiteral("检测记录-异常"), QStringLiteral("告警"),
        QStringLiteral("设备登记"),     QStringLiteral("任务登记")
    };
    for (int i = 0; i < 4; ++i) {
        QCOMPARE(snapshot.trends.at(i).name, expectedNames.at(i));
        QCOMPARE(snapshot.trends.at(i).defaultVisible, true);
    }
    for (int i = 4; i < 6; ++i) {
        QCOMPARE(snapshot.trends.at(i).name, expectedNames.at(i));
        QCOMPARE(snapshot.trends.at(i).defaultVisible, false);
    }
    // 结构：每条恰好 30 点、首日 = 窗口首日、末日 = 今日、按日严格升序
    for (const StatisticsTrendSeries &series : snapshot.trends) {
        QCOMPARE(series.points.size(), 30);
        QCOMPARE(series.points.first().first, kWindowStart);
        QCOMPARE(series.points.last().first, QDate(2026, 10, 13));
        for (int i = 1; i < series.points.size(); ++i)
            QVERIFY(series.points.at(i).first > series.points.at(i - 1).first);
    }
    // 目标探测：首日/10-12/10-13 各 1；窗口前一日未命中（-1）
    QCOMPARE(pointFor(snapshot.trends.at(0), QDate(2026, 9, 14)), qint64(1));
    QCOMPARE(pointFor(snapshot.trends.at(0), QDate(2026, 10, 12)), qint64(1));
    QCOMPARE(pointFor(snapshot.trends.at(0), QDate(2026, 10, 13)), qint64(1));
    QCOMPARE(pointFor(snapshot.trends.at(0), QDate(2026, 9, 13)), qint64(-1));
    // 检测记录：今日 2、昨日 1；空日补 0
    QCOMPARE(pointFor(snapshot.trends.at(1), QDate(2026, 10, 13)), qint64(2));
    QCOMPARE(pointFor(snapshot.trends.at(1), QDate(2026, 10, 12)), qint64(1));
    QCOMPARE(pointFor(snapshot.trends.at(1), QDate(2026, 10, 11)), qint64(0));
    // 检测记录-异常：今日 1、昨日 1；正常记录不进异常序列
    QCOMPARE(pointFor(snapshot.trends.at(2), QDate(2026, 10, 13)), qint64(1));
    QCOMPARE(pointFor(snapshot.trends.at(2), QDate(2026, 10, 12)), qint64(1));
    QCOMPARE(pointFor(snapshot.trends.at(2), QDate(2026, 10, 11)), qint64(0));
    // 告警 / 设备登记 / 任务登记
    QCOMPARE(pointFor(snapshot.trends.at(3), QDate(2026, 10, 13)), qint64(1));
    QCOMPARE(pointFor(snapshot.trends.at(4), QDate(2026, 9, 20)), qint64(1));
    QCOMPARE(pointFor(snapshot.trends.at(5), QDate(2026, 10, 13)), qint64(1));
}

void StatisticsAggregationTest::alarmSummaryMetrics()
{
    StatisticsFullEntityDump dump;
    // 6 条告警：2 条有效样本（1800s、3600s）、2 条未处置、2 条已处置脏数据
    dump.alarms.append(makeAlarm(2, true, utcTime(2026, 10, 12, 6, 0, 0),
                                 utcTime(2026, 10, 12, 6, 30, 0)));   // 有效：1800s
    dump.alarms.append(makeAlarm(3, true, utcTime(2026, 10, 12, 7, 0, 0),
                                 utcTime(2026, 10, 12, 8, 0, 0)));    // 有效：3600s
    dump.alarms.append(makeAlarm(1, false, utcTime(2026, 10, 12, 8, 0, 0), QDateTime()));
    dump.alarms.append(makeAlarm(2, false, utcTime(2026, 10, 13, 9, 0, 0), QDateTime()));
    dump.alarms.append(makeAlarm(2, true, utcTime(2026, 10, 12, 10, 0, 0),
                                 utcTime(2026, 10, 12, 9, 0, 0)));    // 脏数据：负时长 → 剔除均值
    dump.alarms.append(
        makeAlarm(3, true, utcTime(2026, 10, 12, 11, 0, 0), QDateTime())); // 脏数据：handleTime 无效 → 剔除均值

    const StatisticsSnapshot snapshot = aggregateStatistics(dump, makeVocabularies(), kNowUtc);

    // 已处置计数包含脏数据（4），均值只计有效样本 (1800+3600)/2 = 2700s
    QCOMPARE(snapshot.alarmSummary.handled, qint64(4));
    QCOMPARE(snapshot.alarmSummary.unhandled, qint64(2));
    QCOMPARE(snapshot.alarmSummary.hasAvgHandle, true);
    QCOMPARE(snapshot.alarmSummary.avgHandleSeconds, 2700.0);

    // 全未处置：无样本 → hasAvgHandle=false，均值保持 0.0
    StatisticsFullEntityDump unhandledOnly;
    unhandledOnly.alarms.append(makeAlarm(1, false, utcTime(2026, 10, 12, 8, 0, 0), QDateTime()));
    const StatisticsSnapshot unhandledSnapshot =
        aggregateStatistics(unhandledOnly, makeVocabularies(), kNowUtc);
    QCOMPARE(unhandledSnapshot.alarmSummary.handled, qint64(0));
    QCOMPARE(unhandledSnapshot.alarmSummary.unhandled, qint64(1));
    QCOMPARE(unhandledSnapshot.alarmSummary.hasAvgHandle, false);
    QCOMPARE(unhandledSnapshot.alarmSummary.avgHandleSeconds, 0.0);
}

void StatisticsAggregationTest::boundaryWindowsAndInvalidTimestamps()
{
    StatisticsFullEntityDump dump;
    // 4 个目标：今日最后一秒 / 明日零点（不进今日与窗口）/ +08:00 偏移（UTC 落今日）/ 无效时间戳
    dump.targets.append(makeTarget(1, 1, 1, utcTime(2026, 10, 13, 23, 59, 59), 0.5));
    dump.targets.append(makeTarget(1, 1, 1, utcTime(2026, 10, 14, 0, 0, 0), 0.5));
    dump.targets.append(makeTarget(
        2, 2, 2, QDateTime(QDate(2026, 10, 14), QTime(1, 0, 0), Qt::OffsetFromUTC, 8 * 3600),
        0.5));  // 2026-10-14 01:00 +08:00 == 2026-10-13 17:00 UTC → 今日
    dump.targets.append(makeTarget(2, 2, 2, QDateTime(), 0.5));  // 无效时间戳

    const StatisticsSnapshot snapshot = aggregateStatistics(dump, makeVocabularies(), kNowUtc);

    // 总数与分布计入全部 4 条（时间无效不剔除实体）
    QCOMPARE(snapshot.cards.at(0).total, qint64(4));
    QCOMPARE(bucketSumOf(snapshot.distributions.at(0)), qint64(4));
    // 「今日」按 UTC 日比较：23:59:59 与 +08:00 偏移换算后的 17:00 计入；
    // 置信度均值与时间有效性无关（4 × 0.5）
    QCOMPARE(snapshot.cards.at(0).secondaryText,
             QStringLiteral("今日新探测 2 · 平均置信度 0.50"));
    // 趋势：今日 2；明日零点超出窗口 → 未命中
    QCOMPARE(pointFor(snapshot.trends.at(0), QDate(2026, 10, 13)), qint64(2));
    QCOMPARE(pointFor(snapshot.trends.at(0), QDate(2026, 10, 14)), qint64(-1));
}

void StatisticsAggregationTest::emptyDumpBaseline()
{
    const StatisticsSnapshot snapshot =
        aggregateStatistics(StatisticsFullEntityDump(), makeVocabularies(), kNowUtc);

    // 卡片：全零主指标、纯计数副指标（无置信度样本 → 不追加均值段）
    QCOMPARE(snapshot.cards.size(), 5);
    const QStringList expectedSecondary = {
        QStringLiteral("今日新探测 0"), QStringLiteral("近24小时在线 0"),
        QStringLiteral("今日新建 0"),   QStringLiteral("未处置 0"),
        QStringLiteral("异常 0")
    };
    for (int i = 0; i < snapshot.cards.size(); ++i) {
        QCOMPARE(snapshot.cards.at(i).total, qint64(0));
        QCOMPARE(snapshot.cards.at(i).secondaryText, expectedSecondary.at(i));
    }

    // 分布：10 张全空；图型仅等级序维度（威胁等级/告警级别）为柱状
    QCOMPARE(snapshot.distributions.size(), 10);
    for (int i = 0; i < snapshot.distributions.size(); ++i) {
        QVERIFY(snapshot.distributions.at(i).buckets.isEmpty());
        QCOMPARE(snapshot.distributions.at(i).useBar, i == 2 || i == 7);
    }

    // 趋势：6 条 × 30 点全零，窗口与升序结构不变
    QCOMPARE(snapshot.trends.size(), 6);
    for (const StatisticsTrendSeries &series : snapshot.trends) {
        QCOMPARE(series.points.size(), 30);
        for (const QPair<QDate, qint64> &point : series.points)
            QCOMPARE(point.second, qint64(0));
        QCOMPARE(series.points.first().first, kWindowStart);
        QCOMPARE(series.points.last().first, QDate(2026, 10, 13));
    }

    // 告警处理统计全零
    QCOMPARE(snapshot.alarmSummary.handled, qint64(0));
    QCOMPARE(snapshot.alarmSummary.unhandled, qint64(0));
    QCOMPARE(snapshot.alarmSummary.hasAvgHandle, false);
    QCOMPARE(snapshot.alarmSummary.avgHandleSeconds, 0.0);

    // 元数据由 worker 补全：聚合层不填 fetchedAtUtc 与词表副本
    QVERIFY(!snapshot.fetchedAtUtc.isValid());
    QVERIFY(snapshot.vocabularies.isEmpty());
}

QTEST_GUILESS_MAIN(StatisticsAggregationTest)
#include "statistics_aggregation_test.moc"
