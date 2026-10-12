#include "MainWindow/GprView.h"

#include "MainWindow/GprBScanWidget.h"
#include "MainWindow/GprSuspectPanel.h"

#include <QtConcurrent/QtConcurrent>
#include <QComboBox>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>

namespace {

// 首个未禁用通道项（损坏通道在切换器中被禁用，§6.3）：
// 返回其 UserRole 数据，rowOut（可为空）回带行号；无可用通道返回空串。
QString firstEnabledChannelData(const QComboBox *combo, int *rowOut = nullptr)
{
    const auto *model = qobject_cast<const QStandardItemModel *>(combo->model());
    if (model == nullptr)
        return QString();
    for (int row = 0; row < model->rowCount(); ++row) {
        const QStandardItem *item = model->item(row);
        if (item != nullptr && item->isEnabled()) {
            if (rowOut != nullptr)
                *rowOut = row;
            return combo->itemData(row, Qt::UserRole).toString();
        }
    }
    return QString();
}

} // namespace

GprView::GprView(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("gprView"));

    // 异步通道加载结果监听：后台线程产出 → 主线程 onChannelLoadFinished 消费
    m_loadWatcher = new QFutureWatcher<ChannelLoadResult>(this);
    connect(m_loadWatcher, &QFutureWatcher<ChannelLoadResult>::finished,
            this, &GprView::onChannelLoadFinished);

    // 回放 tick：50 ms；1× ≈ 81 道/秒（§5.4 实测点率）
    m_playbackTimer = new QTimer(this);
    m_playbackTimer->setInterval(50);
    connect(m_playbackTimer, &QTimer::timeout, this, &GprView::onPlaybackTick);

    setupUi();
    setState(PageState::Empty, QStringLiteral("尚未加载数据集"));
}

GprView::~GprView()
{
    // 后台任务只访问自身按值副本（不触碰本对象成员），取消即可，无需阻塞等待线程结束
    m_loadWatcher->cancel();
}

bool GprView::loadDataset(const QString &rootDir)
{
    // 重新扫描：复位数据集及派生状态（轨迹、损坏通道黑名单），避免旧目录残留
    m_dataset = GPR::GprDataset();
    m_track = GPR::GprTrack();
    m_brokenChannels.clear();

    if (!m_dataset.scan(rootDir)) {
        // 空态降级（§6.3/§5.5）：指明缺失路径并给出配置说明
        setState(PageState::Empty,
                 QStringLiteral("数据集目录无效或不含成对的 .rad/.rd3 通道文件：%1\n\n"
                                "配置说明：GPR 样例数据根目录由编译期宏 GPR_SAMPLE_DATA_DIR 注入"
                                "（CMake 配置），也可在运行期指定数据集目录；"
                                "目录内需包含成对的 .rad/.rd3 通道文件，可选 .cor RTK 轨迹文件。")
                     .arg(rootDir));
        return false;
    }

    // .cor 轨迹加载失败可容忍：回放条经纬度/时间戳显示占位符 “-”
    if (!m_dataset.corPath().isEmpty())
        m_track.loadFromFile(m_dataset.corPath());

    // 默认选首个频段（字典序，样例数据为 H，§5.4）与首个可用通道
    m_currentBand = m_dataset.bands().first();
    rebuildBandCombo();
    rebuildChannelCombo();

    int firstRow = -1;
    m_currentChannel = firstEnabledChannelData(m_channelCombo, &firstRow);
    if (firstRow >= 0) {
        // 同步切换器显示；itemData 与 m_currentChannel 相同，onChannelComboChanged 早退不重载
        m_channelCombo->setCurrentIndex(firstRow);
    }
    startChannelLoad();
    return true;
}

bool GprView::loadDefaultDataset()
{
#ifdef GPR_SAMPLE_DATA_DIR
    // 编译期注入的样例数据根目录（GPR 静态库 PUBLIC 宏，§5.5）
    return loadDataset(QStringLiteral(GPR_SAMPLE_DATA_DIR));
#else
    // 宏未定义 → 空态降级（§6.3），说明配置途径
    setState(PageState::Empty,
             QStringLiteral("未配置 GPR 样例数据目录（编译期宏 GPR_SAMPLE_DATA_DIR 未定义）。\n\n"
                            "配置说明：请经 CMake 注入样例数据根目录后重新编译，"
                            "或在运行期指定数据集目录。"));
    return false;
#endif
}

