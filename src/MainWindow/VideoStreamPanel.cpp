#include "MainWindow/VideoStreamPanel.h"
#include "MainWindow/VideoOverlayWidget.h"
#include "Common/GlobalStyle.h"

#include <QVideoWidget>
#include <QVideoProbe>
#include <QVideoFrame>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QEvent>

// 标题栏高度 (像素, 对齐原型 --pip-titlebar-h: 24px)
static constexpr int kTitleBarHeight = 24;

VideoStreamPanel::VideoStreamPanel(QWidget *parent)
    : QWidget(parent)
    , m_titleBar(nullptr)
    , m_statusDot(nullptr)
    , m_titleLabel(nullptr)
    , m_swapBtn(nullptr)
    , m_minimizeBtn(nullptr)
    , m_closeBtn(nullptr)
    , m_player(nullptr)
    , m_videoWidget(nullptr)
    , m_probe(nullptr)
    , m_overlay(nullptr)
    , m_videoArea(nullptr)
{
    setupUi();
}

VideoStreamPanel::~VideoStreamPanel()
{
    if (m_player) {
        // 先停止播放再释放媒体资源，确保 GStreamer 管道被正确清理
        // 防止进程退出后 GStreamer 资源残留导致无法重新启动
        m_player->stop();
        m_player->setMedia(QMediaContent());
    }
}

void VideoStreamPanel::setupUi()
{
    setStyleSheet(QString("background-color: %1;")
                  .arg(GlobalStyle::Colors::Background));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // === 标题栏 ===
    createTitleBar();
    mainLayout->addWidget(m_titleBar);

    // === 视频区: QVideoWidget + VideoOverlayWidget 叠加 ===
    // overlay 作为 QVideoWidget 的子 widget, 避免非原生 overlay 与原生
    // QVideoWidget 之间的 backing store 合成冲突 (黑屏闪烁根因)
    m_videoArea = new QWidget(this);
    m_videoArea->setStyleSheet("background-color: #000000;");

    QVBoxLayout *videoLayout = new QVBoxLayout(m_videoArea);
    videoLayout->setContentsMargins(0, 0, 0, 0);
    videoLayout->setSpacing(0);

    m_videoWidget = new QVideoWidget(m_videoArea);
    m_videoWidget->setStyleSheet("background-color: #000000;");
    videoLayout->addWidget(m_videoWidget);

    // HUD 叠加层 (透明, 作为 QVideoWidget 子 widget, 同一原生渲染面)
    m_overlay = new VideoOverlayWidget(m_videoWidget);
    m_overlay->setDeviceInfo(QStringLiteral("UAV-1"), QStringLiteral("UAV-1 侦察无人机"));

    m_videoWidget->installEventFilter(this);
    m_overlay->setGeometry(m_videoWidget->rect());

    mainLayout->addWidget(m_videoArea, 1);

    // QMediaPlayer: 输出到 QVideoWidget (原生渲染)
    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(m_videoWidget);
    // 静音: 真实使用不需要音频, 同时消除 A/V 同步开销减少卡顿
    m_player->setMuted(true);

    // QVideoProbe: 拦截视频帧用于检测证据截图
    m_probe = new QVideoProbe(this);
    m_probe->setSource(m_player);
    connect(m_probe, &QVideoProbe::videoFrameProbed,
            this, &VideoStreamPanel::onFrameProbed);
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &VideoStreamPanel::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &VideoStreamPanel::onDurationChanged);
    connect(m_player, &QMediaPlayer::stateChanged,
            this, &VideoStreamPanel::onStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, &VideoStreamPanel::onMediaStatusChanged);
}

