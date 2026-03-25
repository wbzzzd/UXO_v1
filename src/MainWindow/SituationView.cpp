#include "MainWindow/SituationView.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QPointLight>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DCore/QTransform>
#include <QColor>

#include "Core/Data/Types.h"

class TargetMarkerEntity : public Qt3DCore::QEntity
{
public:
    explicit TargetMarkerEntity(const Core::TargetInfo& target, Qt3DCore::QEntity *parent = nullptr)
        : Qt3DCore::QEntity(parent)
        , m_target(target)
    {
        Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(this);
        transform->setTranslation(target.position);
        transform->setScale(3.0f);
        addComponent(transform);

        Qt3DExtras::QCuboidMesh *mesh = new Qt3DExtras::QCuboidMesh(this);
        mesh->setXExtent(1.0f);
        mesh->setYExtent(1.0f);
        mesh->setZExtent(1.0f);
        addComponent(mesh);

        Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial(this);
        QColor color;
        switch (target.threatLevel) {
            case Core::ThreatLevel::High: color = QColor("#FF5252"); break;
            case Core::ThreatLevel::Medium: color = QColor("#FFB74D"); break;
            case Core::ThreatLevel::Low: color = QColor("#FFF176"); break;
            default: color = QColor("#888888");
        }
        material->setDiffuse(color);
        material->setAmbient(color.darker());
        addComponent(material);

        m_targetId = target.id;
    }

    QString targetId() const { return m_targetId; }

private:
    Core::TargetInfo m_target;
    QString m_targetId;
};

SituationView::SituationView(QWidget *parent)
    : QWidget(parent)
    , m_3dWindow(nullptr)
    , m_rootEntity(nullptr)
    , m_camera(nullptr)
    , m_cameraController(nullptr)
{
    setup3DView();
    setupToolBar();
}

SituationView::~SituationView()
{
}

void SituationView::setup3DView()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_3dWindow = new Qt3DExtras::Qt3DWindow();
    m_3dWindow->setMinimumSize(QSize(400, 300));

    QWidget *container = QWidget::createWindowContainer(m_3dWindow, this);
    container->setStyleSheet("background-color: #1E1E1E;");
    layout->addWidget(container);

    m_rootEntity = new Qt3DCore::QEntity();

    Qt3DExtras::QPlaneMesh *groundMesh = new Qt3DExtras::QPlaneMesh(m_rootEntity);
    groundMesh->setWidth(5000.0f);
    groundMesh->setHeight(5000.0f);

    Qt3DCore::QTransform *groundTransform = new Qt3DCore::QTransform(m_rootEntity);
    groundTransform->setTranslation(QVector3D(2000.0f, -0.5f, 2000.0f));

    Qt3DExtras::QPhongMaterial *groundMaterial = new Qt3DExtras::QPhongMaterial(m_rootEntity);
    groundMaterial->setDiffuse(QColor("#2D4A2D"));
    groundMaterial->setAmbient(QColor("#1A1A1A"));

    m_rootEntity->addComponent(groundMesh);
    m_rootEntity->addComponent(groundTransform);
    m_rootEntity->addComponent(groundMaterial);

    Qt3DExtras::QCuboidMesh *runwayMesh = new Qt3DExtras::QCuboidMesh(m_rootEntity);
    runwayMesh->setXExtent(3000.0f);
    runwayMesh->setYExtent(0.2f);
    runwayMesh->setZExtent(40.0f);

    Qt3DCore::QTransform *runwayTransform = new Qt3DCore::QTransform(m_rootEntity);
    runwayTransform->setTranslation(QVector3D(2000.0f, 0.0f, 2000.0f));

    Qt3DExtras::QPhongMaterial *runwayMaterial = new Qt3DExtras::QPhongMaterial(m_rootEntity);
    runwayMaterial->setDiffuse(QColor("#3D3D3D"));

    m_rootEntity->addComponent(runwayMesh);
    m_rootEntity->addComponent(runwayTransform);
    m_rootEntity->addComponent(runwayMaterial);

    Qt3DRender::QPointLight *light = new Qt3DRender::QPointLight();
    light->setColor(Qt::white);
    light->setIntensity(0.8f);
    m_rootEntity->addComponent(light);

    Qt3DCore::QTransform *lightTransform = new Qt3DCore::QTransform(m_rootEntity);
    lightTransform->setTranslation(QVector3D(2000.0f, 500.0f, 2000.0f));
    m_rootEntity->addComponent(lightTransform);

    m_camera = m_3dWindow->camera();
    m_camera->setPosition(QVector3D(2000.0f, 800.0f, 2500.0f));
    m_camera->setViewCenter(QVector3D(2000.0f, 0.0f, 2000.0f));
    m_camera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));

    m_cameraController = new Qt3DExtras::QOrbitCameraController(m_rootEntity);
    m_cameraController->setCamera(m_camera);
    m_cameraController->setZoomInLimit(50.0f);
    m_cameraController->setLinearSpeed(500.0f);
    m_cameraController->setLookSpeed(500.0f);

    Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector(m_rootEntity);
    cameraSelector->setCamera(m_camera);

    m_3dWindow->setRootEntity(m_rootEntity);
}

