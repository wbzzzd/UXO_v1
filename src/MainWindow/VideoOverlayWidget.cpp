#include "MainWindow/VideoOverlayWidget.h"

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QResizeEvent>

// 闪烁参数: 总时长 1000ms, 每 125ms 切换一次可见状态 -> 8 次切换 = 4 次闪烁
// 设计文档要求 "闪烁 2 次后消失", 取较明显的 4 次闪烁以达到视觉清晰
static constexpr int BLINK_INTERVAL_MS = 125;
static constexpr int BLINK_TOTAL_TICKS = 8;

VideoOverlayWidget::VideoOverlayWidget(QWidget *parent)
    : QWidget(parent)
    , m_visible(false)
    , m_blinkCount(0)
{
    // 透明背景, 不拦截底层视频控件的鼠标事件
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setInterval(BLINK_INTERVAL_MS);
    connect(m_blinkTimer, &QTimer::timeout, this, &VideoOverlayWidget::onBlinkTick);
}

void VideoOverlayWidget::showRedBox(const QRectF& normalizedRect)
{
    m_normalizedRect = normalizedRect;
    m_visible = true;
    m_blinkCount = 0;
    if (!m_blinkTimer->isActive()) {
        m_blinkTimer->start();
    }
    update();
}

void VideoOverlayWidget::clear()
{
    m_blinkTimer->stop();
    m_visible = false;
    m_blinkCount = 0;
    update();
}

void VideoOverlayWidget::onBlinkTick()
{
    ++m_blinkCount;
    if (m_blinkCount >= BLINK_TOTAL_TICKS) {
        // 闪烁周期结束, 隐藏红框
        m_blinkTimer->stop();
        m_visible = false;
    } else {
        // 切换可见状态
        m_visible = !m_visible;
    }
    update();
}

void VideoOverlayWidget::paintEvent(QPaintEvent * /*event*/)
{
    if (!m_visible || m_normalizedRect.isNull()) {
        return;
    }

    // 将归一化坐标 [0,1] 映射到控件实际像素尺寸
    QRectF pixelRect(
        m_normalizedRect.x() * width(),
        m_normalizedRect.y() * height(),
        m_normalizedRect.width() * width(),
        m_normalizedRect.height() * height()
    );

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    // 鲜红色粗边框, 不填充
    QPen pen(QColor(255, 40, 40), 3);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(pixelRect);
}
