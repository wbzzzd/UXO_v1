/**
 * @file AirportSceneFactory.h
 * @brief 3D场景工厂接口
 * @details 定义3D机场场景创建接口，将机场数据转换为Qt3D实体
 */
#ifndef CORE_3D_AIRPORT_SCENE_FACTORY_H
#define CORE_3D_AIRPORT_SCENE_FACTORY_H

#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QOrbitCameraController>

#include "Core/Data/AirportData.h"

namespace Core {

/**
 * @brief 3D机场场景工厂类
 * @details 根据AirportData创建完整的3D机场场景，
 *          包括地面、跑道、滑行道、停机坪、光照等所有元素
 */
class AirportSceneFactory
{
public:
    /**
     * @brief 构造函数
     */
    AirportSceneFactory();
    
    /**
     * @brief 析构函数
     */
    ~AirportSceneFactory();

    /**
     * @brief 创建完整的3D机场场景
     * @param airportData 机场数据
     * @return 场景根实体
     * @details 根据传入的机场数据创建所有3D元素，返回根实体用于渲染
     */
    Qt3DCore::QEntity* createScene(const AirportData& airportData);

    /**
     * @brief 创建地面
     * @param parent 父实体
     * @param groundData 地面数据
     * @return 地面实体
     */
    Qt3DCore::QEntity* createGround(Qt3DCore::QEntity* parent, const GroundInfo& groundData);

    /**
     * @brief 创建跑道（含中线标记）
     * @param parent 父实体
     * @param runwayData 跑道数据
     * @return 跑道实体
     */
    Qt3DCore::QEntity* createRunway(Qt3DCore::QEntity* parent, const RunwayInfo& runwayData);

    /**
     * @brief 创建滑行道
     * @param parent 父实体
     * @param taxiwayData 滑行道数据
     * @return 滑行道实体
     */
    Qt3DCore::QEntity* createTaxiway(Qt3DCore::QEntity* parent, const TaxiwayInfo& taxiwayData);

    /**
     * @brief 创建停机坪
     * @param parent 父实体
     * @param apronData 停机坪数据
     * @return 停机坪实体
     */
    Qt3DCore::QEntity* createApron(Qt3DCore::QEntity* parent, const ApronInfo& apronData);

    /**
     * @brief 创建光照
     * @param parent 父实体
     * @param lightData 光照数据
     * @return 光照实体
     */
    Qt3DCore::QEntity* createLight(Qt3DCore::QEntity* parent, const LightInfo& lightData);

    /**
     * @brief 设置相机
     * @param camera 相机对象
     * @param airportData 机场数据（用于确定初始视角）
     */
    void setupCamera(Qt3DRender::QCamera* camera, const AirportData& airportData);

    /**
     * @brief 创建轨道相机控制器
     * @param parent 父实体
     * @param camera 相机对象
     * @return 相机控制器
     */
    Qt3DExtras::QOrbitCameraController* createCameraController(Qt3DCore::QEntity* parent, 
                                                                  Qt3DRender::QCamera* camera);

private:
    Qt3DCore::QEntity* m_rootEntity;  ///< 当前根实体
};

} // namespace Core

#endif // CORE_3D_AIRPORT_SCENE_FACTORY_H