void SituationView::setupToolBar()
{
}

void SituationView::setCameraView(const QString &view)
{
    if (!m_camera) return;

    if (view == "top") {
        m_camera->setPosition(QVector3D(2000.0f, 1500.0f, 2000.0f));
        m_camera->setViewCenter(QVector3D(2000.0f, 0.0f, 2000.0f));
    } else if (view == "side") {
        m_camera->setPosition(QVector3D(100.0f, 200.0f, 2000.0f));
        m_camera->setViewCenter(QVector3D(2000.0f, 0.0f, 2000.0f));
    }
}

void SituationView::addTargetMarker(const QString &targetId, const QVector3D &position)
{
    for (auto child : m_rootEntity->children()) {
        auto entity = qobject_cast<Qt3DCore::QEntity*>(child);
        if (entity && entity->objectName() == "marker_" + targetId) {
            return;
        }
    }

    Core::TargetInfo target;
    target.id = targetId;
    target.position = position;
    target.threatLevel = Core::ThreatLevel::Medium;

    TargetMarkerEntity *marker = new TargetMarkerEntity(target, m_rootEntity);
    marker->setObjectName("marker_" + targetId);
}

void SituationView::removeTargetMarker(const QString &targetId)
{
    for (auto child : m_rootEntity->children()) {
        auto entity = qobject_cast<Qt3DCore::QEntity*>(child);
        if (entity && entity->objectName() == "marker_" + targetId) {
            entity->deleteLater();
            break;
        }
    }
}

void SituationView::updateTargetPosition(const QString &targetId, const QVector3D &position)
{
    for (auto child : m_rootEntity->children()) {
        auto entity = qobject_cast<Qt3DCore::QEntity*>(child);
        if (entity && entity->objectName() == "marker_" + targetId) {
            auto transforms = entity->componentsOfType<Qt3DCore::QTransform>();
            if (!transforms.isEmpty()) {
                transforms[0]->setTranslation(position);
            }
            break;
        }
    }
}

void SituationView::clearAllTargets()
{
    QList<Qt3DCore::QEntity*> toDelete;
    for (auto child : m_rootEntity->children()) {
        auto entity = qobject_cast<Qt3DCore::QEntity*>(child);
        if (entity && entity->objectName().startsWith("marker_")) {
            toDelete.append(entity);
        }
    }
    for (auto entity : toDelete) {
        entity->deleteLater();
    }
}

void SituationView::focusOnTarget(const QVector3D &position)
{
    if (!m_camera) return;

    QVector3D targetPos = position + QVector3D(0, 50, 50);
    m_camera->setPosition(targetPos);
    m_camera->setViewCenter(position);
}

void SituationView::highlightTarget(const QString &targetId, bool highlight)
{
    for (auto child : m_rootEntity->children()) {
        auto entity = qobject_cast<Qt3DCore::QEntity*>(child);
        if (entity && entity->objectName() == "marker_" + targetId) {
            auto materials = entity->componentsOfType<Qt3DExtras::QPhongMaterial>();
            if (!materials.isEmpty()) {
                auto material = materials[0];
                if (highlight) {
                    material->setSpecular(QColor("#FFFFFF"));
                    material->setShininess(128.0f);
                } else {
                    material->setSpecular(QColor("#000000"));
                    material->setShininess(32.0f);
                }
            }
            break;
        }
    }
}
