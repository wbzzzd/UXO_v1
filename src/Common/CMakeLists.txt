set(COMMON_SOURCES
    MockDataGenerator.cpp
)

add_library(Common STATIC ${COMMON_SOURCES})
target_include_directories(Common PUBLIC ${HEADERS_DIR})
target_link_libraries(Common Qt5::Core Qt5::Gui)
