# 接口规格 · MSDK App ↔ 排弹系统（MQTT 契约 V2.0）

版本：V2.0 ｜ 日期：2026-10-10 ｜ 状态：双方契约，本项目自维护 ｜ 关联需求：REQ-012（Approved）

> 本文收编自《M4E对接开工包》`uxo_m4e_kickstart/docs/接口规格-MSDK对接.md` V1.0（2026-10-08），按 2026-10-09/10 项目裁决改写，V1.0 原文不再维护。与 V1.0 的差异：
>
> 1. **删除激光目标字段**（`measure_target_*`）：经上游资料核查为幻影需求，用户裁决不需要；
> 2. **视频不在本契约范围**：一期 RTSP 直连（客户端 ← 遥控器 8554，App 零改动），见功能设计 §9-2 裁决；
> 3. **RTK 字段维持屏蔽**：设备侧 RTK 已整体屏蔽（交接文档 §5.6）；
> 4. **App 改造由本项目接续**：原"同事侧/接口会议冻结"表述调整为本项目内契约。

## 1. 总览：App 只做一件事——推数据到 EMQX

| 数据 | Topic | QoS | 频率 |
|---|---|---|---|
| 遥测 OSD | `thing/product/{飞机SN}/osd` | 0（允许丢帧保最新） | 建议 1~10Hz |
| 事件 events（HMS 等） | `thing/product/{飞机SN}/events` | 1（必达） | 触发即发 |

- broker 地址在 App 内做成**可配置项**（现场填服务器 IP）；账号密码暂留空（EMQX 当前匿名，开认证后再补）。
- **旧出口双发**：5888 TCP JSON Lines 旧出口一期保留双发（回退便利，客户端不消费），二期视情关闭。
- 时间戳一律 UTC 毫秒，由 App 打点（不可用接收时间顶替）。
- 遥测/事件使用**同一飞机 SN**；`gateway` 填遥控器 SN。

## 2. 遥测接口（OSD）

**Topic**：`thing/product/{飞机SN}/osd` ｜ **QoS**：0 ｜ **格式**：JSON，Cloud API 信封：

```json
{
  "tid": "随机UUID", "bid": "随机UUID",
  "timestamp": 1759920000000,
  "gateway": "遥控器SN",
  "data": {
    "latitude": 30.000123, "longitude": 119.999876,
    "height": 58.5, "elevation": 70.5,
    "horizontal_speed": 6.0, "vertical_speed": 0.5,
    "attitude_head": 45.0, "attitude_pitch": -2.1, "attitude_roll": 1.0,
    "mode_code": 5, "flightmode": "GPS_ATTI", "state": "FLYING",
    "battery": {
      "capacity_percent": 85, "remain_flight_time": 34,
      "return_home_power": 30, "landing_power": 15
    },
    "position_state": { "is_fixed": 2, "quality": 5, "gps_number": 18 },
    "home_distance": 120.5,
    "rth_altitude": 80, "rc_lost_action": 2,
    "89-0-0": {
      "gimbal_pitch": -35.0, "gimbal_roll": 0.0, "gimbal_yaw": 45.0,
      "zoom_factor": 12.0, "payload_index": "89-0-0"
    }
  }
}
```

### 字段表（★ = 排弹核心，缺一不可）

| 字段 | 类型 | 单位/说明 | MSDK 获取途径（交接文档 §4.1 佐证） | 必须 |
|---|---|---|---|---|
| latitude / longitude | number\|null | WGS84 度；**未定位 = null**（见 §3 过滤） | `KeyAircraftLocation3D` | ★ |
| height | number | 相对起飞点高度 m | 同上 | ★ |
| elevation | number | 海拔 m | 同上 | 有则填 |
| horizontal_speed / vertical_speed | number | m/s，**地理系** | `KeyAircraftVelocity` 输出为机体系 `vx/vy/vz`，**换算责任在 App** | ★ |
| attitude_head / pitch / roll | number | 度 | `KeyAircraftAttitude`（head = yaw） | ★ |
| mode_code | int | 飞行模式码（0 悬停 / 3 手动 / 5 航线 / 9 返航 / 10 降落…） | `KeyFCFlightMode` → 枚举映射，映射表 WP2 冻结 | ★ |
| flightmode | string | **真实**飞行模式（如 `GPS_ATTI`） | `KeyFCFlightMode` | ★ |
| state | string | 综合状态机，判定顺序固定、短路返回：`NOT_REGISTERED` → `NO_AIRCRAFT` → `NO_GPS_FIX` → `FLYING` → `STANDBY`（同交接文档 §4.1） | 计算得出 | ★ |
| battery.capacity_percent | int | %（多电池取**最低**） | `KeyBatteryOverviews` | ★ |
| battery.remain_flight_time | int | 分钟 | 电池 Keys | ★ |
| battery.return_home_power / landing_power | int | % | 电池 Keys | 有则填 |
| position_state.is_fixed / quality / gps_number | int | 定位状态/质量/卫星数 | `KeyGPSSatelliteCount` 等 | ★ |
| position_state.rtk_number | int | **当前屏蔽**：恒 0 或缺省，客户端不得依赖（RTK 恢复后另行约定） | — | 屏蔽 |
| home_distance | number | m | 计算/Keys | 有则填 |
| rth_altitude / rc_lost_action | number | m / 枚举 | Keys | 有则填 |
| `89-0-0`.gimbal_pitch / roll / yaw | number | 度 | 云台 Keys（M4E 载荷挂 `89-0-0`） | ★ |
| `89-0-0`.zoom_factor | number | 倍数 | 相机 Keys | ★ |
| wind_speed / wind_direction / storage / total_flight_* / gear / firmware_version | — | 可选增强字段 | Keys | 可选 |

