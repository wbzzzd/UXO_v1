# 指挥席客户端开发与验证指南

版本：V0.2 草稿
状态：待评审

## 1. 文档作用

本文档只回答如何构建、运行、测试和判断任务完成。

- 产品需求与功能状态：`PRODUCT.md`
- 当前与目标架构：`ARCHITECTURE.md`
- 页面、交互和 UI 完成度：`UI.md`
- Agent 强制规则与安全边界：`AGENTS.md`

## 2. 当前工程环境

| 项 | 当前要求 |
|----|----------|
| 平台 | Linux；WSL2 + WSLg 已验证 |
| 构建系统 | CMake 3.16+ |
| 语言 | C++17 |
| Qt | Qt 5.15.15（`uxo-dev` micromamba 环境） |
| Qt 组件 | Core、Widgets、Gui、Concurrent、3DCore、3DRender、3DInput、3DExtras、Network、Sql、Test |
| 可选探测 | ZeroMQ、PostgreSQL；当前无目标链接或使用 |
| 数据平台实例 | 仅集成测试需要：本地数据平台（`127.0.0.1:8080` + 本地 postgres，UXO_DataPlatform 仓）；实例缺席时 `integration_vocabulary_cache`/`integration_data_platform_api`/`integration_contract` 整体 QSKIP |
| ONNX Runtime | 1.23.2；头文件随源码入库，预编译动态库 `libonnxruntime.so.1.23.2` 经 `.gitignore` 不入库，需先本地部署 `third_party/onnxruntime/` 下的动态库才能构建 |
| 中文字体 | Noto CJK 或等价字体 |

演示媒体资产已入库 `assets/media/`：卫星底图 `shenyang_yuhong_satellite.jpg` 与演示视频 `perth_airport_drone_uxo.mp4`，运行期由 MainWindow 按编译期源码树宏（`MEDIA_ASSETS_DIR`）优先、安装布局 `share/uxo/assets/media` 回退解析，与 AI 模型目录的解析方式一致。入库前转码参数：

- 卫星底图：原始 PNG（5376×4864 RGB，约 51MB）转 JPEG 质量 2，约 8.2MB。
- 演示视频：原始 MP4（1080p/30fps/96s，约 56MB）经 x264 CRF 26 / preset veryfast 转码并去除音轨（源无音轨），约 23MB。

原始未压缩素材保存在仓库外，不入库。

GPR 样例数据（MALA 格式，约 235MB）不入库：经编译期宏 `GPR_SAMPLE_DATA_DIR` 以开发期绝对路径注入（默认 `/mnt/d/工作/排弹/数据集/001`），与 `DETECTION_ASSETS_DIR` 模式一致。数据缺失时雷达页显示降级态；GPR 测试中依赖真实数据的用例自动跳过，合成 fixture 用例不受影响。

激活环境：

```bash
eval "$(/home/lin/.local/bin/micromamba shell hook --shell bash)"
micromamba activate uxo-dev
```

也可使用：

```bash
/home/lin/.local/bin/micromamba run -n uxo-dev <command>
```

## 3. 标准构建与运行

从仓库根目录执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

产物：

```text
build/src/App/UXOMissionControl
```

运行：

```bash
./build/src/App/UXOMissionControl
```

WSLg 演示可使用：

```bash
./scripts/run_demo.sh xcb
```

`scripts/build.sh` 会直接调用 `make`，末尾包含交互式安装询问，不作为 agent 自动验证入口。

## 4. 测试

执行全部测试：

```bash
ctest --test-dir build --output-on-failure
```

当前测试：

