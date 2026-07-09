// 启动可见性测试
// 验证 Application::initialize() 会创建并显示标题为"排弹抢修指挥系统 V1.0"的 MainWindow，
// 且状态栏显示"[模拟模式]"标识。

#include "App/Application.h"
#include "MainWindow/MainWindow.h"
#include "MainWindow/StatusBarWidget.h"

#include <QDebug>
#include <QWidget>
#include <QStatusBar>
#include <QLabel>

int main(int argc, char *argv[])
{
    Application app(argc, argv);

    // 初始化必须成功
    if (!app.initialize()) {
        qCritical() << "Given the application initializes successfully";
        return 1;
    }

    // 运行后应创建主窗口
    app.run();

    // 在顶层窗口中查找 MainWindow
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        auto *mainWindow = qobject_cast<MainWindow *>(widget);
        if (mainWindow == nullptr || !mainWindow->isVisible()) {
            continue;
        }

        // 窗口标题必须是"排弹抢修指挥系统 V1.0"
        if (mainWindow->windowTitle() != QStringLiteral("排弹抢修指挥系统 V1.0")) {
            qCritical() << "Then the main command window has the expected title";
            return 1;
        }

        // 状态栏必须显示"[模拟模式]"标识
        bool foundSimLabel = false;
        for (QObject *child : mainWindow->statusBar()->children()) {
            auto *label = qobject_cast<QLabel*>(child);
            if (label && label->isVisible() && label->text().contains(QStringLiteral("模拟模式"))) {
                foundSimLabel = true;
                break;
            }
        }
        if (!foundSimLabel) {
            // StatusBarWidget 是 statusBar 的子控件，递归查找
            for (QObject *child : mainWindow->statusBar()->children()) {
                auto *sbw = qobject_cast<StatusBarWidget*>(child);
                if (sbw) {
                    for (QObject *gc : sbw->children()) {
                        auto *label = qobject_cast<QLabel*>(gc);
                        if (label && label->isVisible() && label->text().contains(QStringLiteral("模拟模式"))) {
                            foundSimLabel = true;
                            break;
                        }
                    }
                }
                if (foundSimLabel) break;
            }
        }
        if (!foundSimLabel) {
            qCritical() << "Then the status bar shows [模拟模式] indicator";
            return 1;
        }

        return 0;
    }

    qCritical() << "Then Application::run() shows the main command window";
    return 1;
}
