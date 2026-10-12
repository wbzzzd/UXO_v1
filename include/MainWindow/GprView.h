#ifndef MAINWINDOW_GPRVIEW_H
#define MAINWINDOW_GPRVIEW_H

// 雷达页主视图（REQ-012 / gpr-display.md §5.3、§6）：经 QStackedWidget index 3 路由（§6.1）。
// 布局（§6.2）：
//   顶部工具栏：数据组切换(H/L) | 通道切换(16 组 T/R 对) | 增益模式(AGC/固定/原始)
//               | 对比度/缩放 | "模拟数据"标注（§6.4）
//   中心：GprBScanWidget（回放游标 + 疑似标记叠加）；状态消息页（加载/空态/错误）
//   底部回放条：[播放/暂停] [速度 1×/2×/4×] [进度条] [当前道 n/N | RTK 经纬度 | 时间戳]
//   右侧：GprSuspectPanel（320px，对齐全局面板宽度）
// 状态规则（§6.3）：正常 / 加载 / 空态（降级：数据路径未配置或目录不存在 → 中心提示 +
//   配置说明文字、回放控件禁用）/ 错误（单通道损坏 → 切换器中禁用并标注，其余可用）/
//   禁用态（播放到底、无检测结果时对应控件禁用）。
// 异步加载：B-Scan 校验 + t0 估计 + 阈值检测经 QtConcurrent 后台执行（加载态不阻塞窗口）；
//   代际计数丢弃过期结果。
// 状态所有权：疑似列表与回放位置属本视图展示态；确认后目标走 SimulationWorkflow
//   权威状态机（§5.3）。确认/误报意图信号同步发出后该候选即从本地复核列表移除
//   （处置完成即离开复核队列，防止重复确认注入重复目标；信号先发使 MainWindow
//   槽内仍能经 suspects() 读取被处置候选）。
// 展示数据为模拟/演示用途（§6.4）。

#include "GPR/GprBScanData.h"
#include "GPR/GprDataset.h"
#include "GPR/GprRadarHeader.h"
#include "GPR/GprThresholdDetector.h"
#include "GPR/GprTrack.h"

#include <QWidget>

class GprBScanWidget;
class GprSuspectPanel;
class QComboBox;
class QLabel;
class QPushButton;
class QSlider;
class QStackedWidget;
class QTimer;
template <typename T> class QFutureWatcher;

class GprView : public QWidget
{
    Q_OBJECT

public:
    explicit GprView(QWidget *parent = nullptr);
    ~GprView() override;

    // 扫描数据集根目录并加载 .cor 轨迹；失败进入空态（降级，§6.3）并返回 false。
    // 成功时默认选首个频段（字典序，样例数据为 H）首通道并启动异步通道加载。
    bool loadDataset(const QString &rootDir);

    // 以编译期宏 GPR_SAMPLE_DATA_DIR 为根目录调用 loadDataset（§5.5）；
    // 宏未定义或路径无效时进入空态并返回 false。
    bool loadDefaultDataset();

    bool hasDataset() const;

    // 当前通道检测候选快照（1 基 id；确认/误报处置后从列表移除）
    QList<GPR::GprSuspect> suspects() const;

    // 道索引 → 地理配准位置（高斯平滑轨迹，与回放条定位口径一致，§5.2）；
    // 轨迹未载入或索引无有效位置时返回 valid=false
    GPR::GprTrackPoint trackPointForTrace(int traceIndex) const;

