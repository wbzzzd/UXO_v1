// 应用生命周期管理实现
// 当前配置/日志/数据库/通信/模块初始化均为占位，直接返回成功，不连接真实设备或外部服务。

#include "App/Application.h"
#include "MainWindow/MainWindow.h"

using namespace App;

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_configPath("./config")
    , m_initialized(false)
    , m_mainWindow(nullptr)
{
    setApplicationName("UXOMissionControl");
    setApplicationVersion("1.0.0");
    setOrganizationName("UXO");
}

Application::~Application()
{
    delete m_mainWindow;
}

// 初始化流程：依次加载配置、日志、数据库、通信、模块，全部成功才返回 true
bool Application::initialize()
{
    if (m_initialized) {
        return true;
    }

    if (!loadConfiguration()) {
        emit errorOccurred("Failed to load configuration");
        return false;
    }

    if (!initializeLogging()) {
        emit errorOccurred("Failed to initialize logging");
        return false;
    }

    if (!initializeDatabase()) {
        emit errorOccurred("Failed to initialize database");
        return false;
    }

    if (!initializeCommunication()) {
        emit errorOccurred("Failed to initialize communication");
        return false;
    }

    if (!initializeModules()) {
        emit errorOccurred("Failed to initialize modules");
        return false;
    }

    m_initialized = true;
    emit initialized();
    return true;
}

// 创建并显示主窗口
void Application::run()
{
    if (m_mainWindow == nullptr) {
        m_mainWindow = new MainWindow();
    }

    m_mainWindow->show();
}

// 以下均为占位实现，直接返回成功，不连接真实配置/日志/数据库/通信/模块
bool Application::loadConfiguration()
{
    return true;
}

bool Application::initializeLogging()
{
    return true;
}

bool Application::initializeDatabase()
{
    return true;
}

bool Application::initializeCommunication()
{
    return true;
}

bool Application::initializeModules()
{
    return true;
}
