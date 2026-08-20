#ifndef MAINWINDOW_VIDEOOVERLAYWIDGET_H
#define MAINWINDOW_VIDEOOVERLAYWIDGET_H

#include <QWidget>
#include <QString>
#include <QTimer>

// 视频画面叠加层: 仅持久 HUD (十字准星 + 四角遥测 + REC)
// 不含检测框、选中、点击命中或闪烁发现框; 鼠标事件透传给下层视频控件。
class VideoOverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoOverlayWidget(QWidget *parent = nullptr);

    // 设置设备信息 (驱动 HUD 左上角文字)
    void setDeviceInfo(const QString& deviceId, const QString& deviceName);
    // 设置遥测数据 (驱动 HUD 右上角经纬度/高度/航向)
    void setTelemetry(double lat, double lng, double alt, double heading);

    // 立即刷新叠加层
    void clear();

    // HUD 动画 (REC 闪烁/时间码) 开关: 仅播放中激活; 停止/暂停时停掉 500ms
    // 重绘定时器, 避免持续重绘停止态原生视频表面造成黑块闪烁
    void setHudActive(bool active);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onHudTick();

private:
    // HUD 定时器 (REC 闪烁 + 时间码更新, 500ms tick)
    QTimer *m_hudTimer;
    bool m_recBlinkOn;

    // 设备信息
    QString m_deviceId;
    QString m_deviceName;

    // 遥测数据
    double m_telemLat;
    double m_telemLng;
    double m_telemAlt;
    double m_telemHeading;

    // 绘制各元素
    void drawCrosshair(QPainter& p);
    void drawHudText(QPainter& p);
    void drawRecIndicator(QPainter& p);
};

#endif
