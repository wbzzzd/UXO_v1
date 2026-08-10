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
    view.resize(1280, 720);
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
