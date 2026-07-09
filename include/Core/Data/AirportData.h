/**
 * @file AirportData.h
 * @brief 机场数据结构定义
 * @details 定义机场各要素（跑道、滑行道、停机坪等）的数据结构，
 *          用于支持3D场景渲染和未来真实机场数据接入
 */
#ifndef CORE_DATA_AIRPORT_DATA_H
#define CORE_DATA_AIRPORT_DATA_H

#include <QString>
#include <QVector3D>
#include <QVector>

namespace Core {

/**
 * @brief 跑道信息结构
 * @details 包含跑道的位置、尺寸、中线标记等信息
 */
struct RunwayInfo {
    QVector3D position;    ///< 跑道中心位置 (x, y, z)
    float length;           ///< 跑道长度 (单位: 米)
    float width;            ///< 跑道宽度 (单位: 米)
    float thickness;        ///< 跑道厚度/高度
    QVector3D direction;    ///< 跑道方向向量 (用于计算中线走向)
    
    QString centerlineColor;    ///< 中线颜色 (如 "#FFD700")
    float centerlineThickness;  ///< 中线厚度
    
    RunwayInfo()
        : position(1500.0f, 0.0f, 1500.0f)
        , length(3000.0f)
        , width(50.0f)
        , thickness(0.3f)
        , direction(1.0f, 0.0f, 0.0f)
        , centerlineColor("#FFD700")
        , centerlineThickness(0.05f)
    {}
};

/**
 * @brief 滑行道信息结构
 * @details 包含滑行道的位置、尺寸信息
 */
struct TaxiwayInfo {
    QVector3D position;    ///< 滑行道中心位置
    float length;          ///< 长度
    float width;            ///< 宽度
    float thickness;       ///< 厚度
    QString name;           ///< 滑行道名称 (如 "A", "B")
    
    TaxiwayInfo()
        : position(800.0f, 0.0f, 1700.0f)
        , length(600.0f)
        , width(200.0f)
        , thickness(0.25f)
        , name("A")
    {}
};

/**
 * @brief 停机坪信息结构
 * @details 包含停机坪的位置、尺寸信息
 */
struct ApronInfo {
    QVector3D position;    ///< 停机坪中心位置
    float length;          ///< 长度
    float width;           ///< 宽度
    float thickness;       ///< 厚度
    int parkingCount;      ///< 停机位数量
    
    ApronInfo()
        : position(300.0f, 0.0f, 1800.0f)
        , length(400.0f)
        , width(400.0f)
        , thickness(0.2f)
        , parkingCount(8)
    {}
};

/**
 * @brief 地面区域信息
 * @details 机场草地/硬化地面区域
 */
struct GroundInfo {
    QVector3D position;    ///< 地面中心位置
    float xExtent;         ///< X方向范围
    float zExtent;         ///< Z方向范围 (对应实际长度)
    float yExtent;         ///< Y方向厚度
    QString color;         ///< 地面颜色
    
    GroundInfo()
        : position(2000.0f, -1.0f, 2000.0f)
        , xExtent(5000.0f)
        , zExtent(5000.0f)
        , yExtent(2.0f)
        , color("#4A7C3F")
    {}
};

/**
 * @brief 光源信息结构
 * @details 用于配置3D场景中的光照
 */
struct LightInfo {
    QVector3D position;    ///< 光源位置
    QString color;         ///< 光源颜色
    float intensity;       ///< 光照强度 (0.0-2.0)
    float attenuation;      ///< 衰减系数
    
    LightInfo()
        : position(1500.0f, 500.0f, 1500.0f)
        , color("#FFFFFF")
        , intensity(1.0f)
        , attenuation(0.001f)
    {}
};

/**
 * @brief 完整机场数据结构
 * @details 包含机场所有要素的数据，支持配置加载和渲染
 */
struct AirportData {
    QString id;                 ///< 机场ID
    QString name;               ///< 机场名称
    QString version;            ///< 数据版本
    
    GroundInfo ground;          ///< 地面信息
    QVector<RunwayInfo> runways;    ///< 跑道列表
    QVector<TaxiwayInfo> taxiways;  ///< 滑行道列表
    QVector<ApronInfo> aprons;      ///< 停机坪列表
    QVector<LightInfo> lights;      ///< 光源列表
    
    float cameraDistance;       ///< 默认相机距离
    QVector3D cameraTarget;    ///< 默认相机目标点
    
    /**
     * @brief 构造函数 - 初始化默认模拟数据
     */
    AirportData();
    
    /**
     * @brief 加载默认模拟数据
     * @details 用于演示和开发测试，后续可从JSON文件加载真实数据
     */
    void loadDefaultData();
    
    /**
     * @brief 从JSON文件加载机场数据
     * @param filePath JSON文件路径
     * @return 是否加载成功
     */
    bool loadFromJson(const QString& filePath);
    
    /**
     * @brief 将机场数据保存为JSON文件
     * @param filePath 保存路径
     * @return 是否保存成功
     */
    bool saveToJson(const QString& filePath) const;
};

} // namespace Core

#endif // CORE_DATA_AIRPORT_DATA_H