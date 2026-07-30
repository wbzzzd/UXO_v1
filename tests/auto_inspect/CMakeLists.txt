# 巡检员 worker 构建配置
# 独立于现有测试，由根 CMakeLists.txt 的 BUILD_AUTO_INSPECT 开关控制。

add_executable(inspector_worker
    inspector_worker.cpp
)

target_include_directories(inspector_worker PRIVATE ${HEADERS_DIR})

target_link_libraries(inspector_worker
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Test
    MainWindow
    Core
    Common
)

# 离屏运行配置，避免依赖显示服务
set_target_properties(inspector_worker PROPERTIES
    AUTOMOC ON
)
