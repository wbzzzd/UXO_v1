set(CORE_SOURCES
    AirportData.cpp
    3D/AirportSceneFactory.cpp
    Simulation/DemoScenarioProvider.cpp
)

add_library(Core STATIC ${CORE_SOURCES})
target_include_directories(Core PUBLIC ${HEADERS_DIR})
target_link_libraries(Core Qt5::Core Qt5::Gui Qt5::3DCore Qt5::3DRender Qt5::3DExtras)