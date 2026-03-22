#ifndef DEVICECONTROLVIEW_H
#define DEVICECONTROLVIEW_H

#include <QWidget>
#include <QTabWidget>

class DroneControlPanel;
class RobotControlPanel;

class DeviceControlView : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceControlView(QWidget *parent = nullptr);
    ~DeviceControlView();

private:
    QTabWidget *m_tabWidget;
    DroneControlPanel *m_dronePanel;
    RobotControlPanel *m_robotPanel;
};

class DroneControlPanel : public QWidget
{
    Q_OBJECT
public:
    explicit DroneControlPanel(QWidget *parent = nullptr);
};

class RobotControlPanel : public QWidget
{
    Q_OBJECT
public:
    explicit RobotControlPanel(QWidget *parent = nullptr);
};

#endif
