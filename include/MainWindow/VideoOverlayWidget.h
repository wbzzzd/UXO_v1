#ifndef MAINWINDOW_VIDEOOVERLAYWIDGET_H
#define MAINWINDOW_VIDEOOVERLAYWIDGET_H

#include <QWidget>
#include <QRectF>
#include <QTimer>

// 视频画面叠加层: 在视频上方显示归一化坐标的红框
// 红框闪烁 2 次后消失 (约 1 秒), 用于探测阶段目标刚被发现时的高亮提示
// 透明背景, 不拦截鼠标事件 (setAttribute 透明 + 不接收焦点)
class VideoOverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoOverlayWidget(QWidget *parent = nullptr);

    // 显示一个红框; normalizedRect 为相对于视频画面的归一化坐标 [0,1]
    // 触发后闪烁 2 次 (1 秒) 自动隐藏
    void showRedBox(const QRectF& normalizedRect);
    // 立即清除红框
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onBlinkTick();

private:
    QRectF m_normalizedRect;  // 归一化坐标 [0,1]
    bool m_visible;           // 当前帧是否可见 (闪烁用)
    int m_blinkCount;         // 已闪烁次数
    QTimer *m_blinkTimer;
};

#endif