| CTest | 证明的能力 | 不证明的能力 |
|-------|------------|--------------|
| `startup_visible` | 主窗口可见、标题正确、模拟模式标识可见 | 配置、日志、数据库、通信初始化有效 |
| `demo_scenario_provider` | 固定场景含 0 目标（空起步）、1 任务、至少 2 设备、无人机航线、检测数据、机场边界 | 多场景、文件加载、数据变化 |
| `simulation_workflow` | 目标选择、四状态顺序、拒绝路径、日志顺序和重置 | 任务、设备、指派、持久化 |
| `simulation_workflow_ui` | 目标表、流程面板、决策状态标签与日志同步；目标表与 2D 战术地图双向高亮 | 真实视频文件渲染；3D 渲染结果；导航；任务分配；设备联动；状态栏紧急停止 |
| `video_render_capture` | XCB 实际窗口中视频帧推进、侧栏目标选择、结束后回到 0s 并保留目标状态 | 其他显示后端；真实设备视频 |
| `stop_select_flicker_repro` | XCB 实际窗口中复现态势页地图工具栏 [开始]/[结束]/[重置] 与左表/地图/探测页三向选中联动（退出码 0=安静、1=事件风暴） | 正式回归断言与 CI 稳定性（临时调试工具） |
| `mos_validation` | MOS P0 输入包络规则（L/W/K/expand/step/minLength/minWidth/backfill/uxoHours/tiers 与弹坑、UXO 范围）的合法/非法/边界判定 | 真实业务阈值、真实安全边界、领域参数选型 |
| `mos_fixture` | 确定性 fixture 生成与合成估算在五个种子向量下与冻结 oracle 一致 | 真实损毁分布、真实修复参数、外部数据 |
| `mos_planner` | MOS 规划器任务规划流程与输出契约（简化 X 全投影 + Y 固定全宽算法） | 完整 Y 轴 1m 离散化算法、斜向搜索 |
| `mos_session` | MOS 规划会话编排与 MainWindow 控制器契约 | 跨线程并发、持久化、外部通信 |
| `mos_decision_view` | 决策页 DEC-* 稳定对象名与被动信号契约（离屏） | 真实交互回流、3D 渲染、外部数据 |
| `mos_decision_ui` | 通过 MainWindow 外壳验证页面栈、导航与决策页接线（happy/invalid/no-solution/no-output/route-regression） | 外壳未接线的场景、真实外部数据、3D 渲染 |
| `mos_performance` | 5 个测量进程（100 预热 + 1000 样本）、100 个哈希进程、点击到渲染、10ms 心跳、内存上限与陈旧/重复/丢失断言 | 真实安全时限（数值为合成本地 fixture 语义） |
| `mos_contract_closure` | 浮点生成器包络、规范字节与可见半径估算闭合 | 真实业务合同、外部存储 |
| `mos_planner_closure` | 独立穷举 oracle、supplied-tier 校验与完成结果重算守卫 | 完整 Y 轴离散化算法、斜向搜索 |
| `mos_ui_closure` | 生成器浮点半径、合成参数措辞与 controller-owned 导出 | 真实导出目标、外部存储、网络 |
| `mos_runway_closure` | 快照参数化几何、缩放/平移、选中档位单方案命中，以及 `influenceRadius × pxPerM` 影响圆半径契约 | 真实跑道几何、3D 渲染、真实安全距离 |
| `gpr_header` | `.rad` 采集参数键值解析与两程时间深度换算 | 未解码字段语义、真实设备全参数 |
| `gpr_bscan` | `.rd3` 整读预载（常驻断言：预载后删源文件仍可访问）与未预载懒加载回退、逐通道 t0 校正（合成 fixture + 真实数据 smoke 与预载性能阈值） | 全数据集整体性能；真实数据用例在无数据环境自动跳过 |
| `gpr_dataset` | 数据目录扫描聚合 H/L × 16 通道清单与缺失通道报告 | 真实目录异常全形态 |
| `gpr_track` | `.cor` RTK 解析、道 1:1 对齐、缺行插值与高斯平滑轨迹点查询 | 真实定位精度、多趟标定 |
| `gpr_detector` | 阈值候选检测管线（合成 B-Scan 命中 + 真实数据 smoke） | 目标类型识别（输出仅为候选，人工复核必需） |
| `gpr_view` | 雷达页稳定对象名、频段/通道切换、降级态、疑似列表确认/误报处置与本地移除、对比度 120ms 防抖（离屏） | 渲染像素正确性、人工集成走查 |
| `integration_api_error` | API 错误信封解析：10 个契约错误码映射、field/code 提取、多错误聚合保留、未知码兜底 | 真实服务端错误触发 |
| `integration_http_client` | HTTP 基址解析（默认回环 127.0.0.1:8080、环境变量覆盖）与 GET/POST/PUT 构造、超时/网络错误映射；非回环基址用例为纯函数夹具，不发真实请求 | 真实服务端业务语义 |
| `integration_entity_mapping` | 五实体 Core 类型与契约 JSON 双向映射、枚举越界拒发、必填字段完整性 | 服务端校验结果、真实持久化 |
| `integration_vocabulary_cache` | 九词表拉取与缓存、未知名称与服务不可用的错误传播（回环负路径） | 词表数据正典内容 |
| `integration_data_platform_api` | 五实体 CRUD/分页/过滤对真实本地实例往返、健康检查 200 路径；pagination_invalid/duplicate_id/foreign_key_violation/not_found 四错误码实况断言 | 破坏性场景、并发、多实例 |
| `integration_contract` | 契约过滤与错误语义对真实实例符合性门禁：invalid_enum/out_of_range/not_null/invalid_format 四码实况断言、错误聚合保留、404 field 变体、PUT 路径与体 id 错配、类型严格性、健康 200 原始形状 | internal_error 实况触发（无可靠触发器） |

