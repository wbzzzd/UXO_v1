#include "MainWindow/VideoStreamPanel.h"
#include "MainWindow/VideoOverlayWidget.h"
#include "Common/GlobalStyle.h"

#include <QVideoWidget>
#include <QVideoProbe>
#include <QVideoFrame>
#include <QUrl>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QEvent>
#include <QTimer>
#include <QTime>

// 标题栏高度 (像素, 对齐原型 --pip-titlebar-h: 24px)
static constexpr int kTitleBarHeight = 24;
// 直播源自动重连参数（REQ-015 WP7 ④ 黑屏修复）: 基础退避 1s 指数增长、
// 封顶 5s、无限重试；稳定播放 ≥30s 后的故障视为新故障从头退避
static constexpr int kReconnectBaseDelayMs = 1000;
static constexpr int kReconnectMaxDelayMs = 5000;
static constexpr int kStablePlayResetMs = 30000;

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
    // 视频流面板背景走全局 QSS containerBg="main"（=%1 Background=#1E1E1E），构造期静态属性，无需 repolish
    setProperty("containerBg", "main");
    setAttribute(Qt::WA_StyledBackground, true);

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
    // (c) VideoBackground(#000000) 不在 containerBg 词表（main/panel/toolbar/transparent），保留内联 setStyleSheet
    m_videoArea->setStyleSheet(
        QStringLiteral("background-color: %1;")
        .arg(GlobalStyle::Colors::VideoBackground));

    QVBoxLayout *videoLayout = new QVBoxLayout(m_videoArea);
    videoLayout->setContentsMargins(0, 0, 0, 0);
    videoLayout->setSpacing(0);

    m_videoWidget = new QVideoWidget(m_videoArea);
    // (c) VideoBackground(#000000) 不在 containerBg 词表，QVideoWidget 也不匹配 QWidget 属性选择器，保留内联 setStyleSheet
    m_videoWidget->setStyleSheet(
        QStringLiteral("background-color: %1;")
        .arg(GlobalStyle::Colors::VideoBackground));
    videoLayout->addWidget(m_videoWidget);

    // HUD 叠加层 (透明, 作为 QVideoWidget 子 widget, 同一原生渲染面)
    m_overlay = new VideoOverlayWidget(m_videoWidget);
    m_overlay->setDeviceInfo(QStringLiteral("UAV-1"), QStringLiteral("UAV-1 侦察无人机"));

    m_videoWidget->installEventFilter(this);
    m_overlay->setGeometry(m_videoWidget->rect());

    // 断流重连提示徽标（WP7 验收②）: 直播链路中断期间在视频区中央显示
    // 「重连中」提示, 直播恢复后隐藏; 默认隐藏不影响正常观感。
    // 作为 QVideoWidget 子 widget 与 HUD 叠加层同层渲染（非原生叠加与
    // 原生视频表面直接叠放会有合成冲突, 同 overlay 的根因处理方式）
    m_reconnectBadge = new QLabel(m_videoWidget);
    m_reconnectBadge->setAlignment(Qt::AlignCenter);
    // (c) 警示橙 #FFB84D 不在全局 QSS 词表, 徽标属一次性提示元素,
    // 保留内联样式（同 VideoBackground 内联先例）
    m_reconnectBadge->setStyleSheet(
        QStringLiteral("background-color: rgba(0, 0, 0, 170);"
                       "color: #FFB84D; border-radius: 6px;"
                       "padding: 6px 14px; font-weight: bold;"));
    m_reconnectBadge->hide();

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
    // 播放器错误信号（error 与 getter 同名，Qt5 须 QOverload 消歧）:
    // 直播源错误按链路中断转入自动重连（REQ-015 WP7 ④ 黑屏修复）
    connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
            this, &VideoStreamPanel::onPlayerError);

    // 直播自动重连定时器: 单次触发，到点重设媒体并重播
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &VideoStreamPanel::onReconnectTimeout);

    m_extractionTimer = new QTimer(this);
    m_extractionTimer->setInterval(3000);
    connect(m_extractionTimer, &QTimer::timeout,
            this, &VideoStreamPanel::onExtractionTimer);
}

