# 20261009 M4E 无人机接入 · 启动评估会话记录

分支：`feature/m4e-drone-integration`（worktree `/home/lin/UXO_v1-m4e-integration`，基于 main `6800091`）

## 本会话目标

1. 确认 WSL 能否读取 Windows 目录中的同事交付物；
2. 评估同事的 MSDK 安卓 App（`D:\工作\排弹\测试数据\Test`）能否继续使用、需哪些重构；
3. 评估《M4E对接开工包-给App同事》（V3 实施计划）合理性；
4. 新建本部分工作专用 worktree 与分支。

## 交付物位置（Windows 侧，WSL 经 /mnt/d 可读，已验证）

- App 源码：`/mnt/d/工作/排弹/测试数据/Test/`（约 4500 行 Java，11 文件，包 `com.example.tcpsender`，applicationId `YY.Bao`，MSDK V5，compileSdk 35 / minSdk 24，git 历史 2 提交）
- 交接文档：`Test/UAS项目交接文档.md`（991 行）
- 开工包：`/mnt/d/工作/排弹/测试数据/M4E对接开工包-给App同事/`
  - `dji_dat_analysis/M4E对接实施计划V3-MSDK路线.docx`（V3 主计划）
  - `uxo_m4e_kickstart/`（接口规格 md、对接操作指南 md、sim_telemetry.py、docker-compose + PostGIS schema）

## App 现状（代码已逐文件核实）

数据链路：MSDK V5 采集 → 遥控器 RC Plus 2 → 北向出口三条：

| 通道 | 现状 | 说明 |
|---|---|---|
| 遥测 | TCP 5888，自定义 JSON Lines，1Hz，手写拼 JSON | TelemetryReporter.java |
| 指令 | TCP 5889，JSON 行，seq 单调 + 3s 心跳超时 + 紧急停止 | CommandServer.java |
| 视频 | RTSP 服务器（遥控器侧，Live555），PC 拉流，H.264 已实测 | StreamManager.java |

交接文档声明与代码一致性抽查全部属实：

- StickController 20Hz 发送、500ms 遥控器超时：属实
- CommandServer seq 单调递增、3s 心跳超时：属实
- PreflightChecker 8 项起飞门槛：属实
- RTK 屏蔽（`TelemetryReporter.RTK_ENABLED = false`）：属实
- **安全缺口 5.1**：`MainActivity.onAction()`（363–387 行）仅检查 `confirm:true`，无 `preflight_ok` 联锁：属实
- **安全缺口 5.2**：虚拟摇杆杆量单位未标定：属实（代码含 +500 试探值）

## 评估结论一：App 能否继续用

**结论：继续用，不整体重构；按"出口层替换 + 采集层扩展"改造，反控链路暂不进 V3。**

直接复用（核心价值，MSDK 踩坑已解决）：
- `SdkBridge`（MSDK 初始化/注册幂等）、`FlightState`（Key 聚合 + 垃圾值过滤）、`PreflightChecker`
- `build.gradle` MSDK 配置（doNotStrip 清单、kotlin-stdlib 匹配、ABI 过滤）、Manifest 权限与 App Key 绑定（applicationId `YY.Bao` 不可改）

需替换（北向出口，对接 V3 内网格式）：
1. TelemetryReporter → MQTT 发布器（Eclipse Paho Android），输出 Cloud API 物模型报文（`tid/bid/timestamp(UTC毫秒)/gateway/data` 信封），topic `thing/product/{SN}/osd`
2. StreamManager RTSP → RTMP 推流至 SRS:1935（MSDK `LiveStreamManager` 原生支持 `LiveStreamType.RTMP`，配置级改动，需实测确认 H.264 与码率参数真实生效——当前码率输入框是摆设）
3. 新增事件通道：HMS → `thing/product/{SN}/events`（QoS 1）

