// REQ-012 W5 五实体 CRUD 适配集成测试（TDD RED 阶段：DataPlatformApi 为桩实现，预期全红）。
// 对本地数据平台实例（默认 http://127.0.0.1:8080，UXO_API_HOST/UXO_API_PORT 覆盖）断言
// 契约 v1.0.3 §2/§3/§6/§7 适配面行为：
//   - 健康检查（§3）：200 + status=ok + database=connected；
//   - 五实体 CRUD（§6.1–§6.5）：POST→201 回显、GET 字段保真、PUT→200 全量更新回显与持久化
//     （update_time 服务端刷新，不断言）；
//   - 列表（§7）：meta 三字段、空页仍 200、越界分页 400 pagination_invalid（field 定位）、
//     起步过滤集 AND 精确匹配（命中在列 / 单参非命中缺席）；
//   - 错误路径：重复主键 409 duplicate_id、外键违规 400 foreign_key_violation、
//     资源不存在 404 not_found；错误断言只看 httpStatus + code + field，不解析 message；
//   - 本地开发库行跨运行持久累积：每次生成全新 UUID，只断言 >= 或成员资格，不断言精确总数。

#include "Integration/DataPlatformApi.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QUuid>
#include <QVector3D>
#include <QtTest>

using namespace Integration;

namespace {

// 每次生成全新 36 位小写 UUID（主键客户端生成；成员资格断言避免跨运行行累积干扰）
static QString newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// 当前 UTC 时间（毫秒精度，W3 toJson/fromJson 往返无损）
static QDateTime utcNow()
{
    return QDateTime::currentDateTimeUtc();
}

// 错误项集合中是否存在指定 code 的条目（不解析 message；code 类型由推导适配）
template <typename Code>
static bool hasErrorCode(const ApiError &error, Code code)
{
    for (const ApiErrorItem &item : error.items) {
        if (item.code == code)
            return true;
    }
    return false;
}

// 错误项集合中是否存在指定 code + field 的条目（契约 §8：field+code 为定位依据）
template <typename Code>
static bool hasErrorItem(const ApiError &error, Code code, const QString &field)
{
    for (const ApiErrorItem &item : error.items) {
        if (item.code == code && item.field == field)
            return true;
    }
    return false;
}

// 列表项成员资格（行持久累积 → 只断言成员资格，不断言精确总数）
template <typename T, typename IdOf>
static bool containsId(const QVector<T> &items, const QString &id, IdOf idOf)
{
    for (const T &item : items) {
        if (idOf(item) == id)
            return true;
    }
    return false;
}

// targets 起步正典值：type=1（航弹）、威胁 3、状态 1（已探测）、契约示例同源坐标/置信度/备注
static Core::TargetInfo makeTarget()
{
    Core::TargetInfo target;
    target.id = newId();
    target.type = static_cast<decltype(target.type)>(1);
    target.position = QVector3D(116.397128f, 39.916527f, 43.5f);
    target.depth = 0.0;
    target.attitude = QVector3D(0.0f, 0.0f, 0.0f);
    target.confidence = 0.87;
    target.threatLevel = static_cast<decltype(target.threatLevel)>(3);
    target.status = static_cast<decltype(target.status)>(1);
    target.imagePath = QStringLiteral("/assets/targets/") + target.id + QStringLiteral(".jpg");
    target.detectTime = utcNow();
    target.updateTime = utcNow();
    target.remark = QStringLiteral("跑道东侧疑似航弹");
    return target;
}

// devices 起步正典值：type=1、状态 1、电量 87.5（W3 钉点：toJson 四舍五入为 88）、可空字段留空
static Core::DeviceInfo makeDevice()
{
    Core::DeviceInfo device;
    device.id = newId();
    device.type = static_cast<decltype(device.type)>(1);
    device.name = QStringLiteral("探测车-01");
    device.status = static_cast<decltype(device.status)>(1);
    device.batteryLevel = 87.5;
    device.position = QVector3D(116.397f, 39.916f, 41.2f);
    device.createTime = utcNow();
    return device;
}

// missions 起步正典值：type=0、优先级 0、状态 0，外键指向本次运行新建实体
static Core::MissionInfo makeMission(const QString &targetId, const QString &deviceId)
{
    Core::MissionInfo mission;
    mission.id = newId();
    mission.type = static_cast<decltype(mission.type)>(0);
    mission.targetId = targetId;
    mission.priority = 0;
    mission.status = static_cast<decltype(mission.status)>(0);
    mission.deviceId = deviceId;
    mission.createTime = utcNow();
    mission.updateTime = utcNow();
    return mission;
}

// alarms 起步正典值：电量低告警、级别 2、未处理
static Core::AlarmInfo makeAlarm(const QString &deviceId)
{
    Core::AlarmInfo alarm;
    alarm.id = newId();
    alarm.type = QStringLiteral("BATTERY_LOW");
    alarm.level = static_cast<decltype(alarm.level)>(2);
    alarm.deviceId = deviceId;
    alarm.isHandled = false;
    alarm.createTime = utcNow();
    return alarm;
}

// detection-records 起步正典值：异常航弹检出、待复核 0、raw_result 结构保真载荷
static DetectionRecordData makeDetectionRecord()
{
    DetectionRecordData record;
    record.id = newId();
    record.analyzedAt = utcNow();
    record.hasAnomaly = true;
    record.className = QStringLiteral("aircraft-bombs");
    record.confidence = 0.92;
    // 原始检测载荷：单框 + 分数表（后续按结构保真断言，非字节级比对）
    QJsonObject box;
    box[QStringLiteral("x")] = 120;
    box[QStringLiteral("y")] = 84;
    box[QStringLiteral("w")] = 57;
    box[QStringLiteral("h")] = 43;
    QJsonArray boxes;
    boxes.append(box);
    QJsonObject scores;
    scores[QStringLiteral("aircraft-bombs")] = 0.92;
    scores[QStringLiteral("background")] = 0.05;
    QJsonObject raw;
    raw[QStringLiteral("boxes")] = boxes;
    raw[QStringLiteral("scores")] = scores;
    record.rawResult = raw;
    record.review = 0;
    return record;
}

} // namespace

