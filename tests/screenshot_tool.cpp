// 截图工具：用 offscreen 平台渲染 DecisionView 并保存 PNG，用于与 HTML 原型对比。
// 仅本地合成 fixture，不连接设备、网络或真实规划会话。

#include "MainWindow/DecisionView.h"
#include "Common/GlobalStyle.h"
#include "Core/MOS/MosFixtureGenerator.h"
#include "Core/MOS/MosPlanner.h"

#include <QApplication>
#include <QPixmap>
#include <QPainter>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 输出路径（命令行第一个参数或默认）
    const QString outPath = app.arguments().size() > 1
        ? app.arguments().at(1)
        : QStringLiteral("/tmp/opencode/screenshots/decision_view.png");

    // 构造合法有解快照（与测试一致：seed=42，3 档，2 弹坑 + 2 UXO）
    Core::MOS::MosRunwayParams params;
    Core::MOS::MosGeneratorParams gen;
    const qint32 seed = 42;
    const auto obstacles = Core::MOS::MosFixtureGenerator::generate(params, gen, seed);
    const auto result = Core::MOS::MosPlanner::planProgressive(obstacles, params);

    Core::MOS::MosPlanningSnapshot snap;
    snap.obstacles = obstacles;
    snap.params = params;
    snap.result = result;
    snap.hasResult = true;
    snap.selectedTier = 1;
    snap.committedRevision = 1;

    // GlobalStyle 在 app 级应用（低优先级），不覆盖 DecisionView 自身 stylesheet，
    // 与 MainWindow 中 setStyleSheet 的层级一致
    qApp->setStyleSheet(GlobalStyle::getMainWindowStyle());
    DecisionView view;

    // 视口参数（命令行第二个参数，格式 "宽x高"，如 1920x1080；缺省 1280x720）。
    // 用于 UI 升级像素门禁的三视口 A/B 采集（1280x720 / 1920x1080 / 3840x2160）；
    // 基线与当前两侧使用同一份工具源码，保证采集路径一致、只有被测代码不同。
    QSize viewport(1280, 720);
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
    view.resize(viewport);
    view.setSnapshot(snap);

    // 显示并处理事件以确保布局完成
    view.show();
    app.processEvents();
    QTimer::singleShot(100, &app, &QApplication::quit);
    app.exec();

    // grab() 渲染到 QPixmap
    QPixmap pixmap = view.grab();

    // 确保输出目录存在
    QFileInfo fi(outPath);
    QDir().mkpath(fi.absolutePath());

    // 保存 PNG
    if (!pixmap.save(outPath, "PNG")) {
        fprintf(stderr, "保存截图失败: %s\n", qPrintable(outPath));
        return 1;
    }
    fprintf(stdout, "截图已保存: %s (%dx%d)\n",
            qPrintable(outPath), pixmap.width(), pixmap.height());
    return 0;
}
