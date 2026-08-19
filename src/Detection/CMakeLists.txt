# Detection static library - ONNX-based UXO detection (PatchCore + YOLOv8-cls)

# ---- ONNX Runtime IMPORTED target ----
set(ONNXRUNTIME_ROOT "${CMAKE_SOURCE_DIR}/third_party/onnxruntime")

add_library(onnxruntime SHARED IMPORTED)
set_target_properties(onnxruntime PROPERTIES
    IMPORTED_LOCATION "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so.1.23.2"
    INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_ROOT}/include"
    INTERFACE_LINK_DIRECTORIES "${ONNXRUNTIME_ROOT}/lib"
)

# ---- Detection static library ----
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

# Propagate RPATH so the executable finds libonnxruntime at runtime
# DEVELOPMENT ONLY: assets path injected as absolute source-dir path.
# For deployment, replace with an install-relative path or Qt resource system.
target_compile_definitions(Detection PUBLIC
    ONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}"
    DETECTION_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)
