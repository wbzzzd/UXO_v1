set(CORE_SOURCES
    AirportData.cpp
    3D/AirportSceneFactory.cpp
    Simulation/DemoScenarioProvider.cpp
    Simulation/SimulationWorkflow.cpp
    Simulation/DroneTelemetrySimulator.cpp
    Simulation/DetectionSimulator.cpp
    Uplink/MqttUplinkClient.cpp
    Uplink/MqttTelemetrySource.cpp
    Uplink/TelemetrySourceManager.cpp
    MOS/MosTypes.cpp
    MOS/MosValidation.cpp
    MOS/MosFixtureGenerator.cpp
    MOS/MosEstimator.cpp
    MOS/MosPlanner.cpp
    MOS/MosPlannerProgressive.cpp
    MOS/MosPlanningSession.cpp
    TrackStore/TrackStoreMapping.cpp
    TrackStore/TrackStore.cpp
)

# AUTOMOC 需要扫描 Q_OBJECT 宏的 header（Simulation/Uplink 各遥测源均派生自 QObject；
# TelemetryTypes.h 与 TrackStoreConfig.h 为纯值类型无 Q_OBJECT，不列入）
set(CORE_HEADERS
    ${HEADERS_DIR}/Core/Simulation/DroneTelemetrySimulator.h
    ${HEADERS_DIR}/Core/Simulation/DetectionSimulator.h
    ${HEADERS_DIR}/Core/Uplink/ITelemetrySource.h
    ${HEADERS_DIR}/Core/Uplink/MqttTelemetrySource.h
    ${HEADERS_DIR}/Core/Uplink/MqttUplinkClient.h
    ${HEADERS_DIR}/Core/Uplink/TelemetrySourceManager.h
    ${HEADERS_DIR}/Core/TrackStore/TrackStoreMapping.h
    ${HEADERS_DIR}/Core/TrackStore/TrackStore.h
)

add_library(Core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(Core PUBLIC ${HEADERS_DIR})
# Paho 三个库直接链接：mqttpp3 NEEDED 依赖 libpaho-mqtt3as.so.1，而 DT_RUNPATH
# 不传递间接依赖，故 3a/3as/mqttpp3 均需出现在 Core 的链接行（WP3 核查结论）
target_link_libraries(Core Qt5::Core Qt5::Gui Qt5::Sql Qt5::3DCore Qt5::3DRender Qt5::3DExtras
    paho-mqttpp3 paho-mqtt3as paho-mqtt3a)
