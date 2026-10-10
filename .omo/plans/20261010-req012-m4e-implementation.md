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
- 状态：**VERIFIED**（2026-10-11）——§9-1 已裁决 Paho MQTT C++（用户确认，机制性闭环：apt .so 链 conda libstdc++ 6.0.34 向后兼容、预编译 .so 不受 `--no-relax` 影响；qtmqtt 源码构建为回退预案）；WP1 替代回路验证已过，sim+amqtt 可作开发数据源（EMQX 验证非阻塞）。**WP3 施工事实（2026-10-10）**：①接口契约已冻结 `include/Core/Uplink/`（TelemetryTypes/ITelemetrySource/TelemetrySourceManager 三头文件，通道不可改）；②Paho 免 sudo 解包至 `~/.local/opt/paho`（libpaho-mqtt 1.3.9 + mqttpp 1.2.0，`apt-get download`+`dpkg -x`；本地经 `-DCMAKE_PREFIX_PATH=~/.local/opt/paho/usr` 注入，repo 保持机器无关）；③worktree `third_party/onnxruntime/lib` symlink 至主仓（.gitignore 缺口修复）；④构建基线落盘：主仓源码独立构建（`/tmp/opencode/uxo-baseline-build`）编译 0 错误，ctest **15/17**，失败 #5 video_render_capture / #6 stop_select_flicker_repro（环境性：ONNX 检测产出为空，与 WP3 无关；**验收口径=不出现新失败**），证据 `/tmp/opencode/logs/wp3-baseline-ctest-summary.txt`；⑤实现双通道并行：通道 A=Core/Uplink+CMake、通道 B=MainWindow UI（文件组零交集，构建验证由主代理统一执行、错误按归属回灌）；cmake/ctest 用 conda env bin 全路径（4.4.0，不在 PATH）；⑥Paho 冒烟实证通过（2026-10-10）：apt 版 mqttpp3+3as+3a 在 conda libstdc++ 6.0.34 运行时下编译/链接/amqtt 收发回环全通（`CONNECT/ROUNDTRIP/DISCONNECT OK`，证据 `/tmp/opencode/paho-smoke/`）；**关键链接约束**：mqttpp3 NEEDED 依赖 `libpaho-mqtt3as.so.1`，而 DT_RUNPATH 不传递间接依赖 → **CMake 必须把 mqttpp3/3as/3a 三个库都 IMPORTED 并直接链接**（通道 A 交付时核查此项）；验证脚本已预置 `/tmp/opencode/wp3-verify-build.sh`（重配置+构建+ctest 一条龙）。**通道 A 交付（2026-10-11，ARTIFACT_READY）**：⑦三源实现（MqttUplinkClient/MqttTelemetrySource/TelemetrySourceManager，冻结三头文件未改）+模拟器重基底+CMake Paho 集成+telemetry_source_manager 无头单测（QTEST_GUILESS_MAIN，7 用例）全部落盘；验证证据：moc×6 + g++ -fsyntax-only（-fPIC）×5 全 OK（`/tmp/opencode/wp3-verify/`）；§9-1 三库链接约束交付核查通过（根 CMakeLists 三 IMPORTED 目标 + src/Core 链接行含 mqttpp3/3as/3a）；验证中修复 1 处缺陷（Q_ARG 宏内聚合初始化逗号分裂→具名局部变量）；构建+ctest 门禁留待主代理统一执行。**主代理构建验证（2026-10-11）**：⑧conda cmake 4.4.0 检测不到 multiarch 三元组（CMAKE_LIBRARY_ARCHITECTURE 为空）→ find_library 漏搜 `<prefix>/lib/x86_64-linux-gnu`，主代理集成修复：根 CMakeLists 三个 find_library 加 `PATH_SUFFIXES x86_64-linux-gnu`（纯追加候选路径，apt 安装与其他平台无副作用，mini-test 实证命中）；⑨全量构建 rc=0（`[100%] Built target UXOMissionControl`），ctest **16/18**，失败仅既有环境性 #5/#6（无新失败，验收口径达成），通道 A 新测试 #18 telemetry_source_manager Passed（0.62s），日志 `/tmp/opencode/logs/wp3-verify-build.log`；⑩sim 替代回路对接验证 **PASS**：/tmp 无头 harness 驱动真实 MqttTelemetrySource 对 amqtt+sim_telemetry.py（speedup=10 全航迹）正向断言——348 帧 OSD（与 WP1 注入数一致，零丢失）、`device_exit_hms` 告警 1 条（QoS1 events 通路）、链路状态机 Connecting→Connected 正常、经纬度/高度/航向全在预期域，证据 `/tmp/opencode/wp3-simloop/`（harness.log 判定明细）；⑪三视口截图验收 **PASS**（2026-10-11）：/tmp harness 实例化真实 MainWindow 离屏渲染（1280×720/1920×1080/3840×2160）——**720p 硬门禁通过**：地图工具栏单行无溢出无裁切，分段按钮「本地模拟/真实链路」完整可见，数据源徽标 [模拟] 位于地图左上；三视口布局均无重叠/错位；证据 `/tmp/opencode/wp3-shots/shot_{1280x720,1920x1080,3840x2160}.png`。**WP3 验收口径全部达成：构建 rc=0、ctest 无新失败（16/18，失败仅既有环境性 #5/#6）、sim 注入与客户端解析两侧打通、数据源互斥切换经 #18 单测覆盖、§6.6 UI 验收通过**

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
