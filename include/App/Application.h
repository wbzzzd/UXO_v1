#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QString>

namespace App {

class Application : public QApplication
{
    Q_OBJECT

public:
    explicit Application(int &argc, char **argv);
    ~Application();

    bool initialize();
    void run();

signals:
    void initialized();
    void errorOccurred(const QString &message);

private:
    bool loadConfiguration();
    bool initializeLogging();
    bool initializeDatabase();
    bool initializeCommunication();
    bool initializeModules();

    QString m_configPath;
    bool m_initialized;
};

}

#endif
