#include "MainWindow/DeviceControlView.h"

DeviceControlView::DeviceControlView(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_dronePanel(nullptr)
    , m_robotPanel(nullptr)
{
    m_tabWidget = new QTabWidget(this);
    m_dronePanel = new DroneControlPanel(this);
    m_robotPanel = new RobotControlPanel(this);
    m_tabWidget->addTab(m_dronePanel, tr("无人机控制"));
    m_tabWidget->addTab(m_robotPanel, tr("机器人控制"));
}

DeviceControlView::~DeviceControlView()
{
}

DroneControlPanel::DroneControlPanel(QWidget *parent)
    : QWidget(parent)
{
}

RobotControlPanel::RobotControlPanel(QWidget *parent)
    : QWidget(parent)
{
}
