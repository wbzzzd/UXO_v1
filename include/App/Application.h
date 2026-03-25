#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QString>
#include <QTimer>

class MainWindow;

class Application : public QApplication
{
    Q_OBJECT

public:
    explicit Application(int &argc, char **argv);
    ~Application();

    bool initialize();
    void run();

    MainWindow* mainWindow() const { return m_mainWindow; }

signals:
    void initialized();
    void errorOccurred(const QString &message);

public slots:
    void showMainWindow();

private:
    bool loadConfiguration();
    bool initializeLogging();
    bool initializeDatabase();
    bool initializeCommunication();
    bool initializeModules();

    QString m_configPath;
    bool m_initialized;
    MainWindow *m_mainWindow;
};

#endif
