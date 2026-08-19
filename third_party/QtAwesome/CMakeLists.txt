# 第三方库 QtAwesome（Font Awesome 字体图标）接入
# 来源: https://github.com/gamecreature/QtAwesome （v7.0.0.4，MIT 许可）
# 裁剪说明: 仅保留库源码与 Free 字体资源（QtAwesomeFree.qrc），
#           去除上游的示例程序、CPack、install、Pro 字体等逻辑
cmake_minimum_required(VERSION 3.16)

# 上游同时支持 Qt5/Qt6；本项目为 Qt5，复用父作用域已找到的 Qt5 组件
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 库目标：源码 + Free 字体资源（AUTORCC 处理 .qrc 内嵌字体）
add_library(QtAwesome
    QtAwesome/QtAwesome.cpp
    QtAwesome/QtAwesomeAnim.cpp
    QtAwesome/QtAwesomeFree.qrc
)

# 头文件以 INTERFACE 方式暴露（使用方 #include "QtAwesome.h"）
target_include_directories(QtAwesome INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/QtAwesome)

target_link_libraries(QtAwesome PUBLIC Qt5::Widgets)
