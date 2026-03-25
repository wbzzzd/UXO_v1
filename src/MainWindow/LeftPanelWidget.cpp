#include "MainWindow/LeftPanelWidget.h"
#include "Common/MockDataGenerator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidgetItem>
#include <QDebug>

LeftPanelWidget::LeftPanelWidget(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_targetTable(nullptr)
    , m_missionTable(nullptr)
    , m_deviceTable(nullptr)
    , m_searchEdit(nullptr)
    , m_typeFilterCombo(nullptr)
    , m_threatFilterCombo(nullptr)
{
    setupUi();

    m_targets = Common::MockDataGenerator::generateTargets(15);
    m_missions = Common::MockDataGenerator::generateMissions(m_targets, 8);
    m_devices = Common::MockDataGenerator::generateDevices(5);

    populateTargetList();
    populateMissionList();
    populateDeviceList();
}

LeftPanelWidget::~LeftPanelWidget()
{
}

void LeftPanelWidget::setupUi()
{
    setMinimumWidth(320);
    setMaximumWidth(320);
    setStyleSheet("background-color: #252526;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            border: none;
            background-color: #252526;
        }
        QTabBar::tab {
            background-color: #2D2D2D;
            color: #AAAAAA;
            padding: 8px 16px;
            border: none;
        }
        QTabBar::tab:selected {
            background-color: #252526;
            color: #FFFFFF;
            border-bottom: 2px solid #4A7A4C;
        }
        QTabBar::tab:hover {
            background-color: #3C3C3C;
        }
    )");

    m_targetTable = new QTableWidget(this);
    m_missionTable = new QTableWidget(this);
    m_deviceTable = new QTableWidget(this);

    setupTargetList();
    setupMissionList();
    setupDeviceList();

    m_tabWidget->addTab(m_targetTable, "目标");
    m_tabWidget->addTab(m_missionTable, "任务");
    m_tabWidget->addTab(m_deviceTable, "设备");

    mainLayout->addWidget(m_tabWidget);
}

void LeftPanelWidget::setupTargetList()
{
    m_targetTable->setColumnCount(4);
    m_targetTable->setHorizontalHeaderLabels({"", "类型", "置信度", "位置"});
    m_targetTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #2D2D2D; color: #FFFFFF; padding: 4px; }");
    m_targetTable->setColumnWidth(0, 30);
    m_targetTable->setColumnWidth(1, 80);
    m_targetTable->setColumnWidth(2, 60);
    m_targetTable->setColumnWidth(3, 100);
    m_targetTable->verticalHeader()->setVisible(false);
    m_targetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_targetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_targetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_targetTable->setAlternatingRowColors(true);
    m_targetTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #252526;
            color: #FFFFFF;
            gridline-color: #3C3C3C;
            border: none;
        }
        QTableWidget::item {
            padding: 4px;
        }
        QTableWidget::item:selected {
            background-color: #2A3F54;
        }
        QTableWidget::item:hover {
            background-color: #2A2A2A;
        }
        QScrollBar:vertical {
            background: #2D2D2D;
            width: 8px;
        }
        QScrollBar::handle:vertical {
            background: #555555;
            border-radius: 4px;
        }
    )");

    connect(m_targetTable, &QTableWidget::itemClicked, this, [this](QTableWidgetItem *item) {
        if (item && item->row() >= 0 && item->row() < m_targets.size()) {
            emit targetSelected(m_targets[item->row()]);
        }
    });

    connect(m_targetTable, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
        if (item && item->row() >= 0 && item->row() < m_targets.size()) {
            emit targetDoubleClicked(m_targets[item->row()]);
        }
    });
}

void LeftPanelWidget::setupMissionList()
{
    m_missionTable->setColumnCount(4);
    m_missionTable->setHorizontalHeaderLabels({"优先级", "任务编号", "执行设备", "状态"});
    m_missionTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #2D2D2D; color: #FFFFFF; padding: 4px; }");
    m_missionTable->verticalHeader()->setVisible(false);
    m_missionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_missionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_missionTable->setAlternatingRowColors(true);
    m_missionTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #252526;
            color: #FFFFFF;
            gridline-color: #3C3C3C;
            border: none;
        }
        QTableWidget::item:selected {
            background-color: #2A3F54;
        }
    )");

    connect(m_missionTable, &QTableWidget::itemClicked, this, [this](QTableWidgetItem *item) {
        if (item && item->row() >= 0 && item->row() < m_missions.size()) {
            emit missionSelected(m_missions[item->row()]);
        }
    });
}

void LeftPanelWidget::setupDeviceList()
{
    m_deviceTable->setColumnCount(3);
    m_deviceTable->setHorizontalHeaderLabels({"设备名称", "状态", "电量"});
    m_deviceTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #2D2D2D; color: #FFFFFF; padding: 4px; }");
    m_deviceTable->verticalHeader()->setVisible(false);
    m_deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceTable->setAlternatingRowColors(true);
    m_deviceTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #252526;
            color: #FFFFFF;
            gridline-color: #3C3C3C;
            border: none;
        }
        QTableWidget::item:selected {
            background-color: #2A3F54;
        }
    )");

    connect(m_deviceTable, &QTableWidget::itemClicked, this, [this](QTableWidgetItem *item) {
        if (item && item->row() >= 0 && item->row() < m_devices.size()) {
            emit deviceSelected(m_devices[item->row()]);
        }
    });
}

