set(MAINWINDOW_SOURCES
    MainWindow.cpp
    SituationView.cpp
    DeviceControlView.cpp
    DecisionView.cpp
    StatusBarWidget.cpp
    LeftPanelWidget.cpp
    TargetDetailPanel.cpp
)

set(MAINWINDOW_HEADERS
    ${HEADERS_DIR}/MainWindow/MainWindow.h
    ${HEADERS_DIR}/MainWindow/SituationView.h
    ${HEADERS_DIR}/MainWindow/DeviceControlView.h
    ${HEADERS_DIR}/MainWindow/DecisionView.h
    ${HEADERS_DIR}/MainWindow/StatusBarWidget.h
    ${HEADERS_DIR}/MainWindow/LeftPanelWidget.h
    ${HEADERS_DIR}/MainWindow/TargetDetailPanel.h
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_library(MainWindow STATIC ${MAINWINDOW_SOURCES} ${MAINWINDOW_HEADERS})
target_include_directories(MainWindow PUBLIC ${HEADERS_DIR})
target_link_libraries(MainWindow 
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::3DCore
    Qt5::3DRender
    Qt5::3DExtras
    Common
)
