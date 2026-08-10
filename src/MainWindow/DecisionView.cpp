// 决策页实现：MOS 起降带规划工作区（P0 Approved）。
// 三栏布局：左损毁目标列表 + 中心跑道俯视图与算法参数 + 右候选方案与当前模拟选择摘要。
// 仅按传入的 MosPlanningSnapshot 副本渲染，被动发出信号；不持有会话状态、不发起规划、不联网。
// 所有数据均为本地合成 fixture 语义，非真实跑道、真实弹坑或真实安全结论。

#include "MainWindow/DecisionView.h"
#include "MainWindow/MosGeneratorDialog.h"
#include "MainWindow/MosParamsPanel.h"
#include "MainWindow/MosRunwayWidget.h"
#include "Common/GlobalStyle.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

DecisionView::DecisionView(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    // GlobalStyle QSS 只覆盖 QMainWindow 背景，DecisionView 作为 QWidget 需显式设置暗色底
    setStyleSheet(QStringLiteral("background-color:%1;").arg(GlobalStyle::Colors::Background));
}

DecisionView::~DecisionView() = default;

void DecisionView::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // === MOS 工具栏 ===
    m_toolbar = new QWidget(this);
    m_toolbar->setFixedHeight(GlobalStyle::Sizes::DecisionToolbarBaseHeight);
    m_toolbar->setStyleSheet(QStringLiteral("background:%1;").arg(GlobalStyle::Colors::ToolbarBackground));
    auto *tbLayout = new QHBoxLayout(m_toolbar);
    tbLayout->setContentsMargins(8, 4, 8, 4);
    tbLayout->setSpacing(6);
    auto *badge = new QLabel(QStringLiteral("P0 · 模拟规划"), m_toolbar);
    badge->setStyleSheet(QStringLiteral("color:%1; border:1px solid %1; padding:1px 6px;")
                             .arg(GlobalStyle::Colors::ThreatMedium));
    tbLayout->addWidget(badge);
    m_tbGen = new QPushButton(QStringLiteral("◈ 生成损毁场景"), m_toolbar);
    m_tbGen->setObjectName(QStringLiteral("DEC-TB-GEN"));
    tbLayout->addWidget(m_tbGen);
    m_tbParams = new QPushButton(QStringLiteral("⚙ 参数设置"), m_toolbar);
    m_tbParams->setObjectName(QStringLiteral("DEC-TB-PARAMS"));
    tbLayout->addWidget(m_tbParams);
    tbLayout->addSpacing(12);
    m_tierButtonContainer = new QWidget(m_toolbar);
    auto *tierLayout = new QHBoxLayout(m_tierButtonContainer);
    tierLayout->setContentsMargins(0, 0, 0, 0);
    tierLayout->setSpacing(4);
    tbLayout->addWidget(m_tierButtonContainer);
    tbLayout->addStretch();
    root->addWidget(m_toolbar);

    // === 三栏 QSplitter：左目标列表 + 中心跑道+参数 + 右候选方案 ===
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setStyleSheet(QStringLiteral("background-color:%1;").arg(GlobalStyle::Colors::Background));

    // 左面板：损毁目标列表
    m_leftPanel = new QWidget(m_splitter);
    m_leftPanel->setFixedWidth(GlobalStyle::Sizes::DecisionLeftPanelBaseWidth);
    m_leftPanel->setStyleSheet(QStringLiteral("background-color:%1;").arg(GlobalStyle::Colors::PanelBackground));
    auto *leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    auto *leftTitle = new QLabel(QStringLiteral("[模拟] 损毁目标列表"), m_leftPanel);
    leftTitle->setStyleSheet(QStringLiteral("background:%1; color:%2; padding:6px;")
                                 .arg(GlobalStyle::Colors::PanelBackground, GlobalStyle::Colors::TextPrimary));
    leftLayout->addWidget(leftTitle);
    m_targetList = new QListWidget(m_leftPanel);
    m_targetList->setObjectName(QStringLiteral("DEC-LP-TARGET-LIST"));
    // 自定义卡片 widget 处理选中态视觉，QListWidget 自身选中背景设为透明
    m_targetList->setStyleSheet("QListWidget{background:#1E1E1E;border:none;}"
                                "QListWidget::item:selected{background:transparent;border:none;}");
    leftLayout->addWidget(m_targetList);
    m_splitter->addWidget(m_leftPanel);

    // 中心区：上方跑道俯视图 + 下方算法参数栏
    m_centerPanel = new QWidget(m_splitter);
    m_centerPanel->setStyleSheet(QStringLiteral("background-color:%1;").arg(GlobalStyle::Colors::Background));
    auto *centerLayout = new QVBoxLayout(m_centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);
    // 跑道标题行：构造时用占位文案，setSnapshot 用快照 params.L/W 覆盖真实尺寸
    m_rwTitle = new QLabel(QStringLiteral("跑道 [模拟]"), m_centerPanel);
    m_rwTitle->setStyleSheet(QStringLiteral("padding:4px 8px; color:%1;").arg(GlobalStyle::Colors::TextSecondary));
    centerLayout->addWidget(m_rwTitle);
    m_runway = new MosRunwayWidget(m_centerPanel);
    m_runway->setObjectName(QStringLiteral("DEC-CE-RUNWAY"));
    centerLayout->addWidget(m_runway, 1);
    m_hScrollBar = new QScrollBar(Qt::Horizontal, m_centerPanel);
    m_hScrollBar->setObjectName(QStringLiteral("DEC-CE-HSCROLL"));
    centerLayout->addWidget(m_hScrollBar);
    // 缩放控件行
    auto *zoomRow = new QHBoxLayout();
    zoomRow->setContentsMargins(8, 2, 8, 2);
    // 图例 + 比例尺（左侧），匹配 HTML .legend / .scale-bar
    auto addLegendItem = [&](const QString &color, const QString &text) {
        auto *dot = new QLabel(m_centerPanel);
        dot->setStyleSheet(QStringLiteral("background:%1;border-radius:4px;"
                                          "min-width:8px;max-width:8px;"
                                          "min-height:8px;max-height:8px;").arg(color));
        zoomRow->addWidget(dot);
        auto *lbl = new QLabel(text, m_centerPanel);
        lbl->setStyleSheet("color:#AAAAAA;font-size:11px;");
        zoomRow->addWidget(lbl);
    };
    addLegendItem(GlobalStyle::Colors::ThreatHigh, QStringLiteral("弹坑"));
    addLegendItem(QStringLiteral("#FFEB3B"), QStringLiteral("未爆弹"));
    addLegendItem(GlobalStyle::Colors::StatusOnline, QStringLiteral("已处理"));
    addLegendItem(GlobalStyle::Colors::ThreatMedium, QStringLiteral("候选档位"));
    auto *scaleBar = new QLabel(QStringLiteral("0 ──┤── 500m"), m_centerPanel);
    scaleBar->setStyleSheet("color:#888888;font-size:11px;"
                            "font-family:'Consolas','Courier New',monospace;");
    zoomRow->addWidget(scaleBar);
    zoomRow->addStretch();
    m_zoomOut = new QPushButton(QStringLiteral("-"), m_centerPanel);
    m_zoomOut->setObjectName(QStringLiteral("DEC-CE-ZOOM-OUT"));
    m_zoomOut->setStyleSheet(QStringLiteral("background-color:%1; color:%2; border:1px solid %3;"
                                             "border-radius:4px; padding:4px 10px; min-width:30px;")
                                 .arg(GlobalStyle::Colors::ToolbarBackground, GlobalStyle::Colors::TextPrimary, GlobalStyle::Colors::Border));
    zoomRow->addWidget(m_zoomOut);
    m_zoomLevel = new QLabel(QStringLiteral("1×"), m_centerPanel);
    m_zoomLevel->setObjectName(QStringLiteral("DEC-CE-ZOOM-LEVEL"));
    zoomRow->addWidget(m_zoomLevel);
    m_zoomIn = new QPushButton(QStringLiteral("+"), m_centerPanel);
    m_zoomIn->setObjectName(QStringLiteral("DEC-CE-ZOOM-IN"));
    m_zoomIn->setStyleSheet(QStringLiteral("background-color:%1; color:%2; border:1px solid %3;"
                                            "border-radius:4px; padding:4px 10px; min-width:30px;")
                                .arg(GlobalStyle::Colors::ToolbarBackground, GlobalStyle::Colors::TextPrimary, GlobalStyle::Colors::Border));
    zoomRow->addWidget(m_zoomIn);
    m_zoomReset = new QPushButton(QStringLiteral("复位"), m_centerPanel);
    m_zoomReset->setObjectName(QStringLiteral("DEC-CE-ZOOM-RESET"));
    m_zoomReset->setStyleSheet(QStringLiteral("background-color:%1; color:%2; border:1px solid %3;"
                                               "border-radius:4px; padding:4px 10px;")
                                   .arg(GlobalStyle::Colors::ToolbarBackground, GlobalStyle::Colors::TextPrimary, GlobalStyle::Colors::Border));
    zoomRow->addWidget(m_zoomReset);
    centerLayout->addLayout(zoomRow);
    // 算法参数栏
    m_paramsPanel = new MosParamsPanel(m_centerPanel);
    centerLayout->addWidget(m_paramsPanel);
    m_splitter->addWidget(m_centerPanel);

    // 右面板：候选方案 + 当前模拟选择摘要 + P1 扩展位
    m_rightPanel = new QWidget(m_splitter);
    m_rightPanel->setFixedWidth(GlobalStyle::Sizes::DecisionRightPanelBaseWidth);
    m_rightPanel->setStyleSheet(QStringLiteral("background-color:%1;").arg(GlobalStyle::Colors::PanelBackground));
    auto *rightLayout = new QVBoxLayout(m_rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    // 5.1 候选起降方案
    auto *plansTitle = new QLabel(QStringLiteral("[模拟] 候选起降方案"), m_rightPanel);
    plansTitle->setStyleSheet(QStringLiteral("background:%1; color:%2; padding:6px;")
                                  .arg(GlobalStyle::Colors::PanelBackground, GlobalStyle::Colors::TextPrimary));
    rightLayout->addWidget(plansTitle);
    m_plansContainer = new QWidget(m_rightPanel);
    m_plansContainer->setObjectName(QStringLiteral("DEC-RP-PLANS"));
    m_plansContainer->setStyleSheet(QStringLiteral("background-color:%1;").arg(GlobalStyle::Colors::PanelBackground));
    m_plansLayout = new QVBoxLayout(m_plansContainer);
    m_plansLayout->setContentsMargins(4, 4, 4, 4);
    m_plansLayout->setSpacing(4);
    rightLayout->addWidget(m_plansContainer);
    // 5.2 当前模拟选择摘要
    auto *detailTitle = new QLabel(QStringLiteral("当前模拟选择"), m_rightPanel);
    detailTitle->setStyleSheet(QStringLiteral("background:%1; color:%2; padding:6px;")
                                   .arg(GlobalStyle::Colors::PanelBackground, GlobalStyle::Colors::TextPrimary));
    rightLayout->addWidget(detailTitle);
    auto *detailBox = new QWidget(m_rightPanel);
    detailBox->setObjectName(QStringLiteral("DEC-RP-DETAIL"));
    detailBox->setStyleSheet(QStringLiteral("background-color:%1;").arg(GlobalStyle::Colors::PanelBackground));
    auto *detailLayout = new QVBoxLayout(detailBox);
    detailLayout->setContentsMargins(8, 4, 8, 4);
    m_detailTier = new QLabel(QStringLiteral("当前模拟选择：档位2·部分处理假设"), detailBox);
    detailLayout->addWidget(m_detailTier);
    m_detailSize = new QLabel(QStringLiteral("起降带：520×18m · 9360m²"), detailBox);
    detailLayout->addWidget(m_detailSize);
    m_detailArea = new QLabel(QStringLiteral("可用面积：9360 m²"), detailBox);
    detailLayout->addWidget(m_detailArea);
    m_detailHours = new QLabel(QStringLiteral("模拟处理耗时：12h"), detailBox);
    detailLayout->addWidget(m_detailHours);
    m_detailDamage = new QLabel(QStringLiteral("涉及损毁点：3"), detailBox);
    detailLayout->addWidget(m_detailDamage);
    m_detailArea2 = new QLabel(QStringLiteral("模拟 Y 区间：-"), detailBox);
    detailLayout->addWidget(m_detailArea2);
    auto *note = new QLabel(QStringLiteral("[模拟] 此处仅用于本地方案对比。选择不构成确认、下发、执行，也不建立真实安全结论。"), detailBox);
    note->setObjectName(QStringLiteral("DEC-RP-DETAIL-NOTE"));
    note->setWordWrap(true);
    // 移除硬编码 font-size:11px，改打 caption 角色跟随 1x/2x 动态缩放
    note->setProperty("mosFontRole", QLatin1String("caption"));
    note->setStyleSheet(QStringLiteral("color:%1;").arg(GlobalStyle::Colors::TextSecondary));
    detailLayout->addWidget(note);
    rightLayout->addWidget(detailBox);
    // 5.3 P1 扩展位（禁用占位）
    m_p1Slot = new QWidget(m_rightPanel);
    m_p1Slot->setObjectName(QStringLiteral("DEC-RP-P1-SLOT"));
    m_p1Slot->setStyleSheet(QStringLiteral("background-color:%1;").arg(GlobalStyle::Colors::PanelBackground));
    auto *p1Layout = new QVBoxLayout(m_p1Slot);
    p1Layout->setContentsMargins(8, 4, 8, 4);
    auto *p1Title = new QLabel(QStringLiteral("P1 · 扩展位（暂未实现）"), m_p1Slot);
    p1Title->setStyleSheet(QStringLiteral("color:%1; border:1px dashed %2; padding:4px;")
                                .arg(GlobalStyle::Colors::TextDisabled, GlobalStyle::Colors::Border));
    p1Layout->addWidget(p1Title);
    for (const auto &name : {QStringLiteral("修复优先级排序"), QStringLiteral("决策草案确认"), QStringLiteral("导出规划报告")}) {
        auto *item = new QLabel(QStringLiteral("P1 · 禁用 · %1").arg(name), m_p1Slot);
        item->setStyleSheet(QStringLiteral("background:%1; color:%2; border:1px dashed %3; padding:3px;")
                                .arg(GlobalStyle::Colors::ToolbarBackground, GlobalStyle::Colors::TextDisabled, GlobalStyle::Colors::Border));
        item->setEnabled(false);
        p1Layout->addWidget(item);
    }
    rightLayout->addWidget(m_p1Slot);
    rightLayout->addStretch();
    m_splitter->addWidget(m_rightPanel);
    m_splitter->setStretchFactor(1, 1);
    root->addWidget(m_splitter, 1);

    // === 状态栏 ===
    auto *sb = new QHBoxLayout();
    sb->setContentsMargins(8, 2, 8, 2);
    sb->setSpacing(12);
    auto *sbDeviceDot = new QLabel(this);
    sbDeviceDot->setStyleSheet(QStringLiteral("background:%1;border-radius:5px;"
                                              "min-width:10px;max-width:10px;"
                                              "min-height:10px;max-height:10px;")
                                   .arg(GlobalStyle::Colors::StatusOnline));
    sb->addWidget(sbDeviceDot);
    m_sbDevice = new QLabel(QStringLiteral("模拟设备状态: 2/2 在线"), this);
    m_sbDevice->setObjectName(QStringLiteral("DEC-SB-DEVICE"));
    m_sbDevice->setStyleSheet(QStringLiteral("color:%1;font-weight:bold;")
                                  .arg(GlobalStyle::Colors::StatusOnline));
    sb->addWidget(m_sbDevice);
    m_sbSim = new QLabel(QStringLiteral("模拟模式"), this);
    m_sbSim->setObjectName(QStringLiteral("DEC-SB-SIM"));
    m_sbSim->setStyleSheet(QStringLiteral("color:%1; border:1px solid %1; padding:0 4px;")
                               .arg(GlobalStyle::Colors::ThreatMedium));
    sb->addWidget(m_sbSim);
    m_sbAlarm = new QLabel(QStringLiteral("[本地模拟，不执行真实处置] 无告警"), this);
    m_sbAlarm->setObjectName(QStringLiteral("DEC-SB-ALARM"));
    m_sbAlarm->setStyleSheet("color:#4CAF50;font-weight:bold;");
    sb->addWidget(m_sbAlarm, 1);
    m_sbTarget = new QLabel(QStringLiteral("当前分析目标：未选择"), this);
    m_sbTarget->setObjectName(QStringLiteral("DEC-SB-TARGET"));
    sb->addWidget(m_sbTarget);
    root->addLayout(sb);

    // 生成器模态（懒构造避免无谓开销，但测试需要存在）
    m_generatorDialog = new MosGeneratorDialog(this);
    m_generatorDialog->hide();

    // === 信号接线（被动转发，不发起业务）===
    connect(m_tbGen, &QPushButton::clicked, this, [this](){
        emit generatorRequested();
        openGenerator();
    });
    connect(m_tbParams, &QPushButton::clicked, this, [this](){ m_paramsPanel->setFocus(); });
    connect(m_paramsPanel, &MosParamsPanel::replanRequested, this, [this](){ emit replanRequested(); });
    connect(m_targetList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
        if (item) {
            const QString id = item->data(Qt::UserRole).toString();
            selectTarget(id);
            emit targetSelected(id);
        }
    });
    connect(m_runway, &MosRunwayWidget::targetClicked, this, [this](const QString &id){
        selectTarget(id);
        emit targetSelected(id);
    });
    connect(m_runway, &MosRunwayWidget::tierClicked, this, [this](int idx){
        emit tierSelected(idx);
    });
    connect(m_generatorDialog, &MosGeneratorDialog::applied,
            this, [this](const Core::MOS::MosGeneratorParams &p, qint32 s){
        emit generatorApplied(p, s);
    });
    connect(m_generatorDialog, &MosGeneratorDialog::jsonExportRequested,
            this, [this](const QString &path){ emit exportRequested(path); });
    // +/- 按钮与滚轮共用 m_runway->zoomBy，缩放显示由 zoomChanged 信号回填
    connect(m_zoomIn, &QPushButton::clicked, this, [this](){ m_runway->zoomBy(0.25); });
    connect(m_zoomOut, &QPushButton::clicked, this, [this](){ m_runway->zoomBy(-0.25); });
    connect(m_zoomReset, &QPushButton::clicked, this, [this](){ m_runway->resetView(); });
    // 几何缩放变化时同步顶部缩放显示与滚动条范围
    connect(m_runway, &MosRunwayWidget::zoomChanged, this, [this](double z){
        m_zoomLevel->setText(QStringLiteral("%1×").arg(z, 0, 'f', 2));
        const int range = m_runway->panRangeX();
        m_hScrollBar->setRange(-range, range);
        m_hScrollBar->setPageStep(m_runway->width());
        QSignalBlocker blocker(m_hScrollBar);
        m_hScrollBar->setValue(m_runway->panX());
    });
    connect(m_hScrollBar, &QScrollBar::valueChanged, this, [this](int v){
        m_runway->setPanX(v);
    });
    connect(m_runway, &MosRunwayWidget::panXChanged, this, [this](int px){
        QSignalBlocker blocker(m_hScrollBar);
        m_hScrollBar->setValue(px);
    });
}

void DecisionView::openGenerator()
{
    m_generatorDialog->exec();
}
