# Detection 静态库 - 基于 ONNX 的 UXO 检测（PatchCore + YOLOv8-cls）

# ---- ONNX Runtime IMPORTED target ----
set(ONNXRUNTIME_ROOT "${CMAKE_SOURCE_DIR}/third_party/onnxruntime")

add_library(onnxruntime SHARED IMPORTED)
set_target_properties(onnxruntime PROPERTIES
    IMPORTED_LOCATION "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so.1.23.2"
    INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_ROOT}/include"
    INTERFACE_LINK_DIRECTORIES "${ONNXRUNTIME_ROOT}/lib"
)

# ---- Detection 静态库 ----
set(CMAKE_AUTOMOC ON)

add_library(Detection STATIC
    PatchCoreDetector.cpp
    YoloClassifier.cpp
    DetectionEngine.cpp
    ${CMAKE_SOURCE_DIR}/include/Detection/DetectionEngine.h
)

target_include_directories(Detection PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Detection PUBLIC
    onnxruntime
    Qt5::Core
    Qt5::Gui
    Qt5::Concurrent
)

# 编译期注入 ONNX Runtime 根路径与资产目录宏
# 注意：仅为开发期便利，资产目录以源码树绝对路径注入；
# 正式部署时应改为 install 相对路径或 Qt 资源系统。
# （RPATH 由根 CMakeLists 的 CMAKE_BUILD_RPATH 统一设置）
target_compile_definitions(Detection PUBLIC
    ONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}"
    DETECTION_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)
