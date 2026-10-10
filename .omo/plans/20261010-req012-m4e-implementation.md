# REQ-012 真实 M4E 无人机数据接入 — 实现计划

状态：已确认动工（2026-10-10 用户确认）；各 WP 状态见 §2 各节
关联需求：[REQ-012](../../docs/requirements/REQ-012.md)（Approved，2026-10-10）
关联设计：[m4e-drone-integration](../../docs/features/m4e-drone-integration.md)（Approved，2026-10-10）
前置记录：[20261009-m4e-integration-kickoff](20261009-m4e-integration-kickoff.md)（App 评估与全部范围裁决）

## 0. 前置（用户处置）

- [x] **Test 仓基线**：`gradle/wrapper/gradle-wrapper.properties` 与 `gradlew.bat` 未提交改动已按用户指示还原（2026-10-10，`git restore` 后工作区干净）。**VERIFIED**

## 1. 总览

契约先行、三线并行、联调即验收：

```
WP1 契约(docs/interfaces/,本仓)──前提──┬→ WP2 App MQTT 出口(Test 仓)──┐
                             ├→ WP3 客户端数据接入(本仓 src/)──┼→ WP4 HMS 双侧 → WP7 联调=验收
                             └→ WP6 TrackStore(本仓)──────────┘
WP5 视频播放原型(本仓,独立,随时可做,不阻塞主线)
```

## 2. 工作包

### WP1 契约与部署物（`docs/interfaces/` + 仓根 compose + `scripts/`，一切的前提）

- 接口报文规格文档：收编开工包接口规格为双方契约并自行维护。主题 `thing/product/{sn}/osd`（QoS 0）、`thing/product/{sn}/events`（QoS 1）；Cloud API 信封 `tid/bid/timestamp/gateway/data`；OSD 字段表对齐移交工程《UAS项目交接文档.md》§4.1（state 状态机判定顺序、坐标有效性过滤；RTK 字段维持屏蔽）；HMS events 字段与级别映射（0/1/2 → 提示/警告/严重）。附带裁决建议：App 侧 5888 TCP 旧出口一期**保留双发**（回退便利，客户端不消费），二期视情关闭。
- docker-compose：EMQX 单服务（1883/8083/18083）；PostgreSQL+PostGIS 编入但默认不启用（供 WP6 开关）。
- sim 工具说明：参考开工包 `sim_telemetry.py`，注明来源，定位开发/CI 工具。
- 验证：compose up 后 mosquitto_pub → mosquitto_sub 自环通；sim 发报文可被订阅。
- 状态：**ARTIFACT_READY**（2026-10-10）。产物：`docs/interfaces/接口规格-MSDK对接.md`（契约 V2.0，含 5888 双发建议；附录 A=模拟器差异、附录 B=自环验证命令）、仓根 `docker-compose.yml`、`scripts/sim_telemetry.py`（收编自开工包，md5 收编时核对一致；仅一处适配：连接失败提示 compose 路径指向仓根）。已过静态检查（compose YAML/sim 语法/契约样例 JSON 无激光字段）；**运行时验证（2026-10-10 替代回路已通过）**：本 WSL 无 docker 且 sudo 不可用，改用 pip 用户态 amqtt 0.12.1（纯 Python broker，127.0.0.1:1883）+ paho 订阅端完成 sim→broker→订阅 全回路——sim 以 speedup 10 跑完整任务自然退出（0)，收到 **348 条 OSD**（契约★字段齐：经纬高/速度/姿态/mode_code/电池/position_state/89-0-0 云台）、**1 条 device_exit_hms**（QoS1，契约附录 A 既定差异）、**1 条网关 state 拓扑**（下挂 1 子设备），断言脚本全过；证据 `.omo/evidence/wp1-wsl-verify/`（gitignored，本地留档）。**遗留**：EMQX compose 启动验证仍待有 docker 的环境（命令见契约附录 B），不阻塞 WP3（开发数据源可用 sim+amqtt 回路）。（2026-10-10 治理裁决：自 `deploy/` 迁出，见会话记录第 14 条。）

### WP2 App MQTT 出口（Test 仓，约 1–2 周）