测试通过只能证明表中列出的行为，不能用于声明整个客户端或某个未覆盖面板已完成。离屏 UI 测试中的 Qt3D 上下文警告不影响当前断言，因此全部测试通过也不证明三维场景成功渲染。`video_render_capture` 与 `stop_select_flicker_repro` 必须在 XCB 和可见显示环境中运行。

`integration_vocabulary_cache`、`integration_data_platform_api` 与 `integration_contract` 需要本地数据平台实例（`127.0.0.1:8080` + 本地 postgres）正在运行；实例缺席时这些套件整体 QSKIP，全量通过不代表其断言已执行，需直接运行对应测试二进制确认 `Totals` 行无 skipped。2026-10-12 已在监督下完成一次性破坏性实测（pg_ctl 停库约 66 秒）：健康检查 503 专属形状（`{"status":"unavailable","database":"unreachable"}`）与 `database_unavailable`（503，读/写路径）对真实实例断言通过，与契约 §3/§8 文档值逐字一致；`internal_error`（500）无可靠触发器，仅由 `integration_api_error` 解析层覆盖。

## 5. 当前构建目标

```text
UXOMissionControl executable
  -> MainWindow static library
     -> Core static library
     -> Detection static library
     -> Common static library
```

- `src/App`：应用入口和生命周期。
- `src/MainWindow`：Qt UI 组合与交互。
- `src/Detection`：ONNX 双阶段推理引擎（PatchCore 异常检测 + YOLOv8-cls 分类），链接 ONNX Runtime 与 `Qt5::Concurrent`。
- `src/Core`：数据类型、模拟工作流和 3D 场景数据。
- `src/Common`：全局样式与当前未接入的 MockDataGenerator。

真实模块关系以 `ARCHITECTURE.md` 和各级 `CMakeLists.txt` 为准。

## 6. 开发规则

- 功能开发必须关联 `docs/requirements/` 中已批准的需求 ID。
- 新功能必须先有状态为 `Approved` 的 `docs/features/<feature>.md`。
- Core 不依赖 MainWindow；UI 不自行成为业务状态权威。
- 代码注释使用中文。
- 模拟实现使用 `Mock`、`Simulation` 或 `Demo` 命名，并在 UI 中标明模拟语义。
- 不为没有调用方的未来模块创建空接口或目录。
- 不修改与当前任务无关的代码和用户改动。
- 真实设备控制和真实安全执行链不属于本仓库交付范围，普通任务授权不能解除该边界。未来如需客户端接入外部通信或存储，只能先批准最小适配边界；客户端不得绕过外部安全系统直接执行真实动作。