新增开发（不是"格式翻译"，是新 MSDK Key 采集，V3 docx 低估了这部分）：
- 激光测距 `measure_target_latitude/longitude/altitude`（★★ 排弹核心，当前 FlightState 完全没有，M4E 上 MSDK V5 是否暴露该 Key 未验证）
- 云台姿态、`zoom_factor`、`position_state`/RTK、`remain_flight_time`、`elevation`（当前均无）
- UTC 毫秒时间戳（当前为本地时间字符串）
- 前台服务保活（V3 高风险项，当前 App 无 Service）

建议不移植（安全）：`CommandServer`/`StickController`/`FlightActions` 反控链路。理由：携带两个未关闭 P0 缺口（5.1 联锁绕过、5.2 杆量未标定），且 V3 范围本就不含 PC 侧控制。零杆量校验逻辑可保留。

工作量粗估：出口替换 1–2 周；新采集字段（尤其激光，含真机验证）约 1 周起。

## 评估结论二：开工包 / V3 计划是否合理

**结论：总体合理，架构决策正确；4 个具体问题需在进入 M1 前修正。**

优点：
- "不动内网侧、App 做出口替换"原则正确，EMQX/SRS/PostgreSQL 链路不变，改动面最小
- 采用 Cloud API 物模型作线格式，V2（Pilot 2 上云）回退路径保持同构，决策留有余地
- 模拟器先行 + M1 接口冻结会 + M1–M4 可验证退出标准，协同方式正确
- 风险表现实（Android 后台保活标 HIGH、H.265 需确认、字段映射未冻结）

问题（按优先级）：
1. **`uxo_m4e_kickstart/docs/对接操作指南.md` 过时**：第 3–4 步仍是 V2 路线（部署 DJI-Cloud-API-Demo、Pilot 2 绑定登录），与 V3 docx"不再需要开源 Demo"直接矛盾，会误导 App 同事。M1 会议前必须修订。
2. **App 侧工作量被低估**：V3 docx 称"App 侧做一层翻译——工作量小"。格式翻译确实小，但激光/云台/RTK 是新 MSDK Key 开发，且 `measure_target_*` 在 M4E 上可读性未验证（接口规格 §4 自己标了 ★★ 待办）。**建议 M1 退出标准增加"激光测距 Key 真机实测可读"，不能仅冻结字段名。**
3. **反控链路范围未声明**：计划只字未提 5889 控制通道去留。建议书面明确：M1–M4 只做数据上行，PC 侧控制不在范围（对应 App 侧两个 P0 缺口也就不阻塞进度）。
4. **带宽现实性**：三镜头并发 RTMP 约 8.3Mbps 上行，现场自建 WiFi 需实测；V3 已建议先单主镜头，保持该默认。

另注：schema（5 表 + PostGIS）设计合理；10Hz 遥测入库约 3.6 万行/小时/机，无压力；`task_id` 关联机制 V3 阶段三覆盖。EMQX 匿名 / SRS 无鉴权 / RTSP 硬编码凭据（uas/uas123）在排爆内网阶段一可接受，需列入外场前硬化清单。

## 本仓（UXO_v1）需求状态核查

当前需求注册表无任何条目覆盖真实 M4E 接入：REQ-009（无人机探测态势演示，`Implemented`）为本地模拟遥测链路，恰是本接入要替换的数据源。按 AGENTS.md 规则，实现前须先完成：新 REQ（Draft→Approved）+ `docs/features/<feature>.md`（Draft→Approved）。

## 后续步骤执行情况（2026-10-09 更新）

1. ✅ 已完成：REQ-012（`docs/requirements/REQ-012.md`，`Draft`，独立候选）与 feature 设计稿（`docs/features/m4e-drone-integration.md`，`Draft`）已起草，注册表索引（`docs/requirements/README.md`）已更新；待用户评审批准。设计要点：
   - 数据源抽象：REQ-009 模拟链路保留为可切换模拟源，真实源为 MQTT/RTMP 接入，二者互斥；
   - 安全边界：REQ-012 批准即构成只读外部接入（MQTT 订阅/RTMP 拉流）+ 轨迹库写入的明确授权；客户端代码层无下行指令路径；
   - 待确认事项 7 条，关键路径为激光测距 Key 真机实测（建议列入 M1 退出标准）；
   - 架构 grounding：遥测源复用 `DroneTelemetrySimulator`，视频复用 `VideoStreamPanel`，目标落位复用 REQ-009 链路。
