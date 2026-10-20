#ifndef CORE_UPLINK_TELEMETRYTYPES_H
#define CORE_UPLINK_TELEMETRYTYPES_H

// 上行链路公共类型（REQ-015 M4E 接入 WP3）
// 内部同构遥测结构与数据源/链路状态枚举：本地模拟源与真实链路源（MQTT）
// 输出同一形状，UI 不感知来源差异。

#include <QMetaType>
#include <QString>

namespace Core::Uplink {

// 无人机遥测数据（一期：贴合 HUD 需求 LAT/LON/ALT/HDG）
// 字段单位与既有 DroneTelemetrySimulator 信号一致；后续按需扩展（电量/速度等）。
struct TelemetryData {
    double latitude  = 0.0;  // 纬度（度）
    double longitude = 0.0;  // 经度（度）
    double altitude  = 0.0;  // 高度（米）
    double heading   = 0.0;  // 航向（度，0-360）
};

// 数据源模式
enum class SourceMode {
    Simulation,  // 本地模拟航线
    RealLink     // 真实 MQTT 链路
};

// 链路状态（仅真实链路进入状态机；模拟模式不经过）
enum class LinkState {
    Disconnected,  // 断链（未连接或连接断开）
    Connecting,    // 连接中
    Connected,     // 已连接（正常接收 OSD）
    Stalled        // 停滞（>5s 无 OSD，由 TelemetrySourceManager 判定）
};

// 告警事件（REQ-015 WP4）：结构化携带 HMS 级别，取代 WP3 的 code+text 纯文本信号
// （WP3 在源头把 level 折叠进文案导致 UI 无法按级别着色/映射，WP4 改为数值直传）。
// level 语义对齐 docs/interfaces/接口规格-MSDK对接.md events 契约：
//   0/1/2 = HMS 提示/警告/严重；-1 = 无级别事件（device_exit_hms 解除通知、未识别 method 透传）。
// level→Core::AlarmLevel 的映射表在 UI 转换槽冻结（MainWindow），源头只传数值。
struct AlertEvent {
    QString code;    // 告警事件码（HMS code，或 events 报文的 method 名）
    int level = -1;  // HMS 级别 0/1/2；-1 表示无级别事件
    QString text;    // 事件正文（展示文案；UI 负责加 [HMS] 前缀与级别着色）
};

}

Q_DECLARE_METATYPE(Core::Uplink::TelemetryData)
Q_DECLARE_METATYPE(Core::Uplink::SourceMode)
Q_DECLARE_METATYPE(Core::Uplink::LinkState)
Q_DECLARE_METATYPE(Core::Uplink::AlertEvent)

#endif