bool GprView::hasDataset() const
{
    return !m_dataset.bands().isEmpty();
}

void GprView::onBandComboChanged(int)
{
    m_currentBand = m_bandCombo->currentText();
    emit bandChanged(m_currentBand);   // UI 契约测试探针（§7.3）

    rebuildChannelCombo();

    // 频段切换后默认选首个可用通道（跳过损坏禁用项，§6.3）
    int firstRow = -1;
    m_currentChannel = firstEnabledChannelData(m_channelCombo, &firstRow);
    if (firstRow >= 0) {
        // 同步切换器显示；itemData 与 m_currentChannel 相同，onChannelComboChanged 早退不重载
        m_channelCombo->setCurrentIndex(firstRow);
    }
    emit channelChanged(m_currentChannel);
    startChannelLoad();
}

void GprView::onChannelComboChanged(int index)
{
    const QString channel = m_channelCombo->itemData(index, Qt::UserRole).toString();
    // 空数据（清空重建瞬间）或与当前一致（程序化同步选中）时不重复加载
    if (channel.isEmpty() || channel == m_currentChannel)
        return;
    m_currentChannel = channel;
    emit channelChanged(m_currentChannel);   // UI 契约测试探针（§7.3）
    startChannelLoad();
}

void GprView::onGainModeComboChanged(int)
{
    m_bscanWidget->setGainMode(
        static_cast<GprBScanWidget::GainMode>(m_gainModeCombo->currentData().toInt()));
}

void GprView::onContrastSliderChanged(int value)
{
    // 滑条 10–300 → 系数 0.1–3.0（100 = 1.0）
    m_bscanWidget->setContrast(value / 100.0);
}

void GprView::onZoomSliderChanged(int value)
{
    // 滑条 10–400 → 系数 0.1–4.0（100 = 1.0）
    m_bscanWidget->setZoom(value / 100.0);
}

void GprView::onPlayPauseClicked()
{
    // 仅正常态可回放（§6.3）
    if (m_state != PageState::Normal)
        return;

    if (m_playing) {
        // 暂停
        m_playing = false;
        m_playPauseBtn->setText(QStringLiteral("播放"));
        m_playbackTimer->stop();
        return;
    }

    // 播放到底后再次点击：从头回放
    if (m_cursorTrace >= m_scan.traceCount() - 1)
        setCursorTrace(0);

    m_playing = true;
    m_playPauseBtn->setText(QStringLiteral("暂停"));
    m_playbackTimer->start();
}

void GprView::onSpeedComboChanged(int)
{
    m_speed = m_speedCombo->currentData().toDouble();
}

void GprView::onProgressSliderMoved(int value)
{
    setCursorTrace(value);
}

void GprView::onPlaybackTick()
{
    // 非正常态（通道切换/加载/出错）不推进：避免在已解除绑定的旧数据上移动游标；
    // 播放状态保留，回到正常态后从当前游标继续
    if (m_state != PageState::Normal || !m_scan.isValid())
        return;

    // 步进 = 50 ms × 81 道/秒 × 速度倍率（1× ≈ 81 道/秒，§5.4 实测点率），至少 1 道
    const int advance = qMax(1, qRound(81.0 * m_speed * 0.05));
    const int next = m_cursorTrace + advance;
    const int lastTrace = m_scan.traceCount() - 1;
    if (next >= lastTrace) {
        // 播放到底：游标停在最后一道并自动暂停（§6.3 禁用态）
        setCursorTrace(lastTrace);
        m_playing = false;
        m_playPauseBtn->setText(QStringLiteral("播放"));
        m_playbackTimer->stop();
        return;
    }
    setCursorTrace(next);
}

void GprView::onChannelLoadFinished()
{
    const ChannelLoadResult result = m_loadWatcher->result();
    // 过期结果（期间用户又切换了频段/通道）：静默丢弃
    if (result.generation != m_loadGeneration)
        return;
    applyLoadResult(result);
}