2. 待办：将"App 改造清单 + 计划修订 4 条"整理为给 App 同事的反馈（M1 会议输入）。
3. 本会话所有产出（含本记录、REQ-012、feature 设计稿、注册表索引）均未提交，待用户确认。

## 用户评审问答与范围裁决（2026-10-09）

用户对草稿提出 5 问，结论与裁决：

1. **App 是否改 C++**：不能。MSDK V5 仅 Android（Java/Kotlin），运行于 RC Plus 2（Android 系统）。一致性靠接口报文格式（Cloud API 物模型 JSON）保证；`sim_telemetry.py` 保留为开发/测试工具（模拟链路验收依赖），不进产品运行链路。
2. **RTMP vs RTSP**：RTMP（推流至 SRS，服务器 DVR 录像 + 多席位分发，延迟 1–3s，App 小改动）为开工包默认；RTSP（客户端直连遥控器拉流，~0.5s 已实测，App 零改动，无服务器录像，单席位限定）为可选。**裁决结果：列入待确认事项**（feature 设计 §9-2 对比表），用户后续裁决，影响 App 改造清单与 deploy 链路组成。
3. **真机 + 遥控器的作用**：客户端开发不依赖真机（模拟链路先行，不阻塞）；真机供 App 侧改造验证，早期不用飞（开机静止即可验证遥测/视频上行）。建议真机优先给 App 同事。
4. **UI 设计深度**：用户指出只见提纲级。已将 feature 设计 §6 扩为 6.1–6.5：数据源控件位置与状态机（断链不自动切回模拟）、真实/模拟标识、连接状态指示（>5s 无报文判停滞）、HMS 告警级别映射、目标表说明。
5. **激光测距需求来源**：经 grep `docs/sources/` 全部上游资料确认：SRS/SDD/硬件规格书中的"激光"均为 LiDAR 点云（DS-02）、地形跟随高度计、定向能武器（ED-02），**无"M4E 激光打点标记疑似弹"需求**；`measure_target_*` 系开工包从 DJI 数据能力反推的幻影需求。**用户裁决：不需要**。已从 REQ-012 与 feature 设计删除（LaserTargetIngest、laser_targets 入库、原 §9-1 关键路径）；目标坐标来源保持 REQ-009 检测推算路径；原最大风险项（激光 Key 真机实测）随之消失，App 侧新采集工作量收窄。

6. **RTMP/RTSP 后续裁决（2026-10-09）**：用户追问"服务器"含义（纯内网机器，运行时无公网流量；仅部署准备期镜像下载与大疆激活需临时外网）与 RTMP/RTSP 互改工作量后确认——一期 RTSP 直连（App 零改动、延迟 ~0.5s 已实测、链路今日已通）；预留服务器汇聚演进位：集中录像/多席位时引入 MediaMTX/SRS，届时再定"服务器拉 RTSP"（App 仍零改动）或"App 改推 RTMP"（约 1–2 天，RTSP→RTMP 总切换成本约 2–3 天）。设计护栏：视频源配置化 URL、播放面板协议/对端不感知。已写入 feature 设计 §1.3/§2.1/§5 与 §9-2；遥测链路（MQTT/EMQX）维持原设计。

修改后待评审文件：`docs/requirements/REQ-012.md`（删激光、验收收窄、备注记裁决）、`docs/features/m4e-drone-integration.md`（§1.3 记裁决、§2/§3/§5 删激光、§6 详细化、§9 重排；§2.1/§5/§9-2 视频链路 RTSP 一期已裁决）。

