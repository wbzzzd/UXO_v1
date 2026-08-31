#include "MainWindow/TacticalMapWidget.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QVBoxLayout>
#include <QTimer>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QPainterPath>
#include <QPolygonF>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QFileInfo>
#include <QDebug>

// 场景像素尺寸（逻辑坐标系，视图自适应缩放）
static constexpr double SCENE_SIZE = 1000.0;
// 已检测目标中心点半径（像素）
static constexpr double DOT_RADIUS = 7.0;
// 待检测目标标记半径（像素）
static constexpr double PENDING_RADIUS = 7.0;
// 脉冲环初始半径
static constexpr double PULSE_INIT_RADIUS = DOT_RADIUS;
// 脉冲动画总相位数
static constexpr int PULSE_PHASES = 20;
// 脉冲周期（ms）
static constexpr int PULSE_INTERVAL_MS = 50;

TacticalMapWidget::TacticalMapWidget(QWidget *parent)
    : QWidget(parent)
    , m_view(nullptr)
    , m_scene(nullptr)
    , m_pulseTimer(nullptr)
    , m_imageRect(0.0, 0.0, SCENE_SIZE, SCENE_SIZE)  // 无底图时回退到全场景正方形
    , m_droneMarker(nullptr)
    , m_droneLabel(nullptr)
    , m_trackPath(nullptr)
    , m_pulsePhase(0)
{
    // 默认机场边界（沈阳于洪全胜机场），setAirportBounds 可覆盖
    m_bounds.north = 41.840;
    m_bounds.south = 41.805;
    m_bounds.west = 123.278;
    m_bounds.east = 123.320;

    setupUi();
    m_pulseTimer = new QTimer(this);
    m_pulseTimer->setInterval(PULSE_INTERVAL_MS);
    connect(m_pulseTimer, &QTimer::timeout, this, &TacticalMapWidget::onPulseTick);
    m_pulseTimer->start();
}

TacticalMapWidget::~TacticalMapWidget() = default;