### 6.1 Worktree 与主分支合并门禁

- 每个 agent 只在自己负责的 worktree 和分支中修改、测试、提交；不得在共享 `main` worktree 代替其他 agent 整理、提交或合并其工作。
- “检查完成”“继续工作”或其他一般性确认，不等于授权将其他分支合并到 `main`；合并授权必须明确指向具体分支和合并动作。
- 各分支完成后先推送自己的远端分支，由 GitHub Pull Request 作为进入 `main` 的唯一入口；必须先经过变更审核和必要的检查，再由用户或已授权的集成人员在远端执行合并。
- 禁止通过本地 `git merge` 后直接 `git push origin main` 绕过 Pull Request 审核。若只是本地验证合并结果，必须使用临时集成 worktree 或临时分支，且不得把该结果推送到 `main`。
- 分支合并到 `main` 后，其他仍在进行的分支必须继续保留自己的 worktree、index 和未提交文件；不得使用 `reset`、`clean`、全局 stash 或覆盖式恢复清理这些工作。
- 发现职责范围或合并目标不明确时，停止合并操作并询问，不得根据自己提出的集成顺序推断授权。

## 7. 质量门禁

当前可执行门禁：

- CMake 配置成功。
- `UXOMissionControl` 目标构建成功。
- 受影响的 CTest 全部通过。
- `git diff --check` 无空白错误。
- 暂存内容不包含 `.env`、明文密钥或凭据。
- UI 修改完成对应分辨率的实际运行和视觉检查。

当前尚未具备：

- CI 流水线。
- 覆盖率门禁。
- clang-tidy/cppcheck 门禁。
- CPack 或其他版本化发布包。
- 自动生成的需求追踪报告。

Agent 不得声称尚未具备的门禁已经存在。

## 8. 完成定义

任务完成必须同时满足：

1. 实现结果符合批准需求和功能设计，不以现有代码反向修改需求。
2. 相关构建和测试通过；新增行为有成功与失败场景测试。
3. 实际运行入口已验证，不以类存在、信号存在或测试编译通过代替功能验证。
4. 未违反模拟与真实接入安全边界。
5. 更新受影响的核心文档；工作记录只进入 `.omo/plans/` 或会话记录。
6. Git 差异只包含当前任务内容，且通过空白与密钥检查。
7. 最终报告明确说明已验证内容、未验证内容和遗留风险。

## 9. 文档读取路由

Agent 默认只加载 `AGENTS.md`，按任务读取相关文档：

| 任务 | 必读文档 |
|------|----------|
| 产品、需求、路线 | `PRODUCT.md` |
| 模块、状态、业务逻辑 | 相关需求 + `ARCHITECTURE.md` + 本文档 |
| 页面与交互 | 相关需求 + `UI.md` + 本文档 |
| 构建、测试、发布 | 本文档 |
| 已批准功能 | 对应 `features/<feature>.md` 及其明确引用章节 |

`docs/archive/`（含 `development/` 已归档工程快照）、`docs/research/`、旧 SRS/SDD/UI 和 `docs/sources/`（Draft 设计输入与来源资料）只在追溯来源或核验依据时按需读取。

## 10. 发布状态

当前仓库只有本地构建产物，没有版本化发布物：

- 无 CPack 或安装包。
- 无 CI 构建产物归档。
- 可执行文件没有自动注入 Git 版本。
- `scripts/build.sh` 的交互式 `sudo make install` 不等同于发布流程。

在发布要求得到批准前，不把本地 `build/` 产物描述为正式发布版本。