void VideoStreamPanel::createTitleBar()
{
    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName(QStringLiteral("pipTitleBar"));
    // (a) 标题栏背景走全局 QSS containerBg="toolbar"（=%2 ToolbarBackground=#2D2D2D），构造期静态属性先于 setFixedHeight
    m_titleBar->setProperty("containerBg", "toolbar");
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);
    m_titleBar->setFixedHeight(kTitleBarHeight);

    QHBoxLayout *layout = new QHBoxLayout(m_titleBar);
    layout->setContentsMargins(8, 0, 4, 0);
    layout->setSpacing(6);

    // 绿色在线状态点
    m_statusDot = new QLabel(m_titleBar);
    // (a) 状态点圆角走全局 QSS cardRadius="true"（border-radius:4px），构造期静态属性先于 setFixedSize/addWidget
    m_statusDot->setProperty("cardRadius", "true");
    m_statusDot->setAttribute(Qt::WA_StyledBackground, true);
    m_statusDot->setFixedSize(8, 8);
    // (a) StatusOnline(#4CAF50) 不在 containerBg 词表，背景色保留内联
    m_statusDot->setStyleSheet(
        QStringLiteral("background-color: %1;")
        .arg(GlobalStyle::Colors::StatusOnline));
    layout->addWidget(m_statusDot);

    // 设备名称
    m_titleLabel = new QLabel(QStringLiteral("UAV-1 侦察无人机"), m_titleBar);
    // (a) 标题主文本色走全局 QSS textColor="white"（=%3 TextPrimary=#FFFFFF），构造期静态属性先于 addWidget
    m_titleLabel->setProperty("textColor", "white");
    // 像素回归修复（批次3门禁）：基线本面板裸样式表 #1E1E1E 级联到标题标签（标题栏 #2D2D2D 上可见），
    // 属性化后级联消失，labelBg 按标签恢复不透明底
    m_titleLabel->setProperty("labelBg", "main");
    // 属性转换（批次5）：11px 字号走 fontSize 词汇；border:none 为无操作声明
    // （基线 QLabel 规则无边框，标题栏样式表亦无 QLabel 边框级联），删除后像素不变
    m_titleLabel->setProperty("fontSize", "11");
    layout->addWidget(m_titleLabel);

    layout->addStretch();

    // (a) 透明底+主文字色+hover描边色走全局 QSS btnVariant="flat"（bg=transparent, color=%3 TextPrimary, border=none, font-size=%20px=12, hover bg=%4 Border）
    // flat 默认 padding=4px 12px，实际需 2px 6px，保留内联覆盖

    // 主次切换按钮
    m_swapBtn = new QPushButton(QStringLiteral("⇄"), m_titleBar);
    m_swapBtn->setObjectName(QStringLiteral("pipSwapButton"));
    m_swapBtn->setToolTip(QStringLiteral("主次切换"));
    // (a) btnVariant="flat" 覆盖透明底+主色+hover；padding 与 flat 默认 4px 12px 不同，保留内联
    m_swapBtn->setProperty("btnVariant", "flat");
    m_swapBtn->setStyleSheet("padding: 2px 6px;");
    m_swapBtn->setFixedSize(24, 20);
    layout->addWidget(m_swapBtn);

    // 最小化按钮
    m_minimizeBtn = new QPushButton(QStringLiteral("-"), m_titleBar);
    m_minimizeBtn->setObjectName(QStringLiteral("pipMinimizeButton"));
    m_minimizeBtn->setToolTip(QStringLiteral("最小化"));
    // (a) btnVariant="flat" 同 swapBtn
    m_minimizeBtn->setProperty("btnVariant", "flat");
    m_minimizeBtn->setStyleSheet("padding: 2px 6px;");
    m_minimizeBtn->setFixedSize(24, 20);
    layout->addWidget(m_minimizeBtn);

    // 关闭按钮
    m_closeBtn = new QPushButton(QStringLiteral("✕"), m_titleBar);
    m_closeBtn->setObjectName(QStringLiteral("pipCloseButton"));
    m_closeBtn->setToolTip(QStringLiteral("关闭"));
    // (a) btnVariant="flat" 覆盖透明底+主色+字号；padding 与 hover(DangerRed) 与 flat 默认不同，保留内联覆盖
    m_closeBtn->setProperty("btnVariant", "flat");
    m_closeBtn->setStyleSheet(QString(
        "QPushButton { padding: 2px 6px; }"
        "QPushButton:hover { background-color: %1; color: %2; }")
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
        // ② 视频区缩放时保持重连徽标居中（仅可见时调整, 避免无谓 move）
        if (m_reconnectBadge && m_reconnectBadge->isVisible()) {
            centerReconnectBadge();
        }
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
    // 切回文件源: 复位直播标志并清空缓存帧（防跨源旧帧进入抽帧证据）
    m_liveSource = false;
    m_liveClock.invalidate();
    m_lastFrame = QImage();
    // 撤销挂起的自动重连（旧直播源的重连对新源无意义）
    stopReconnect();
    m_player->setMedia(QUrl::fromLocalFile(path));
}