void VideoStreamPanel::createTitleBar()
{
    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName(QStringLiteral("pipTitleBar"));
    m_titleBar->setFixedHeight(kTitleBarHeight);
    m_titleBar->setStyleSheet(
        QStringLiteral("QWidget#pipTitleBar { background-color: %1; }")
        .arg(GlobalStyle::Colors::ToolbarBackground));

    QHBoxLayout *layout = new QHBoxLayout(m_titleBar);
    layout->setContentsMargins(8, 0, 4, 0);
    layout->setSpacing(6);

    // 绿色在线状态点
    m_statusDot = new QLabel(m_titleBar);
    m_statusDot->setFixedSize(8, 8);
    m_statusDot->setStyleSheet(
        QStringLiteral("background-color: %1; border-radius: 4px;")
        .arg(GlobalStyle::Colors::StatusOnline));
    layout->addWidget(m_statusDot);

    // 设备名称
    m_titleLabel = new QLabel(QStringLiteral("UAV-1 侦察无人机"), m_titleBar);
    m_titleLabel->setStyleSheet(
        QStringLiteral("color: %1; font-size: 11px; border: none;")
        .arg(GlobalStyle::Colors::TextPrimary));
    layout->addWidget(m_titleLabel);

    layout->addStretch();

    const QString btnStyle = QString(
        "QPushButton { background-color: transparent; color: %1; border: none; "
        "padding: 2px 6px; font-size: 12px; }"
        "QPushButton:hover { background-color: %2; }")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::Border);

    // 主次切换按钮
    m_swapBtn = new QPushButton(QStringLiteral("⇄"), m_titleBar);
    m_swapBtn->setObjectName(QStringLiteral("pipSwapButton"));
    m_swapBtn->setToolTip(QStringLiteral("主次切换"));
    m_swapBtn->setStyleSheet(btnStyle);
    m_swapBtn->setFixedSize(24, 20);
    layout->addWidget(m_swapBtn);

    // 最小化按钮
    m_minimizeBtn = new QPushButton(QStringLiteral("-"), m_titleBar);
    m_minimizeBtn->setObjectName(QStringLiteral("pipMinimizeButton"));
    m_minimizeBtn->setToolTip(QStringLiteral("最小化"));
    m_minimizeBtn->setStyleSheet(btnStyle);
    m_minimizeBtn->setFixedSize(24, 20);
    layout->addWidget(m_minimizeBtn);

    // 关闭按钮
    m_closeBtn = new QPushButton(QStringLiteral("✕"), m_titleBar);
    m_closeBtn->setObjectName(QStringLiteral("pipCloseButton"));
    m_closeBtn->setToolTip(QStringLiteral("关闭"));
    m_closeBtn->setStyleSheet(QString(
        "QPushButton { background-color: transparent; color: %1; border: none; "
        "padding: 2px 6px; font-size: 12px; }"
        "QPushButton:hover { background-color: %2; color: %3; }")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::DangerRed)
        .arg(GlobalStyle::Colors::TextPrimary));
    m_closeBtn->setFixedSize(24, 20);
    layout->addWidget(m_closeBtn);

    connect(m_swapBtn, &QPushButton::clicked, this, &VideoStreamPanel::swapRequested);
    connect(m_minimizeBtn, &QPushButton::clicked, this, &VideoStreamPanel::minimizeRequested);
    connect(m_closeBtn, &QPushButton::clicked, this, &VideoStreamPanel::closeRequested);
}

bool VideoStreamPanel::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_videoWidget && event->type() == QEvent::Resize) {
        m_overlay->setGeometry(m_videoWidget->rect());
    }
    return QWidget::eventFilter(watched, event);
}

void VideoStreamPanel::setDeviceTitle(const QString& title)
{
    m_titleLabel->setText(title);
    m_overlay->setDeviceInfo(title.left(title.indexOf(QLatin1Char(' '))),
                             title);
}

void VideoStreamPanel::setMinimized(bool minimized)
{
    m_videoArea->setVisible(!minimized);
}

void VideoStreamPanel::loadVideo(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }
    m_player->setMedia(QUrl::fromLocalFile(path));
}

void VideoStreamPanel::play()
{
    m_player->play();
    m_stopped = false;
}

void VideoStreamPanel::pause()
{
    m_player->pause();
}

void VideoStreamPanel::stop()
{
    m_player->stop();
    m_lastFrame = QImage();
    m_stopped = true;
}

void VideoStreamPanel::seek(qint64 ms)
{
    m_player->setPosition(ms);
}

qint64 VideoStreamPanel::position() const
{
    return m_player->position();
}

qint64 VideoStreamPanel::duration() const
{
    return m_player->duration();
}

QImage VideoStreamPanel::currentFrameSnapshot() const
{
    return m_lastFrame;
}

bool VideoStreamPanel::hasFrame() const
{
    return !m_lastFrame.isNull();
}

VideoOverlayWidget* VideoStreamPanel::overlay() const
{
    return m_overlay;
}

void VideoStreamPanel::onPositionChanged(qint64 ms)
{
    emit positionChanged(ms);
}

void VideoStreamPanel::onDurationChanged(qint64 ms)
{
    emit durationChanged(ms);
}

void VideoStreamPanel::onStateChanged(QMediaPlayer::State state)
{
    emit stateChanged(state);
}

void VideoStreamPanel::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    // 媒体到达结尾: 暂停播放并发出 videoEnded 信号
    if (status == QMediaPlayer::EndOfMedia) {
        m_player->pause();
        emit videoEnded();
    }
}

void VideoStreamPanel::onFrameProbed(const QVideoFrame &frame)
{
    // 停止状态下不接收帧，避免在 stop() 后仍写入 m_lastFrame
    if (m_stopped) {
        return;
    }
    QVideoFrame copy(frame);
    if (!copy.map(QAbstractVideoBuffer::ReadOnly)) {
        return;
    }
    const QImage::Format fmt = QVideoFrame::imageFormatFromPixelFormat(copy.pixelFormat());
    if (fmt != QImage::Format_Invalid) {
        m_lastFrame = QImage(copy.bits(), copy.width(), copy.height(),
                             copy.bytesPerLine(), fmt).copy();
    }
    copy.unmap();
}
