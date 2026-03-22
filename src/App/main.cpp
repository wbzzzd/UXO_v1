#include "App/Application.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    App::Application app(argc, argv);

    if (!app.initialize()) {
        qCritical() << "Application initialization failed";
        return -1;
    }

    qInfo() << "Application started successfully";
    return app.exec();
}