- 施工图：先读《UAS项目交接文档.md》§2（通道方向是反的）/§4（协议规格）/§6（网络拓扑）/§7（实施指引）+ `SdkBridge`/`FlightState` 源码。
- 新增 MQTT 出口模块（Java 侧 MQTT 客户端选型实施时定）：按 WP1 规格发 OSD（QoS 0）；broker 地址可配（交接文档 §5.5 设备侧网络限制不影响——broker 在 PC/服务器侧）。
- 验证：手柄常驻真机，mosquitto_sub 收到规格全字段 OSD。
- 状态：未开工（依赖 WP0、WP1）

### WP3 客户端数据接入（本仓 `src/`）

- 先裁决 §9-1：查当前 Qt 5 版本与 QtMQTT 附加模块可用性，不可用则 Eclipse Paho MQTT C++。**核查结果（2026-10-10）**：Qt 5.15.15（conda uxo-dev）无 QtMqtt 模块，apt 无 qtmqtt 包；apt 有 `libpaho-mqtt-dev` + `libpaho-mqttpp-dev` → 建议 Paho MQTT C++（纯 C++ 库、与 Qt 无 ABI 耦合）。**已裁决采用（2026-10-10 用户确认）**。
- `Core/Uplink` 数据源接口：`DroneTelemetrySimulator` 改造为模拟源；新增 `MqttUplinkClient`/`MqttTelemetrySource` 真实源（解析 Cloud API 信封，输出同构内部遥测结构）。
- UI：数据源控件（§6.1 状态机，断链**不自动切回**模拟）、数据源徽标（§6.2）、连接状态（§6.3，>5s 无 OSD 判停滞）。
- 验证：sim 注入与真实 App 输出两侧均通；数据源互斥切换正确；UI 按设计 §6.6 验收（分段按钮/徽标 token/文案，1280×720 工具栏不溢出 + 三视口截图）；Release 构建 + ctest 不低于基线。
- 状态：**可动工**——§9-1 已裁决 Paho MQTT C++（2026-10-10 用户确认）；WP1 替代回路验证已过，sim+amqtt 可作开发数据源（EMQX 验证非阻塞）

### WP4 HMS 双侧

- App 侧：HMS 采集 → `events`（QoS 1），按 WP1 规格。
- 客户端：`EventIngest` → 告警/日志（§6.4 级别映射表实现时冻结）。
- 验证：sim 注入 HMS 事件 → 告警级别正确；真机 HMS 事件（如低电量）随联调观察。
- 状态：未开工（依赖 WP1 + WP2/WP3）

### WP5 视频播放（独立，随时）

- §9-3 原型：QMediaPlayer + GStreamer 直连 RTSP 实测延迟与稳定性（App 侧已实测 ~0.5s）；不达标再评 FFmpeg/mpv 嵌入。
- 接入：`VideoStreamPanel` 视频源配置化 URL、协议与对端不感知（§9-2 护栏）；HUD-only 叠加不变。
- 验证：手柄常驻拉流稳定、延迟可接受。
- 状态：未开工

### WP6 TrackStore（依赖 WP1，授权已生效）

- PostgreSQL+PostGIS schema：`trajectory`/`events` 两表（参考开工包字段设计）；可选开关，默认关闭；`laser_targets` 不启用。
- 验证：开关开启后联调数据落库正确；关闭时零写入。
- 状态：未开工

### WP7 联调 = REQ-012 验收

- 开机静止：客户端显示真实遥测/实时视频/HMS 事件，UI 常显"真实数据"标识。
- 手持/短距移动：态势地图显示真实航迹。
- 数据源互斥切换正确；只读边界（客户端代码层无下行路径）；Release 构建退出 0，ctest 不低于实施前基线。
- 状态：未开工

## 3. 执行纪律

- 并行通道：WP1/WP2/WP3/WP5 文件组互不冲突，可并行；WP2 在 Test 独立仓，提交消息用中文、说明变更内容与目的。
- 每个 WP 状态按 未开工/WORKING/ARTIFACT_READY/VERIFIED/FAILED 更新于本文；实现、构建、测试拆独立阶段，长命令后台执行 + 轮询。
- 安全边界：5889 反控链路不移植；交接文档 §5.1（联锁绕过）/§5.2（杆量未标定）缺口不在本计划触碰；模拟/真实数据源 UI 常显区分。
- 本仓改动不主动提交 git，待用户明确要求；验收有证据后回写核心文档（设计 §8 清单）并将 REQ-012 状态更新为 `Implemented`。
