/**
 * @file SituationView.cpp
 * @brief 态势显示视图实现
 * @details 使用AirportSceneFactory创建3D机场场景，实现数据驱动的渲染架构
 */
#include "MainWindow/SituationView.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QToolBar>
#include <QAction>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QPointLight>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QRenderSettings>
#include <QColor>

#include "Core/Data/Types.h"

/**
 * @brief 目标标记实体类
 * @details 用于在3D场景中显示可疑物品位置，根据威胁等级显示不同颜色
 */
class TargetMarkerEntity : public Qt3DCore::QEntity
{
public:
    explicit TargetMarkerEntity(const Core::TargetInfo& target, Qt3DCore::QEntity *parent = nullptr)
        : Qt3DCore::QEntity(parent)
        , m_target(target)
    {
        // 创建变换组件 - 设置位置和缩放
        Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(this);
        transform->setTranslation(target.position);
        transform->setScale(3.0f);
        addComponent(transform);

        // 创建立方体网格
        Qt3DExtras::QCuboidMesh *mesh = new Qt3DExtras::QCuboidMesh(this);
        mesh->setXExtent(1.0f);
        mesh->setYExtent(1.0f);
        mesh->setZExtent(1.0f);
        addComponent(mesh);

        // 根据威胁等级设置颜色
        Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial(this);
        QColor color;
        switch (target.threatLevel) {
            case Core::ThreatLevel::High: color = QColor("#FF5252"); break;     // 红色 - 高危
            case Core::ThreatLevel::Medium: color = QColor("#FFB74D"); break;   // 橙色 - 中危
            case Core::ThreatLevel::Low: color = QColor("#FFF176"); break;       // 黄色 - 低危
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
    , m_toolBar(nullptr)
    , m_zoomLabel(nullptr)
    , m_positionLabel(nullptr)
    , m_sceneFactory(new Core::AirportSceneFactory())
{
    setup3DView();
    setupToolBar();
}

SituationView::~SituationView()
{
    delete m_sceneFactory;
}

void SituationView::setup3DView()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 创建3D窗口
    m_3dWindow = new Qt3DExtras::Qt3DWindow();
    m_3dWindow->setMinimumSize(QSize(400, 300));
    m_3dWindow->setSurfaceType(QSurface::OpenGLSurface);
    m_3dWindow->format().setSamples(4);
    
    // 设置天空背景色
    Qt3DExtras::QForwardRenderer *frameGraph = m_3dWindow->defaultFrameGraph();
    frameGraph->setClearColor(QColor("#87CEEB"));  // 浅蓝色天空

    // 创建窗口容器
    QWidget *container = QWidget::createWindowContainer(m_3dWindow, this);
    container->setStyleSheet("background-color: #1E1E1E;");
    layout->addWidget(container);

    // 使用工厂创建机场3D场景
    m_airportData = Core::AirportData();  // 加载默认模拟数据
    m_rootEntity = m_sceneFactory->createScene(m_airportData);

    // 设置相机
    m_camera = m_3dWindow->camera();
    m_sceneFactory->setupCamera(m_camera, m_airportData);

    // 创建相机控制器
    m_cameraController = m_sceneFactory->createCameraController(m_rootEntity, m_camera);

    // 设置根实体
    m_3dWindow->setRootEntity(m_rootEntity);
}

void SituationView::setupToolBar()
{
    QHBoxLayout *mainLayout = qobject_cast<QHBoxLayout*>(layout());
    if (!mainLayout) {
        // 重新设置布局为水平布局
        delete layout();
        mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        
        // 重新添加3D窗口
        QWidget *container = QWidget::createWindowContainer(m_3dWindow, this);
        container->setStyleSheet("background-color: #1E1E1E;");
        mainLayout->addWidget(container, 1);
    }
    
    // 创建工具栏（右侧竖直方向）
    QWidget *toolBarContainer = new QWidget(this);
    toolBarContainer->setFixedWidth(60);
    toolBarContainer->setStyleSheet(
        "background-color: rgba(30, 30, 30, 200);"
        "border-left: 1px solid #3D3D3D;"
    );
    
    QVBoxLayout *toolBarLayout = new QVBoxLayout(toolBarContainer);
    toolBarLayout->setContentsMargins(5, 10, 5, 10);
    toolBarLayout->setSpacing(8);
    toolBarLayout->setAlignment(Qt::AlignTop);
    
    // 标题
    QLabel *titleLabel = new QLabel("视角", toolBarContainer);
    titleLabel->setStyleSheet("color: #AAAAAA; font-size: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    toolBarLayout->addWidget(titleLabel);
    
    // 俯视按钮
    QPushButton *btnTop = new QPushButton("俯", toolBarContainer);
    btnTop->setFixedSize(40, 30);
    btnTop->setToolTip("俯视图");
    btnTop->setStyleSheet(
        "QPushButton { background-color: #0078D7; color: white; border: none; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: #1984D8; }"
        "QPushButton:pressed { background-color: #005A9E; }"
    );
    connect(btnTop, &QPushButton::clicked, [this]() { setCameraView("top"); updateZoomLabel(); });
    toolBarLayout->addWidget(btnTop);
    
    // 侧视按钮
    QPushButton *btnSide = new QPushButton("侧", toolBarContainer);
    btnSide->setFixedSize(40, 30);
    btnSide->setToolTip("侧视图");
    btnSide->setStyleSheet(
        "QPushButton { background-color: #4A4A4A; color: #DDDDDD; border: 1px solid #5A5A5A; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: #5A5A5A; }"
    );
    connect(btnSide, &QPushButton::clicked, [this]() { setCameraView("side"); updateZoomLabel(); });
    toolBarLayout->addWidget(btnSide);
    
    // 3D视角按钮
    QPushButton *btn3D = new QPushButton("3D", toolBarContainer);
    btn3D->setFixedSize(40, 30);
    btn3D->setToolTip("3D视角");
    btn3D->setStyleSheet(
        "QPushButton { background-color: #4A4A4A; color: #DDDDDD; border: 1px solid #5A5A5A; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: #5A5A5A; }"
    );
    connect(btn3D, &QPushButton::clicked, [this]() { setCameraView("3d"); updateZoomLabel(); });
    toolBarLayout->addWidget(btn3D);
    
    toolBarLayout->addSpacing(10);
    
    // 复位按钮
    QPushButton *btnReset = new QPushButton("复位", toolBarContainer);
    btnReset->setFixedSize(40, 30);
    btnReset->setToolTip("复位视角");
    btnReset->setStyleSheet(
        "QPushButton { background-color: #D9534F; color: white; border: none; border-radius: 4px; font-size: 11px; }"
        "QPushButton:hover { background-color: #E74C3C; }"
    );
    connect(btnReset, &QPushButton::clicked, this, &SituationView::resetCameraView);
    toolBarLayout->addWidget(btnReset);
    
    toolBarLayout->addSpacing(20);
    
    // 缩放级别标签
    m_zoomLabel = new QLabel("100%", toolBarContainer);
    m_zoomLabel->setStyleSheet("color: #888888; font-size: 10px;");
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setFixedHeight(20);
    toolBarLayout->addWidget(m_zoomLabel);
    
    // 位置信息标签
    m_positionLabel = new QLabel("位置", toolBarContainer);
    m_positionLabel->setStyleSheet("color: #666666; font-size: 9px;");
    m_positionLabel->setAlignment(Qt::AlignCenter);
    m_positionLabel->setWordWrap(true);
    m_positionLabel->setFixedHeight(40);
    toolBarLayout->addWidget(m_positionLabel);
    
    toolBarLayout->addStretch();
    
    mainLayout->addWidget(toolBarContainer);
    
    updateZoomLabel();
}

void SituationView::setCameraView(const QString &view)
{
    if (!m_camera) return;

    QVector3D target = m_airportData.cameraTarget;
    
    if (view == "top") {
        // 俯视图 - 从正上方看
        m_camera->setPosition(QVector3D(target.x(), 1500.0f, target.z()));
        m_camera->setViewCenter(target);
    } else if (view == "side") {
        // 侧视图 - 从侧面看
        m_camera->setPosition(QVector3D(100.0f, 200.0f, target.z()));
        m_camera->setViewCenter(target);
    } else if (view == "3d") {
        // 3D视角 - 默认倾斜视角
        m_camera->setPosition(QVector3D(target.x() + 500.0f, 500.0f, target.z() + 700.0f));
        m_camera->setViewCenter(target);
    }
}

void SituationView::addTargetMarker(const QString &targetId, const QVector3D &position)
{
    // 检查目标是否已存在
    for (auto child : m_rootEntity->children()) {
        auto entity = qobject_cast<Qt3DCore::QEntity*>(child);
        if (entity && entity->objectName() == "marker_" + targetId) {
            return;
        }
    }

    // 创建目标信息
    Core::TargetInfo target;
    target.id = targetId;
    target.position = position;
    target.threatLevel = Core::ThreatLevel::Medium;

    // 创建3D标记
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

    // 移动相机到目标附近
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

void SituationView::loadAirportData(const Core::AirportData& airportData)
{
    m_airportData = airportData;
    
    // 重新创建场景
    if (m_rootEntity) {
        m_rootEntity->deleteLater();
    }
    m_rootEntity = m_sceneFactory->createScene(m_airportData);
    
    // 更新相机
    if (m_camera) {
        m_sceneFactory->setupCamera(m_camera, m_airportData);
    }
    
    // 更新根实体
    m_3dWindow->setRootEntity(m_rootEntity);
    
    // 重新创建控制器
    if (m_cameraController) {
        delete m_cameraController;
    }
    m_cameraController = m_sceneFactory->createCameraController(m_rootEntity, m_camera);
}

void SituationView::resetCameraView()
{
    if (!m_camera) return;
    m_sceneFactory->setupCamera(m_camera, m_airportData);
    updateZoomLabel();
}

void SituationView::updateZoomLabel()
{
    if (!m_camera || !m_zoomLabel || !m_positionLabel) return;
    
    // 计算相机与目标的距离作为"缩放"参考
    QVector3D pos = m_camera->position();
    QVector3D target = m_camera->viewCenter();
    float distance = (pos - target).length();
    
    // 估算缩放比例（基于默认距离800）
    int zoomPercent = static_cast<int>(800.0f / distance * 100.0f);
    m_zoomLabel->setText(QString("缩放: %1%").arg(zoomPercent));
    
    // 更新位置信息
    m_positionLabel->setText(QString("X:%1\nY:%2\nZ:%3")
        .arg(static_cast<int>(pos.x()))
        .arg(static_cast<int>(pos.y()))
        .arg(static_cast<int>(pos.z())));
}