class IntegrationDataPlatformApiTest : public QObject
{
    Q_OBJECT

private slots:
    // 实例缺席探测：健康端点传输层失败（连不上/超时）按 S4 环境跳过整套件；其余失败继续执行以暴露真实断言
    void initTestCase()
    {
        HealthStatus hs;
        ApiError probeError;
        if (!api.health(&hs, &probeError)) {
            for (const ApiErrorItem &item : probeError.items) {
                if (item.code == ApiErrorCode::NetworkError || item.code == ApiErrorCode::Timeout)
                    QSKIP("本地数据平台实例未运行（S4 环境跳过）");
            }
        }
    }

    // (1) 健康检查：200 + status=ok + database=connected
    void healthOk()
    {
        HealthStatus hs;
        ApiError err;
        QVERIFY2(api.health(&hs, &err), "health 应成功");
        QCOMPARE(hs.httpStatus, 200);
        QCOMPARE(hs.status, QStringLiteral("ok"));
        QCOMPARE(hs.database, QStringLiteral("connected"));
    }

    // (2) targets CRUD：创建回显 → 单查字段保真 → 全量更新持久化（update_time 服务端刷新，不断言）
    void targetCrudRoundtrip()
    {
        const Core::TargetInfo target = makeTarget();

        ApiError err;
        Core::TargetInfo echo;
        QVERIFY2(api.createTarget(target, &echo, &err), "createTarget 应成功");
        QCOMPARE(echo.id, target.id);
        QCOMPARE(echo.remark, target.remark);

        Core::TargetInfo fetched;
        QVERIFY2(api.getTarget(target.id, &fetched, &err), "getTarget 应成功");
        QCOMPARE(fetched.id, target.id);
        QCOMPARE(static_cast<int>(fetched.type), 1);
        QVERIFY(qFuzzyCompare(fetched.position.x(), 116.397128f));
        QVERIFY(qFuzzyCompare(fetched.position.y(), 39.916527f));
        QVERIFY(qFuzzyCompare(fetched.position.z(), 43.5f));
        QVERIFY(qFuzzyCompare(fetched.confidence, 0.87));
        QCOMPARE(static_cast<int>(fetched.threatLevel), 3);
        QCOMPARE(static_cast<int>(fetched.status), 1);
        QCOMPARE(fetched.imagePath, target.imagePath);
        QCOMPARE(fetched.detectTime.toUTC(), target.detectTime.toUTC());
        QCOMPARE(fetched.remark, QStringLiteral("跑道东侧疑似航弹"));

        // 更新：备注/状态/置信度；回显与再次单查均应持久化
        Core::TargetInfo updated = fetched;
        updated.remark = QStringLiteral("更新备注");
        updated.status = static_cast<decltype(updated.status)>(2);
        updated.confidence = 0.5;
        Core::TargetInfo updateEcho;
        QVERIFY2(api.updateTarget(updated, &updateEcho, &err), "updateTarget 应成功");
        QCOMPARE(updateEcho.remark, QStringLiteral("更新备注"));

        Core::TargetInfo refetched;
        QVERIFY2(api.getTarget(target.id, &refetched, &err), "更新后 getTarget 应成功");
        QCOMPARE(refetched.remark, QStringLiteral("更新备注"));
        QCOMPARE(static_cast<int>(refetched.status), 2);
        QVERIFY(qFuzzyCompare(refetched.confidence, 0.5));
    }

