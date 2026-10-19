set(INTEGRATION_SOURCES
    DataPlatformApi.cpp
    ApiError.cpp
    HttpClient.cpp
    # W3 实体映射：契约 §6 五实体字段表与客户端领域模型的 JSON 转换
    EntityMapping.cpp
    # W4 词表与模型类映射：只读缓存（契约 §4/§5）
    VocabularyCache.cpp
)

# AUTOMOC 需要扫描 Q_OBJECT 宏的 header（当前骨架无 QObject，W2 扩展时补充）
set(INTEGRATION_HEADERS
    ${HEADERS_DIR}/Integration/DataPlatformApi.h
    ${HEADERS_DIR}/Integration/VocabularyCache.h
)

add_library(Integration STATIC ${INTEGRATION_SOURCES} ${INTEGRATION_HEADERS})
target_include_directories(Integration PUBLIC ${HEADERS_DIR})
target_link_libraries(Integration Core Qt5::Core Qt5::Network)