// 直播源加载（REQ-015 WP5 裁决④⑥）: 面板只接收 URL、不解析协议与对端（§9-2 护栏）；
// 换源清空缓存帧，直播时钟待 play() 首次启动
void VideoStreamPanel::loadStreamUrl(const QString& url)
{
    if (url.isEmpty()) {
        return;
    }
    m_liveSource = true;
    m_liveUrl = QUrl(url);  // 缓存直播源: 中断后据此重建管道
    m_liveClock.invalidate();
    m_lastFrame = QImage();
    stopReconnect();  // 手动换源=新一轮连接预算，撤销旧重连排程
    m_player->setMedia(QUrl(url));
}

void VideoStreamPanel::play()
{
    // 直播源首次播放启动本地时钟（换源失效后重播重新起算）
    if (m_liveSource && !m_liveClock.isValid()) {
        m_liveClock.start();
    }
    m_player->play();
    m_extractionTimer->start();
    // 起播即开稳态窗口: 直播稳定播放 ≥30s 后的故障按新故障从头退避
    if (m_liveSource) {
        m_stablePlayStamp.start();
    }
}

void VideoStreamPanel::pause()
{
    m_player->pause();
    m_extractionTimer->stop();
}

void VideoStreamPanel::seek(qint64 ms)
{
    // 直播流无进度语义, seek 禁用（REQ-015 WP5 裁决④）
    if (m_liveSource) {
        return;
    }
    m_player->setPosition(ms);
}

qint64 VideoStreamPanel::position() const
{
    // 直播源用本地时钟（RTSP 流的播放器位置无意义）
    if (m_liveSource) {
        return m_liveClock.isValid() ? m_liveClock.elapsed() : 0;
    }
    return m_player->position();
}