    // (3) devices CRUD：电量 87.5→88 回显（W3 钉点）、可空字段空串回显、改名更新持久化
    void deviceCrudRoundtrip()
    {
        const Core::DeviceInfo device = makeDevice();

        ApiError err;
        Core::DeviceInfo echo;
        QVERIFY2(api.createDevice(device, &echo, &err), "createDevice 应成功");
        QCOMPARE(echo.id, device.id);
        QCOMPARE(echo.batteryLevel, 88.0);
        QVERIFY(echo.manufacturer.isEmpty());
        QVERIFY(echo.model.isEmpty());
        QVERIFY(echo.serialNumber.isEmpty());

        Core::DeviceInfo fetched;
        QVERIFY2(api.getDevice(device.id, &fetched, &err), "getDevice 应成功");
        QCOMPARE(fetched.id, device.id);
        QCOMPARE(static_cast<int>(fetched.type), 1);
        QCOMPARE(fetched.name, QStringLiteral("探测车-01"));
        QCOMPARE(static_cast<int>(fetched.status), 1);
        QCOMPARE(fetched.batteryLevel, 88.0);

        Core::DeviceInfo updated = fetched;
        updated.name = QStringLiteral("探测车-02");
        Core::DeviceInfo updateEcho;
        QVERIFY2(api.updateDevice(updated, &updateEcho, &err), "updateDevice 应成功");
        QCOMPARE(updateEcho.name, QStringLiteral("探测车-02"));

        Core::DeviceInfo refetched;
        QVERIFY2(api.getDevice(device.id, &refetched, &err), "更新后 getDevice 应成功");
        QCOMPARE(refetched.name, QStringLiteral("探测车-02"));
    }

    // (4) missions CRUD：先建依赖 target/device，创建回显 priority=0，状态更新为 3 持久化
    void missionCrudRoundtrip()
    {
        ApiError err;
        Core::TargetInfo depTargetEcho;
        QVERIFY2(api.createTarget(makeTarget(), &depTargetEcho, &err), "依赖 target 创建应成功");
        Core::DeviceInfo depDeviceEcho;
        QVERIFY2(api.createDevice(makeDevice(), &depDeviceEcho, &err), "依赖 device 创建应成功");

        const Core::MissionInfo mission = makeMission(depTargetEcho.id, depDeviceEcho.id);
        Core::MissionInfo echo;
        QVERIFY2(api.createMission(mission, &echo, &err), "createMission 应成功");
        QCOMPARE(echo.id, mission.id);
        QCOMPARE(echo.priority, 0);

        Core::MissionInfo fetched;
        QVERIFY2(api.getMission(mission.id, &fetched, &err), "getMission 应成功");
        QCOMPARE(fetched.targetId, depTargetEcho.id);
        QCOMPARE(fetched.deviceId, depDeviceEcho.id);
        QCOMPARE(static_cast<int>(fetched.status), 0);

        Core::MissionInfo updated = fetched;
        updated.status = static_cast<decltype(updated.status)>(3);
        Core::MissionInfo updateEcho;
        QVERIFY2(api.updateMission(updated, &updateEcho, &err), "updateMission 应成功");
        QCOMPARE(static_cast<int>(updateEcho.status), 3);

        Core::MissionInfo refetched;
        QVERIFY2(api.getMission(mission.id, &refetched, &err), "更新后 getMission 应成功");
        QCOMPARE(static_cast<int>(refetched.status), 3);
    }

