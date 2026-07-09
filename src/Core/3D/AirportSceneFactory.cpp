/**
 * @file AirportSceneFactory.cpp
 * @brief 3D场景工厂实现
 * @details 实现AirportSceneFactory类，将机场数据转换为Qt3D实体
 */
#include "Core/3D/AirportSceneFactory.h"

#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DRender/QPointLight>
#include <QColor>

namespace Core {

AirportSceneFactory::AirportSceneFactory()
    : m_rootEntity(nullptr)
{
}

AirportSceneFactory::~AirportSceneFactory()
{
}

Qt3DCore::QEntity* AirportSceneFactory::createScene(const AirportData& airportData)
{
    m_rootEntity = new Qt3DCore::QEntity();

    // 创建地面
    createGround(m_rootEntity, airportData.ground);

    // 创建跑道
    for (const auto& runway : airportData.runways) {
        createRunway(m_rootEntity, runway);
    }

    // 创建滑行道
    for (const auto& taxiway : airportData.taxiways) {
        createTaxiway(m_rootEntity, taxiway);
    }

    // 创建停机坪
    for (const auto& apron : airportData.aprons) {
        createApron(m_rootEntity, apron);
    }

    // 创建光照
    for (const auto& light : airportData.lights) {
        createLight(m_rootEntity, light);
    }

    return m_rootEntity;
}

Qt3DCore::QEntity* AirportSceneFactory::createGround(Qt3DCore::QEntity* parent, const GroundInfo& groundData)
{
    Qt3DCore::QEntity* groundEntity = new Qt3DCore::QEntity(parent);

    // 创建地面几何体
    Qt3DExtras::QCuboidMesh* groundMesh = new Qt3DExtras::QCuboidMesh();
    groundMesh->setXExtent(groundData.xExtent);
    groundMesh->setYExtent(groundData.yExtent);
    groundMesh->setZExtent(groundData.zExtent);

    // 创建变换组件
    Qt3DCore::QTransform* groundTransform = new Qt3DCore::QTransform();
    groundTransform->setTranslation(groundData.position);

    // 创建材质
    Qt3DExtras::QPhongMaterial* groundMaterial = new Qt3DExtras::QPhongMaterial();
    groundMaterial->setDiffuse(QColor(groundData.color));
    groundMaterial->setAmbient(QColor(groundData.color).darker(150));
    groundMaterial->setShininess(0.0f);

    // 添加组件到实体
    groundEntity->addComponent(groundMesh);
    groundEntity->addComponent(groundTransform);
    groundEntity->addComponent(groundMaterial);

    return groundEntity;
}

Qt3DCore::QEntity* AirportSceneFactory::createRunway(Qt3DCore::QEntity* parent, const RunwayInfo& runwayData)
{
    Qt3DCore::QEntity* runwayEntity = new Qt3DCore::QEntity(parent);

    // 跑道主体
    Qt3DExtras::QCuboidMesh* runwayMesh = new Qt3DExtras::QCuboidMesh();
    runwayMesh->setXExtent(runwayData.length);
    runwayMesh->setYExtent(runwayData.thickness);
    runwayMesh->setZExtent(runwayData.width);

    Qt3DCore::QTransform* runwayTransform = new Qt3DCore::QTransform();
    runwayTransform->setTranslation(runwayData.position);

    Qt3DExtras::QPhongMaterial* runwayMaterial = new Qt3DExtras::QPhongMaterial();
    runwayMaterial->setDiffuse(QColor("#3A3A3A"));  // 深灰色沥青
    runwayMaterial->setAmbient(QColor("#2A2A2A"));
    runwayMaterial->setShininess(10.0f);

    runwayEntity->addComponent(runwayMesh);
    runwayEntity->addComponent(runwayTransform);
    runwayEntity->addComponent(runwayMaterial);

    // 跑道中线标记
    Qt3DCore::QEntity* centerlineEntity = new Qt3DCore::QEntity(parent);

    Qt3DExtras::QCuboidMesh* centerlineMesh = new Qt3DExtras::QCuboidMesh();
    centerlineMesh->setXExtent(runwayData.length - 100.0f);  // 略短于跑道
    centerlineMesh->setYExtent(runwayData.centerlineThickness);
    centerlineMesh->setZExtent(2.0f);

    Qt3DCore::QTransform* centerlineTransform = new Qt3DCore::QTransform();
    centerlineTransform->setTranslation(QVector3D(
        runwayData.position.x(),
        runwayData.position.y() + runwayData.thickness + runwayData.centerlineThickness,
        runwayData.position.z()
    ));

    Qt3DExtras::QPhongMaterial* centerlineMaterial = new Qt3DExtras::QPhongMaterial();
    centerlineMaterial->setDiffuse(QColor(runwayData.centerlineColor));
    centerlineMaterial->setAmbient(QColor(runwayData.centerlineColor).darker(150));

    centerlineEntity->addComponent(centerlineMesh);
    centerlineEntity->addComponent(centerlineTransform);
    centerlineEntity->addComponent(centerlineMaterial);

    return runwayEntity;
}

Qt3DCore::QEntity* AirportSceneFactory::createTaxiway(Qt3DCore::QEntity* parent, const TaxiwayInfo& taxiwayData)
{
    Qt3DCore::QEntity* taxiwayEntity = new Qt3DCore::QEntity(parent);

    Qt3DExtras::QCuboidMesh* taxiwayMesh = new Qt3DExtras::QCuboidMesh();
    taxiwayMesh->setXExtent(taxiwayData.length);
    taxiwayMesh->setYExtent(taxiwayData.thickness);
    taxiwayMesh->setZExtent(taxiwayData.width);

    Qt3DCore::QTransform* taxiwayTransform = new Qt3DCore::QTransform();
    taxiwayTransform->setTranslation(taxiwayData.position);

    Qt3DExtras::QPhongMaterial* taxiwayMaterial = new Qt3DExtras::QPhongMaterial();
    taxiwayMaterial->setDiffuse(QColor("#4A4A4A"));
    taxiwayMaterial->setAmbient(QColor("#3A3A3A"));

    taxiwayEntity->addComponent(taxiwayMesh);
    taxiwayEntity->addComponent(taxiwayTransform);
    taxiwayEntity->addComponent(taxiwayMaterial);

    return taxiwayEntity;
}

Qt3DCore::QEntity* AirportSceneFactory::createApron(Qt3DCore::QEntity* parent, const ApronInfo& apronData)
{
    Qt3DCore::QEntity* apronEntity = new Qt3DCore::QEntity(parent);

    Qt3DExtras::QCuboidMesh* apronMesh = new Qt3DExtras::QCuboidMesh();
    apronMesh->setXExtent(apronData.length);
    apronMesh->setYExtent(apronData.thickness);
    apronMesh->setZExtent(apronData.width);

    Qt3DCore::QTransform* apronTransform = new Qt3DCore::QTransform();
    apronTransform->setTranslation(apronData.position);

    Qt3DExtras::QPhongMaterial* apronMaterial = new Qt3DExtras::QPhongMaterial();
    apronMaterial->setDiffuse(QColor("#505050"));
    apronMaterial->setAmbient(QColor("#404040"));

    apronEntity->addComponent(apronMesh);
    apronEntity->addComponent(apronTransform);
    apronEntity->addComponent(apronMaterial);

    return apronEntity;
}

Qt3DCore::QEntity* AirportSceneFactory::createLight(Qt3DCore::QEntity* parent, const LightInfo& lightData)
{
    Qt3DCore::QEntity* lightEntity = new Qt3DCore::QEntity(parent);

    Qt3DRender::QPointLight* light = new Qt3DRender::QPointLight();
    light->setColor(QColor(lightData.color));
    light->setIntensity(lightData.intensity);
    light->setConstantAttenuation(0.0f);
    light->setLinearAttenuation(lightData.attenuation);
    light->setQuadraticAttenuation(0.0f);

    Qt3DCore::QTransform* lightTransform = new Qt3DCore::QTransform();
    lightTransform->setTranslation(lightData.position);

    lightEntity->addComponent(light);
    lightEntity->addComponent(lightTransform);

    return lightEntity;
}

void AirportSceneFactory::setupCamera(Qt3DRender::QCamera* camera, const AirportData& airportData)
{
    if (!camera) return;

    // 设置相机位置（在机场后上方）
    QVector3D cameraPos = airportData.cameraTarget + QVector3D(0.0f, airportData.cameraDistance, airportData.cameraDistance * 0.5f);
    camera->setPosition(cameraPos);
    camera->setViewCenter(airportData.cameraTarget);
    camera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
    camera->setFieldOfView(60.0f);
}

Qt3DExtras::QOrbitCameraController* AirportSceneFactory::createCameraController(
    Qt3DCore::QEntity* parent, Qt3DRender::QCamera* camera)
{
    Qt3DExtras::QOrbitCameraController* controller = new Qt3DExtras::QOrbitCameraController(parent);
    controller->setCamera(camera);
    controller->setZoomInLimit(30.0f);
    controller->setLinearSpeed(1000.0f);
    controller->setLookSpeed(500.0f);
    return controller;
}

} // namespace Core