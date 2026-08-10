// Widget 树导出工具：输出 DecisionView 所有子控件的几何、颜色、字体、样式表信息为文本。
// 用于与 HTML 原型做代码级视觉对比。

#include "MainWindow/DecisionView.h"
#include "Common/GlobalStyle.h"
#include "Core/MOS/MosFixtureGenerator.h"
#include "Core/MOS/MosPlanner.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QFrame>
#include <QGroupBox>
#include <QScrollArea>
#include <QTimer>
#include <QFile>
#include <QTextStream>

// 递归导出 widget 树
static void dumpWidgetTree(QWidget *w, QTextStream &out, int depth = 0)
{
    const QString indent(depth * 2, ' ');

    // 基本信息
    QString cls = w->metaObject()->className();
    QString name = w->objectName();
    QRect r = w->geometry();
    QRect vis = w->visibleRegion().boundingRect();
    bool visible = w->isVisible();

    // 样式信息
    QPalette pal = w->palette();
    QColor bg = pal.color(w->backgroundRole());
    QColor fg = pal.color(QPalette::WindowText);
    QString styleSheet = w->styleSheet();
    QFont font = w->font();

    // 文本（如果有）
    QString text;
    bool checkable = false, checked = false, enabled = true;
    if (auto *lbl = qobject_cast<QLabel*>(w)) text = lbl->text();
    else if (auto *btn = qobject_cast<QPushButton*>(w)) {
        text = btn->text();
        checkable = btn->isCheckable();
        checked = btn->isChecked();
        enabled = btn->isEnabled();
    }

    // 输出
    out << indent << "[" << cls << "]";
    if (!name.isEmpty()) out << " name='" << name << "'";
    if (!text.isEmpty()) out << " text='" << text.left(80) << "'";
    out << QString(" geom=(%1,%2,%3x%4)").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
    out << QString(" visible=%1").arg(visible);
    if (checkable) out << " checkable";
    if (checked) out << " CHECKED";
    if (!enabled) out << " disabled";
    if (visible) {
        out << QString(" bg=%1").arg(bg.name());
        out << QString(" fg=%1").arg(fg.name());
        out << QString(" font=%1pt").arg(font.pointSize());
        if (font.bold()) out << " bold";
    }
    if (!styleSheet.isEmpty()) {
        // 只输出前 200 字符的样式表
        out << "\n" << indent << "  styleSheet: " << styleSheet.left(200).replace("\n", " ");
    }
    out << "\n";

    // 递归子控件
    const auto children = w->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto *child : children) {
        dumpWidgetTree(child, out, depth + 1);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const QString outPath = app.arguments().size() > 1
        ? app.arguments().at(1)
        : QStringLiteral("/tmp/opencode/screenshots/widget_tree.txt");

    // 构造合法有解快照
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
    view.show();
    app.processEvents();
    QTimer::singleShot(100, &app, &QApplication::quit);
    app.exec();

    // 确保输出目录存在
    QFileInfo fi(outPath);
    QDir().mkpath(fi.absolutePath());

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "无法打开输出文件: %s\n", qPrintable(outPath));
        return 1;
    }
    QTextStream out(&file);
    out << "=== DecisionView Widget Tree Dump ===\n";
    out << "Size: " << view.width() << "x" << view.height() << "\n";
    out << "ViewportScale: " << view.viewportScale() << "\n\n";
    dumpWidgetTree(&view, out);
    out.flush();
    file.close();

    fprintf(stdout, "Widget tree saved: %s\n", qPrintable(outPath));
    return 0;
}
