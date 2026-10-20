# GPR 静态库 - MALA 探地雷达样例数据解析与阈值候选检测（REQ-013）
# 纯数据层：仅依赖 Qt5::Core，不含 Widgets 代码，解析与检测逻辑以纯数据测试覆盖。
# 设计基线：docs/features/gpr-display.md §5.2；格式与检测结论：§5.4（Python 预研已验证）。

add_library(GPR STATIC
    GprRadarHeader.cpp
    GprBScanData.cpp
    GprDataset.cpp
    GprTrack.cpp
    GprThresholdDetector.cpp
    ${CMAKE_SOURCE_DIR}/include/GPR/GprRadarHeader.h
    ${CMAKE_SOURCE_DIR}/include/GPR/GprBScanData.h
    ${CMAKE_SOURCE_DIR}/include/GPR/GprDataset.h
    ${CMAKE_SOURCE_DIR}/include/GPR/GprTrack.h
    ${CMAKE_SOURCE_DIR}/include/GPR/GprThresholdDetector.h
)

target_include_directories(GPR PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(GPR PUBLIC
    Qt5::Core
)

# 样例数据目录宏（与 DETECTION_ASSETS_DIR 同构）：开发期绝对路径注入，235MB 数据不入库；
# 可用 -DGPR_SAMPLE_DATA_DIR=... 在配置期覆盖；路径无效时雷达页进入降级态（见功能设计 §3.2）。
set(GPR_SAMPLE_DATA_DIR "/mnt/d/工作/排弹/数据集/001" CACHE PATH "GPR 样例数据目录（外部路径引用，不入库）")
target_compile_definitions(GPR PUBLIC
    GPR_SAMPLE_DATA_DIR="${GPR_SAMPLE_DATA_DIR}"
)
