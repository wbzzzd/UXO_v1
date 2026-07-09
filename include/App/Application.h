// 应用生命周期管理
// 负责 QApplication 初始化、配置加载、日志/数据库/通信/模块初始化（当前为占位）和主窗口创建。

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QString>
#include <QTimer>

class MainWindow;

// 应用主类，管理初始化流程和主窗口生命周期
class Application : public QApplication
{
    Q_OBJECT

public:
    explicit Application(int &argc, char **argv);
    ~Application();

    bool initialize();  // 执行初始化流程，成功返回 true
    void run();         // 创建并显示主窗口

    MainWindow* mainWindow() const { return m_mainWindow; }

signals:
    void initialized();
    void errorOccurred(const QString &message);

public slots:
    void showMainWindow();

private:
    bool loadConfiguration();      // 加载配置，当前为占位
    bool initializeLogging();       // 初始化日志，当前为占位
    bool initializeDatabase();      // 初始化数据库，当前为占位
    bool initializeCommunication(); // 初始化通信，当前为占位
    bool initializeModules();       // 初始化业务模块，当前为占位

    QString m_configPath;
    bool m_initialized;
    MainWindow *m_mainWindow;
};

#endif