void LeftPanelWidget::populateTargetList()
{
    m_targetTable->setRowCount(m_targets.size());

    for (int i = 0; i < m_targets.size(); ++i) {
        const Core::TargetInfo &target = m_targets[i];

        QColor threatColor;
        switch (target.threatLevel) {
            case Core::ThreatLevel::High: threatColor = QColor("#FF5252"); break;
            case Core::ThreatLevel::Medium: threatColor = QColor("#FFB74D"); break;
            case Core::ThreatLevel::Low: threatColor = QColor("#FFF176"); break;
            default: threatColor = QColor("#888888");
        }

        QTableWidgetItem *checkItem = new QTableWidgetItem();
        checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        checkItem->setCheckState(Qt::Unchecked);
        m_targetTable->setItem(i, 0, checkItem);

        QTableWidgetItem *typeItem = new QTableWidgetItem(target.typeName);
        typeItem->setForeground(threatColor);
        m_targetTable->setItem(i, 1, typeItem);

        QTableWidgetItem *confItem = new QTableWidgetItem(QString::number(target.confidence * 100, 'f', 0) + "%");
        m_targetTable->setItem(i, 2, confItem);

        QString posStr = QString("X:%1 Y:%2").arg(int(target.position.x())).arg(int(target.position.z()));
        m_targetTable->setItem(i, 3, new QTableWidgetItem(posStr));

        m_targetTable->setRowHeight(i, 40);
    }
}

void LeftPanelWidget::populateMissionList()
{
    m_missionTable->setRowCount(m_missions.size());

    for (int i = 0; i < m_missions.size(); ++i) {
        const Core::MissionInfo &mission = m_missions[i];

        QString priorityStr;
        QColor priorityColor;
        switch (mission.priority) {
            case 0: priorityStr = "P0"; priorityColor = QColor("#FF5252"); break;
            case 1: priorityStr = "P1"; priorityColor = QColor("#FFB74D"); break;
            default: priorityStr = "P2"; priorityColor = QColor("#FFF176");
        }

        QTableWidgetItem *priItem = new QTableWidgetItem(priorityStr);
        priItem->setForeground(priorityColor);
        m_missionTable->setItem(i, 0, priItem);

        m_missionTable->setItem(i, 1, new QTableWidgetItem(mission.id));
        m_missionTable->setItem(i, 2, new QTableWidgetItem(mission.deviceId));

        QString statusStr;
        switch (mission.status) {
            case Core::MissionStatus::Planned: statusStr = "规划中"; break;
            case Core::MissionStatus::PendingApproval: statusStr = "待审批"; break;
            case Core::MissionStatus::Approved: statusStr = "已批准"; break;
            case Core::MissionStatus::Executing: statusStr = "执行中"; break;
            case Core::MissionStatus::Completed: statusStr = "已完成"; break;
            case Core::MissionStatus::Failed: statusStr = "失败"; break;
            default: statusStr = "未知";
        }
        m_missionTable->setItem(i, 3, new QTableWidgetItem(statusStr));

        m_missionTable->setRowHeight(i, 40);
    }
}

void LeftPanelWidget::populateDeviceList()
{
    m_deviceTable->setRowCount(m_devices.size());

    for (int i = 0; i < m_devices.size(); ++i) {
        const Core::DeviceInfo &device = m_devices[i];

        QTableWidgetItem *nameItem = new QTableWidgetItem(device.name);
        m_deviceTable->setItem(i, 0, nameItem);

        QString statusStr;
        QColor statusColor;
        switch (device.status) {
            case Core::DeviceStatus::Online:
            case Core::DeviceStatus::Idle: statusStr = "在线"; statusColor = QColor("#4CAF50"); break;
            case Core::DeviceStatus::Busy: statusStr = "任务中"; statusColor = QColor("#FFB74D"); break;
            case Core::DeviceStatus::Offline: statusStr = "离线"; statusColor = QColor("#888888"); break;
            case Core::DeviceStatus::Error: statusStr = "故障"; statusColor = QColor("#FF5252"); break;
            default: statusStr = "未知"; statusColor = QColor("#888888");
        }
        QTableWidgetItem *statusItem = new QTableWidgetItem(statusStr);
        statusItem->setForeground(statusColor);
        m_deviceTable->setItem(i, 1, statusItem);

        QString batteryStr = QString::number(device.batteryLevel, 'f', 0) + "%";
        QColor batteryColor;
        if (device.batteryLevel > 60) batteryColor = QColor("#4CAF50");
        else if (device.batteryLevel > 20) batteryColor = QColor("#FFB74D");
        else batteryColor = QColor("#FF5252");
        QTableWidgetItem *batteryItem = new QTableWidgetItem(batteryStr);
        batteryItem->setForeground(batteryColor);
        m_deviceTable->setItem(i, 2, batteryItem);

        m_deviceTable->setRowHeight(i, 40);
    }
}

void LeftPanelWidget::setTargets(const QVector<Core::TargetInfo> &targets)
{
    m_targets = targets;
    populateTargetList();
}

void LeftPanelWidget::setMissions(const QVector<Core::MissionInfo> &missions)
{
    m_missions = missions;
    populateMissionList();
}

void LeftPanelWidget::setDevices(const QVector<Core::DeviceInfo> &devices)
{
    m_devices = devices;
    populateDeviceList();
}

void LeftPanelWidget::onRefreshTargets()
{
    m_targets = Common::MockDataGenerator::generateTargets(15);
    populateTargetList();
}

void LeftPanelWidget::onFilterChanged()
{
}

void LeftPanelWidget::onSearchTextChanged(const QString &text)
{
    for (int i = 0; i < m_targetTable->rowCount(); ++i) {
        bool match = false;
        for (int j = 0; j < m_targetTable->columnCount(); ++j) {
            QTableWidgetItem *item = m_targetTable->item(i, j);
            if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        m_targetTable->setRowHidden(i, !match && !text.isEmpty());
    }
}
