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
