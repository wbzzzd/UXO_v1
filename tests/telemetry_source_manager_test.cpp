// 遥测数据源管理器单测（REQ-012 M4E WP3；WP4 告警用例改断言结构化 AlertEvent 三字段）
// 覆盖：默认态、模拟遥测转发、模式切换语义（保持运行状态）、真实链路连接失败路径、
// OSD/事件解析契约（docs/interfaces/接口规格-MSDK对接.md）、真实链路停滞状态机。
// 用 QTEST_GUILESS_MAIN 而非 QTEST_MAIN：Core 目标经普通 target_link_libraries 传递
// Qt5::Gui（QT_GUI_LIB 被定义），QTEST_MAIN 会选中 QGuiApplication 而要求 QPA 平台
// 插件，无头环境不可用；本测试纯 QCoreApplication。

#include <QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>

#include "Core/Simulation/DemoScenarioProvider.h"
#include "Core/Simulation/DroneTelemetrySimulator.h"
#include "Core/Uplink/ITelemetrySource.h"
#include "Core/Uplink/MqttTelemetrySource.h"
#include "Core/Uplink/TelemetrySourceManager.h"
#include "Core/Uplink/TelemetryTypes.h"

namespace {

// 两航点测试航线：模拟遥测沿航段插值，断言取航段区间界（与 tick 时机无关）
QVector<Core::Simulation::DroneWaypoint> makeRoute()
{
    return {
        {39.123456789, 116.123456789, 100.0, 45.0},
        {39.223456789, 116.223456789, 200.0, 135.0},
    };
}

// 构造契约 §2 OSD 信封（data 内含经纬度/高度/航向）；altField 切换 height/elevation
QString osdPayload(double lat, double lng, double alt, double heading,
                   const QString &altField = QStringLiteral("height"))
{
    QJsonObject data;
    data.insert(QStringLiteral("latitude"), lat);
    data.insert(QStringLiteral("longitude"), lng);
    data.insert(altField, alt);
    data.insert(QStringLiteral("attitude_head"), heading);
    QJsonObject envelope;
    envelope.insert(QStringLiteral("data"), data);
    return QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
}

// 构造契约 §4 events 信封（method + data）
QString eventsPayload(const QString &method, const QJsonObject &data)
{
    QJsonObject envelope;
    envelope.insert(QStringLiteral("method"), method);
    envelope.insert(QStringLiteral("data"), data);
    return QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
}

// 经元对象系统调用 MqttTelemetrySource 私有槽（等价于 MQTT 消息已到达的分发路径）
void invokeMessageArrived(Core::Uplink::MqttTelemetrySource *source,
                          const QString &topic, const QString &payload)
{
    QVERIFY2(QMetaObject::invokeMethod(source, "onClientMessageArrived",
                                       Q_ARG(QString, topic),
                                       Q_ARG(QString, payload)),
             "onClientMessageArrived 调用失败");
}

} // namespace

class TelemetrySourceManagerTest : public QObject
{
    Q_OBJECT

private slots:
    // 默认态：模拟模式、断链、未运行、模拟源可访问
    void managerDefaults();

    // 模拟模式遥测经管理器转发，值在航线区间内
    void simulationTelemetryForwarding();

    // 模式切换基础语义：modeChanged 恰发一次、同模式幂等、未运行切换不动链路状态
    void modeSwitchBasics();

    // 切换保持运行状态：运行中切真实链路→真实源自动启动；切回模拟→自动重启并恢复遥测
    void modeSwitchPreservesRunning();

    // 真实链路连接失败路径：Connecting→Disconnected 两拍；stop 去重；重试再走一轮
    void realLinkConnectFailure();

    // OSD/events 解析契约：无效坐标、elevation 回退、HMS 结构化级别直传（WP4）、
    // 通用事件、非契约主题
    void osdAndEventsParsing();

    // 真实链路停滞状态机：Connected→超时 Stalled→遥测恢复 Connected；模拟模式超时无操作
    void stallStateMachine();
};

void TelemetrySourceManagerTest::managerDefaults()
{
    Core::Uplink::TelemetrySourceManager manager;
    QCOMPARE(manager.mode(), Core::Uplink::SourceMode::Simulation);
    QCOMPARE(manager.linkState(), Core::Uplink::LinkState::Disconnected);
    QVERIFY(!manager.isRunning());
    QVERIFY(manager.simulationSource() != nullptr);
}

