cmake_minimum_required(VERSION 3.16)
project(UXOMissionControl VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    3DCore
    3DRender
    3DInput
    3DExtras
    Network
    Sql
    Test
    Multimedia
    MultimediaWidgets
)

find_package(ZeroMQ QUIET)
find_package(PostgreSQL QUIET)

set(HEADERS_DIR ${CMAKE_SOURCE_DIR}/include)
set(SOURCES_DIR ${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src/Common)
add_subdirectory(src/Core)
add_subdirectory(src/MainWindow)
add_subdirectory(src/App)

# 测试配置
enable_testing()

# 启动可见性测试：验证主窗口标题和可见性
add_executable(UXOMissionControlStartupTest
    tests/startup_visible_test.cpp
    ${SOURCES_DIR}/App/Application.cpp
    ${HEADERS_DIR}/App/Application.h
)
set_target_properties(UXOMissionControlStartupTest PROPERTIES AUTOMOC ON)
target_include_directories(UXOMissionControlStartupTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlStartupTest
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    MainWindow
    Core
    Common
)
add_test(NAME startup_visible COMMAND UXOMissionControlStartupTest)
set_tests_properties(startup_visible PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# 模拟场景测试：验证演示场景数据完整性
add_executable(UXOMissionControlDemoScenarioTest
    tests/demo_scenario_provider_test.cpp
    ${SOURCES_DIR}/Core/Simulation/DemoScenarioProvider.cpp
)
target_include_directories(UXOMissionControlDemoScenarioTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlDemoScenarioTest
    Qt5::Core
    Qt5::Gui
)
add_test(NAME demo_scenario_provider COMMAND UXOMissionControlDemoScenarioTest)

# 模拟工作流测试：验证内存状态流转、拒绝操作和日志顺序
add_executable(UXOMissionControlSimulationWorkflowTest
    tests/simulation_workflow_test.cpp
)
target_include_directories(UXOMissionControlSimulationWorkflowTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlSimulationWorkflowTest
    Qt5::Core
    Qt5::Gui
    Qt5::Test
    Core
)
add_test(NAME simulation_workflow COMMAND UXOMissionControlSimulationWorkflowTest)

# 模拟工作流 UI 契约测试：使用离屏平台验证阶段 4 的本地模拟交互
add_executable(UXOMissionControlSimulationWorkflowUiTest
    tests/simulation_workflow_ui_test.cpp
)
target_include_directories(UXOMissionControlSimulationWorkflowUiTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlSimulationWorkflowUiTest
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Test
    MainWindow
    Core
    Common
)
add_test(NAME simulation_workflow_ui COMMAND UXOMissionControlSimulationWorkflowUiTest)
set_tests_properties(simulation_workflow_ui PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_OPENGL=software;LIBGL_ALWAYS_SOFTWARE=1"
)

# 真实 XCB 行为验证：视频渲染、证据细节叠加层状态与结束/重置回退。
add_executable(UXOMissionControlVideoRenderCaptureTest
    tests/video_render_capture_test.cpp
)
target_include_directories(UXOMissionControlVideoRenderCaptureTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlVideoRenderCaptureTest
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Test
    MainWindow
    Core
    Common
)
if(DEFINED ENV{DISPLAY} AND NOT "$ENV{DISPLAY}" STREQUAL "")
    add_test(NAME video_render_capture COMMAND UXOMissionControlVideoRenderCaptureTest)
    set_tests_properties(video_render_capture PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()

# 巡检员：随机点击 + 自洽性检查，独立进程循环运行。
# 默认关闭，不影响主程序和现有测试构建。需显式开启：
#   cmake -DBUILD_AUTO_INSPECT=ON ...
option(BUILD_AUTO_INSPECT "Build the auto inspector (random UI clicking + consistency checks)" OFF)
if(BUILD_AUTO_INSPECT)
    add_subdirectory(tests/auto_inspect)
endif()

# MOS P0 输入包络校验测试：覆盖每条包络规则的合法/非法/边界用例
add_executable(UXOMissionControlMosValidationTest
    tests/mos_validation_test.cpp
)
target_include_directories(UXOMissionControlMosValidationTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlMosValidationTest
    Qt5::Core
    Qt5::Test
    Core
)
add_test(NAME mos_validation COMMAND UXOMissionControlMosValidationTest)

# MOS P0 确定性生成器与合成估算测试：五个种子向量与冻结 oracle 比对
add_executable(UXOMissionControlMosFixtureTest
    tests/mos_fixture_test.cpp
)
target_include_directories(UXOMissionControlMosFixtureTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlMosFixtureTest
    Qt5::Core
    Qt5::Test
    Core
)
add_test(NAME mos_fixture COMMAND UXOMissionControlMosFixtureTest)

# MOS P0 规划器测试：验证任务规划流程与输出契约
add_executable(UXOMissionControlMosPlannerTest
    tests/mos_planner_test.cpp
)
target_include_directories(UXOMissionControlMosPlannerTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlMosPlannerTest
    Qt5::Core
    Qt5::Test
    Core
)
add_test(NAME mos_planner COMMAND UXOMissionControlMosPlannerTest)

# MOS 规划会话控制器测试：验证会话编排与 MainWindow 控制器契约
add_executable(UXOMissionControlMosSessionTest
    tests/mos_session_test.cpp
)
target_include_directories(UXOMissionControlMosSessionTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlMosSessionTest
    Qt5::Core
    Qt5::Widgets
    Qt5::Test
    MainWindow
    Core
    Common
)
add_test(NAME mos_session COMMAND UXOMissionControlMosSessionTest)

# MOS 决策页离屏 UI 契约测试：验证 DEC-* 稳定对象名与被动信号
add_executable(UXOMissionControlMosDecisionViewTest
    tests/mos_decision_view_test.cpp
)
target_include_directories(UXOMissionControlMosDecisionViewTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlMosDecisionViewTest
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Test
    MainWindow
    Core
    Common
)
add_test(NAME mos_decision_view COMMAND UXOMissionControlMosDecisionViewTest)
set_tests_properties(mos_decision_view PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_OPENGL=software;LIBGL_ALWAYS_SOFTWARE=1"
)

# MOS 决策页集成 UI 契约测试：通过 MainWindow 外壳验证页面栈、导航与决策页接线
# 外壳未接线前每个场景在 shell 校验步即失败（failing-first TDD）。
# 通过 MOS_UI_SCENARIO 选择 happy|invalid|no-solution|no-output|route-regression。
add_executable(UXOMissionControlMosDecisionUiTest
    tests/mos_decision_ui_test.cpp
)
target_include_directories(UXOMissionControlMosDecisionUiTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlMosDecisionUiTest
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Test
    MainWindow
    Core
    Common
)
add_test(NAME mos_decision_ui COMMAND UXOMissionControlMosDecisionUiTest)
set_tests_properties(mos_decision_ui PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_OPENGL=software;LIBGL_ALWAYS_SOFTWARE=1"
)

# MOS P0 Release 性能/确定性门禁测试：5 个全新测量进程（共 100 预热 + 1000 样本）、
# 100 个全新哈希进程、点击到渲染、10ms 心跳、内存上限与陈旧/重复/丢失断言。
# 通过 --scenario=happy|stale-revision|threshold-regression|seed-mismatch|no-solution 选择场景，
# --output <json> 写机器可读证据，--max-p99-ms <value> 注入 p99 阈值；私有子进程模式由
# --child-measure=W:S / --child-hash=N 触发。所有数值为合成本地 fixture 语义，非真实安全时限。
add_executable(UXOMissionControlMosPerformanceTest
    tests/mos_performance_test.cpp
    tests/mos_performance_scenarios.cpp
)
target_include_directories(UXOMissionControlMosPerformanceTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlMosPerformanceTest
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Test
    MainWindow
    Core
    Common
)
# 捕获 git 短提交供环境元数据记录（失败回退 unknown）
execute_process(COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE MOS_GIT_COMMIT_VAR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(NOT MOS_GIT_COMMIT_VAR)
    set(MOS_GIT_COMMIT_VAR "unknown")
endif()
# 传入构建类型与 git 提交供环境元数据记录
target_compile_definitions(UXOMissionControlMosPerformanceTest PRIVATE
    MOS_BUILD_TYPE="$<CONFIG>"
    MOS_GIT_COMMIT="${MOS_GIT_COMMIT_VAR}"
)
add_test(NAME mos_performance COMMAND UXOMissionControlMosPerformanceTest)
set_tests_properties(mos_performance PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_OPENGL=software;LIBGL_ALWAYS_SOFTWARE=1"
    TIMEOUT 300
)

# MOS P0 最终合同闭合测试：浮点生成器包络、规范字节与可见半径估算。
add_executable(UXOMissionControlMosContractClosureTest
    tests/mos_contract_closure_test.cpp
)
target_include_directories(UXOMissionControlMosContractClosureTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlMosContractClosureTest
    Qt5::Core
    Qt5::Test
    Core
)
add_test(NAME mos_contract_closure COMMAND UXOMissionControlMosContractClosureTest)

# MOS P0 规划器闭合测试：独立穷举 oracle、supplied-tier 校验与完成结果重算守卫。
add_executable(UXOMissionControlMosPlannerClosureTest
    tests/mos_planner_closure_test.cpp
)
target_include_directories(UXOMissionControlMosPlannerClosureTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlMosPlannerClosureTest
    Qt5::Core
    Qt5::Widgets
    Qt5::Test
    MainWindow
    Core
    Common
)
add_test(NAME mos_planner_closure COMMAND UXOMissionControlMosPlannerClosureTest)

# MOS P0 UI 闭合测试：生成器浮点半径、合成参数措辞与 controller-owned 导出。
add_executable(UXOMissionControlMosUiClosureTest
    tests/mos_ui_closure_test.cpp
)
target_include_directories(UXOMissionControlMosUiClosureTest PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControlMosUiClosureTest
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Test
    MainWindow
    Core
    Common
)
add_test(NAME mos_ui_closure COMMAND UXOMissionControlMosUiClosureTest)
set_tests_properties(mos_ui_closure PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_OPENGL=software;LIBGL_ALWAYS_SOFTWARE=1"
)

# 截图工具：用 offscreen 平台渲染 DecisionView 并保存 PNG，用于与 HTML 原型对比。
# 不进入 CTest，仅手动构建运行：cmake --build build --target ScreenshotTool
add_executable(ScreenshotTool
    tests/screenshot_tool.cpp
)
target_include_directories(ScreenshotTool PRIVATE ${HEADERS_DIR})
target_link_libraries(ScreenshotTool
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    MainWindow
    Core
    Common
)
set_target_properties(ScreenshotTool PROPERTIES AUTOMOC ON)

# Widget 树导出工具：输出所有子控件的几何/颜色/字体/样式表为文本，用于代码级视觉对比。
# 不进入 CTest，仅手动构建运行：cmake --build build --target WidgetDump
add_executable(WidgetDump
    tests/widget_dump.cpp
)
target_include_directories(WidgetDump PRIVATE ${HEADERS_DIR})
target_link_libraries(WidgetDump
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    MainWindow
    Core
    Common
)
set_target_properties(WidgetDump PROPERTIES AUTOMOC ON)

# MOS P0 跑道闭合测试：快照参数化几何、实际缩放/平移与变换后命中测试。
add_executable(UXOMissionControlMosRunwayClosureTest
    tests/mos_runway_closure_test.cpp
)
target_include_directories(UXOMissionControlMosRunwayClosureTest PRIVATE
    ${HEADERS_DIR}
    ${CMAKE_SOURCE_DIR}/src/MainWindow
)
target_link_libraries(UXOMissionControlMosRunwayClosureTest
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Test
    MainWindow
    Core
    Common
)
add_test(NAME mos_runway_closure COMMAND UXOMissionControlMosRunwayClosureTest)
set_tests_properties(mos_runway_closure PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_OPENGL=software;LIBGL_ALWAYS_SOFTWARE=1"
)