void GprView::setupUi()
{
    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- 中间列：工具栏 / 中心栈 / 回放条（§6.2）----
    auto *centerColumn = new QWidget(this);
    auto *centerLayout = new QVBoxLayout(centerColumn);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    // 顶部工具栏：数据组 | 通道 | 增益 | 对比度/缩放 | “模拟数据”标注
    auto *toolbar = new QWidget(centerColumn);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 4, 8, 4);
    toolbarLayout->setSpacing(6);

    // 数据组（频段 H/L）
    toolbarLayout->addWidget(new QLabel(QStringLiteral("数据组"), toolbar));
    m_bandCombo = new QComboBox(toolbar);
    m_bandCombo->setObjectName(QStringLiteral("gprBandCombo"));
    toolbarLayout->addWidget(m_bandCombo);

    // 通道（16 组 T/R 对）
    toolbarLayout->addWidget(new QLabel(QStringLiteral("通道"), toolbar));
    m_channelCombo = new QComboBox(toolbar);
    m_channelCombo->setObjectName(QStringLiteral("gprChannelCombo"));
    toolbarLayout->addWidget(m_channelCombo);

    // 增益模式：默认选中首项 AGC（§5.4 显示参数建议）
    toolbarLayout->addWidget(new QLabel(QStringLiteral("增益"), toolbar));
    m_gainModeCombo = new QComboBox(toolbar);
    m_gainModeCombo->setObjectName(QStringLiteral("gprGainModeCombo"));
    m_gainModeCombo->addItem(QStringLiteral("AGC"),
                             static_cast<int>(GprBScanWidget::GainMode::Agc));
    m_gainModeCombo->addItem(QStringLiteral("固定"),
                             static_cast<int>(GprBScanWidget::GainMode::Fixed));
    m_gainModeCombo->addItem(QStringLiteral("原始"),
                             static_cast<int>(GprBScanWidget::GainMode::Raw));
    toolbarLayout->addWidget(m_gainModeCombo);

    // 对比度：10–300 → 系数 0.1–3.0（1.0 = 默认）
    toolbarLayout->addWidget(new QLabel(QStringLiteral("对比度"), toolbar));
    m_contrastSlider = new QSlider(Qt::Horizontal, toolbar);
    m_contrastSlider->setObjectName(QStringLiteral("gprContrastSlider"));
    m_contrastSlider->setRange(10, 300);
    m_contrastSlider->setValue(100);
    m_contrastSlider->setFixedWidth(90);
    toolbarLayout->addWidget(m_contrastSlider);

    // 缩放：10–400 → 系数 0.1–4.0（1.0 = 适配控件宽度）
    toolbarLayout->addWidget(new QLabel(QStringLiteral("缩放"), toolbar));
    m_zoomSlider = new QSlider(Qt::Horizontal, toolbar);
    m_zoomSlider->setObjectName(QStringLiteral("gprZoomSlider"));
    m_zoomSlider->setRange(10, 400);
    m_zoomSlider->setValue(100);
    m_zoomSlider->setFixedWidth(90);
    toolbarLayout->addWidget(m_zoomSlider);

    toolbarLayout->addStretch();

    // “模拟数据”标注（§6.4 强制）：排版沿用地图工具栏 simTag 的属性模式
    m_simTagLabel = new QLabel(QStringLiteral("模拟数据"), toolbar);
    m_simTagLabel->setObjectName(QStringLiteral("gprSimTagLabel"));
    m_simTagLabel->setProperty("labelRole", QStringLiteral("caption"));
    m_simTagLabel->setProperty("textColor", QStringLiteral("medium"));
    toolbarLayout->addWidget(m_simTagLabel);

    centerLayout->addWidget(toolbar);

    // 中心栈：index 0 = B-Scan 剖面；index 1 = 加载/空态/错误消息页
    m_centerStack = new QStackedWidget(centerColumn);
    m_centerStack->setObjectName(QStringLiteral("gprCenterStack"));

    // objectName gprBScanWidget 由 GprBScanWidget 自身构造函数设置（UI 契约，§7.3）
    m_bscanWidget = new GprBScanWidget(m_centerStack);
    m_centerStack->addWidget(m_bscanWidget);          // index 0

    m_stateLabel = new QLabel(m_centerStack);
    m_stateLabel->setObjectName(QStringLiteral("gprStateLabel"));
    m_stateLabel->setAlignment(Qt::AlignCenter);
    m_stateLabel->setWordWrap(true);
    // 次级文字色（状态提示不与正文争夺注意力）：沿用 labelRole/textColor 属性模式
    m_stateLabel->setProperty("labelRole", QStringLiteral("caption"));
    m_stateLabel->setProperty("textColor", QStringLiteral("medium"));
    m_centerStack->addWidget(m_stateLabel);           // index 1

    centerLayout->addWidget(m_centerStack, 1);

    // 底部回放条：[播放/暂停] [速度] [进度条] [当前道 n/N | RTK 经纬度 | 时间戳]
    auto *playbackBar = new QWidget(centerColumn);
    auto *playbackLayout = new QHBoxLayout(playbackBar);
    playbackLayout->setContentsMargins(8, 4, 8, 4);
    playbackLayout->setSpacing(6);

    m_playPauseBtn = new QPushButton(QStringLiteral("播放"), playbackBar);
    m_playPauseBtn->setObjectName(QStringLiteral("gprPlayPauseButton"));
    playbackLayout->addWidget(m_playPauseBtn);

    playbackLayout->addWidget(new QLabel(QStringLiteral("速度"), playbackBar));
    m_speedCombo = new QComboBox(playbackBar);
    m_speedCombo->setObjectName(QStringLiteral("gprSpeedCombo"));
    m_speedCombo->addItem(QStringLiteral("1×"), 1.0);
    m_speedCombo->addItem(QStringLiteral("2×"), 2.0);
    m_speedCombo->addItem(QStringLiteral("4×"), 4.0);
    playbackLayout->addWidget(m_speedCombo);

    m_progressSlider = new QSlider(Qt::Horizontal, playbackBar);
    m_progressSlider->setObjectName(QStringLiteral("gprProgressSlider"));
    m_progressSlider->setRange(0, 0);                 // 有效数据加载后按道数重设
    playbackLayout->addWidget(m_progressSlider, 1);   // 进度条吃满剩余宽度

    m_positionLabel = new QLabel(QStringLiteral("-"), playbackBar);
    m_positionLabel->setObjectName(QStringLiteral("gprPositionLabel"));
    playbackLayout->addWidget(m_positionLabel);

    centerLayout->addWidget(playbackBar);

    rootLayout->addWidget(centerColumn, 1);

    // ---- 右侧：疑似目标面板（320px，对齐全局面板宽度，§6.2）----
    // objectName gprSuspectPanel 由 GprSuspectPanel 自身构造函数设置（UI 契约，§7.3）
    m_suspectPanel = new GprSuspectPanel(this);
    m_suspectPanel->setFixedWidth(320);
    rootLayout->addWidget(m_suspectPanel);

    // ---- 信号连接（默认项填充完毕后再连接，避免构造期触发）----
    connect(m_bandCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GprView::onBandComboChanged);
    connect(m_channelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GprView::onChannelComboChanged);
    connect(m_gainModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GprView::onGainModeComboChanged);
    connect(m_contrastSlider, &QSlider::valueChanged, this, &GprView::onContrastSliderChanged);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &GprView::onZoomSliderChanged);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &GprView::onPlayPauseClicked);
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GprView::onSpeedComboChanged);
    // 进度条值变化（拖动/键盘）→ 游标；回放期间的程序赋值经 blockSignals 防止回环
    connect(m_progressSlider, &QSlider::valueChanged, this, &GprView::onProgressSliderMoved);

    // 疑似目标列表选中行 → 回放游标跳转到对应道（联动便利，§6.2）
    connect(m_suspectPanel, &GprSuspectPanel::suspectSelected, this, [this](int suspectId) {
        for (const GPR::GprSuspect &suspect : m_suspects) {
            if (suspect.id == suspectId) {
                setCursorTrace(suspect.traceIndex);
                return;
            }
        }
    });
    // 确认/误报：先同步转发意图信号（MainWindow 槽内仍能经 suspects() 读取该候选），
    // 再从本地复核列表移除（候选处置完成即离开队列，防止重复确认注入重复目标，§5.3）
    connect(m_suspectPanel, &GprSuspectPanel::confirmRequested, this, [this](int suspectId) {
        emit suspectConfirmRequested(suspectId);
        removeSuspect(suspectId);
    });
    connect(m_suspectPanel, &GprSuspectPanel::falsePositiveRequested, this, [this](int suspectId) {
        emit suspectFalsePositiveRequested(suspectId);
        removeSuspect(suspectId);
    });
}