qint64 VideoStreamPanel::duration() const
{
    // 直播流无时长语义, 恒 0（REQ-015 WP5 裁决④）
    if (m_liveSource) {
        return 0;
    }
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

bool VideoStreamPanel::isLiveSource() const
{
    return m_liveSource;
}

VideoOverlayWidget* VideoStreamPanel::overlay() const
{
    return m_overlay;
}

void VideoStreamPanel::onPositionChanged(qint64 ms)
{
    // 直播源改发本地时钟位置（RTSP 播放器位置无意义）
    if (m_liveSource) {
        emit positionChanged(m_liveClock.isValid() ? m_liveClock.elapsed() : 0);
        return;
    }
    emit positionChanged(ms);
}

void VideoStreamPanel::onDurationChanged(qint64 ms)
{
    // 直播流无时长语义, 抑制 durationChanged（REQ-015 WP5 裁决④）
    if (m_liveSource) {
        return;
    }
    emit durationChanged(ms);
}

void VideoStreamPanel::onStateChanged(QMediaPlayer::State state)
{
    // HUD 动画 (REC 闪烁/时间码) 仅播放中驱动; 停止/暂停时停掉 500ms 重绘,
    // 避免持续重绘停止态原生视频表面 (结束->选中后黑块闪烁根因)
    m_overlay->setHudActive(state == QMediaPlayer::PlayingState);
    // ② 直播恢复播放=链路已通, 撤下「重连中」徽标（与 BufferedMedia 双保险,
    // 覆盖后端不发 BufferedMedia 的场景; 断流重试中的短暂 Playing 会被随后的
    // 错误重新点亮徽标, 该瞬时闪隐可接受）
    if (state == QMediaPlayer::PlayingState) {
        hideReconnectBadge();
    }
    emit stateChanged(state);
}

void VideoStreamPanel::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    // 直播流无「结尾」语义（REQ-015 WP5 裁决④）: 不发 videoEnded、不 pause。
    // 但 EndOfMedia/InvalidMedia 对直播意味着「链路中断」（如中转重启截断流），
    // 转入自动重连而非静默黑屏（REQ-015 WP7 ④ 黑屏修复）
    if (m_liveSource) {
        // ② 媒体数据实际缓冲到位 = 链路已恢复, 撤下「重连中」徽标
        if (status == QMediaPlayer::BufferedMedia) {
            hideReconnectBadge();
        }
        if (status == QMediaPlayer::EndOfMedia
                || status == QMediaPlayer::InvalidMedia) {
            scheduleReconnect(QStringLiteral("媒体状态 %1").arg(int(status)));
        }
        return;
    }
    // 媒体到达结尾: 暂停播放并发出 videoEnded 信号
    if (status == QMediaPlayer::EndOfMedia) {
        m_player->pause();
        emit videoEnded();
    }
}

void VideoStreamPanel::onFrameProbed(const QVideoFrame &frame)
{
    QVideoFrame copy(frame);
    if (!copy.map(QAbstractVideoBuffer::ReadOnly)) {
        return;
    }
    const QImage::Format fmt = QVideoFrame::imageFormatFromPixelFormat(copy.pixelFormat());
    if (fmt != QImage::Format_Invalid) {
        m_lastFrame = QImage(copy.bits(), copy.width(), copy.height(),
                             copy.bytesPerLine(), fmt).copy();
        // 新帧到位, 递增序号允许下一次抽帧送检
        ++m_frameSerial;
    }
    copy.unmap();
}

void VideoStreamPanel::onExtractionTimer()
{
    // 幽灵目标修复: 仅在拦截到新帧后才送检; 视频流停滞/中断时
    // m_lastFrame 冻结不变, 不再把同一帧反复送检测引擎产生重复目标
    if (m_lastFrame.isNull() || m_frameSerial == m_lastExtractedSerial) {
        return;
    }
    m_lastExtractedSerial = m_frameSerial;
    // 时间戳统一走 position(): 文件源=播放器位置, 直播源=本地时钟（REQ-015 WP5 裁决④）
    emit frameExtracted(m_lastFrame, position());
}

// 撤销挂起的自动重连并复位计数（换源时调用: 旧源的重连对新源无意义）
void VideoStreamPanel::stopReconnect()
{
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    m_reconnectAttempts = 0;
    m_stablePlayStamp.invalidate();
    // ② 换源撤销重连时同步撤下断流提示徽标
    hideReconnectBadge();
}

