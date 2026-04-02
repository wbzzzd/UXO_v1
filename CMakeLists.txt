cmake_minimum_required(VERSION 3.16)
project(UXOMissionControl VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    3DCore
    3DRender
    3DInput
    3DExtras
    Network
    Sql
)

find_package(ZeroMQ QUIET)
find_package(PostgreSQL QUIET)

set(HEADERS_DIR ${CMAKE_SOURCE_DIR}/include)
set(SOURCES_DIR ${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src/Common)
add_subdirectory(src/Core)
add_subdirectory(src/MainWindow)
add_subdirectory(src/App)
