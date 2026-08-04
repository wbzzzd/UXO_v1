#include "MainWindow/TacticalMapWidget.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QVBoxLayout>
#include <QTimer>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QLinearGradient>
#include <QResizeEvent>

// 战场坐标系范围 (米) - 与 DemoScenarioProvider 保持一致
static constexpr double MAP_MIN_M = 0.0;
static constexpr double MAP_MAX_M = 5000.0;
// 场景像素尺寸 (逻辑坐标系, 视图自适应缩放)
static constexpr double SCENE_SIZE = 1000.0;
// 中心红点半径 (像素)
static constexpr double DOT_RADIUS = 6.0;
// 脉冲环初始半径
static constexpr double PULSE_INIT_RADIUS = DOT_RADIUS;
// 脉冲动画总相位数 (一个完整周期)
static constexpr int PULSE_PHASES = 20;
// 脉冲周期 (ms)
static constexpr int PULSE_INTERVAL_MS = 50;

TacticalMapWidget::TacticalMapWidget(QWidget *parent)
    : QWidget(parent)
    , m_pulsePhase(0)
{
    setupUi();
    // 启动脉冲动画定时器
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

    // 占位卫星底图: 用线性渐变模拟地形 (深绿到浅绿, 后续可替换为真实卫星图)
    QPixmap bg(SCENE_SIZE, SCENE_SIZE);
    bg.fill(Qt::transparent);
    QPainter p(&bg);
    QLinearGradient grad(0, 0, SCENE_SIZE, SCENE_SIZE);
    grad.setColorAt(0.0, QColor(34, 60, 34));      // 深绿
    grad.setColorAt(0.5, QColor(60, 90, 50));      // 中绿
    grad.setColorAt(1.0, QColor(80, 110, 60));     // 浅绿
    p.fillRect(bg.rect(), grad);
    // 绘制浅色网格线作为坐标参考 (每 500m 一条 = SCENE_SIZE/10)
    p.setPen(QPen(QColor(255, 255, 255, 30), 1));
    for (int i = 1; i < 10; ++i) {
        double x = i * SCENE_SIZE / 10.0;
        p.drawLine(x, 0, x, SCENE_SIZE);
        p.drawLine(0, x, SCENE_SIZE, x);
    }
    p.end();
    m_scene->addPixmap(bg);

    m_view = new QGraphicsView(this);
    m_view->setScene(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setBackgroundBrush(QColor(20, 30, 20));
    m_view->setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
    setLayout(layout);
}

QPointF TacticalMapWidget::mapToScene(const QVector3D& pos) const
{
    // 米坐标 (0..5000) -> 场景坐标 (0..SCENE_SIZE)
    double x = (pos.x() - MAP_MIN_M) / (MAP_MAX_M - MAP_MIN_M) * SCENE_SIZE;
    double y = (pos.y() - MAP_MIN_M) / (MAP_MAX_M - MAP_MIN_M) * SCENE_SIZE;
    return QPointF(x, y);
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
        return;  // 已存在, 避免重复添加
    }

    QPointF sc = mapToScene(target.position);

    // 中心红点
    QPen dotPen(Qt::black, 1);
    QBrush dotBrush(QColor(220, 40, 40));
    auto *dot = m_scene->addEllipse(sc.x() - DOT_RADIUS, sc.y() - DOT_RADIUS,
                                    DOT_RADIUS * 2, DOT_RADIUS * 2, dotPen, dotBrush);

    // 脉冲环 (初始与红点重合, 由 onPulseTick 驱动放大+淡出)
    QPen pulsePen(QColor(220, 40, 40, 200), 2);
    auto *pulse = m_scene->addEllipse(sc.x() - PULSE_INIT_RADIUS,
                                      sc.y() - PULSE_INIT_RADIUS,
                                      PULSE_INIT_RADIUS * 2, PULSE_INIT_RADIUS * 2,
                                      pulsePen, Qt::NoBrush);

    // ID 标签 (目标 ID 显示在红点右上方)
    auto *label = m_scene->addText(target.id);
    QFont f = label->font();
    f.setPointSize(8);
    f.setBold(true);
    label->setFont(f);
    label->setDefaultTextColor(QColor(255, 220, 220));
    label->setPos(sc.x() + DOT_RADIUS + 2, sc.y() - DOT_RADIUS - 14);

    TargetGraphics g{dot, pulse, label, target.id};
    m_items.insert(target.id, g);
}

void TacticalMapWidget::clearTargets()
{
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        m_scene->removeItem(it->dot);
        m_scene->removeItem(it->pulse);
        m_scene->removeItem(it->label);
        delete it->dot;
        delete it->pulse;
        delete it->label;
    }
    m_items.clear();
    m_selectedId.clear();
}

void TacticalMapWidget::setSelectedTarget(const QString& targetId)
{
    // 取消旧选中: 恢复红点边框
    if (!m_selectedId.isEmpty() && m_items.contains(m_selectedId)) {
        m_items[m_selectedId].dot->setPen(QPen(Qt::black, 1));
    }
    m_selectedId = targetId;
    // 设置新选中: 加粗黄色边框
    if (!m_selectedId.isEmpty() && m_items.contains(m_selectedId)) {
        m_items[m_selectedId].dot->setPen(QPen(QColor(255, 220, 0), 2));
    }
}

void TacticalMapWidget::onPulseTick()
{
    // 推进相位, 计算每个目标脉冲环的半径与透明度
    m_pulsePhase = (m_pulsePhase + 1) % PULSE_PHASES;
    // 归一化进度 [0, 1)
    double t = static_cast<double>(m_pulsePhase) / PULSE_PHASES;
    // 半径从初始值放大到约 4 倍
    double radius = PULSE_INIT_RADIUS + t * (DOT_RADIUS * 3);
    // 透明度从 200 淡出到 0
    int alpha = static_cast<int>(200.0 * (1.0 - t));

    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        auto *pulse = it->pulse;
        auto *dot = it->dot;
        QPointF center = dot->rect().center();
        pulse->setRect(center.x() - radius, center.y() - radius,
                       radius * 2, radius * 2);
        QPen p = pulse->pen();
        p.setColor(QColor(220, 40, 40, alpha));
        pulse->setPen(p);
    }
}