    // 当前频段/通道 id（确认目标溯源写入 radarDataPath）；未加载时为空串
    QString currentBand() const;
    QString currentChannel() const;

signals:
    // 频段/通道切换通知（UI 契约测试探针，§7.3）
    void bandChanged(const QString &band);
    void channelChanged(const QString &channel);
    // 疑似目标操作意图（阶段 3 由 MainWindow 接线做目标注入）
    void suspectConfirmRequested(int suspectId);
    void suspectFalsePositiveRequested(int suspectId);

private slots:
    void onBandComboChanged(int index);
    void onChannelComboChanged(int index);
    void onGainModeComboChanged(int index);
    void onContrastSliderChanged(int value);
    void onZoomSliderChanged(int value);
    void onPlayPauseClicked();
    void onSpeedComboChanged(int index);
    void onProgressSliderMoved(int value);
    void onPlaybackTick();
    void onChannelLoadFinished();

private:
    // §6.3 状态；Error 用于当前通道加载失败提示（单通道损坏同时在切换器禁用标注）
    enum class PageState { Normal, Loading, Empty, Error };

    // 异步通道加载结果（可拷贝；后台线程产出，主线程经 QFutureWatcher 消费）
    struct ChannelLoadResult {
        int generation = 0;                 // 启动时的代际计数，过期结果丢弃
        QString band;
        QString channel;
        bool headerOk = false;
        bool scanOk = false;
        GPR::GprRadarHeader header;
        GPR::GprBScanData scan;
        QList<GPR::GprSuspect> suspects;
    };

    void setupUi();
    void setState(PageState state, const QString &message = QString());
    void rebuildBandCombo();
    void rebuildChannelCombo();             // 损坏通道禁用并标注（§6.3 错误态）
    void startChannelLoad();                // 进入加载态并启动后台加载
    void applyLoadResult(const ChannelLoadResult &result);
    void updatePlaybackBar();               // 当前道 n/N | RTK 经纬度 | 时间戳（UTC 存储、本地显示，§5.4）
    void setCursorTrace(int traceIndex);
    void removeSuspect(int suspectId);          // 处置后从 m_suspects 移除并刷新面板/标记

    // objectName 契约（gpr_view UI 契约测试，§7.3）：
    //   gprBandCombo / gprChannelCombo / gprGainModeCombo / gprContrastSlider / gprZoomSlider
    //   gprSimTagLabel（"模拟数据"标注）/ gprCenterStack / gprStateLabel / gprBScanWidget
    //   gprPlayPauseButton / gprSpeedCombo / gprProgressSlider / gprPositionLabel / gprSuspectPanel
    QComboBox *m_bandCombo = nullptr;
    QComboBox *m_channelCombo = nullptr;
    QComboBox *m_gainModeCombo = nullptr;
    QSlider *m_contrastSlider = nullptr;
    QSlider *m_zoomSlider = nullptr;
    QLabel *m_simTagLabel = nullptr;
    QStackedWidget *m_centerStack = nullptr;
    QLabel *m_stateLabel = nullptr;
    GprBScanWidget *m_bscanWidget = nullptr;
    QPushButton *m_playPauseBtn = nullptr;
    QComboBox *m_speedCombo = nullptr;
    QSlider *m_progressSlider = nullptr;
    QLabel *m_positionLabel = nullptr;
    GprSuspectPanel *m_suspectPanel = nullptr;

    PageState m_state = PageState::Empty;
    GPR::GprDataset m_dataset;
    GPR::GprTrack m_track;                  // .cor 轨迹（内部 UTC 存储，显示转本地）
    QString m_currentBand;
    QString m_currentChannel;
    QStringList m_brokenChannels;           // "band/channel"，加载失败通道（切换器禁用标注）

    GPR::GprRadarHeader m_header;           // 当前通道头（仅主线程读写）
    GPR::GprBScanData m_scan;               // 当前通道数据（仅主线程读写；后台结果经 applyLoadResult 移交）
    QList<GPR::GprSuspect> m_suspects;

    QFutureWatcher<ChannelLoadResult> *m_loadWatcher = nullptr;
    int m_loadGeneration = 0;               // 代际计数：丢弃过期后台结果

    QTimer *m_playbackTimer = nullptr;      // 50 ms tick；1× ≈ 81 道/秒（§5.4 实测点率）
    int m_cursorTrace = -1;
    double m_speed = 1.0;                   // 1 / 2 / 4
    bool m_playing = false;
};

#endif // MAINWINDOW_GPRVIEW_H