7. **遥测链路裁决（2026-10-09）**：用户追问 MQTT 本质，经分层解释（MQTT 是应用层协议、跑在 TCP 之上；现状=TCP+自定义格式点对点，目标=TCP+MQTT 经 EMQX 中转；"飞行数据"即遥测，飞行控制不经本系统）与两方案优劣对比后，确认**维持 MQTT + Cloud API 物模型设计**。否决 TCP 直连最省方案（App 零改动，但客户端需 TCP+MQTT 双源、模拟器需改造、放弃报文标准化）。核心论据：一期验收模拟链路 `sim_telemetry.py` 即 MQTT，单一 MQTT 源同构兼容模拟/真实；HMS 告警 QoS 1 不丢；多消费者（TrackStore/未来席位）App 零改动。至此链路三项裁决全部落定：不含激光、视频 RTSP 直连一期、遥测 MQTT。已记入 feature 设计 §1.3。

8. **下行边界表述修正（2026-10-09）**：用户指出"客户端不发任何指令"表述错误——客户端只是不发送直接控制飞机的指令，航迹规划等设置类指令是产品能力。核查确认：该表述系 assistant 过度概括（依据 AGENTS.md 安全边界默认禁止 + 5889 反控链路 P0 缺口评估），非上游文档原文；代码事实支持用户（REQ-007 MOS 规划模块已实现本地功能）。用户确认修正表述（航迹下发暂不立项）：REQ-012 验收/备注与 feature 设计 §2.2 改为三层边界——本需求不含下行；直接飞行控制（杆量/起降/云台）为安全边界持续禁止项，任何需求不得引入；任务设置类指令属后续独立功能。同次答疑记录：轨迹库=PostgreSQL+PostGIS 任务存档（复盘/留证/回放，§9-4 待授权）；视频链路部署难度外差异补全（弱网行为：RTSP 丢包花屏即恢复不累积、RTMP 延迟滚雪球；网络拓扑：RTSP 需同网段直连、RTMP 可跨网段；故障点与生态）；vendor=外部交付物副本收编本仓 `deploy/`（§9-5，倾向整体拷贝+注明来源版本）。

9. **模拟链路降级与开工包定位（2026-10-09）**：用户裁决"开工包不重要，不合理处扔掉"，并质疑有真机为何模拟验收。assistant 说明模拟链路三真实理由（排期并行：App 的 MQTT 出口尚不存在，真机遥测验收物理上需等同事改造；开发回路：无法每轮编译测试都发动真机；故障定位/CI 回归/HMS 事件注入均需可控源），但承认其"正式验收层"身份系抬高——有真机，正式验收即应真机（且可很便宜：开机静止验遥测/视频/HMS，手持/短距移动验航迹，不需真飞任务）。用户确认"**降级不删除**"：REQ-012 验收重写为真机单层；模拟链路降为开发/CI 工具；开工包处置——SRS 出真实链路、对接指南作废、部署物不收编（自写极简 compose：EMQX；PostgreSQL 待 §9-4）、接口报文规格作为双方契约收编本仓 `deploy/` 文档（§9-5 vendor 问题随裁决消解）；§1.3 MQTT 裁决论据同步去除对模拟链路的依赖（QoS 1/订阅解耦/Cloud API 标准/V2 回退为独立依据）。连带结论：RTSP 直连后真实链路必备服务器组件仅 EMQX（+PostgreSQL 待批）。

