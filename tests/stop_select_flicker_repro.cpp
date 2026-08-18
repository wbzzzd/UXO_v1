// 调试复现测试（问题③，临时）：用户序列 开始 -> 结束 -> 点左侧目标，
// 被动统计 PiP 子树 + 目标详情浮层的 Paint/Expose/ZOrderChange 事件风暴。
// 本环境为 Wayland+Xwayland，QScreen::grabWindow(0) 抓根窗口为全黑（无合成内容），
// 屏幕级取证不可用，改用 Qt 事件级取证：闪烁必然伴随反复重绘/重排/提层。
// 事件明细落盘 /tmp/opencode/flicker_repro/events.log；退出码 0=安静 1=事件风暴。

#include "MainWindow/MainWindow.h"
#include "MainWindow/TargetDetailOverlay.h"
#include "MainWindow/VideoStreamPanel.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QTest>

namespace {

struct Widgets {
    QPushButton *startButton = nullptr;
    QPushButton *stopButton = nullptr;
    QTableWidget *targetTable = nullptr;
    VideoStreamPanel *videoPanel = nullptr;
    TargetDetailOverlay *detailOverlay = nullptr;
};

struct EventCounts {
    int paint = 0;
    int expose = 0;
    int show = 0;
    int hide = 0;
    int move = 0;
    int resize = 0;
    int zorder = 0;
    int update = 0;
    int total() const { return paint + expose + show + hide + move + resize + zorder + update; }
};

QString eventName(QEvent::Type t)
{
    switch (t) {
    case QEvent::Paint: return QStringLiteral("Paint");
    case QEvent::Expose: return QStringLiteral("Expose");
    case QEvent::Show:
    case QEvent::ShowToParent: return QStringLiteral("Show");
    case QEvent::Hide:
    case QEvent::HideToParent: return QStringLiteral("Hide");
    case QEvent::Move: return QStringLiteral("Move");
    case QEvent::Resize: return QStringLiteral("Resize");
    case QEvent::ZOrderChange: return QStringLiteral("ZOrderChange");
    case QEvent::UpdateLater: return QStringLiteral("UpdateLater");
    case QEvent::UpdateRequest: return QStringLiteral("UpdateRequest");
    default: return QStringLiteral("Other");
    }
}

class StormCounter : public QObject {
public:
    using QObject::QObject;

    bool eventFilter(QObject *watched, QEvent *event) override {
        const QEvent::Type t = event->type();
        switch (t) {
        case QEvent::Paint: ++m_counts.paint; break;
        case QEvent::Expose: ++m_counts.expose; break;
        case QEvent::Show:
        case QEvent::ShowToParent: ++m_counts.show; break;
        case QEvent::Hide:
        case QEvent::HideToParent: ++m_counts.hide; break;
        case QEvent::Move: ++m_counts.move; break;
        case QEvent::Resize: ++m_counts.resize; break;
        case QEvent::ZOrderChange: ++m_counts.zorder; break;
        case QEvent::UpdateLater:
        case QEvent::UpdateRequest: ++m_counts.update; break;
        default: return QObject::eventFilter(watched, event);
        }
        if (m_recording && m_log.size() < 400) {
            m_log << QStringLiteral("%1 [%2] %3")
                         .arg(QTime::currentTime().toString(QStringLiteral("hh:mm:ss.zzz")),
                              watched->metaObject()->className(),
                              eventName(t));
        }
        return false;
    }

    const EventCounts &counts() const { return m_counts; }
    void setRecording(bool on) { m_recording = on; }
    const QStringList &log() const { return m_log; }

private:
    EventCounts m_counts;
    bool m_recording = false;
    QStringList m_log;
};

void installRecursive(QObject *root, StormCounter *counter)
{
    root->installEventFilter(counter);
    for (QObject *child : root->children()) {
        installRecursive(child, counter);
    }
}

// 用户序列: 点【结束】 -> 等待 -> 点左侧目标行 0 -> 分窗口统计事件率。
// 两种入口模式共用 (快速模式: 播放 ~9s 即结束; eom 模式: 播放到视频自然结束)。
// grab 参数: 突发/持续窗口内每 250ms 对整窗 QWidget::grab() 落盘, 供像素级差分。
void runSelectSequence(QApplication *appPtr, const Widgets &w, StormCounter &counter,
                       const QString &outDir, QWidget *root, bool grabMode)
{
    w.stopButton->click();
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    if (w.targetTable->rowCount() == 0) {
        qCritical() << "[FlickerRepro] no target rows - detection produced nothing";
        appPtr->exit(3);
        return;
    }

    QTest::qWait(500);
    const EventCounts baseline = counter.counts();
    const qint64 baselineMs = 500;

    const QTableWidgetItem *item = w.targetTable->item(0, 0);
    QTest::mouseClick(w.targetTable->viewport(), Qt::LeftButton, Qt::NoModifier,
                      w.targetTable->visualItemRect(item).center());
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    const EventCounts clickEnd = counter.counts();

    auto grabLoop = [root, grabMode](const char *phase, int steps) {
        for (int i = 0; i < steps; ++i) {
            QTest::qWait(250);
            if (grabMode && root != nullptr) {
                root->grab().save(QStringLiteral("%1/grab_%2_%3.png").arg(
                    QStringLiteral("/tmp/opencode/flicker_repro"), QLatin1String(phase), QString::number(i)));
            }
        }
    };

    counter.setRecording(true);
    grabLoop("burst", 4);           // 点击突发余波 (合法一次性重排)
    const EventCounts burstEnd = counter.counts();
    grabLoop("sust", 16);           // 持续窗口: 持续闪烁在此显现
    counter.setRecording(false);
    const EventCounts after = counter.counts();

    QFile logFile(outDir + QStringLiteral("/events.log"));
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream ts(&logFile);
        for (const QString &line : counter.log()) {
            ts << line << '\n';
        }
    }

