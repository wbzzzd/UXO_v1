// 启动可见性测试
// 验证 Application::initialize() 会创建并显示标题为"排弹抢修指挥系统 V1.0"的 MainWindow。

#include "App/Application.h"
#include "MainWindow/MainWindow.h"

#include <QDebug>
#include <QWidget>

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

        return 0;
    }

    qCritical() << "Then Application::run() shows the main command window";
    return 1;
}