QList<GPR::GprSuspect> GprView::suspects() const
{
    return m_suspects;
}

GPR::GprTrackPoint GprView::trackPointForTrace(int traceIndex) const
{
    return m_track.smoothedPointForTrace(traceIndex);
}

QString GprView::currentBand() const
{
    return m_currentBand;
}

QString GprView::currentChannel() const
{
    return m_currentChannel;
}

// 处置（确认/误报）后从本地复核列表移除并刷新 B-Scan 标记与面板；
// 列表中无该 id 时不触碰面板（面板可被外部直接 setSuspects 喂展示数据，见契约测试）
void GprView::removeSuspect(int suspectId)
{
    for (int i = 0; i < m_suspects.size(); ++i) {
        if (m_suspects[i].id == suspectId) {
            m_suspects.removeAt(i);
            m_bscanWidget->setSuspects(m_suspects);
            m_suspectPanel->setSuspects(m_suspects);
            return;
        }
    }
}

void GprView::setState(PageState state, const QString &message)
{
    m_state = state;

    // 页面切换（§6.3）：正常态显示 B-Scan，其余状态显示消息页
    if (state == PageState::Normal) {
        m_centerStack->setCurrentIndex(0);
    } else {
        m_stateLabel->setText(message);
        m_centerStack->setCurrentIndex(1);
    }

    // 控件可用性（§6.3）：
    //   空态：回放条与工具栏全部禁用（数据路径未配置 → 降级）；
    //   加载态：回放条禁用（无完整数据可回放），工具栏保持可用；
    //   正常态：已有有效数据，回放条可用；
    //   错误态：回放条禁用（当前通道未加载成功），工具栏保持可用以便切换其余通道。
    const bool playbackEnabled = (state == PageState::Normal);
    const bool toolbarEnabled = (state != PageState::Empty);
    m_playPauseBtn->setEnabled(playbackEnabled);
    m_speedCombo->setEnabled(playbackEnabled);
    m_progressSlider->setEnabled(playbackEnabled);
    m_bandCombo->setEnabled(toolbarEnabled);
    m_channelCombo->setEnabled(toolbarEnabled);
    m_gainModeCombo->setEnabled(toolbarEnabled);
    m_contrastSlider->setEnabled(toolbarEnabled);
    m_zoomSlider->setEnabled(toolbarEnabled);
}