10. **App 工程移交与开发计划修订（2026-10-10）**：用户明确同事交付物即 Test 工程本体（`D:\工作\排弹\测试数据\Test`，独立 git 仓库，2 提交；含 46KB《UAS项目交接文档.md》：三通道协议规格 5888/5889/8554、六项安全缺口——与本仓此前评估的 P0 一致、链路/GPS/视频/三通道压测已验证结论），且**后续 App 改造由本项目接续承担**；用户可手柄常驻开发调试。影响：①REQ-012 能力/备注与 feature §1.3/§2.2/§7/§9-6 中"外部依赖/同事仓库"表述全部改为"本项目接续、代码不收入本仓"；②§9-6 联调窗口消解（无外部排期依赖）；③开发顺序修订为：契约先行（接口规格收编 `deploy/` + EMQX 极简 compose = M1 接口冻结实质）→ App 线 MQTT 出口改造（OSD QoS 0，mosquitto_sub 即时自验）→ 客户端线数据源抽象 + MqttUplinkClient（对 sim/手动注入开发，出口通后切真实联调）→ HMS 双侧（events QoS 1）→ 真机联调即 REQ-012 验收；视频 RTSP 已通（交接文档 §8 佐证），§9-3 播放原型随时可验。④原"整理 M1 会议输入给 App 同事"待办随移交作废（App 工作已归本项目）。§9 剩余待确认减为 3 条：MQTT 库选型、播放方案、TrackStore 授权。Test 仓两处 gradle wrapper 未提交改动已提示用户先行处置（接手基线）。批准门禁不变：REQ-012 与 feature 均 Draft→Approved 后方可生成实现计划。

11. **REQ-012 与 feature 设计获批（2026-10-10）**：用户批准两份文档，状态均 Draft→Approved（REQ-012 文件、注册表索引与 NEXT 段、feature 头部及 §4/§7 措辞同步更新）。随批准生效：①只读外部接入（MQTT 订阅/视频拉流）+ 轨迹库写入授权（REQ-012 备注既定措辞）；②§9-4 TrackStore 范围裁决落定（`trajectory`/`events` 两表、可选开关默认关闭、`laser_targets` 不启用）；③实现门禁解除。§9 剩余 2 条转为实施期裁决：§9-1 MQTT 库选型（先查 Qt 5 模块可用性）、§9-3 播放方案（原型验证后定）。已生成实现计划 `.omo/plans/20261010-req012-m4e-implementation.md`（WP0 前置 Test 仓基线处置；WP1 契约先行；WP2 App 出口/WP3 客户端/WP5 视频可并行；WP4 HMS、WP6 TrackStore、WP7 联调=验收），**待用户确认后动工**。全部产出（含本轮 4 文件文档变更 + 计划文件）仍未提交 git，待用户明确要求。

12. **UI 施工细化裁决（2026-10-10）**：用户就动工前 UI 修改三问裁决——①数据源控件=**分段按钮**（`[本地模拟|真实链路]` 并排、当前项高亮、约 180px、置探测工具栏按钮组左侧）；②视频 PiP **不加**数据源角标（真实/模拟区分依赖中心区徽标常显 + HUD 遥测跟随）；③徽标配色委托 assistant 对齐 `design-system.md` 后定具体值：**真实=信息蓝 `#42A5F5`**（`--color-tier-blue`，设计本意即区别于威胁色阶，不占用设备在线绿 `#4CAF50` 语义）、断链=`#FF5252`（`--color-status-error`）、连接中/数据停滞=`#FFB74D`（`--color-status-busy`），全部颜色+文字双编码。另补写 HUD 遥测跟随数据源规则（§6 原未写死：真实链路 HUD 显示真实 OSD）。细化落 feature **§6.6**（控件规格、token、HMS 告警 `[HMS]` 前缀、文案定稿、三视口验收证据含 1280×720 工具栏溢出检查）；实现计划 WP3 验证同步。UI.md 回写（§3 结构树工具栏行/§4.3 矩阵新增行/§6 TARGET 表 REQ-012 行）留待 Implemented 阶段（生命周期规定）。配色具体值已呈现用户，待确认。

