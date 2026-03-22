#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

class StatusBarWidget : public QStatusBar
{
    Q_OBJECT

public:
    explicit StatusBarWidget(QWidget *parent = nullptr);
    ~StatusBarWidget();

    void updateDeviceStatus(int online, int total);
    void addAlarm(const QString &message);
    void clearAlarms();

signals:
    void emergencyStopClicked();

private slots:
    void onEmergencyStop();

private:
    QLabel *m_deviceStatusLabel;
    QLabel *m_batteryLabel;
    QScrollArea *m_alarmScrollArea;
    QLabel *m_alarmLabel;
    QPushButton *m_emergencyStopBtn;
};

#endif
