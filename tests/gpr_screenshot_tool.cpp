// GPR 雷达页截图工具（REQ-013 / gpr-display.md §7 验收）：
// offscreen 渲染 GprView（默认样例数据集），按三视口（1280x720 / 1920x1080 / 3840x2160）
// 保存 PNG，用于雷达页布局、B-Scan 成像与 [模拟] 标注的视觉核对。
// 仅本地样例数据（GPR_SAMPLE_DATA_DIR 宏），不连接设备或网络；
// 数据缺失退出码 2，首通道加载失败/超时退出码 3，保存失败退出码 4。

#include "MainWindow/GprView.h"
#include "Common/GlobalStyle.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QPushButton>
#include <QTimer>

namespace {

// 让事件循环空转 ms（等待布局/重绘稳定）
void spinEvents(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

// 等待首通道异步加载完成（回放按钮启用 = 正常态 §6.3），超时返回 false；
// Loading 态下状态页同样可见，不能凭状态页可见性提前判失败，只能等按钮启用或超时
bool waitForNormalState(GprView &view, int timeoutMs)
{
    auto *playPause = view.findChild<QPushButton *>(QStringLiteral("gprPlayPauseButton"));
    if (playPause == nullptr)
        return false;
    QElapsedTimer timer;
    timer.start();
    while (!playPause->isEnabled()) {
        if (timer.elapsed() > timeoutMs)
            return false;
        spinEvents(100);
    }
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // 与 MainWindow 一致：app 级全局样式（低优先级），不覆盖控件自身样式
    qApp->setStyleSheet(GlobalStyle::getMainWindowStyle());

    const QString outDir = app.arguments().size() > 1
        ? app.arguments().at(1)
        : QStringLiteral("/tmp/opencode/screenshots");
    QDir().mkpath(outDir);

    GprView view;
    if (!view.loadDefaultDataset()) {
        qWarning("GPR 样例数据缺失（GPR_SAMPLE_DATA_DIR 未定义或路径无效），无法采集雷达页截图");
        return 2;
    }
    view.resize(1280, 720);
    view.show();
    // /mnt/d（WSL DrvFs）逐道小读较慢，首通道加载实测约 40s，等待上限放宽至 180s
    if (!waitForNormalState(view, 180000)) {
        qWarning("GPR 首通道异步加载失败或超时，无法采集雷达页截图");
        return 3;
    }

    const QSize viewports[] = { QSize(1280, 720), QSize(1920, 1080), QSize(3840, 2160) };
    for (const QSize &viewport : viewports) {
        view.resize(viewport);
        spinEvents(500);   // 布局/重绘稳定后抓帧
        const QString path = QStringLiteral("%1/gpr_view_%2x%3.png")
                                 .arg(outDir)
                                 .arg(viewport.width())
                                 .arg(viewport.height());
        if (!view.grab().save(path)) {
            qWarning() << "截图保存失败:" << path;
            return 4;
        }
        qInfo() << "saved" << path;
    }
    return 0;
}