    // (5) alarms CRUD：未处理创建回显，处理后 isHandled/handleResult/handleTime 持久化
    void alarmCrudRoundtrip()
    {
        ApiError err;
        Core::DeviceInfo depDeviceEcho;
        QVERIFY2(api.createDevice(makeDevice(), &depDeviceEcho, &err), "依赖 device 创建应成功");

        const Core::AlarmInfo alarm = makeAlarm(depDeviceEcho.id);
        Core::AlarmInfo echo;
        QVERIFY2(api.createAlarm(alarm, &echo, &err), "createAlarm 应成功");
        QCOMPARE(echo.id, alarm.id);
        QCOMPARE(echo.isHandled, false);

        Core::AlarmInfo fetched;
        QVERIFY2(api.getAlarm(alarm.id, &fetched, &err), "getAlarm 应成功");
        QCOMPARE(fetched.type, QStringLiteral("BATTERY_LOW"));
        QCOMPARE(static_cast<int>(fetched.level), 2);
        QCOMPARE(fetched.deviceId, depDeviceEcho.id);
        QCOMPARE(fetched.isHandled, false);

        Core::AlarmInfo updated = fetched;
        updated.isHandled = true;
        updated.handleResult = QStringLiteral("已更换电池");
        updated.handleTime = utcNow();
        Core::AlarmInfo updateEcho;
        QVERIFY2(api.updateAlarm(updated, &updateEcho, &err), "updateAlarm 应成功");
        QCOMPARE(updateEcho.isHandled, true);
        QCOMPARE(updateEcho.handleResult, QStringLiteral("已更换电池"));

        Core::AlarmInfo refetched;
        QVERIFY2(api.getAlarm(alarm.id, &refetched, &err), "更新后 getAlarm 应成功");
        QCOMPARE(refetched.isHandled, true);
        QCOMPARE(refetched.handleResult, QStringLiteral("已更换电池"));
        QCOMPARE(refetched.handleTime.toUTC(), updated.handleTime.toUTC());
    }

    // (6) detection-records CRUD：raw_result 结构保真（非字节级），复核状态更新持久化
    void detectionRecordCrudRoundtrip()
    {
        const DetectionRecordData record = makeDetectionRecord();

        ApiError err;
        DetectionRecordData echo;
        QVERIFY2(api.createDetectionRecord(record, &echo, &err), "createDetectionRecord 应成功");
        QCOMPARE(echo.id, record.id);
        QCOMPARE(echo.review, 0);
        QCOMPARE(echo.className, QStringLiteral("aircraft-bombs"));
        // raw_result 结构保真：boxes 数组 1 项、框字段与分数表键值在位
        QVERIFY(echo.rawResult.isObject());
        const QJsonObject rawEcho = echo.rawResult.toObject();
        QVERIFY(rawEcho.contains(QStringLiteral("boxes")));
        QCOMPARE(rawEcho.value(QStringLiteral("boxes")).toArray().size(), 1);
        const QJsonObject boxEcho = rawEcho.value(QStringLiteral("boxes")).toArray().at(0).toObject();
        QCOMPARE(boxEcho.value(QStringLiteral("x")).toInt(), 120);
        QCOMPARE(boxEcho.value(QStringLiteral("y")).toInt(), 84);
        QCOMPARE(boxEcho.value(QStringLiteral("w")).toInt(), 57);
        QCOMPARE(boxEcho.value(QStringLiteral("h")).toInt(), 43);
        QVERIFY(rawEcho.contains(QStringLiteral("scores")));
        QCOMPARE(rawEcho.value(QStringLiteral("scores")).toObject()
                      .value(QStringLiteral("aircraft-bombs")).toDouble(), 0.92);

        DetectionRecordData fetched;
        QVERIFY2(api.getDetectionRecord(record.id, &fetched, &err), "getDetectionRecord 应成功");
        QCOMPARE(fetched.hasAnomaly, true);
        QVERIFY(qFuzzyCompare(fetched.confidence, 0.92));
        QCOMPARE(fetched.review, 0);

        DetectionRecordData updated = fetched;
        updated.review = 2;
        updated.reviewedAt = utcNow();
        DetectionRecordData updateEcho;
        QVERIFY2(api.updateDetectionRecord(updated, &updateEcho, &err), "updateDetectionRecord 应成功");
        QCOMPARE(updateEcho.review, 2);

        DetectionRecordData refetched;
        QVERIFY2(api.getDetectionRecord(record.id, &refetched, &err), "更新后 getDetectionRecord 应成功");
        QCOMPARE(refetched.review, 2);
        QCOMPARE(refetched.reviewedAt.toUTC(), updated.reviewedAt.toUTC());
    }

