#ifndef MAINWINDOW_ALERTPANEL_H
#define MAINWINDOW_ALERTPANEL_H

#include <QWidget>
#include <QList>
#include <QString>
#include "Core/Data/Types.h"

class QLabel;
class QVBoxLayout;
class QTimer;

class AlertPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AlertPanel(QWidget *parent = nullptr);
    ~AlertPanel();

    void addAlert(const Core::AlarmInfo& alarm);
    void clearAlerts();

signals:
    void alertClicked(const QString& alertId);

private:
    void setupUi();
    void refreshList();

    QVBoxLayout *m_mainLayout;
    QLabel *m_titleLabel;
    QWidget *m_listContainer;
    QVBoxLayout *m_listLayout;
    QList<Core::AlarmInfo> m_alerts;
};

#endif
