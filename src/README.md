# 子模块CMake配置模板
# 每个子模块都需要包含:
# 1. 自身的CMakeLists.txt
# 2. 至少一个基础类文件

# 示例: src/ModuleName/CMakeLists.txt
# set(ModuleName_SOURCES
#     ModuleName.cpp
#     ModuleClass1.cpp
#     ModuleClass2.cpp
# )
#
# add_library(ModuleName MODULE ${ModuleName_SOURCES})
# target_link_libraries(ModuleName PRIVATE Qt5::Core Qt5::Widgets)