    auto rate = [](int events, qint64 ms) { return ms > 0 ? double(events) * 1000.0 / double(ms) : 0.0; };
    const double baselineRate = rate(baseline.total(), baselineMs);
    const double burstRate = rate(burstEnd.total() - clickEnd.total(), 1000);
    const double sustainedRate = rate(after.total() - burstEnd.total(), 4000);

    qInfo() << "[FlickerRepro] detailVisible=" << w.detailOverlay->isVisible()
            << " baseline(0.5s): rate/s=" << baselineRate
            << " | burst(1s): rate/s=" << burstRate
            << "paint=" << burstEnd.paint - clickEnd.paint
            << "show/move/resize=" << burstEnd.show - clickEnd.show
            << "/" << burstEnd.move - clickEnd.move
            << "/" << burstEnd.resize - clickEnd.resize
            << " | sustained(4s): total=" << after.total() - burstEnd.total()
            << "paint=" << after.paint - burstEnd.paint
            << "rate/s=" << sustainedRate;

    const bool storm = sustainedRate > 5.0;
    qInfo() << "[FlickerRepro] VERDICT:" << (storm ? "EVENT-STORM-DETECTED" : "QUIET")
            << " logLines=" << counter.log().size();
    appPtr->exit(storm ? 1 : 0);
}

} // namespace

int main(int argc, char *argv[])
{
    const bool eomMode = (argc > 1 && QByteArray(argv[1]) == QByteArrayLiteral("eom"));
    const bool grabMode = (argc > 1 && QByteArray(argv[1]) == QByteArrayLiteral("grab"));
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    window.raise();
    window.activateWindow();
    QTest::qWait(300);

    Widgets w;
    w.startButton = window.findChild<QPushButton *>(QStringLiteral("mapToolbarStart"));
    w.stopButton = window.findChild<QPushButton *>(QStringLiteral("mapToolbarStop"));
    w.targetTable = window.findChild<QTableWidget *>(QStringLiteral("targetTable"));
    w.videoPanel = window.findChild<VideoStreamPanel *>(QStringLiteral("videoPiP"));
    w.detailOverlay = window.findChild<TargetDetailOverlay *>(QStringLiteral("targetDetailOverlay"));

    if (!w.startButton || !w.stopButton || !w.targetTable || !w.videoPanel || !w.detailOverlay) {
        qCritical() << "[FlickerRepro] required widgets missing";
        return 2;
    }

    StormCounter counter;
    installRecursive(w.videoPanel, &counter);
    installRecursive(w.detailOverlay, &counter);

    const QString outDir = QStringLiteral("/tmp/opencode/flicker_repro");
    QDir().mkpath(outDir);
    auto *appPtr = &app;

    QTimer::singleShot(700, w.startButton, &QPushButton::click);

    if (!eomMode) {
        // t≈9.2s: 快速模式--播放 ~8.5s 后直接点【结束】
        QTimer::singleShot(9200, appPtr, [appPtr, w, &counter, outDir, &window, grabMode]() {
            runSelectSequence(appPtr, w, counter, outDir, &window, grabMode);
        });
    } else {
        // eom 模式--播放到视频自然结束 (EndOfMedia 暂停) 后 1s 再点【结束】。
        // 复现用户真实操作: 看完整段探测视频 -> 点结束 -> 点左侧目标。
        QTimer *guard = new QTimer(&window);
        guard->setSingleShot(true);
        QObject::connect(w.videoPanel, &VideoStreamPanel::videoEnded, guard, [appPtr, w, &counter, outDir, guard, &window, grabMode]() {
            if (!guard->isActive()) {
                return;
            }
            guard->stop();
            qInfo() << "[FlickerRepro] video ended - stopping and selecting in 1s";
            QTimer::singleShot(1000, appPtr, [appPtr, w, &counter, outDir, &window, grabMode]() {
                runSelectSequence(appPtr, w, counter, outDir, &window, grabMode);
            });
        });
        // 安全兜底: 时长未知时 240s 后强制走序列, 防挂死
        QObject::connect(guard, &QTimer::timeout, guard, [appPtr, w, &counter, outDir, &window, grabMode]() {
            qWarning() << "[FlickerRepro] videoEnded not seen - forcing sequence";
            runSelectSequence(appPtr, w, counter, outDir, &window, grabMode);
        });
        guard->start(240000);
    }

    return app.exec();
}
