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
#include "Common/GlobalStyle.h"

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
        // 威胁等级色取全局令牌（值与原字面量逐值一致，像素无损）
        switch (target.threatLevel) {
            case Core::ThreatLevel::High: color = QColor(GlobalStyle::Colors::ThreatHigh); break;     // 红色 - 高危
            case Core::ThreatLevel::Medium: color = QColor(GlobalStyle::Colors::ThreatMedium); break;   // 橙色 - 中危
            case Core::ThreatLevel::Low: color = QColor(GlobalStyle::Colors::ThreatLow); break;       // 黄色 - 低危
            default: color = QColor(GlobalStyle::Colors::TextDisabled);
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
    frameGraph->setClearColor(QColor("#87CEEB"));  // 浅蓝色天空（3D 场景清屏色，无 QSS 词汇对应，保留字面量并附说明）

    // 创建窗口容器
    QWidget *container = QWidget::createWindowContainer(m_3dWindow, this);
    // QWidget 容器不在全局 QSS 覆盖范围，需显式设置暗色背景
    container->setStyleSheet(QStringLiteral("background-color: %1;").arg(GlobalStyle::Colors::Background));
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
        // QWidget 容器不在全局 QSS 覆盖范围，需显式设置暗色背景
        container->setStyleSheet(QStringLiteral("background-color: %1;").arg(GlobalStyle::Colors::Background));
        mainLayout->addWidget(container, 1);
    }
    
    // 创建工具栏（右侧竖直方向）
    QWidget *toolBarContainer = new QWidget(this);
    toolBarContainer->setFixedWidth(60);
    // 半透明 rgba(alpha=200) 与 Runway(#3D3D3D) 左边框无对应词表（containerBg 无 alpha、edgeBorder=Border#3C3C3C≠Runway），保留内联
    toolBarContainer->setStyleSheet(QStringLiteral(
        "background-color: rgba(30, 30, 30, 200);"
        "border-left: 1px solid %1;"
    ).arg(GlobalStyle::Colors::Runway));
    
    QVBoxLayout *toolBarLayout = new QVBoxLayout(toolBarContainer);
    toolBarLayout->setContentsMargins(5, 4, 5, 4);
    toolBarLayout->setSpacing(4);
    toolBarLayout->setAlignment(Qt::AlignTop);
    
    // 标题
    QLabel *titleLabel = new QLabel("视角", toolBarContainer);
    // 辅助文本色走全局 QSS textColor="secondary"（=TextSecondary，构造期静态属性先于 addWidget）；10px 字号无对应 labelRole（body2/caption=12px），保留内联
    titleLabel->setProperty("textColor", "secondary");
    // 属性转换（批次5）：10px 字号走 fontSize 词汇（先于 addWidget）
    titleLabel->setProperty("fontSize", "10");
    titleLabel->setAlignment(Qt::AlignCenter);
    toolBarLayout->addWidget(titleLabel);
    
    // 俯视按钮
    QPushButton *btnTop = new QPushButton("俯", toolBarContainer);
    btnTop->setFixedSize(40, 28);
    btnTop->setToolTip("俯视图");
    // 蓝色梯度 #0078D7/#1984D8/#005A9E 未在 token 表、无对应 btnVariant（实心蓝底+pressed 梯度），保留内联；white 文字已替换为 Colors::TextPrimary 令牌
    btnTop->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #0078D7; color: %1; border: none; border-radius: 4px; font-size: 12px; min-width: 0px; max-width: 40px; padding: 0px; }"
        "QPushButton:hover { background-color: #1984D8; }"
        "QPushButton:pressed { background-color: #005A9E; }"
    ).arg(GlobalStyle::Colors::TextPrimary));
    connect(btnTop, &QPushButton::clicked, [this]() { setCameraView("top"); updateZoomLabel(); });
    toolBarLayout->addWidget(btnTop);
    
    // 侧视按钮
    QPushButton *btnSide = new QPushButton("侧", toolBarContainer);
    btnSide->setFixedSize(40, 28);
    btnSide->setToolTip("侧视图");
    // 描边灰按钮无对应 btnVariant（subtle/flat 透明或异色底），需自定义 min-width/padding 适配 60px 工具栏，保留内联；颜色已用 Colors:: 令牌
    btnSide->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; font-size: 12px; min-width: 0px; max-width: 40px; padding: 0px; }"
        "QPushButton:hover { background-color: %3; }"
    ).arg(GlobalStyle::Colors::ButtonGray, GlobalStyle::Colors::TextPrimary, GlobalStyle::Colors::BorderLight));
    connect(btnSide, &QPushButton::clicked, [this]() { setCameraView("side"); updateZoomLabel(); });
    toolBarLayout->addWidget(btnSide);
    
    // 3D视角按钮
    QPushButton *btn3D = new QPushButton("3D", toolBarContainer);
    btn3D->setFixedSize(40, 28);
    btn3D->setToolTip("3D视角");
    // 描边灰按钮无对应 btnVariant（subtle/flat 透明或异色底），需自定义 min-width/padding 适配 60px 工具栏，保留内联；颜色已用 Colors:: 令牌
    btn3D->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; font-size: 12px; min-width: 0px; max-width: 40px; padding: 0px; }"
        "QPushButton:hover { background-color: %3; }"
    ).arg(GlobalStyle::Colors::ButtonGray, GlobalStyle::Colors::TextPrimary, GlobalStyle::Colors::BorderLight));
    connect(btn3D, &QPushButton::clicked, [this]() { setCameraView("3d"); updateZoomLabel(); });
    toolBarLayout->addWidget(btn3D);
    
    toolBarLayout->addSpacing(0);
    
    // 复位按钮
    QPushButton *btnReset = new QPushButton("复位", toolBarContainer);
    btnReset->setFixedSize(40, 28);
    btnReset->setToolTip("复位视角");
    // 红色梯度 #D9534F/#E74C3C 未在 token 表、无对应 btnVariant（实心红底），保留内联；white 文字已替换为 Colors::TextPrimary 令牌
    btnReset->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #D9534F; color: %1; border: none; border-radius: 4px; font-size: 11px; min-width: 0px; max-width: 40px; padding: 0px; }"
        "QPushButton:hover { background-color: #E74C3C; }"
    ).arg(GlobalStyle::Colors::TextPrimary));
    connect(btnReset, &QPushButton::clicked, this, &SituationView::resetCameraView);
    toolBarLayout->addWidget(btnReset);
    
    toolBarLayout->addSpacing(2);
    
    // 缩放级别标签
    m_zoomLabel = new QLabel("100%", toolBarContainer);
    // TextDisabled(#888888) 无语义相符的 textColor 词表（offline 耦合 StatusOffline 且语义不符）、10px 无 labelRole，保留内联
    // 属性转换（批次5）：禁用色+10px 逐值等价走 textColor/fontSize 词汇（先于 addWidget）
    m_zoomLabel->setProperty("textColor", "disabled");
    m_zoomLabel->setProperty("fontSize", "10");
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setFixedHeight(20);
    toolBarLayout->addWidget(m_zoomLabel);
    
    // 位置信息标签
    m_positionLabel = new QLabel("位置", toolBarContainer);
    // TextDim(#666666) 无对应 textColor 词表、9px 无 labelRole，保留内联
    m_positionLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 9px;").arg(GlobalStyle::Colors::TextDim));
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
                    // Qt 命名色替代 hex 字面量（值与 #FFFFFF/#000000 一致，像素无损）
                    material->setSpecular(QColor(Qt::white));
                    material->setShininess(128.0f);
                } else {
                    material->setSpecular(QColor(Qt::black));
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
