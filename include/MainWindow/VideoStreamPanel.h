#ifndef MAINWINDOW_VIDEOSTREAMPANEL_H
#define MAINWINDOW_VIDEOSTREAMPANEL_H

// 探测阶段视频面板: 标题栏(24px) + 视频画面区(QVideoWidget + QMediaPlayer)
// 标题栏含: 绿色状态点 + 设备名称 + 主次切换/最小化/关闭 3 按钮
// VideoOverlayWidget 作为透明叠加层覆盖在视频画面上, 仅持久显示 HUD
// QVideoProbe 拦截视频帧用于检测证据截图

#include <QWidget>
#include <QMediaPlayer>
#include <QImage>

class QLabel;
class QPushButton;
class QVideoWidget;
class QVideoProbe;
class QVideoFrame;
class VideoOverlayWidget;

class VideoStreamPanel : public QWidget
{
    Q_OBJECT

public:
    explicit VideoStreamPanel(QWidget *parent = nullptr);
    ~VideoStreamPanel();

    // 加载本地视频文件
    void loadVideo(const QString& path);

    // 播放控制接口
    void play();
    void pause();
    void stop();
    void seek(qint64 ms);

    // 当前播放位置 (ms), 未播放时返回 0
    qint64 position() const;
    // 视频总时长 (ms), 未加载时返回 0
    qint64 duration() const;

    // 当前视频帧的深拷贝快照（detached），无帧时返回空 QImage
    QImage currentFrameSnapshot() const;
    // 是否已拦截到至少一帧（用于测试断言）
    bool hasFrame() const;

    // 视频叠加层访问 (供外部设置 HUD 设备信息与遥测)
    VideoOverlayWidget* overlay() const;

    // 设置标题栏设备名称
    void setDeviceTitle(const QString& title);

    // 最小化/恢复
    void setMinimized(bool minimized);

signals:
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void stateChanged(QMediaPlayer::State state);
    // 视频播放结束（到达末尾）
    void videoEnded();
    // PiP 标题栏按钮信号
    void swapRequested();
    void minimizeRequested();
    void closeRequested();

public slots:
    void onFrameProbed(const QVideoFrame &frame);

private slots:
    void onPositionChanged(qint64 ms);
    void onDurationChanged(qint64 ms);
    void onStateChanged(QMediaPlayer::State state);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    void setupUi();
    void createTitleBar();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

    // 标题栏控件
    QWidget *m_titleBar;
    QLabel *m_statusDot;
    QLabel *m_titleLabel;
    QPushButton *m_swapBtn;
    QPushButton *m_minimizeBtn;
    QPushButton *m_closeBtn;

    // 视频区控件
    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    QVideoProbe *m_probe;
    QImage m_lastFrame;
    bool m_stopped = false;
    VideoOverlayWidget *m_overlay;
    QWidget *m_videoArea;
};

#endif
