#ifndef MAINWINDOW_VIDEOSTREAMPANEL_H
#define MAINWINDOW_VIDEOSTREAMPANEL_H

// 探测阶段视频面板: 标题栏(24px) + 视频画面区(QVideoWidget + QMediaPlayer)
// 标题栏含: 绿色状态点 + 设备名称 + 主次切换/最小化/关闭 3 按钮
// VideoOverlayWidget 作为透明叠加层覆盖在视频画面上, 仅持久显示 HUD
// QVideoProbe 拦截视频帧用于检测证据截图

#include <QWidget>
#include <QMediaPlayer>
#include <QElapsedTimer>
#include <QImage>
#include <QUrl>

class QLabel;
class QPushButton;
class QVideoWidget;
class QVideoProbe;
class QVideoFrame;
class QTimer;
class VideoOverlayWidget;

class VideoStreamPanel : public QWidget
{
    Q_OBJECT

public:
    explicit VideoStreamPanel(QWidget *parent = nullptr);
    ~VideoStreamPanel();

    // 加载本地视频文件（本地模拟源）
    void loadVideo(const QString& path);
    // 加载链路视频流 URL（真实链路源，REQ-015 WP5 裁决④⑥；
    // 面板不感知协议与对端，仅接收源——§9-2 护栏）
    void loadStreamUrl(const QString& url);
    // 当前是否为直播源（loadStreamUrl 置位，loadVideo 复位）
    bool isLiveSource() const;

    // 播放控制接口
    void play();
    void pause();
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
    // 定时抽帧: 每 3 秒发出当前帧与播放位置 (ms), 供 ONNX 探测分析
    void frameExtracted(const QImage& frame, qint64 timestampMs);
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
    void onExtractionTimer();
    // 直播源播放器错误: 按链路中断转入自动重连（WP7 ④ 黑屏修复）
    void onPlayerError(QMediaPlayer::Error error);
    // 重连定时器到点: 重设媒体并重播
    void onReconnectTimeout();

private:
    void setupUi();
    void createTitleBar();
    // 直播链路中断后排程一次自动重连（指数退避，仅直播源生效）
    void scheduleReconnect(const QString& reason);
    // 撤销挂起的重连并复位计数（换源时调用）
    void stopReconnect();
    // === 断流重连提示徽标（WP7 验收②: 断流期间画面给出可感知的重连提示）===
    // 显示「重连中」徽标并更新尝试次数（每次排程重连时调用）
    void showReconnectBadge();
    // 隐藏徽标（直播恢复缓冲/恢复播放或切源时调用）
    void hideReconnectBadge();
    // 按当前文本自适应尺寸并居中到视频区（显示时与视频区缩放时调用）
    void centerReconnectBadge();

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
    // 帧序号: 每拦截到一帧有效视频帧递增, 用于抽帧去重
    quint64 m_frameSerial = 0;
    // 上次抽帧时已送检的帧序号; 与 m_frameSerial 相等表示无新帧
    quint64 m_lastExtractedSerial = 0;
    // 直播源状态（REQ-015 WP5 裁决④）: URL 源置位，文件语义点按直播适配
    bool m_liveSource = false;
    // 直播本地时钟: play() 首次启动、换源失效；直播 position/抽帧时间戳走此时钟
    QElapsedTimer m_liveClock;
    // === 直播源自动重连状态（REQ-015 WP7 ④ 黑屏修复）===
    // 直播源 URL 缓存: 中断后据此重建管道（面板仍不解析协议，§9-2 护栏不变）
    QUrl m_liveUrl;
    // 单次触发重连定时器（指数退避控制）
    QTimer *m_reconnectTimer = nullptr;
    // 本轮故障已连续重连次数（决定下次退避时长）
    int m_reconnectAttempts = 0;
    // 本次起播时刻: 稳定播放 ≥30s 后的故障按新故障从头退避
    QElapsedTimer m_stablePlayStamp;
    QTimer *m_extractionTimer = nullptr;
    VideoOverlayWidget *m_overlay;
    QWidget *m_videoArea;
    // 断流重连提示徽标: 覆盖在视频画面中央, 默认隐藏（WP7 验收②）
    QLabel *m_reconnectBadge = nullptr;
};

#endif