13. **计划动工，WP0 完成、WP1 落地（2026-10-10）**：用户确认实现计划动工（"启动"）；Test 仓 gradle 两处改动按用户指示**还原**（`git restore`，工作区干净，WP0 VERIFIED）。**WP1 ARTIFACT_READY**，`deploy/` 新增 5 产物：①`docs/接口规格-MSDK对接.md` **契约 V2.0**（收编开工包 V1.0 按裁决改写：删激光 `measure_target_*`；视频 RTSP 移出契约；`rtk_number` 屏蔽恒 0/缺省；OSD 字段表含 `state` 状态机判定顺序/`flightmode`/坐标有效性过滤，对齐交接文档 §4.1；HMS `level` 0/1/2→提示/警告/严重；5888 旧出口一期保留双发建议；客户端忽略未知字段前向兼容）；②`docker-compose.yml`（EMQX 5.8 默认 1883/8083/18083；postgis 16-3.4 `profile=trackstore` 默认不启用，供 WP6）；③`deploy/README.md`（启动与自环验证命令）；④`sim/README.md`（来源、开发/CI 定位、与契约 4 项差异：激光字段/rtk_number/state 话题非契约/sim 不发 `state`/`flightmode`）；⑤`sim/sim_telemetry.py` 原封收编（md5 一致）。静态检查已过（compose YAML 结构/sim 语法/契约样例 JSON 无激光字段）；**运行时验证（compose up + pub/sub 自环 + sim 订阅）挂起**：本 WSL 无 docker/mosquitto-clients/paho，需有 docker 的环境执行（命令见 `deploy/README.md`）。**§9-1 MQTT 库选型核查**：Qt 5.15.15（conda uxo-dev）无 QtMqtt 模块、apt 无 qtmqtt 包；apt 有 `libpaho-mqtt-dev`/`libpaho-mqttpp-dev` → 建议 **Paho MQTT C++**（纯 C++ 库，与 Qt 无 ABI 耦合），待用户确认裁决。全部产物仍未提交 git，待用户明确要求。
14. **deploy/ 撤并——仓库治理裁决（2026-10-10）**：用户指出顶层目录应按**种类**而非功能划分（"如果这个功能就要单开文件夹，以后很多工作都要这么单开"）。裁决撤销功能专属 `deploy/`：契约→`docs/interfaces/`（新增子目录，全系统外部接口契约唯一家，含 README 索引；docs/README.md 文档地图同步）、compose→仓根 `docker-compose.yml`（全系统中间件一份清单）、sim→`scripts/sim_telemetry.py`（仅一处适配：连接失败提示 compose 路径指向仓根）；原 `deploy/README.md` 与 `deploy/sim/README.md` 内容吸收为契约附录 A（模拟器差异）/附录 B（本地自环验证）。REQ-012、feature 设计（§2.1/§5/§8 回写清单/§9-5）、实现计划 WP1 路径引用全部同步；本记录第 9/13 条等历史表述保留不改，路径以本条为准。未来新接入归位规则：契约进 `docs/interfaces/`、工具进 `scripts/`、中间件进根 compose——不再新设功能专属顶层目录。

15. **§9-1 裁决落地与 WP1 运行时验证（2026-10-10）**：用户三项确认——①§9-1 采用 **Eclipse Paho MQTT C++**（feature §9-1、实现计划 WP3 同步为已裁决，WP3 转可动工）；②批准提交 git（分三笔：文档/部署物/计划）；③WP1 运行时验证在本 WSL 以**替代回路**完成：本机无 docker 且 sudo 需密码不可用，改用 pip 用户态 amqtt 0.12.1（纯 Python broker，127.0.0.1:1883）+ paho 订阅端，sim 以 speedup 10 跑完整任务自然退出（0)——**348 条 OSD**（契约★字段齐：经纬高/速度/姿态/mode_code/电池/position_state/89-0-0 云台俯仰与变焦）、**1 条 `device_exit_hms`**（QoS1，契约附录 A 既定差异）、**1 条网关 state 拓扑**（下挂 1 子设备），断言脚本全过；证据 `.omo/evidence/wp1-wsl-verify/`（gitignored，本地留档）。**遗留**：EMQX compose 启动验证仍待有 docker 的环境（命令见契约附录 B），不阻塞 WP3（客户端开发数据源可用 sim+amqtt 回路）。教训：首次验证因 `pkill -f "amqtt"` 自匹配命令行杀掉执行壳（无任何日志即超时），改为 `$!` 捕获 PID 显式 kill 后通过——长命令收尾禁用品名 pkill -f。