void TacticalMapWidget::setupUi()
{
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, SCENE_SIZE, SCENE_SIZE);

    // 深色底色（卫星图加载前或加载失败时显示）
    m_scene->setBackgroundBrush(QColor(20, 30, 20));

    m_view = new QGraphicsView(this);
    m_view->setScene(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setBackgroundBrush(QColor(20, 30, 20));
    m_view->setFrameShape(QFrame::NoFrame);
    // QGraphicsView 拦截鼠标事件，设为透明让点击穿透到 TacticalMapWidget::mousePressEvent
    m_view->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    // 监听视图自身 Resize：布局激活后视图拿到真实尺寸时重适配场景（见 eventFilter）
    m_view->installEventFilter(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
    setLayout(layout);
}

void TacticalMapWidget::setSatelliteImage(const QString& path)
{
    // 移除旧底图项
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (auto *pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(item)) {
            m_scene->removeItem(pixmapItem);
            delete pixmapItem;
            break;  // 只移除底图，保留目标标记
        }
    }

    QPixmap satellite(path);
    if (satellite.isNull()) {
        qWarning() << "TacticalMapWidget: 卫星图加载失败:" << path;
        // 加载失败时回退到全场景正方形, 保证经纬度映射仍可用
        m_imageRect = QRectF(0.0, 0.0, SCENE_SIZE, SCENE_SIZE);
        return;
    }

    // Aspect-fit 缩放: 保持原始宽高比完整放入 SCENE_SIZE 正方形内, 不裁剪
    // 例如 2000x1800 资产 -> 缩放后 1000x900, 上下各留 50px 信箱(letterbox)区
    QPixmap scaled = satellite.scaled(QSize(SCENE_SIZE, SCENE_SIZE),
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation);
    // 信箱偏移: 在场景内居中放置缩放后的底图, 使上下/左右留白对称
    double offsetX = (SCENE_SIZE - scaled.width()) / 2.0;
    double offsetY = (SCENE_SIZE - scaled.height()) / 2.0;
    // 记录底图实际显示矩形, latLngToScene 据此映射经纬度, 确保叠加层与底图像素对齐
    m_imageRect = QRectF(offsetX, offsetY, scaled.width(), scaled.height());

    QGraphicsPixmapItem *bgItem = m_scene->addPixmap(scaled);
    bgItem->setPos(offsetX, offsetY);  // 放置到信箱偏移位置, 而非默认 (0,0)
    bgItem->setZValue(-1000);  // 底图在最底层

    // 底图常在窗口已显示后才加载, 此时不会触发 resizeEvent, 需主动 fitInView;
    // 否则视图保持默认 1:1 变换, 1000x1000 场景只露出左上角而显示为深色背景
    if (m_view->viewport()->width() > 0 && m_view->viewport()->height() > 0) {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void TacticalMapWidget::setAirportBounds(const Core::Simulation::AirportBounds& bounds)
{
    m_bounds = bounds;
}

QPointF TacticalMapWidget::latLngToScene(double lat, double lng) const
{
    // 经纬度线性映射到底图显示矩形 m_imageRect (aspect-fit 后含信箱偏移)
    // 机场边界四角正好对齐底图四角, 使叠加层与卫星像素严格对齐
    // 经度 -> X 轴（西=左=m_imageRect.left(), 东=右=m_imageRect.right()）
    double x = m_imageRect.x()
             + (lng - m_bounds.west) / (m_bounds.east - m_bounds.west) * m_imageRect.width();
    // 纬度 -> Y 轴（北=上=m_imageRect.top(), 南=下=m_imageRect.bottom()）
    double y = m_imageRect.y()
             + (m_bounds.north - lat) / (m_bounds.north - m_bounds.south) * m_imageRect.height();
    return QPointF(x, y);
}

QPointF TacticalMapWidget::mapToScene(const QVector3D& pos) const
{
    // TargetInfo.position: x=经度, y=纬度, z=高度
    return latLngToScene(pos.y(), pos.x());
}

QColor TacticalMapWidget::threatColor(const Core::TargetInfo& target) const
{
    switch (target.threatLevel) {
    case Core::ThreatLevel::High:
    case Core::ThreatLevel::Critical:
        return QColor(255, 82, 82);
    case Core::ThreatLevel::Medium:
        return QColor(255, 183, 77);
    case Core::ThreatLevel::Low:
        return QColor(255, 241, 118);
    default:
        return QColor(136, 136, 136);
    }
}

bool TacticalMapWidget::isPending(const Core::TargetInfo& target) const
{
    return target.status == Core::TargetStatus::Pending
        || target.status == Core::TargetStatus::Unknown;
}

void TacticalMapWidget::setTargets(const QVector<Core::TargetInfo>& targets)
{
    clearTargets();
    for (const auto& t : targets) {
        addTarget(t);
    }
}

void TacticalMapWidget::addTarget(const Core::TargetInfo& target)
{
    if (m_items.contains(target.id)) {
        return;
    }

    QPointF sc = mapToScene(target.position);
    bool pending = isPending(target);
    QColor color = threatColor(target);

    QGraphicsEllipseItem *dot = nullptr;
    QGraphicsEllipseItem *glow = nullptr;
    QGraphicsEllipseItem *pulse = nullptr;

    if (pending) {
        QPen dotPen(QColor(136, 136, 136, 128), 1, Qt::DashLine);
        dot = m_scene->addEllipse(sc.x() - PENDING_RADIUS, sc.y() - PENDING_RADIUS,
                                  PENDING_RADIUS * 2, PENDING_RADIUS * 2,
                                  dotPen, Qt::NoBrush);
    } else {
        // 已检测目标：实心圆 + 威胁等级颜色 + 发光外圈
        QPen dotPen(Qt::black, 1);
        QBrush dotBrush(color);
        dot = m_scene->addEllipse(sc.x() - DOT_RADIUS, sc.y() - DOT_RADIUS,
                                  DOT_RADIUS * 2, DOT_RADIUS * 2, dotPen, dotBrush);
        // 发光外圈
        QPen glowPen(QColor(color.red(), color.green(), color.blue(), 80), 3);
        glow = m_scene->addEllipse(sc.x() - DOT_RADIUS - 2, sc.y() - DOT_RADIUS - 2,
                                   DOT_RADIUS * 2 + 4, DOT_RADIUS * 2 + 4,
                                   glowPen, Qt::NoBrush);
        // 脉冲环
        QPen pulsePen(QColor(color.red(), color.green(), color.blue(), 200), 2);
        pulse = m_scene->addEllipse(sc.x() - PULSE_INIT_RADIUS,
                                    sc.y() - PULSE_INIT_RADIUS,
                                    PULSE_INIT_RADIUS * 2, PULSE_INIT_RADIUS * 2,
                                    pulsePen, Qt::NoBrush);
    }

    // ID 标签
    QString labelText = pending ? target.id + QStringLiteral("?") : target.id;
    auto *label = m_scene->addText(labelText);
    QFont f = label->font();
    f.setPointSize(8);
    f.setBold(true);
    label->setFont(f);
    label->setDefaultTextColor(pending ? QColor(136, 136, 136) : QColor(255, 255, 255));
    label->setPos(sc.x() - DOT_RADIUS - 4, sc.y() + DOT_RADIUS + 2);

    TargetGraphics g{dot, glow, pulse, label, target.id, pending, color};
    m_items.insert(target.id, g);
}

void TacticalMapWidget::clearTargets()
{
    QList<QGraphicsItem*> toRemove;
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        if (it->glow) { toRemove.append(it->glow); }
        if (it->dot) { toRemove.append(it->dot); }
        if (it->pulse) { toRemove.append(it->pulse); }
        if (it->label) { toRemove.append(it->label); }
    }
    for (QGraphicsItem *item : toRemove) {
        m_scene->removeItem(item);
        delete item;
    }
    m_items.clear();
    m_selectedId.clear();
}

void TacticalMapWidget::setSelectedTarget(const QString& targetId)
{
    if (!m_selectedId.isEmpty() && m_items.contains(m_selectedId)) {
        auto &g = m_items[m_selectedId];
        if (g.pending) {
            g.dot->setPen(QPen(QColor(136, 136, 136, 128), 1, Qt::DashLine));
        } else {
            g.dot->setPen(QPen(Qt::black, 1));
        }
    }
    m_selectedId = targetId;
    if (!m_selectedId.isEmpty() && m_items.contains(m_selectedId)) {
        m_items[m_selectedId].dot->setPen(QPen(QColor(255, 255, 255), 2));
    }
}

QString TacticalMapWidget::selectedTargetId() const
{
    return m_selectedId;
}

int TacticalMapWidget::targetCount() const
{
    return m_items.size();
}

void TacticalMapWidget::setDronePosition(double lat, double lng, double heading)
{
    QPointF sc = latLngToScene(lat, lng);

    if (m_droneMarker == nullptr) {
        // 首次创建无人机标记：蓝色三角形
        QPolygonF triangle;
        triangle << QPointF(0, -10) << QPointF(8, 8) << QPointF(-8, 8);
        m_droneMarker = m_scene->addPolygon(triangle,
                                             QPen(QColor(33, 150, 243), 1),
                                             QBrush(QColor(33, 150, 243)));
        m_droneMarker->setZValue(100);

        m_droneLabel = m_scene->addText(QStringLiteral("UAV-1"));
        QFont f = m_droneLabel->font();
        f.setPointSize(8);
        f.setBold(true);
        m_droneLabel->setFont(f);
        m_droneLabel->setDefaultTextColor(QColor(33, 150, 243));
        m_droneLabel->setZValue(100);
    }

    // 更新位置和旋转
    m_droneMarker->setPos(sc);
    m_droneMarker->setRotation(heading);
    m_droneLabel->setPos(sc.x() - 20, sc.y() + 12);
}

void TacticalMapWidget::addTrackPoint(double lat, double lng)
{
    QPointF sc = latLngToScene(lat, lng);
    m_trackPoints.append(sc);

    if (m_trackPoints.size() < 2) {
        return;
    }

    // 重建航迹路径（虚线）
    if (m_trackPath) {
        m_scene->removeItem(m_trackPath);
        delete m_trackPath;
    }

    QPainterPath path;
    path.moveTo(m_trackPoints.first());
    for (int i = 1; i < m_trackPoints.size(); ++i) {
        path.lineTo(m_trackPoints[i]);
    }

    QPen trackPen(QColor(33, 150, 243, 180), 2, Qt::DashLine);
    m_trackPath = m_scene->addPath(path, trackPen);
    m_trackPath->setZValue(50);
}

void TacticalMapWidget::clearDroneTrack()
{
    m_trackPoints.clear();
    if (m_trackPath) {
        m_scene->removeItem(m_trackPath);
        delete m_trackPath;
        m_trackPath = nullptr;
    }
    if (m_droneMarker) {
        m_scene->removeItem(m_droneMarker);
        delete m_droneMarker;
        m_droneMarker = nullptr;
    }
    if (m_droneLabel) {
        m_scene->removeItem(m_droneLabel);
        delete m_droneLabel;
        m_droneLabel = nullptr;
    }
}

void TacticalMapWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || m_view == nullptr) {
        QWidget::mousePressEvent(event);
        return;
    }

    // mapToScene 需要 viewport 坐标，不能用 view->mapFromGlobal（返回的是 view widget 坐标）
    QPointF scenePos = m_view->mapToScene(m_view->viewport()->mapFromGlobal(event->globalPos()));
    const QList<QGraphicsItem*> hitItems = m_scene->items(scenePos);
    for (QGraphicsItem *item : hitItems) {
        for (auto it = m_items.begin(); it != m_items.end(); ++it) {
            if (it->dot == item) {
                emit targetClicked(it->id);
                setSelectedTarget(it->id);
                event->accept();
                return;
            }
        }
    }

    QWidget::mousePressEvent(event);
}

void TacticalMapWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 确保全场景始终可见，目标不会因视图尺寸变化而消失或聚集在角落
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

bool TacticalMapWidget::eventFilter(QObject *obj, QEvent *event)
{
    // 父控件 resizeEvent 触发时布局尚未激活, 视口仍是旧尺寸(构造期为默认 98x28),
    // 此时 fitInView 会把缩放冻结在错误值(启动时底图缩成屏幕中央小点);
    // 视图自身 Resize 事件发生在布局给出真实尺寸之后, 以此为准重适配。
    // 注意 Resize 事件先于 QGraphicsView 自身 resizeEvent 到达本过滤器, 此刻
    // 视口子控件几何尚未同步更新, 直接 fit 仍会读到旧视口, 故排队到本轮事件
    // 链处理完毕后再执行, 保证读到最终视口几何
    if (obj == m_view && event->type() == QEvent::Resize) {
        QMetaObject::invokeMethod(this, [this] {
            m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
        }, Qt::QueuedConnection);
    }
    return QWidget::eventFilter(obj, event);
}

void TacticalMapWidget::onPulseTick()
{
    m_pulsePhase = (m_pulsePhase + 1) % PULSE_PHASES;
    double t = static_cast<double>(m_pulsePhase) / PULSE_PHASES;
    double radius = PULSE_INIT_RADIUS + t * (DOT_RADIUS * 3);
    int alpha = static_cast<int>(200.0 * (1.0 - t));

    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        if (it->pending || it->pulse == nullptr || it->dot == nullptr) {
            continue;
        }
        auto *pulse = it->pulse;
        auto *dot = it->dot;
        QPointF center = dot->rect().center();
        pulse->setRect(center.x() - radius, center.y() - radius,
                       radius * 2, radius * 2);
        QPen p = pulse->pen();
        p.setColor(QColor(it->color.red(), it->color.green(), it->color.blue(), alpha));
        pulse->setPen(p);
    }
}
