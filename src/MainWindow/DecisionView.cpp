// 决策页实现：MOS 起降带规划工作区（P0 Approved）。
// 三栏布局：左损毁目标列表 + 中心跑道俯视图与算法参数 + 右候选方案与当前模拟选择摘要。
// 仅按传入的 MosPlanningSnapshot 副本渲染，被动发出信号；不持有会话状态、不发起规划、不联网。
// 所有数据均为本地合成 fixture 语义，非真实跑道、真实弹坑或真实安全结论。

#include "MainWindow/DecisionView.h"
#include "MainWindow/MosGeneratorDialog.h"
#include "MainWindow/MosParamsPanel.h"
#include "MainWindow/MosPlanningController.h"
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
    // 批次6：根底色收敛为 containerBg="main" 属性词汇；WA_StyledBackground 让 QWidget 绘制样式背景
    setProperty("containerBg", "main");
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
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
    // 批次6：工具栏底色收敛为 containerBg="toolbar" 属性词汇
    m_toolbar->setProperty("containerBg", "toolbar");
    m_toolbar->setAttribute(Qt::WA_StyledBackground, true);
    auto *tbLayout = new QHBoxLayout(m_toolbar);
    tbLayout->setContentsMargins(8, 4, 8, 4);
    tbLayout->setSpacing(6);
    auto *badge = new QLabel(QStringLiteral("P0 · 模拟规划"), m_toolbar);
    // 批次6：威胁徽标收敛为 chipStyle 属性词汇
    badge->setProperty("chipStyle", "warnBadge");
    tbLayout->addWidget(badge);
    m_tbGen = new QPushButton(QStringLiteral("◈ 生成损毁场景"), m_toolbar);
    m_tbGen->setObjectName(QStringLiteral("DEC-TB-GEN"));
    // 批次6：工具栏容器化后裸级联底色消失，toolBg 保底工具栏底色（基础按钮为绿色主按钮）
    m_tbGen->setProperty("btnVariant", "toolBg");
    tbLayout->addWidget(m_tbGen);
    m_tbParams = new QPushButton(QStringLiteral("⚙ 参数设置"), m_toolbar);
    m_tbParams->setObjectName(QStringLiteral("DEC-TB-PARAMS"));
    // 批次6：同 toolBg 保底
    m_tbParams->setProperty("btnVariant", "toolBg");
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
    // 批次6：splitter 底色收敛为 containerBg="main"；把手底色由 GlobalStyle 的
    // QSplitter[containerBg="main"]::handle 规则恢复（原裸级联效果）
    m_splitter->setProperty("containerBg", "main");
    m_splitter->setAttribute(Qt::WA_StyledBackground, true);

    // 左面板：损毁目标列表
    m_leftPanel = new QWidget(m_splitter);
    m_leftPanel->setFixedWidth(GlobalStyle::Sizes::DecisionLeftPanelBaseWidth);
    // 批次6：面板底色收敛为 containerBg="panel" 属性词汇
    m_leftPanel->setProperty("containerBg", "panel");
    m_leftPanel->setAttribute(Qt::WA_StyledBackground, true);
    auto *leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    auto *leftTitle = new QLabel(QStringLiteral("[模拟] 损毁目标列表"), m_leftPanel);
    // 批次6：面板通栏标题收敛为 sectionTitle 属性词汇
    leftTitle->setProperty("sectionTitle", "panel");
    leftLayout->addWidget(leftTitle);
    m_targetList = new QListWidget(m_leftPanel);
    m_targetList->setObjectName(QStringLiteral("DEC-LP-TARGET-LIST"));
    // 自定义卡片 widget 处理选中态视觉，QListWidget 自身选中背景设为透明；
    // 背景/边框显式声明 Background 令牌：父级 m_leftPanel 裸样式表的 Panel 级联
    // 在 Qt 合并顺序中晚于应用级全局 QListWidget 规则，省略本规则会使列表底色
    // 由 #1E1E1E 变为 #252526（批次3 曾因此回归，像素门禁 A/B 已证实）。
    // 批次6：m_leftPanel 转 containerBg 后裸级联消失，补 QScrollBar 规则维持
    // 列表滚动条槽底 PanelBackground（原由裸级联提供，防止回落全局 ToolbarBackground）
    m_targetList->setStyleSheet(QStringLiteral(
        "QListWidget{background-color:%1;border:none;}"
        "QListWidget::item:selected{background:transparent;border:none;}"
        "QScrollBar{background-color:%2;}")
                                    .arg(GlobalStyle::Colors::Background,
                                         GlobalStyle::Colors::PanelBackground));
    leftLayout->addWidget(m_targetList);
    m_splitter->addWidget(m_leftPanel);

    // 中心区：上方跑道俯视图 + 下方算法参数栏
    m_centerPanel = new QWidget(m_splitter);
    // 批次6：中心区底色收敛为 containerBg="main" 属性词汇
    m_centerPanel->setProperty("containerBg", "main");
    m_centerPanel->setAttribute(Qt::WA_StyledBackground, true);
    auto *centerLayout = new QVBoxLayout(m_centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);
    // 跑道标题行：构造时用占位文案，setSnapshot 用快照 params.L/W 覆盖真实尺寸
    m_rwTitle = new QLabel(QStringLiteral("跑道 [模拟]"), m_centerPanel);
    // 批次6：内嵌小标题收敛为 sectionTitle="inset" 属性词汇
    m_rwTitle->setProperty("sectionTitle", "inset");
    centerLayout->addWidget(m_rwTitle);
    m_runway = new MosRunwayWidget(m_centerPanel);
    m_runway->setObjectName(QStringLiteral("DEC-CE-RUNWAY"));
    centerLayout->addWidget(m_runway, 1);
    m_hScrollBar = new QScrollBar(Qt::Horizontal, m_centerPanel);
    m_hScrollBar->setObjectName(QStringLiteral("DEC-CE-HSCROLL"));
    // 批次6：中心区容器化后裸级联消失，槽底用 containerBg="main" 维持 Background
    // （全局 QScrollBar 规则的槽底是 ToolbarBackground，省略会使槽底变色）
    m_hScrollBar->setProperty("containerBg", "main");
    centerLayout->addWidget(m_hScrollBar);
    // 缩放控件行
    auto *zoomRow = new QHBoxLayout();
    zoomRow->setContentsMargins(8, 2, 8, 2);
    // 图例 + 比例尺（左侧），匹配 HTML .legend / .scale-bar
    // 批次6：圆点颜色与文字样式收敛为 statusDot/textColor/fontSize 属性词汇
    auto addLegendItem = [&](const QString &variant, const QString &text) {
        auto *dot = new QLabel(m_centerPanel);
        dot->setProperty("statusDot", variant);
        zoomRow->addWidget(dot);
        auto *lbl = new QLabel(text, m_centerPanel);
        // 图例文字：辅助文本色 + 11px 小字档
        lbl->setProperty("textColor", "secondary");
        lbl->setProperty("fontSize", "11");
        zoomRow->addWidget(lbl);
    };
    addLegendItem(QStringLiteral("high"), QStringLiteral("弹坑"));
    // uxo 黄（未爆弹图例）无等值 token，收敛登记在 GlobalStyle statusDot="uxo" 规则
    addLegendItem(QStringLiteral("uxo"), QStringLiteral("未爆弹"));
    addLegendItem(QStringLiteral("online"), QStringLiteral("已处理"));
    addLegendItem(QStringLiteral("medium"), QStringLiteral("候选档位"));
    auto *scaleBar = new QLabel(QStringLiteral("0 ──┤── 500m"), m_centerPanel);
    // 批次6：比例尺文字收敛为 textColor/fontSize/fontFamily 属性词汇（禁用色+11px+等宽）
    scaleBar->setProperty("textColor", "disabled");
    scaleBar->setProperty("fontSize", "11");
    scaleBar->setProperty("fontFamily", "mono");
    zoomRow->addWidget(scaleBar);
    zoomRow->addStretch();
    m_zoomOut = new QPushButton(QStringLiteral("-"), m_centerPanel);
    m_zoomOut->setObjectName(QStringLiteral("DEC-CE-ZOOM-OUT"));
    // 批次6：缩放按钮内联样式收敛为 btnVariant="zoom"（含 30px 最小宽）
    m_zoomOut->setProperty("btnVariant", "zoom");
    zoomRow->addWidget(m_zoomOut);
    m_zoomLevel = new QLabel(QStringLiteral("1×"), m_centerPanel);
    m_zoomLevel->setObjectName(QStringLiteral("DEC-CE-ZOOM-LEVEL"));
    zoomRow->addWidget(m_zoomLevel);
    m_zoomIn = new QPushButton(QStringLiteral("+"), m_centerPanel);
    m_zoomIn->setObjectName(QStringLiteral("DEC-CE-ZOOM-IN"));
    // 批次6：同 btnVariant="zoom"
    m_zoomIn->setProperty("btnVariant", "zoom");
    zoomRow->addWidget(m_zoomIn);
    m_zoomReset = new QPushButton(QStringLiteral("复位"), m_centerPanel);
    m_zoomReset->setObjectName(QStringLiteral("DEC-CE-ZOOM-RESET"));
    // 批次6：复位按钮用 zoomReset 变体（外观同 zoom、不声明 min-width）：基线复位钮回落
    // 基础规则 min-width=80px 实宽 102px，误用 zoom 会因 30px 最小宽缩至 52px 并使整行右移
    m_zoomReset->setProperty("btnVariant", "zoomReset");
    zoomRow->addWidget(m_zoomReset);
    centerLayout->addLayout(zoomRow);
    // 算法参数栏
    m_paramsPanel = new MosParamsPanel(m_centerPanel);
    centerLayout->addWidget(m_paramsPanel);
    m_splitter->addWidget(m_centerPanel);

    // 右面板：候选方案 + 当前模拟选择摘要 + P1 扩展位
    m_rightPanel = new QWidget(m_splitter);
    m_rightPanel->setFixedWidth(GlobalStyle::Sizes::DecisionRightPanelBaseWidth);
    // 批次6：右面板底色收敛为 containerBg="panel" 属性词汇
    m_rightPanel->setProperty("containerBg", "panel");
    m_rightPanel->setAttribute(Qt::WA_StyledBackground, true);
    auto *rightLayout = new QVBoxLayout(m_rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    // 5.1 候选起降方案
    auto *plansTitle = new QLabel(QStringLiteral("[模拟] 候选起降方案"), m_rightPanel);
    // 批次6：面板通栏标题收敛为 sectionTitle="panel"
    plansTitle->setProperty("sectionTitle", "panel");
    rightLayout->addWidget(plansTitle);
    m_plansContainer = new QWidget(m_rightPanel);
    m_plansContainer->setObjectName(QStringLiteral("DEC-RP-PLANS"));
    // 批次6：候选方案容器底色收敛为 containerBg="panel"
    m_plansContainer->setProperty("containerBg", "panel");
    m_plansContainer->setAttribute(Qt::WA_StyledBackground, true);
    m_plansLayout = new QVBoxLayout(m_plansContainer);
    m_plansLayout->setContentsMargins(4, 4, 4, 4);
    m_plansLayout->setSpacing(4);
    rightLayout->addWidget(m_plansContainer);
    // 5.2 当前模拟选择摘要
    auto *detailTitle = new QLabel(QStringLiteral("当前模拟选择"), m_rightPanel);
    // 批次6：面板通栏标题收敛为 sectionTitle="panel"
    detailTitle->setProperty("sectionTitle", "panel");
    rightLayout->addWidget(detailTitle);
    auto *detailBox = new QWidget(m_rightPanel);
    detailBox->setObjectName(QStringLiteral("DEC-RP-DETAIL"));
    // 批次6：摘要容器底色收敛为 containerBg="panel"
    detailBox->setProperty("containerBg", "panel");
    detailBox->setAttribute(Qt::WA_StyledBackground, true);
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
    note->setProperty("mosFontRole", "caption");
    // 批次6：说明文字色收敛为 textColor="secondary" 属性词汇
    note->setProperty("textColor", "secondary");
    detailLayout->addWidget(note);
    rightLayout->addWidget(detailBox);
    // 5.3 P1 扩展位（禁用占位）
    m_p1Slot = new QWidget(m_rightPanel);
    m_p1Slot->setObjectName(QStringLiteral("DEC-RP-P1-SLOT"));
    // 批次6：P1 扩展位底色收敛为 containerBg="panel"
    m_p1Slot->setProperty("containerBg", "panel");
    m_p1Slot->setAttribute(Qt::WA_StyledBackground, true);
    auto *p1Layout = new QVBoxLayout(m_p1Slot);
    p1Layout->setContentsMargins(8, 4, 8, 4);
    auto *p1Title = new QLabel(QStringLiteral("P1 · 扩展位（暂未实现）"), m_p1Slot);
    // 批次6：插槽标题收敛为 slotStyle="title" 属性词汇
    p1Title->setProperty("slotStyle", "title");
    p1Layout->addWidget(p1Title);
    for (const auto &name : {QStringLiteral("修复优先级排序"), QStringLiteral("决策草案确认"), QStringLiteral("导出规划报告")}) {
        auto *item = new QLabel(QStringLiteral("P1 · 禁用 · %1").arg(name), m_p1Slot);
        // 批次6：占位条目收敛为 slotStyle="item"
        item->setProperty("slotStyle", "item");
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
    // 批次6：设备在线圆点收敛为 statusDot="device"（10px 圆点 5px 圆角）
    sbDeviceDot->setProperty("statusDot", "device");
    sb->addWidget(sbDeviceDot);
    m_sbDevice = new QLabel(QStringLiteral("模拟设备状态: 2/2 在线"), this);
    m_sbDevice->setObjectName(QStringLiteral("DEC-SB-DEVICE"));
    // 批次6：状态文字收敛为 textColor="online" + fontWeight="bold" 属性词汇
    m_sbDevice->setProperty("textColor", "online");
    m_sbDevice->setProperty("fontWeight", "bold");
    sb->addWidget(m_sbDevice);
    m_sbSim = new QLabel(QStringLiteral("模拟模式"), this);
    m_sbSim->setObjectName(QStringLiteral("DEC-SB-SIM"));
    // 批次6：模拟标签收敛为 chipStyle="simTag"
    m_sbSim->setProperty("chipStyle", "simTag");
    sb->addWidget(m_sbSim);
    m_sbAlarm = new QLabel(QStringLiteral("[本地模拟，不执行真实处置] 无告警"), this);
    m_sbAlarm->setObjectName(QStringLiteral("DEC-SB-ALARM"));
    // 批次6：告警文字收敛为 textColor="online" + fontWeight="bold"
    m_sbAlarm->setProperty("textColor", "online");
    m_sbAlarm->setProperty("fontWeight", "bold");
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
    m_generatorDialog->setRunwayParams(currentParams());
    m_generatorDialog->exec();
}

void DecisionView::setMosController(Core::MOS::MosPlanningController *controller)
{
    m_generatorDialog->setController(controller);
    m_generatorDialog->setRunwayParams(currentParams());
}