void GprView::rebuildBandCombo()
{
    // 重建期间屏蔽信号：避免程序化变更触发 onBandComboChanged 重入加载
    const bool blocked = m_bandCombo->blockSignals(true);
    m_bandCombo->clear();
    m_bandCombo->addItems(m_dataset.bands());
    const int bandIndex = m_bandCombo->findText(m_currentBand);
    if (bandIndex >= 0)
        m_bandCombo->setCurrentIndex(bandIndex);
    m_bandCombo->blockSignals(blocked);
}

void GprView::rebuildChannelCombo()
{
    // 重建期间屏蔽信号：避免程序化变更触发 onChannelComboChanged 重入加载
    const bool blocked = m_channelCombo->blockSignals(true);
    m_channelCombo->clear();
    const QStringList channels = m_dataset.channels(m_currentBand);
    for (const QString &channel : channels) {
        m_channelCombo->addItem(channel, channel);   // 文本 = 通道名，UserRole = 通道名
        // §6.3 错误态：损坏通道在切换器中禁用并标注“（损坏）”
        if (m_brokenChannels.contains(m_currentBand + QLatin1Char('/') + channel)) {
            const int row = m_channelCombo->count() - 1;
            if (auto *model = qobject_cast<QStandardItemModel *>(m_channelCombo->model())) {
                if (QStandardItem *item = model->item(row))
                    item->setEnabled(false);
            }
            m_channelCombo->setItemText(row, channel + QStringLiteral("（损坏）"));
        }
    }
    // 保持当前通道选中（若仍存在）
    const int currentRow = m_channelCombo->findData(m_currentChannel, Qt::UserRole);
    if (currentRow >= 0)
        m_channelCombo->setCurrentIndex(currentRow);
    m_channelCombo->blockSignals(blocked);
}

