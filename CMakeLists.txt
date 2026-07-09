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
    WebSockets
    Sql
    Charts
)

find_package(ZeroMQ QUIET)
find_package(PostgreSQL QUIET)

set(HEADERS_DIR ${CMAKE_SOURCE_DIR}/include)
set(SOURCES_DIR ${CMAKE_SOURCE_DIR}/src)

include_directories(${HEADERS_DIR})

# 收集源文件
set(APP_SOURCES
    ${SOURCES_DIR}/App/Application.cpp
    ${SOURCES_DIR}/Core/Simulation/DemoScenarioProvider.cpp
    ${SOURCES_DIR}/MainWindow/MainWindow.cpp
    ${SOURCES_DIR}/MainWindow/SituationView.cpp
    ${SOURCES_DIR}/MainWindow/DeviceControlView.cpp
    ${SOURCES_DIR}/MainWindow/DecisionView.cpp
    ${SOURCES_DIR}/MainWindow/StatusBarWidget.cpp
)

set(SOURCES
    ${SOURCES_DIR}/App/main.cpp
    ${APP_SOURCES}
)

# 收集头文件
set(HEADERS
    ${HEADERS_DIR}/App/Application.h
    ${HEADERS_DIR}/Core/Simulation/DemoScenarioProvider.h
    ${HEADERS_DIR}/MainWindow/MainWindow.h
    ${HEADERS_DIR}/MainWindow/SituationView.h
    ${HEADERS_DIR}/MainWindow/DeviceControlView.h
    ${HEADERS_DIR}/MainWindow/DecisionView.h
    ${HEADERS_DIR}/MainWindow/StatusBarWidget.h
    ${HEADERS_DIR}/Core/Data/Types.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::3DCore
    Qt5::3DRender
    Qt5::3DInput
    Qt5::3DExtras
    Qt5::Network
    Qt5::WebSockets
    Qt5::Sql
    Qt5::Charts
)

if(ZeroMQ_FOUND)
    target_link_libraries(${PROJECT_NAME} ZeroMQ::ZeroMQ)
endif()

if(PostgreSQL_FOUND)
    target_link_libraries(${PROJECT_NAME} PostgreSQL::PostgreSQL)
endif()

enable_testing()

add_executable(UXOMissionControlStartupTest
    tests/startup_visible_test.cpp
    ${APP_SOURCES}
    ${HEADERS}
)

target_link_libraries(UXOMissionControlStartupTest
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::3DCore
    Qt5::3DRender
    Qt5::3DInput
    Qt5::3DExtras
    Qt5::Network
    Qt5::WebSockets
    Qt5::Sql
    Qt5::Charts
)

add_test(NAME startup_visible COMMAND UXOMissionControlStartupTest)
set_tests_properties(startup_visible PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(UXOMissionControlDemoScenarioTest
    tests/demo_scenario_provider_test.cpp
    ${SOURCES_DIR}/Core/Simulation/DemoScenarioProvider.cpp
    ${HEADERS_DIR}/Core/Simulation/DemoScenarioProvider.h
)

target_link_libraries(UXOMissionControlDemoScenarioTest
    Qt5::Core
    Qt5::Gui
)

add_test(NAME demo_scenario_provider COMMAND UXOMissionControlDemoScenarioTest)
