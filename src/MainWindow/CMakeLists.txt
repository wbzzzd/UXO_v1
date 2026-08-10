set(MAINWINDOW_SOURCES
    MainWindow.cpp
    StatusBarWidget.cpp
    LeftPanelWidget.cpp
    NavigationWidget.cpp
    VideoStreamPanel.cpp
    VideoOverlayWidget.cpp
    TacticalMapWidget.cpp
    DeviceResourceBar.cpp
    TargetDetailOverlay.cpp
)

set(MAINWINDOW_HEADERS
    ${HEADERS_DIR}/MainWindow/MainWindow.h
    ${HEADERS_DIR}/MainWindow/StatusBarWidget.h
    ${HEADERS_DIR}/MainWindow/LeftPanelWidget.h
    ${HEADERS_DIR}/MainWindow/NavigationWidget.h
    ${HEADERS_DIR}/MainWindow/VideoStreamPanel.h
    ${HEADERS_DIR}/MainWindow/VideoOverlayWidget.h
    ${HEADERS_DIR}/MainWindow/TacticalMapWidget.h
    ${HEADERS_DIR}/MainWindow/DeviceResourceBar.h
    ${HEADERS_DIR}/MainWindow/TargetDetailOverlay.h
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_library(MainWindow STATIC ${MAINWINDOW_SOURCES} ${MAINWINDOW_HEADERS})
target_include_directories(MainWindow PUBLIC ${HEADERS_DIR})
target_link_libraries(MainWindow 
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::Multimedia
    Qt5::MultimediaWidgets
    Qt5::3DCore
    Qt5::3DRender
    Qt5::3DExtras
    Common
    Core
)