void GprView::startChannelLoad()
{
    const GPR::GprDatasetEntry entry = m_dataset.entryFor(m_currentBand, m_currentChannel);
    if (!entry.isValid()) {
        setState(PageState::Error,
                 QStringLiteral("通道条目缺失：%1/%2").arg(m_currentBand, m_currentChannel));
        return;
    }

    setState(PageState::Loading,
             QStringLiteral("正在加载通道 %1/%2 …").arg(m_currentBand, m_currentChannel));

    // 线程安全：后台任务启动前，先在 GUI 线程解除旧数据绑定；
    // 此后后台线程只访问自身按值副本，主线程结果经 applyLoadResult 移交
    m_bscanWidget->setScan(nullptr, nullptr);

    // 代际计数：加载期间用户再次切换频段/通道时，旧结果在 onChannelLoadFinished 中丢弃
    const int generation = ++m_loadGeneration;
    m_loadWatcher->setFuture(QtConcurrent::run([entry, generation]() {
        ChannelLoadResult r;
        r.generation = generation;
        r.band = entry.band;
        r.channel = entry.channel;
        r.headerOk = r.header.loadFromFile(entry.radPath);
        if (r.headerOk)
            r.scanOk = r.scan.loadFromFile(entry.rd3Path, r.header.samples());
        if (r.scanOk) {
            GPR::GprThresholdDetector detector;
            r.suspects = detector.detect(r.scan, r.header);
        }
        return r;
    }));
}

void GprView::applyLoadResult(const ChannelLoadResult &result)
{
    if (!result.headerOk || !result.scanOk) {
        // §6.3 错误态：损坏通道记入黑名单，在切换器中禁用并标注，其余通道保持可用
        const QString key = result.band + QLatin1Char('/') + result.channel;
        if (!m_brokenChannels.contains(key))
            m_brokenChannels.append(key);
        rebuildChannelCombo();
        setState(PageState::Error,
                 QStringLiteral("通道 %1/%2 加载失败（文件损坏或格式不符），已在切换器中禁用")
                     .arg(result.band, result.channel));
        return;
    }

    // 拷贝代价低：GprBScanData 按路径懒加载，仅元数据与隐式共享缓存队列参与拷贝
    m_header = result.header;
    m_scan = result.scan;
    m_suspects = result.suspects;

    m_bscanWidget->setScan(&m_scan, &m_header);
    m_bscanWidget->setSuspects(m_suspects);
    m_suspectPanel->setSuspects(m_suspects);

    m_progressSlider->setRange(0, m_scan.traceCount() - 1);
    setCursorTrace(0);
    setState(PageState::Normal);
}

void GprView::updatePlaybackBar()
{
    // 无有效数据：整条占位显示
    if (!m_scan.isValid() || m_scan.traceCount() <= 0) {
        m_positionLabel->setText(QStringLiteral("-"));
        return;
    }

    // 坐标/时间戳占位；轨迹有效且该道有平滑点时填充实测值
    QString coordText = QStringLiteral("经度 - 纬度 -");
    QString timeText = QStringLiteral("-");
    if (m_track.isValid()) {
        const GPR::GprTrackPoint point = m_track.smoothedPointForTrace(m_cursorTrace);
        if (point.valid) {
            coordText = QStringLiteral("经度 %1 纬度 %2")
                            .arg(point.longitude, 0, 'f', 6)
                            .arg(point.latitude, 0, 'f', 6);
            // .cor 时间戳：UTC 存储、本地显示（§5.4）
            timeText = point.timestampUtc.toLocalTime()
                           .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        }
    }

    // 当前道按 1 基道号显示（对齐 .cor 道号口径；游标内部 0 基）
    m_positionLabel->setText(QStringLiteral("当前道 %1/%2 | %3 | %4")
                                 .arg(m_cursorTrace + 1)
                                 .arg(m_scan.traceCount())
                                 .arg(coordText, timeText));
}

void GprView::setCursorTrace(int traceIndex)
{
    // 无有效数据时不设置游标（保持隐藏）
    if (!m_scan.isValid())
        return;

    // 夹取到 [0, 道数-1]
    m_cursorTrace = qBound(0, traceIndex, m_scan.traceCount() - 1);
    m_bscanWidget->setCursorTrace(m_cursorTrace);

    // 程序赋值屏蔽信号：防止触发 onProgressSliderMoved 回环
    const bool blocked = m_progressSlider->blockSignals(true);
    m_progressSlider->setValue(m_cursorTrace);
    m_progressSlider->blockSignals(blocked);

    updatePlaybackBar();
}