    // (7) 列表分页：第 1 页 meta 原样带回；远超总页数仍 200 空页且 total 保持（§7）
    void listPaginationAndEmptyPage()
    {
        ApiError err;
        Core::TargetInfo echo;
        QVERIFY2(api.createTarget(makeTarget(), &echo, &err), "分页样本 target 创建应成功");

        // page=1/page_size=1：本页恰 1 项；total 为过滤后跨页总数（行累积 → 只断言下限）
        PagedResult<Core::TargetInfo> first;
        QVERIFY2(api.listTargets(TargetFilter{}, PageQuery{1, 1}, &first, &err), "listTargets 第 1 页应成功");
        QCOMPARE(first.items.size(), 1);
        QCOMPARE(first.page, 1);
        QCOMPARE(first.pageSize, 1);
        QVERIFY(first.total >= 1);

        // 页码远超总页数：空 items 且 200，meta 的 total 不失真
        PagedResult<Core::TargetInfo> far;
        QVERIFY2(api.listTargets(TargetFilter{}, PageQuery{999999, 20}, &far, &err), "listTargets 空页应成功");
        QVERIFY(far.items.isEmpty());
        QVERIFY(far.total >= 1);
    }

    // (8) 分页越界 400：page=0 与 page_size=101 均以 pagination_invalid + field 定位
    void paginationInvalid400()
    {
        PagedResult<Core::TargetInfo> out;

        // page 越下界（§7 page 自 1 起）
        ApiError pageErr;
        QVERIFY2(!api.listTargets(TargetFilter{}, PageQuery{0, 20}, &out, &pageErr), "page=0 应失败");
        QCOMPARE(pageErr.httpStatus, 400);
        QVERIFY2(hasErrorItem(pageErr, ApiErrorCode::PaginationInvalid, QStringLiteral("page")),
                 "400 载荷应含 code=pagination_invalid 且 field=page");

        // page_size 越上界（§7 page_size 1–100）
        ApiError sizeErr;
        QVERIFY2(!api.listTargets(TargetFilter{}, PageQuery{1, 101}, &out, &sizeErr), "page_size=101 应失败");
        QCOMPARE(sizeErr.httpStatus, 400);
        QVERIFY2(hasErrorItem(sizeErr, ApiErrorCode::PaginationInvalid, QStringLiteral("page_size")),
                 "400 载荷应含 code=pagination_invalid 且 field=page_size");
    }

