set(APP_SOURCES
    main.cpp
    Application.cpp
)

set(APP_HEADERS
    ${HEADERS_DIR}/App/Application.h
)

set(CMAKE_AUTOMOC ON)

add_executable(UXOMissionControl ${APP_SOURCES} ${APP_HEADERS})
target_include_directories(UXOMissionControl PRIVATE ${HEADERS_DIR})
target_link_libraries(UXOMissionControl 
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::3DCore
    Qt5::3DRender
    Qt5::3DExtras
    Qt5::3DInput
    MainWindow
)

# 安装主程序到 bin/（onnxruntime 库与模型资产的安装规则见根 CMakeLists）
install(TARGETS UXOMissionControl RUNTIME DESTINATION bin)
