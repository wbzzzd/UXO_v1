#ifndef MAINWINDOW_LEFTPANELWIDGET_H
#define MAINWINDOW_LEFTPANELWIDGET_H

#include <QWidget>
#include <QVector>
#include <QString>
#include "Core/Data/Types.h"

class QTabWidget;
class QTableWidget;
class QTableWidgetItem;
class QLineEdit;
class QComboBox;
class QPushButton;

class LeftPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LeftPanelWidget(QWidget *parent = nullptr);
    ~LeftPanelWidget();

    void setTargets(const QVector<Core::TargetInfo>& targets);
    void setMissions(const QVector<Core::MissionInfo>& missions);
    void setDevices(const QVector<Core::DeviceInfo>& devices);

signals:
    void targetSelected(const Core::TargetInfo& target);
    void targetDoubleClicked(const Core::TargetInfo& target);
    void missionSelected(const Core::MissionInfo& mission);
    void deviceSelected(const Core::DeviceInfo& device);

public slots:
    void onRefreshTargets();
    void onFilterChanged();
    void onSearchTextChanged(const QString& text);

private:
    void setupUi();
    void setupTargetList();
    void setupMissionList();
    void setupDeviceList();
    void populateTargetList();
    void populateMissionList();
    void populateDeviceList();
    void updateStatusTabs();  // 根据实际数据更新状态标签计数

    QTabWidget *m_tabWidget;
    QTableWidget *m_targetTable;
    QTableWidget *m_missionTable;
    QTableWidget *m_deviceTable;

    QLineEdit *m_searchEdit;
    QComboBox *m_typeFilterCombo;
    QComboBox *m_threatFilterCombo;

    QPushButton *m_statusTabPending;    // 待处置
    QPushButton *m_statusTabExecuting; // 处置中
    QPushButton *m_statusTabCompleted;  // 已完成

    QVector<Core::TargetInfo> m_targets;
    QVector<Core::MissionInfo> m_missions;
    QVector<Core::DeviceInfo> m_devices;
};

#endif