    // (9) 起步过滤集：五实体全参命中 → 本行在列；单参非命中 → 本行缺席（AND 精确匹配）
    void filtersAllParams()
    {
        ApiError err;

        // 起步正典值各建一行（依赖行同样全新 UUID）
        Core::TargetInfo targetEcho;
        QVERIFY2(api.createTarget(makeTarget(), &targetEcho, &err), "过滤样本 target 创建应成功");
        Core::DeviceInfo deviceEcho;
        QVERIFY2(api.createDevice(makeDevice(), &deviceEcho, &err), "过滤样本 device 创建应成功");
        Core::MissionInfo missionEcho;
        QVERIFY2(api.createMission(makeMission(targetEcho.id, deviceEcho.id), &missionEcho, &err),
                 "过滤样本 mission 创建应成功");
        Core::AlarmInfo alarmEcho;
        QVERIFY2(api.createAlarm(makeAlarm(deviceEcho.id), &alarmEcho, &err), "过滤样本 alarm 创建应成功");
        DetectionRecordData recordEcho;
        QVERIFY2(api.createDetectionRecord(makeDetectionRecord(), &recordEcho, &err),
                 "过滤样本 detection-record 创建应成功");

        const PageQuery wide{1, 100};

        // targets：type+status+threat_level 全命中 → 成员在列；status=2 单参非命中 → 缺席
        {
            TargetFilter filter;
            filter.type = 1;
            filter.status = 1;
            filter.threatLevel = 3;
            PagedResult<Core::TargetInfo> result;
            QVERIFY2(api.listTargets(filter, wide, &result, &err), "targets 全参过滤列表应成功");
            QVERIFY(result.total >= 1);
            QVERIFY2(containsId(result.items, targetEcho.id,
                                [](const Core::TargetInfo &t) { return t.id; }),
                     "全参命中过滤应包含本次创建的 target");

            TargetFilter miss;
            miss.status = 2; // 本次样本 status=1
            PagedResult<Core::TargetInfo> missResult;
            QVERIFY2(api.listTargets(miss, wide, &missResult, &err), "targets 单参过滤列表应成功");
            QVERIFY2(!containsId(missResult.items, targetEcho.id,
                                 [](const Core::TargetInfo &t) { return t.id; }),
                     "status=2 过滤后本次创建的 target（status=1）不应在列");
        }

        // devices：type+status 全命中 → 在列；type=2 单参非命中 → 缺席
        {
            DeviceFilter filter;
            filter.type = 1;
            filter.status = 1;
            PagedResult<Core::DeviceInfo> result;
            QVERIFY2(api.listDevices(filter, wide, &result, &err), "devices 全参过滤列表应成功");
            QVERIFY2(containsId(result.items, deviceEcho.id,
                                [](const Core::DeviceInfo &d) { return d.id; }),
                     "全参命中过滤应包含本次创建的 device");

            DeviceFilter miss;
            miss.type = 2; // 本次样本 type=1
            PagedResult<Core::DeviceInfo> missResult;
            QVERIFY2(api.listDevices(miss, wide, &missResult, &err), "devices 单参过滤列表应成功");
            QVERIFY2(!containsId(missResult.items, deviceEcho.id,
                                 [](const Core::DeviceInfo &d) { return d.id; }),
                     "type=2 过滤后本次创建的 device（type=1）不应在列");
        }

        // missions：type+status+target_id+device_id 全命中 → 在列；status=1 单参非命中 → 缺席
        {
            MissionFilter filter;
            filter.type = 0;
            filter.status = 0;
            filter.targetId = targetEcho.id;
            filter.deviceId = deviceEcho.id;
            PagedResult<Core::MissionInfo> result;
            QVERIFY2(api.listMissions(filter, wide, &result, &err), "missions 全参过滤列表应成功");
            QVERIFY2(containsId(result.items, missionEcho.id,
                                [](const Core::MissionInfo &m) { return m.id; }),
                     "全参命中过滤应包含本次创建的 mission");

            MissionFilter miss;
            miss.status = 1; // 本次样本 status=0
            PagedResult<Core::MissionInfo> missResult;
            QVERIFY2(api.listMissions(miss, wide, &missResult, &err), "missions 单参过滤列表应成功");
            QVERIFY2(!containsId(missResult.items, missionEcho.id,
                                 [](const Core::MissionInfo &m) { return m.id; }),
                     "status=1 过滤后本次创建的 mission（status=0）不应在列");
        }

        // alarms：level+is_handled=0（false）全命中 → 在列；is_handled=1 单参非命中 → 缺席
        {
            AlarmFilter filter;
            filter.level = 2;
            filter.isHandled = 0;
            PagedResult<Core::AlarmInfo> result;
            QVERIFY2(api.listAlarms(filter, wide, &result, &err), "alarms 全参过滤列表应成功");
            QVERIFY2(containsId(result.items, alarmEcho.id,
                                [](const Core::AlarmInfo &a) { return a.id; }),
                     "全参命中过滤应包含本次创建的 alarm");

            AlarmFilter miss;
            miss.isHandled = 1; // 本次样本未处理
            PagedResult<Core::AlarmInfo> missResult;
            QVERIFY2(api.listAlarms(miss, wide, &missResult, &err), "alarms 单参过滤列表应成功");
            QVERIFY2(!containsId(missResult.items, alarmEcho.id,
                                 [](const Core::AlarmInfo &a) { return a.id; }),
                     "is_handled=1 过滤后本次创建的 alarm（未处理）不应在列");
        }

        // detection-records：review+class_name+has_anomaly=1 命中 → 在列；targetId 空则跳过该参数；
        // review=1 单参非命中 → 缺席
        {
            DetectionRecordFilter filter;
            filter.review = 0;
            filter.className = QStringLiteral("aircraft-bombs");
            filter.hasAnomaly = 1; // 本次样本 targetId 为空，过滤参数跳过 target_id
            PagedResult<DetectionRecordData> result;
            QVERIFY2(api.listDetectionRecords(filter, wide, &result, &err),
                     "detection-records 全参过滤列表应成功");
            QVERIFY2(containsId(result.items, recordEcho.id,
                                [](const DetectionRecordData &r) { return r.id; }),
                     "全参命中过滤应包含本次创建的 detection-record");

            DetectionRecordFilter miss;
            miss.review = 1; // 本次样本 review=0
            PagedResult<DetectionRecordData> missResult;
            QVERIFY2(api.listDetectionRecords(miss, wide, &missResult, &err),
                     "detection-records 单参过滤列表应成功");
            QVERIFY2(!containsId(missResult.items, recordEcho.id,
                                 [](const DetectionRecordData &r) { return r.id; }),
                     "review=1 过滤后本次创建的 detection-record（review=0）不应在列");
        }
    }

