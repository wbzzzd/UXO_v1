#ifndef MAINWINDOW_VIDEOSTREAMPANEL_H
#define MAINWINDOW_VIDEOSTREAMPANEL_H

#include <QWidget>
#include <QMediaPlayer>

class QVideoWidget;
class QStackedLayout;
class QLabel;
class VideoOverlayWidget;

// 探测阶段视频面板: QMediaPlayer + QVideoWidget 单画面播放
// 替代原 4 格 QLabel 文本占位; 暴露播放控制接口与位置变化信号
// VideoOverlayWidget 作为透明叠加层覆盖在视频画面上, 由外部触发红框
class VideoStreamPanel : public QWidget
{
    Q_OBJECT

public:
    explicit VideoStreamPanel(QWidget *parent = nullptr);
    ~VideoStreamPanel();

    // 加载本地视频文件; path 为空时进入空状态
    void loadVideo(const QString& path);

    // 播放控制接口 (供探测工具栏 / DetectionTimelineController 调用)
    void play();
    void pause();
    void stop();
    void seek(qint64 ms);

    // 当前播放位置 (ms), 未播放时返回 0
    qint64 position() const;
    // 视频总时长 (ms), 未加载时返回 0
    qint64 duration() const;

    // 视频叠加层访问 (供外部触发红框)
    VideoOverlayWidget* overlay() const;

signals:
    // 播放位置变化 (ms)
    void positionChanged(qint64 ms);
    // 视频总时长变化 (ms)
    void durationChanged(qint64 ms);
    // 播放状态变化 (QMediaPlayer::State)
    void stateChanged(QMediaPlayer::State state);

private slots:
    void onPositionChanged(qint64 ms);
    void onDurationChanged(qint64 ms);
    void onStateChanged(QMediaPlayer::State state);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    void setupUi();
    void showEmptyHint(bool visible);

    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    VideoOverlayWidget *m_overlay;
    QLabel *m_emptyHint;        // 空状态提示 "等待开始"
    QStackedLayout *m_stackLayout;
};

#endif
