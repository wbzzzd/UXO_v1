set(MAINWINDOW_SOURCES
    MainWindow.cpp
    DecisionView.cpp
    DetectionView.cpp
    DecisionViewLayout.cpp
    DecisionViewSnapshot.cpp
    DecisionViewTier.cpp
    StatusBarWidget.cpp
    LeftPanelWidget.cpp
    NavigationWidget.cpp
    VideoStreamPanel.cpp
    VideoOverlayWidget.cpp
    TacticalMapWidget.cpp
    DeviceResourceBar.cpp
    TargetDetailOverlay.cpp
    AlertPanel.cpp
    MosPlanningController.cpp
    MosRunwayWidget.cpp
    MosRunwayWidgetInteraction.cpp
    MosParamsPanel.cpp
    MosGeneratorDialog.cpp
    TargetCardWidget.cpp
    PlanCardWidget.cpp
    UiIcons.cpp
)

set(MAINWINDOW_HEADERS
    ${HEADERS_DIR}/MainWindow/MainWindow.h
    ${HEADERS_DIR}/MainWindow/DecisionView.h
    ${HEADERS_DIR}/MainWindow/DetectionView.h
    ${HEADERS_DIR}/MainWindow/StatusBarWidget.h
    ${HEADERS_DIR}/MainWindow/LeftPanelWidget.h
    ${HEADERS_DIR}/MainWindow/NavigationWidget.h
    ${HEADERS_DIR}/MainWindow/VideoStreamPanel.h
    ${HEADERS_DIR}/MainWindow/VideoOverlayWidget.h
    ${HEADERS_DIR}/MainWindow/TacticalMapWidget.h
    ${HEADERS_DIR}/MainWindow/DeviceResourceBar.h
    ${HEADERS_DIR}/MainWindow/TargetDetailOverlay.h
    ${HEADERS_DIR}/MainWindow/AlertPanel.h
    ${HEADERS_DIR}/MainWindow/MosPlanningController.h
    ${HEADERS_DIR}/MainWindow/MosRunwayWidget.h
    ${HEADERS_DIR}/MainWindow/MosParamsPanel.h
    ${HEADERS_DIR}/MainWindow/MosGeneratorDialog.h
    ${HEADERS_DIR}/MainWindow/TargetCardWidget.h
    ${HEADERS_DIR}/MainWindow/PlanCardWidget.h
    ${HEADERS_DIR}/MainWindow/UiIcons.h
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
    QtAwesome
    Detection
)
