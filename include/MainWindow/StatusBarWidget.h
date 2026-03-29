#ifndef MAINWINDOW_STATUSBARWIDGET_H
#define MAINWINDOW_STATUSBARWIDGET_H

#include <QWidget>
#include <QString>

class QLabel;
class QPushButton;
class QScrollArea;
class QHBoxLayout;

class StatusBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBarWidget(QWidget *parent = nullptr);
    ~StatusBarWidget();

    void updateDeviceStatus(int onlineCount, int totalCount);
    void addAlarm(const QString& message);
    void setMinBatteryLevel(int level);

signals:
    void emergencyStopClicked();

public slots:
    void onEmergencyStop();

private:
    void setupUi();
    QWidget* createSeparator();

    QLabel *m_deviceStatusLabel;
    QLabel *m_batteryLabel;
    QScrollArea *m_alarmScrollArea;
    QWidget *m_alarmContainer;
    QHBoxLayout *m_alarmLayout;
    QPushButton *m_emergencyStopBtn;
};

#endif