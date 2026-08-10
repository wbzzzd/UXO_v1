set(CORE_SOURCES
    AirportData.cpp
    3D/AirportSceneFactory.cpp
    Simulation/DemoScenarioProvider.cpp
    Simulation/SimulationWorkflow.cpp
    Simulation/DroneTelemetrySimulator.cpp
    Simulation/DetectionSimulator.cpp
    MOS/MosTypes.cpp
    MOS/MosValidation.cpp
    MOS/MosFixtureGenerator.cpp
    MOS/MosEstimator.cpp
    MOS/MosPlanner.cpp
    MOS/MosPlannerProgressive.cpp
    MOS/MosPlanningSession.cpp
)

# AUTOMOC 需要扫描 Q_OBJECT 宏的 header (DroneTelemetrySimulator / DetectionSimulator 派生自 QObject)
set(CORE_HEADERS
    ${HEADERS_DIR}/Core/Simulation/DroneTelemetrySimulator.h
    ${HEADERS_DIR}/Core/Simulation/DetectionSimulator.h
)

add_library(Core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(Core PUBLIC ${HEADERS_DIR})
target_link_libraries(Core Qt5::Core Qt5::Gui Qt5::3DCore Qt5::3DRender Qt5::3DExtras)