void TelemetrySourceManagerTest::simulationTelemetryForwarding()
{
    Core::Uplink::TelemetrySourceManager manager;
    manager.simulationSource()->loadRoute(makeRoute(), 200.0);

    QSignalSpy telemetrySpy(&manager, &Core::Uplink::TelemetrySourceManager::telemetryUpdated);
    manager.start();
    QVERIFY(manager.isRunning());
    QTRY_VERIFY(telemetrySpy.count() >= 1);

    const Core::Uplink::TelemetryData data =
        telemetrySpy.first().at(0).value<Core::Uplink::TelemetryData>();
    // 线性插值任意时刻的值必在两航点连线区间内
    QVERIFY(data.latitude >= 39.123456789 && data.latitude <= 39.223456789);
    QVERIFY(data.longitude >= 116.123456789 && data.longitude <= 116.223456789);
    QVERIFY(data.altitude >= 100.0 && data.altitude <= 200.0);
    QVERIFY(data.heading >= 45.0 && data.heading <= 135.0);

    manager.stop();
    QVERIFY(!manager.isRunning());
}

void TelemetrySourceManagerTest::modeSwitchBasics()
{
    Core::Uplink::TelemetrySourceManager manager;
    QSignalSpy modeSpy(&manager, &Core::Uplink::TelemetrySourceManager::modeChanged);
    QSignalSpy linkSpy(&manager, &Core::Uplink::TelemetrySourceManager::linkStateChanged);

    manager.setMode(Core::Uplink::SourceMode::RealLink);
    QCOMPARE(manager.mode(), Core::Uplink::SourceMode::RealLink);
    QCOMPARE(modeSpy.count(), 1);
    // 新契约（死锁修复）：进入真实链路即自动启动连接，运行标志恒置位（与本机 1883 有无网关无关）
    QVERIFY(manager.isRunning());

    manager.setMode(Core::Uplink::SourceMode::RealLink);  // 同模式幂等
    QCOMPARE(modeSpy.count(), 1);

    manager.setMode(Core::Uplink::SourceMode::Simulation);
    QCOMPARE(manager.mode(), Core::Uplink::SourceMode::Simulation);
    QCOMPARE(modeSpy.count(), 2);

    // 新契约：真实链路自动启动时同步发出 Connecting→Disconnected 两拍
    // （MqttUplinkClient::connectToBroker 同步连接、失败即回 Disconnected）；
    // 切回模拟时链路状态已是 Disconnected，不再发拍。
    // 注意：本断言要求本机 1883 无 MQTT 网关在线——网关在线时 Connecting 会保持到
    // 切回模拟才补一拍 Disconnected（count=3），Connected 随后经队列回调到达
    QCOMPARE(linkSpy.count(), 2);
    QCOMPARE(manager.linkState(), Core::Uplink::LinkState::Disconnected);
}

void TelemetrySourceManagerTest::modeSwitchPreservesRunning()
{
    Core::Uplink::TelemetrySourceManager manager;
    manager.simulationSource()->loadRoute(makeRoute(), 200.0);
    QSignalSpy telemetrySpy(&manager, &Core::Uplink::TelemetrySourceManager::telemetryUpdated);

    manager.start();
    QVERIFY(manager.isRunning());

    // 运行中切真实链路：真实源自动启动（本机 1883 有无网关均不影响运行标志）
    manager.setMode(Core::Uplink::SourceMode::RealLink);
    QVERIFY(manager.isRunning());

    // 切回模拟：模拟源自动重启并恢复遥测输出
    manager.setMode(Core::Uplink::SourceMode::Simulation);
    QVERIFY(manager.isRunning());
    QTRY_VERIFY(telemetrySpy.count() >= 1);

    manager.stop();
    QVERIFY(!manager.isRunning());
}

void TelemetrySourceManagerTest::realLinkConnectFailure()
{
    // 独立源直连 127.0.0.1:1（本机必然拒绝），不经管理器验证源内状态机
    Core::Uplink::MqttTelemetrySource source(QStringLiteral("127.0.0.1"), 1, nullptr);
    QSignalSpy linkSpy(&source, &Core::Uplink::ITelemetrySource::linkStateChanged);

    source.start();
    // 同步失败路径：Connecting → Disconnected 恰两拍
    QCOMPARE(linkSpy.count(), 2);
    QCOMPARE(linkSpy.at(0).at(0).value<Core::Uplink::LinkState>(),
             Core::Uplink::LinkState::Connecting);
    QCOMPARE(linkSpy.at(1).at(0).value<Core::Uplink::LinkState>(),
             Core::Uplink::LinkState::Disconnected);
    QVERIFY(source.isRunning());  // 启动标志保持（重试语义），连接失败不清除

    source.stop();  // 已 Disconnected，去重不重发
    QCOMPARE(linkSpy.count(), 2);
    QVERIFY(!source.isRunning());

    source.start();  // 重试再走一轮 Connecting → Disconnected
    QCOMPARE(linkSpy.count(), 4);
    source.stop();
}

