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

# 巡检员：随机点击 + 自洽性检查，独立进程循环运行。
# 默认关闭，不影响主程序和现有测试构建。需显式开启：
#   cmake -DBUILD_AUTO_INSPECT=ON ...
option(BUILD_AUTO_INSPECT "Build the auto inspector (random UI clicking + consistency checks)" OFF)
if(BUILD_AUTO_INSPECT)
    add_subdirectory(tests/auto_inspect)
endif()