    // (10) 重复主键 409：同一实体二次 POST → duplicate_id
    void duplicateId409()
    {
        ApiError err;
        Core::TargetInfo echo;
        QVERIFY2(api.createTarget(makeTarget(), &echo, &err), "首次 createTarget 应成功");

        Core::TargetInfo dup = echo; // 同 id 完整资源表示再次 POST
        Core::TargetInfo dupEcho;
        QVERIFY2(!api.createTarget(dup, &dupEcho, &err), "重复主键 createTarget 应失败");
        QCOMPARE(err.httpStatus, 409);
        QVERIFY2(hasErrorCode(err, ApiErrorCode::DuplicateId), "409 载荷应含 code=duplicate_id");
    }

    // (11) 外键违规 400：mission.target_id 指向不存在 UUID，其余字段合法
    void foreignKeyViolation400()
    {
        const Core::MissionInfo mission = makeMission(newId(), QString());
        ApiError err;
        Core::MissionInfo echo;
        QVERIFY2(!api.createMission(mission, &echo, &err), "外键违规 createMission 应失败");
        QCOMPARE(err.httpStatus, 400);
        QVERIFY2(hasErrorCode(err, ApiErrorCode::ForeignKeyViolation),
                 "400 载荷应含 code=foreign_key_violation");
    }

    // (12) 资源不存在 404：GET 随机 UUID → not_found；PUT 合法实体但主键未落库 → 404
    void notFound404()
    {
        ApiError err;
        Core::TargetInfo out;
        QVERIFY2(!api.getTarget(newId(), &out, &err), "getTarget 不存在 id 应失败");
        QCOMPARE(err.httpStatus, 404);
        QVERIFY2(hasErrorCode(err, ApiErrorCode::NotFound), "404 载荷应含 code=not_found");

        Core::TargetInfo ghost = makeTarget(); // 合法实体、随机全新主键
        Core::TargetInfo echo;
        QVERIFY2(!api.updateTarget(ghost, &echo, &err), "updateTarget 不存在 id 应失败");
        QCOMPARE(err.httpStatus, 404);
    }

private:
    // 全部网络访问统一经此成员（同步适配面，契约 §1.8 单席位低频）
    DataPlatformApi api;
};

QTEST_GUILESS_MAIN(IntegrationDataPlatformApiTest)

#include "integration_data_platform_api_test.moc"
