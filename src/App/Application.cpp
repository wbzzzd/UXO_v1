#include "App/Application.h"

using namespace App;

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_configPath("./config")
    , m_initialized(false)
{
    setApplicationName("UXOMissionControl");
    setApplicationVersion("1.0.0");
    setOrganizationName("UXO");
}

Application::~Application()
{
}

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

void Application::run()
{
}

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
