// 主窗口门禁采集工具：用 offscreen 平台渲染 MainWindow，导航到指定页面（默认 DEC-NAV-03
// 决策页）后保存整窗 PNG，用于 UI 升级像素门禁的基线/当前 A/B 比对。
// 仅本地 mock 数据（MainWindow 构造时 loadMockData），不连接设备、网络或真实规划会话。

#include "MainWindow/MainWindow.h"

#include <QApplication>
#include <QAbstractButton>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QPixmap>
#include <QStringList>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 参数 1：输出 PNG 路径（缺省 /tmp/opencode/gate/main.png）
    const QString outPath = app.arguments().size() > 1
        ? app.arguments().at(1)
        : QStringLiteral("/tmp/opencode/gate/main.png");

    // 参数 2：视口 "宽x高"（缺省 1920x1080，与历批像素门禁基准视口一致；
    // 三视口门禁传 1280x720 / 3840x2160 采集其余两档）
    QSize viewport(1920, 1080);
    if (app.arguments().size() > 2) {
        const QStringList parts = app.arguments().at(2).split(QLatin1Char('x'));
        if (parts.size() == 2) {
            bool okW = false, okH = false;
            const int w = parts.at(0).toInt(&okW);
            const int h = parts.at(1).toInt(&okH);
            if (okW && okH && w > 0 && h > 0)
                viewport = QSize(w, h);
        }
    }

    // 参数 3：目标导航页对象名后缀（缺省 "03" = 决策页 DEC-NAV-03）
    const QString navId = app.arguments().size() > 3
        ? app.arguments().at(3)
        : QStringLiteral("03");

    // 稳定等待：跑事件循环直到超时，确保布局、样式打磨与延迟填充完成
    // （与历批门禁 recipe 的 settle 语义一致：切页后首次采集给足 2000ms）
    auto settle = [&app](int ms) {
        Q_UNUSED(app);
        QEventLoop loop;
        QTimer::singleShot(ms, &loop, &QEventLoop::quit);
        loop.exec();
    };

    MainWindow win;
    win.resize(viewport);
    win.show();
    settle(2000);

    auto *nav = win.findChild<QAbstractButton *>(QStringLiteral("DEC-NAV-%1").arg(navId));
    if (nav == nullptr) {
        fprintf(stderr, "导航按钮缺失: DEC-NAV-%s\n", qPrintable(navId));
        return 1;
    }
    nav->click();
    settle(800);

    QFileInfo fi(outPath);
    QDir().mkpath(fi.absolutePath());
    const QPixmap pixmap = win.grab();
    if (!pixmap.save(outPath, "PNG")) {
        fprintf(stderr, "保存截图失败: %s\n", qPrintable(outPath));
        return 1;
    }
    fprintf(stdout, "主窗口截图已保存: %s (页面 DEC-NAV-%s, %dx%d)\n",
            qPrintable(outPath), qPrintable(navId), pixmap.width(), pixmap.height());
    return 0;
}