// 直播链路中断后排程自动重连: 退避 1s→2s→4s→封顶 5s 无限重试；
// 稳定播放 ≥30s 后的故障按新故障从头退避；同一故障周期内重复错误只排程一次
void VideoStreamPanel::scheduleReconnect(const QString& reason)
{
    if (!m_liveSource) {
        return;  // 文件源错误不属于链路中断，保持原有语义不自动恢复
    }
    if (m_reconnectTimer->isActive()) {
        return;  // 本周期已排程（error 与 InvalidMedia 常并发到达），不叠加
    }
    if (m_stablePlayStamp.isValid()
            && m_stablePlayStamp.elapsed() >= kStablePlayResetMs) {
        // 上一轮连接曾稳定运行: 视为新故障，退避从头计
        m_reconnectAttempts = 0;
    }
    m_stablePlayStamp.invalidate();
    // 移位封顶 3 防 int 溢出（1000<<3=8000 已超 5s 上限，qMin 兜底）
    const int shift = qMin(m_reconnectAttempts, 3);
    const int delayMs = qMin(kReconnectMaxDelayMs, kReconnectBaseDelayMs << shift);
    ++m_reconnectAttempts;
    qWarning() << "[VideoStreamPanel] 直播链路中断:" << reason
               << "|" << m_reconnectAttempts << "次重连将于" << delayMs << "ms 后执行";
    // ② 断流期间在视频区中央显示「重连中」徽标（带尝试次数）
    showReconnectBadge();
    // 停掉残留的半死管道，重连时经 setMedia 重建
    m_player->stop();
    m_reconnectTimer->start(delayMs);
}

// 播放器错误回调: 直播源的网络/资源错误按链路中断转入自动重连
void VideoStreamPanel::onPlayerError(QMediaPlayer::Error error)
{
    if (!m_liveSource) {
        return;
    }
    scheduleReconnect(QStringLiteral("播放器错误 %1: %2")
                          .arg(int(error))
                          .arg(m_player->errorString()));
}

// 重连到点: 重设媒体强制重建 GStreamer 管道（半死管道仅 play() 无法复活）；
// 经 play() 统一处理稳态窗口与抽帧定时器，本地直播时钟保持连续（时间戳单调）
void VideoStreamPanel::onReconnectTimeout()
{
    if (!m_liveSource) {
        return;  // 等待期间已切回文件源，放弃本次重连
    }
    qWarning() << "[VideoStreamPanel] 自动重连: setMedia + play";
    m_player->setMedia(m_liveUrl);
    play();
}

// === 断流重连提示徽标（WP7 验收②: 断流期间画面给出可感知的重连提示）===

// 显示徽标: 更新尝试次数文案并居中到视频区, 同时输出时间戳日志供验收取证
void VideoStreamPanel::showReconnectBadge()
{
    if (!m_reconnectBadge) {
        return;
    }
    m_reconnectBadge->setText(
        QStringLiteral("视频连接中断，正在重连…（第 %1 次）")
            .arg(m_reconnectAttempts));
    centerReconnectBadge();
    m_reconnectBadge->raise();
    m_reconnectBadge->show();
    qWarning() << "[VideoStreamPanel]"
               << QTime::currentTime().toString("hh:mm:ss.zzz")
               << "断流提示徽标已显示（第" << m_reconnectAttempts
               << "次重连已排程）";
}

// 隐藏徽标: 直播恢复/切源时调用; 仅在可见时隐藏并打日志, 避免刷屏
void VideoStreamPanel::hideReconnectBadge()
{
    if (!m_reconnectBadge || !m_reconnectBadge->isVisible()) {
        return;
    }
    m_reconnectBadge->hide();
    qWarning() << "[VideoStreamPanel]"
               << QTime::currentTime().toString("hh:mm:ss.zzz")
               << "断流提示徽标已隐藏（直播恢复）";
}

// 徽标居中: 按当前文本自适应尺寸后摆放到视频区中央（父 widget 坐标系）
void VideoStreamPanel::centerReconnectBadge()
{
    if (!m_reconnectBadge || !m_videoWidget) {
        return;
    }
    m_reconnectBadge->adjustSize();
    const QRect r = m_videoWidget->rect();
    m_reconnectBadge->move(r.center().x() - m_reconnectBadge->width() / 2,
                           r.center().y() - m_reconnectBadge->height() / 2);
}
