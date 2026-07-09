#include "MainWindow/LeftPanelWidget.h"
#include "Core/Simulation/DemoScenarioProvider.h"
#include "Common/GlobalStyle.h"

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
    // 数据由 MainWindow::loadMockData() 通过 setTargets/setMissions/setDevices 注入
}

LeftPanelWidget::~LeftPanelWidget()
{
}

void LeftPanelWidget::setupUi()
{
    setMinimumWidth(GlobalStyle::Sizes::LeftPanelWidth);
    setMaximumWidth(GlobalStyle::Sizes::LeftPanelWidth);
    setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::PanelBackground));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // 添加搜索和筛选工具栏
    QWidget *toolBar = new QWidget(this);
    toolBar->setStyleSheet(QString("background-color: %1; border-radius: 4px; margin-bottom: 8px;")
        .arg(GlobalStyle::Colors::ToolbarBackground));
    QHBoxLayout *toolBarLayout = new QHBoxLayout(toolBar);
    toolBarLayout->setContentsMargins(8, 6, 8, 6);
    toolBarLayout->setSpacing(8);

    // 搜索框
    m_searchEdit = new QLineEdit(toolBar);
    m_searchEdit->setPlaceholderText("搜索目标...");
    m_searchEdit->setStyleSheet(QString(
        "QLineEdit { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; padding: 4px 8px; }"
        "QLineEdit:focus { border: 1px solid %4; }"
        "QLineEdit::placeholder { color: %5; }")
        .arg(GlobalStyle::Colors::Background)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Colors::PrimaryGreen)
        .arg(GlobalStyle::Colors::TextDisabled));
    m_searchEdit->setFixedHeight(28);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &LeftPanelWidget::onSearchTextChanged);
    toolBarLayout->addWidget(m_searchEdit, 1);

    // 筛选按钮
    QPushButton *filterBtn = new QPushButton("筛选", toolBar);
    filterBtn->setFixedSize(50, 28);
    filterBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(GlobalStyle::Colors::ToolbarBackground)
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Colors::Border));
    connect(filterBtn, &QPushButton::clicked, this, &LeftPanelWidget::onFilterChanged);
    toolBarLayout->addWidget(filterBtn);

    // 刷新按钮
    QPushButton *refreshBtn = new QPushButton("刷新", toolBar);
    refreshBtn->setFixedSize(50, 28);
    refreshBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(GlobalStyle::Colors::PrimaryGreen)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::PrimaryGreenHover));
    connect(refreshBtn, &QPushButton::clicked, this, &LeftPanelWidget::onRefreshTargets);
    toolBarLayout->addWidget(refreshBtn);

    mainLayout->addWidget(toolBar);

    // 状态子标签（待处置/处置中/已完成）
    QWidget *statusTabs = new QWidget(this);
    statusTabs->setStyleSheet(QString("background-color: %1; border-radius: 4px; margin-bottom: 8px;")
        .arg(GlobalStyle::Colors::ToolbarBackground));
    QHBoxLayout *statusLayout = new QHBoxLayout(statusTabs);
    statusLayout->setContentsMargins(4, 4, 4, 4);
    statusLayout->setSpacing(0);

    QString statusTabStyle = QString(
        "QPushButton { background-color: transparent; color: %1; border: none; border-bottom: 2px solid transparent; padding: 8px 12px; font-size: 13px; }"
        "QPushButton:hover { background-color: %2; }"
        "QPushButton[selected=\"true\"] { color: %3; border-bottom: 2px solid %4; }")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::PrimaryGreen);

    QPushButton *tab1 = new QPushButton("待处置 (8)", statusTabs);
    tab1->setProperty("selected", true);
    tab1->setStyleSheet(statusTabStyle);
    statusLayout->addWidget(tab1);

    QPushButton *tab2 = new QPushButton("处置中 (5)", statusTabs);
    tab2->setStyleSheet(statusTabStyle);
    statusLayout->addWidget(tab2);

    QPushButton *tab3 = new QPushButton("已完成 (2)", statusTabs);
    tab3->setStyleSheet(statusTabStyle);
    statusLayout->addWidget(tab3);

    mainLayout->addWidget(statusTabs);

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

// 刷新：重新加载模拟场景数据
void LeftPanelWidget::onRefreshTargets()
{
    Core::Simulation::DemoScenario scenario = Core::Simulation::DemoScenarioProvider::create();
    m_targets = scenario.targets;
    m_missions = scenario.missions;
    m_devices = scenario.devices;
    populateTargetList();
    populateMissionList();
    populateDeviceList();
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
