#include "MainWindow/VideoOverlayWidget.h"

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QDateTime>
#include <QResizeEvent>

// HUD 定时器: 驱动 REC 闪烁 + 时间码更新 (500ms tick)
static constexpr int HUD_INTERVAL_MS = 500;
// 十字准星半尺寸 (像素)
static constexpr double CROSSHAIR_HALF = 20.0;
// REC 指示器红点半径
static constexpr double REC_DOT_RADIUS = 4.0;

VideoOverlayWidget::VideoOverlayWidget(QWidget *parent)
    : QWidget(parent)
    , m_hudTimer(nullptr)
    , m_recBlinkOn(true)
    , m_deviceId(QStringLiteral("UAV-1"))
    , m_deviceName(QStringLiteral("UAV-1 侦察无人机"))
    , m_telemLat(0.0)
    , m_telemLng(0.0)
    , m_telemAlt(0.0)
    , m_telemHeading(0.0)
{
    // 透明背景, 叠加层不消耗鼠标事件 (透传给下层视频控件)
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    m_hudTimer = new QTimer(this);
    m_hudTimer->setInterval(HUD_INTERVAL_MS);
    connect(m_hudTimer, &QTimer::timeout, this, &VideoOverlayWidget::onHudTick);
    m_hudTimer->start();
}

void VideoOverlayWidget::setDeviceInfo(const QString& deviceId, const QString& deviceName)
{
    m_deviceId = deviceId;
    m_deviceName = deviceName;
    update();
}

void VideoOverlayWidget::setTelemetry(double lat, double lng, double alt, double heading)
{
    m_telemLat = lat;
    m_telemLng = lng;
    m_telemAlt = alt;
    m_telemHeading = heading;
    update();
}

void VideoOverlayWidget::clear()
{
    update();
}

void VideoOverlayWidget::onHudTick()
{
    m_recBlinkOn = !m_recBlinkOn;
    update();
}

void VideoOverlayWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    drawCrosshair(p);
    drawHudText(p);
    drawRecIndicator(p);
}

void VideoOverlayWidget::drawCrosshair(QPainter& p)
{
    double cx = width() / 2.0;
    double cy = height() / 2.0;
    p.setPen(QPen(QColor(255, 255, 255, 128), 1));
    p.drawLine(QPointF(cx - CROSSHAIR_HALF, cy), QPointF(cx + CROSSHAIR_HALF, cy));
    p.drawLine(QPointF(cx, cy - CROSSHAIR_HALF), QPointF(cx, cy + CROSSHAIR_HALF));
    p.setBrush(QColor(255, 255, 255, 180));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(cx, cy), 2.0, 2.0);
}

void VideoOverlayWidget::drawHudText(QPainter& p)
{
    QFont f(QStringLiteral("monospace"), 10);
    f.setStyleHint(QFont::Monospace);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255, 217));

    const int margin = 8;
    const int lineH = 13;

    // 左上角: 设备 ID + 名称
    p.drawText(margin, margin + lineH, m_deviceId);
    p.drawText(margin, margin + lineH * 2, m_deviceName);

    // 右上角: 经纬度 + 高度 + 航向（来自无人机遥测）
    QString tr1 = QStringLiteral("LAT: %1").arg(m_telemLat, 0, 'f', 4);
    QString tr2 = QStringLiteral("LON: %1").arg(m_telemLng, 0, 'f', 4);
    QString tr3 = QStringLiteral("ALT: %1m").arg(m_telemAlt, 0, 'f', 0);
    QString tr4 = QStringLiteral("HDG: %1°").arg(m_telemHeading, 0, 'f', 0);
    int trX = width() - margin;
    p.drawText(trX - 100, margin + lineH, tr1);
    p.drawText(trX - 100, margin + lineH * 2, tr2);
    p.drawText(trX - 100, margin + lineH * 3, tr3);
    p.drawText(trX - 100, margin + lineH * 4, tr4);

    // 右下角: 时间码
    QString timecode = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    int blY = height() - margin - lineH;
    p.drawText(trX - 80, blY, timecode);
}

void VideoOverlayWidget::drawRecIndicator(QPainter& p)
{
    if (!m_recBlinkOn) {
        return;
    }
    double cx = width() / 2.0;
    double cy = 12.0;

    p.setBrush(QColor(211, 47, 47));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(cx - 20, cy), REC_DOT_RADIUS, REC_DOT_RADIUS);

    QFont f(QStringLiteral("monospace"), 9, QFont::Bold);
    f.setStyleHint(QFont::Monospace);
    p.setFont(f);
    p.setPen(QColor(211, 47, 47));
    p.drawText(static_cast<int>(cx - 12), static_cast<int>(cy + 4), QStringLiteral("REC"));
}
