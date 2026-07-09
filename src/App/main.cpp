// 程序入口：创建应用、初始化、显示主窗口、进入事件循环

#include "App/Application.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    App::Application app(argc, argv);

    // 初始化应用：配置、日志、数据库、通信、模块（当前均为占位）
    if (!app.initialize()) {
        qCritical() << "Application initialization failed";
        return -1;
    }

    qInfo() << "Application started successfully";
    // 创建并显示主窗口
    app.run();

    // 进入 Qt 事件循环
    return app.exec();
}
