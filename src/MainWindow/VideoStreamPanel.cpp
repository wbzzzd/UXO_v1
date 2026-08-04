#include "MainWindow/VideoStreamPanel.h"
#include "MainWindow/VideoOverlayWidget.h"
#include "Common/GlobalStyle.h"

#include <QVideoWidget>
#include <QStackedLayout>
#include <QLabel>
#include <QVBoxLayout>

VideoStreamPanel::VideoStreamPanel(QWidget *parent)
    : QWidget(parent)
    , m_player(nullptr)
    , m_videoWidget(nullptr)
    , m_overlay(nullptr)
    , m_emptyHint(nullptr)
    , m_stackLayout(nullptr)
{
    setupUi();
}

VideoStreamPanel::~VideoStreamPanel()
{
    // QWidget 父子关系自动释放子控件; QMediaPlayer 显式 stop 避免后台播放
    if (m_player) {
        m_player->stop();
    }
}

void VideoStreamPanel::setupUi()
{
    setStyleSheet(QString("background-color: %1;")
                  .arg(GlobalStyle::Colors::Background));

    // QVideoWidget 作为视频输出
    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setStyleSheet("background-color: #000000;");

    // 红框叠加层 (透明, 覆盖在视频画面上)
    m_overlay = new VideoOverlayWidget(this);

    // 空状态提示: 黑屏中央显示 "等待开始"
    m_emptyHint = new QLabel("等待开始", this);
    m_emptyHint->setAlignment(Qt::AlignCenter);
    m_emptyHint->setStyleSheet(
        "color: #888; font-size: 18px; background-color: #000;");

    // 用 QStackedLayout(StackAll) 把视频 / 叠加层 / 空状态提示叠在同一区域
    m_stackLayout = new QStackedLayout(this);
    m_stackLayout->setStackingMode(QStackedLayout::StackAll);
    m_stackLayout->setContentsMargins(0, 0, 0, 0);
    m_stackLayout->addWidget(m_videoWidget);
    m_stackLayout->addWidget(m_overlay);
    m_stackLayout->addWidget(m_emptyHint);

    // 初始空状态: 显示提示, 视频不可见
    showEmptyHint(true);

    // QMediaPlayer (本类持有, 视频输出指向 m_videoWidget)
    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(m_videoWidget);

    connect(m_player, &QMediaPlayer::positionChanged,
            this, &VideoStreamPanel::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &VideoStreamPanel::onDurationChanged);
    connect(m_player, &QMediaPlayer::stateChanged,
            this, &VideoStreamPanel::onStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, &VideoStreamPanel::onMediaStatusChanged);
}

void VideoStreamPanel::showEmptyHint(bool visible)
{
    // 空状态提示可见时覆盖在视频上方; 不可见时让视频画面显示
    m_emptyHint->setVisible(visible);
    m_overlay->setVisible(!visible);
}

void VideoStreamPanel::loadVideo(const QString& path)
{
    if (path.isEmpty()) {
        m_player->setMedia(QMediaContent());
        showEmptyHint(true);
        return;
    }
    m_player->setMedia(QUrl::fromLocalFile(path));
    showEmptyHint(false);
}

void VideoStreamPanel::play()
{
    if (m_player->mediaStatus() == QMediaPlayer::NoMedia) {
        return;  // 无媒体时不播放
    }
    m_player->play();
}

void VideoStreamPanel::pause()
{
    m_player->pause();
}

void VideoStreamPanel::stop()
{
    m_player->stop();
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
    // 媒体到达结尾时自动停止, 保留最后一帧 (符合 80s 结束行为)
    if (status == QMediaPlayer::EndOfMedia) {
        m_player->pause();
    }
}
