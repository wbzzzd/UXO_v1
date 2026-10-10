#ifndef CORE_UPLINK_TELEMETRYTYPES_H
#define CORE_UPLINK_TELEMETRYTYPES_H

// 上行链路公共类型（REQ-012 M4E 接入 WP3）
// 内部同构遥测结构与数据源/链路状态枚举：本地模拟源与真实链路源（MQTT）
// 输出同一形状，UI 不感知来源差异。

#include <QMetaType>

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

}

Q_DECLARE_METATYPE(Core::Uplink::TelemetryData)
Q_DECLARE_METATYPE(Core::Uplink::SourceMode)
Q_DECLARE_METATYPE(Core::Uplink::LinkState)

#endif