void TelemetrySourceManagerTest::osdAndEventsParsing()
{
    // 独立源（不 start，仅驱动解析路径）：端口 1 保证无真实连接
    Core::Uplink::MqttTelemetrySource source(QStringLiteral("127.0.0.1"), 1, nullptr);
    QSignalSpy telemetrySpy(&source, &Core::Uplink::ITelemetrySource::telemetryUpdated);
    QSignalSpy alertSpy(&source, &Core::Uplink::ITelemetrySource::alertReceived);
    const QString osdTopic = QStringLiteral("thing/product/7KABC2D00A00BC/osd");
    const QString eventsTopic = QStringLiteral("thing/product/7KABC2D00A00BC/events");

    // 契约 §2 全字段：值原样透传
    invokeMessageArrived(&source, osdTopic, osdPayload(39.9, 116.4, 120.5, 88.0));
    QCOMPARE(telemetrySpy.count(), 1);
    const Core::Uplink::TelemetryData data =
        telemetrySpy.first().at(0).value<Core::Uplink::TelemetryData>();
    QCOMPARE(data.latitude, 39.9);
    QCOMPARE(data.longitude, 116.4);
    QCOMPARE(data.altitude, 120.5);
    QCOMPARE(data.heading, 88.0);

    // 契约 §3：未定位哨兵 (0,0) 整帧丢弃
    invokeMessageArrived(&source, osdTopic, osdPayload(0.0, 0.0, 50.0, 0.0));
    QCOMPARE(telemetrySpy.count(), 1);

    // 契约 §3：纬度缺失（null）即未定位，丢弃
    {
        QJsonObject inner;
        inner.insert(QStringLiteral("longitude"), 116.4);
        QJsonObject envelope;
        envelope.insert(QStringLiteral("data"), inner);
        invokeMessageArrived(&source, osdTopic,
                             QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact)));
        QCOMPARE(telemetrySpy.count(), 1);
    }

    // 契约 §2：height 缺失回退 elevation
    invokeMessageArrived(&source, osdTopic,
                         osdPayload(39.9, 116.4, 66.6, 88.0, QStringLiteral("elevation")));
    QCOMPARE(telemetrySpy.count(), 2);
    QCOMPARE(telemetrySpy.last().at(0).value<Core::Uplink::TelemetryData>().altitude, 66.6);

    // 契约 §4：HMS data.list 字符串码 + level 2 → 严重（WP4：level 数值直传，feature §6.4）
    {
        QJsonObject item;
        item.insert(QStringLiteral("code"), QStringLiteral("battery_low"));
        item.insert(QStringLiteral("level"), 2);
        QJsonObject hmsData;
        hmsData.insert(QStringLiteral("list"), QJsonArray{item});
        invokeMessageArrived(&source, eventsTopic, eventsPayload(QStringLiteral("hms"), hmsData));
        QCOMPARE(alertSpy.count(), 1);
        const Core::Uplink::AlertEvent event =
            alertSpy.first().at(0).value<Core::Uplink::AlertEvent>();
        QCOMPARE(event.code, QStringLiteral("battery_low"));
        QCOMPARE(event.level, 2);
        QCOMPARE(event.text, QStringLiteral("HMS 告警（严重）"));
    }

    // 契约 §4 防御：数值码兼容 + 级别越界（9）按无级别 -1，正文回退未知文案
    {
        QJsonObject item;
        item.insert(QStringLiteral("code"), 1200);
        item.insert(QStringLiteral("level"), 9);
        QJsonObject hmsData;
        hmsData.insert(QStringLiteral("list"), QJsonArray{item});
        invokeMessageArrived(&source, eventsTopic, eventsPayload(QStringLiteral("hms"), hmsData));
        QCOMPARE(alertSpy.count(), 2);
        const Core::Uplink::AlertEvent event =
            alertSpy.last().at(0).value<Core::Uplink::AlertEvent>();
        QCOMPARE(event.code, QStringLiteral("1200"));
        QCOMPARE(event.level, -1);
        QCOMPARE(event.text, QStringLiteral("HMS 告警（未知）"));
    }

    // level 字段缺失：非 0/1/2 一律按无级别 -1（缺失不得误判为 0 提示）
    {
        QJsonObject item;
        item.insert(QStringLiteral("code"), QStringLiteral("compass_error"));
        QJsonObject hmsData;
        hmsData.insert(QStringLiteral("list"), QJsonArray{item});
        invokeMessageArrived(&source, eventsTopic, eventsPayload(QStringLiteral("hms"), hmsData));
        QCOMPARE(alertSpy.count(), 3);
        const Core::Uplink::AlertEvent event =
            alertSpy.last().at(0).value<Core::Uplink::AlertEvent>();
        QCOMPARE(event.code, QStringLiteral("compass_error"));
        QCOMPARE(event.level, -1);
        QCOMPARE(event.text, QStringLiteral("HMS 告警（未知）"));
    }

    // 空码项跳过（不产生告警）
    {
        QJsonObject item;
        item.insert(QStringLiteral("level"), 1);
        QJsonObject hmsData;
        hmsData.insert(QStringLiteral("list"), QJsonArray{item});
        invokeMessageArrived(&source, eventsTopic, eventsPayload(QStringLiteral("hms"), hmsData));
        QCOMPARE(alertSpy.count(), 3);
    }

    // device_exit_hms：HMS 解除（网关既定事件），无级别语义 level=-1，文案保持
    invokeMessageArrived(&source, eventsTopic,
                         eventsPayload(QStringLiteral("device_exit_hms"), QJsonObject()));
    QCOMPARE(alertSpy.count(), 4);
    {
        const Core::Uplink::AlertEvent event =
            alertSpy.last().at(0).value<Core::Uplink::AlertEvent>();
        QCOMPARE(event.code, QStringLiteral("device_exit_hms"));
        QCOMPARE(event.level, -1);
        QCOMPARE(event.text, QStringLiteral("机身 HMS 告警已解除"));
    }

    // 契约 §4：其他 method 按通用事件透传（前向兼容），code=method、无级别
    invokeMessageArrived(&source, eventsTopic,
                         eventsPayload(QStringLiteral("device_online"), QJsonObject()));
    QCOMPARE(alertSpy.count(), 5);
    {
        const Core::Uplink::AlertEvent event =
            alertSpy.last().at(0).value<Core::Uplink::AlertEvent>();
        QCOMPARE(event.code, QStringLiteral("device_online"));
        QCOMPARE(event.level, -1);
        QCOMPARE(event.text, QStringLiteral("事件：device_online"));
    }

    // 契约 §5：非契约主题（网关 state 拓扑）不消费
    invokeMessageArrived(&source, QStringLiteral("thing/product/GW001/state"),
                         osdPayload(39.9, 116.4, 10.0, 0.0));
    QCOMPARE(telemetrySpy.count(), 2);
    QCOMPARE(alertSpy.count(), 5);
}

