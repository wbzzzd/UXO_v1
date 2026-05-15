#ifndef MAINWINDOW_DEVICESTATUSPANEL_H
#define MAINWINDOW_DEVICESTATUSPANEL_H

#include <QWidget>
#include <QList>
#include "Core/Data/Types.h"

class QLabel;
class QPushButton;
class QVBoxLayout;

class DeviceStatusPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceStatusPanel(QWidget *parent = nullptr);
    ~DeviceStatusPanel();

    void setDevices(const QVector<Core::DeviceInfo>& devices);
    void updateDeviceStatus(const QString& deviceId, Core::DeviceStatus status, double battery);

signals:
    void deviceClicked(const QString& deviceId);
    void openConsoleRequested(const QString& deviceId);

private:
    void setupUi();
    void refreshList();

    struct DeviceCard {
        QWidget *card;
        QLabel *nameLabel;
        QLabel *statusDot;
        QLabel *statusText;
        QLabel *batteryLabel;
        QPushButton *consoleBtn;
        Core::DeviceInfo info;
    };

    QVBoxLayout *m_mainLayout;
    QLabel *m_titleLabel;
    QWidget *m_listContainer;
    QVBoxLayout *m_listLayout;
    QList<DeviceCard> m_cards;
    QVector<Core::DeviceInfo> m_devices;
};

#endif
