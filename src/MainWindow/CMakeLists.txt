set(MAINWINDOW_SOURCES
    MainWindow.cpp
    SituationView.cpp
    DeviceControlView.cpp
    DecisionView.cpp
    DecisionViewLayout.cpp
    DecisionViewSnapshot.cpp
    DecisionViewTier.cpp
    StatusBarWidget.cpp
    LeftPanelWidget.cpp
    TargetDetailPanel.cpp
    NavigationWidget.cpp
    VideoStreamPanel.cpp
    RightPanelWidget.cpp
    DeviceStatusPanel.cpp
    DecisionSuggestionPanel.cpp
    AlertPanel.cpp
    DetectionControlPanel.cpp
    BatchOperationBar.cpp
    MosPlanningController.cpp
    MosRunwayWidget.cpp
    MosRunwayWidgetInteraction.cpp
    MosParamsPanel.cpp
    MosGeneratorDialog.cpp
    TargetCardWidget.cpp
    PlanCardWidget.cpp
)

set(MAINWINDOW_HEADERS
    ${HEADERS_DIR}/MainWindow/MainWindow.h
    ${HEADERS_DIR}/MainWindow/SituationView.h
    ${HEADERS_DIR}/MainWindow/DeviceControlView.h
    ${HEADERS_DIR}/MainWindow/DecisionView.h
    ${HEADERS_DIR}/MainWindow/StatusBarWidget.h
    ${HEADERS_DIR}/MainWindow/LeftPanelWidget.h
    ${HEADERS_DIR}/MainWindow/TargetDetailPanel.h
    ${HEADERS_DIR}/MainWindow/NavigationWidget.h
    ${HEADERS_DIR}/MainWindow/VideoStreamPanel.h
    ${HEADERS_DIR}/MainWindow/RightPanelWidget.h
    ${HEADERS_DIR}/MainWindow/DeviceStatusPanel.h
    ${HEADERS_DIR}/MainWindow/DecisionSuggestionPanel.h
    ${HEADERS_DIR}/MainWindow/AlertPanel.h
    ${HEADERS_DIR}/MainWindow/DetectionControlPanel.h
    ${HEADERS_DIR}/MainWindow/BatchOperationBar.h
    ${HEADERS_DIR}/MainWindow/MosPlanningController.h
    ${HEADERS_DIR}/MainWindow/MosRunwayWidget.h
    ${HEADERS_DIR}/MainWindow/MosParamsPanel.h
    ${HEADERS_DIR}/MainWindow/MosGeneratorDialog.h
    ${HEADERS_DIR}/MainWindow/TargetCardWidget.h
    ${HEADERS_DIR}/MainWindow/PlanCardWidget.h
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
    Core
)