**禁止字段**：`measure_target_*`（激光目标，需求已删除）。客户端对未知字段一律忽略（前向兼容）。

## 3. 数据质量约定

1. **坐标有效性过滤**（沿用交接文档 §4.1，App 侧已实现在 `FlightState`）：MSDK 未定位时返回 `(0,0)`，必须过滤；纬度 `|v| ≤ 90 且 > 1e-7`，经度 `|v| ≤ 180 且 > 1e-7`；无效时 `latitude/longitude/height` 输出 `null`。**接收方收到 null 即未定位，无需再过滤 0 值。**
2. **断线重连**：MQTT 断开后指数退避重连（1s 起，上限 60s），重连后继续推数。
3. **后台存活**：App 前台服务保活，息屏 30 分钟不断流（联调专项测试项）。
4. **客户端停滞判定**：>5s 无 OSD 报文判"数据停滞"（UI 契约 §6.3），QoS 0 丢帧以 `timestamp` 为准。

## 4. 事件接口（events）

**Topic**：`thing/product/{飞机SN}/events` ｜ **QoS**：1（必达）｜ 触发即发。

```json
{
  "tid": "随机UUID", "bid": "随机UUID",
  "timestamp": 1759920000000,
  "gateway": "遥控器SN",
  "method": "hms",
  "data": { "list": [ { "code": "0x1A0B0001", "level": 1 } ] },
  "need_reply": 0
}
```

- `method = "hms"`：HMS 告警，一期消费；其他 `method` 取值允许出现，客户端转发不消费（前向兼容）。
- `data.list[]` 每项至少含 `code`（大疆 HMS 错误码，字符串）与 `level`；`args`/`component_index`/`sensor_index` 等可选字段原样透传。
- **级别映射**：`level` 0 / 1 / 2 → 提示 / 警告 / 严重（UI 契约 §6.4；告警文字带 `[HMS]` 前缀，映射表实现时冻结）。

## 5. Topic 汇总与订阅

| Topic | 方向 | 说明 |
|---|---|---|
| `thing/product/{sn}/osd` | App → EMQX → 客户端 | 遥测，契约 |
| `thing/product/{sn}/events` | App → EMQX → 客户端 | 事件，契约 |
| `thing/product/{gateway}/state` | sim → EMQX | 模拟器设备上线拓扑（简化），**非契约**，客户端一期不消费 |

客户端可按 `thing/product/+/osd`、`thing/product/+/events` 通配订阅。

## 6. 验收口径（对齐 REQ-012 真机单层）

- 开机静止：`mosquitto_sub` 抓包可见本规格 ★ 字段全齐；客户端显示真实遥测、HMS 事件与"真实数据"标识。
- 手持/短距移动：态势地图显示真实航迹。
- 联调观察项（沿 V1.0）：连续 40 分钟遥测无丢失、App 无崩溃；断网恢复 10 秒内自动重连续传。

## 附录 A：开发/CI 模拟器（`scripts/sim_telemetry.py`）与契约差异

本仓 `scripts/sim_telemetry.py` 为开发回路与 CI 回归工具，**不是验收依据**（REQ-012 验收为真机单层）。原封收编自开工包 `simulator/sim_telemetry.py`（2026-10-08，md5 `5e0d022eb7bc24baac773f48718756c7` 收编时核对一致）；开工包不再演进，本仓副本为唯一使用版本（仅一处适配：连接失败提示中的 compose 路径指向仓根）。

用法：`python3 scripts/sim_telemetry.py [--broker 127.0.0.1] [--port 1883] [--sn 1581F5BKSIMUL001] [--speedup 1.0]`；依赖 `pip install paho-mqtt`；Ctrl+C 停止。

模拟器输出与本契约存在以下已知差异，客户端实现不得依赖这些差异：

1. **激光字段**：模拟器在"激光标记"航段会发 `measure_target_*`——需求已删除，**非契约字段**，客户端应忽略（这也是"客户端忽略未知字段"规则的天然测试）。
2. **RTK 字段**：模拟器发 `position_state.rtk_number` 非零值——真实设备 RTK 已整体屏蔽（交接文档 §5.6），契约规定该字段恒 0 或缺省。
3. **state 话题**：连接时向 `thing/product/{gateway}/state` 发设备上线拓扑——**非契约**，客户端一期不消费。
4. **state/flightmode 字段**：模拟器不发 `state`/`flightmode`（契约 ★ 字段）——模拟注入开发期间客户端应按字段缺省容错处理；真机联调时由 App 侧按契约输出。

## 附录 B：本地自环验证（需 docker 与 mosquitto-clients）

```bash
docker compose up -d                 # 仓根启动 EMQX（首次需拉镜像，需临时外网）
# 自环验证（另开一个终端订阅）：
mosquitto_sub -h 127.0.0.1 -t 'thing/product/+/osd' -v &
mosquitto_pub -h 127.0.0.1 -t 'thing/product/TESTSN/osd' -m '{"test":1}'
# sim 注入验证：
python3 scripts/sim_telemetry.py --broker 127.0.0.1
```

EMQX Dashboard：`http://127.0.0.1:18083`（默认 admin/public，仅限本机开发）。