void TelemetrySourceManagerTest::stallStateMachine()
{
    Core::Uplink::TelemetrySourceManager manager;
    manager.setMode(Core::Uplink::SourceMode::RealLink);  // 未运行，无真实连接尝试
    QSignalSpy linkSpy(&manager, &Core::Uplink::TelemetrySourceManager::linkStateChanged);
    QSignalSpy telemetrySpy(&manager, &Core::Uplink::TelemetrySourceManager::telemetryUpdated);

    // 私有槽经元对象系统驱动（与真实源信号连接路径一致）：Connected 起算停滞窗口
    QVERIFY(QMetaObject::invokeMethod(&manager, "onSourceLinkState",
                                      Q_ARG(Core::Uplink::LinkState,
                                            Core::Uplink::LinkState::Connected)));
    QCOMPARE(linkSpy.count(), 1);
    QCOMPARE(linkSpy.last().at(0).value<Core::Uplink::LinkState>(),
             Core::Uplink::LinkState::Connected);

    // 停滞超时（不实际等 5s，直接触发槽）：Connected → Stalled
    QVERIFY(QMetaObject::invokeMethod(&manager, "onStallTimeout"));
    QCOMPARE(linkSpy.count(), 2);
    QCOMPARE(linkSpy.last().at(0).value<Core::Uplink::LinkState>(),
             Core::Uplink::LinkState::Stalled);

    // 停滞后收到遥测：即时恢复 Connected 并转发遥测
    // （Q_ARG 是宏，聚合初始化里的逗号会被当作宏参数分隔符，需先具名构造再传入）
    const Core::Uplink::TelemetryData recovered{39.9, 116.4, 100.0, 45.0};
    QVERIFY(QMetaObject::invokeMethod(&manager, "onSourceTelemetry",
                                      Q_ARG(Core::Uplink::TelemetryData, recovered)));
    QCOMPARE(linkSpy.count(), 3);
    QCOMPARE(linkSpy.last().at(0).value<Core::Uplink::LinkState>(),
             Core::Uplink::LinkState::Connected);
    QCOMPARE(telemetrySpy.count(), 1);

    // 断链：停止停滞计时
    QVERIFY(QMetaObject::invokeMethod(&manager, "onSourceLinkState",
                                      Q_ARG(Core::Uplink::LinkState,
                                            Core::Uplink::LinkState::Disconnected)));
    QCOMPARE(linkSpy.count(), 4);

    // 切回模拟后再触发停滞超时：无操作（模拟模式无停滞语义）
    manager.setMode(Core::Uplink::SourceMode::Simulation);
    QVERIFY(QMetaObject::invokeMethod(&manager, "onStallTimeout"));
    QCOMPARE(linkSpy.count(), 4);
}

QTEST_GUILESS_MAIN(TelemetrySourceManagerTest)

#include "telemetry_source_manager_test.moc"
