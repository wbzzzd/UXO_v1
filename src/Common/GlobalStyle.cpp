/**
 * @file Common/GlobalStyle.cpp
 * @brief 全局样式实现
 * @details 实现UI设计文档中定义的所有视觉规范
 * @author 开发团队
 * @date 2024-01-01
 * @version 1.0.0
 */

#include "Common/GlobalStyle.h"
#include <QString>

namespace GlobalStyle {

QString getMainWindowStyle()
{
    return QString(R"(
        /* 主窗口样式 */
        QMainWindow {
            background-color: %1;
        }

        /* 菜单栏 */
        QMenuBar {
            background-color: %2;
            color: %3;
            border-bottom: 1px solid %4;
            font-size: %5px;
        }
        QMenuBar::item {
            padding: 6px 12px;
            background: transparent;
        }
        QMenuBar::item:selected {
            background-color: %4;
        }
        QMenuBar::item:pressed {
            background-color: %6;
        }

        /* 菜单 */
        QMenu {
            background-color: %2;
            color: %3;
            border: 1px solid %4;
            font-size: %5px;
        }
        QMenu::item {
            padding: 8px 30px;
        }
        QMenu::item:selected {
            background-color: %6;
        }
        QMenu::separator {
            height: 1px;
            background-color: %4;
            margin: 4px 0px;
        }

        /* 工具栏 */
        QToolBar {
            background-color: %2;
            border: none;
            spacing: 8px;
            padding: 8px;
        }
        QToolBar::separator {
            background-color: %4;
            width: 1px;
            margin: 4px 8px;
        }

        /* 按钮 */
        QPushButton {
            background-color: %6;
            color: %3;
            border: none;
            border-radius: 4px;
            padding: 6px 16px;
            font-size: %5px;
            min-width: %7px;
        }
        QPushButton:hover {
            background-color: %8;
        }
        QPushButton:pressed {
            background-color: %9;
        }
        QPushButton:disabled {
            background-color: %4;
            color: %10;
        }
        /* 选中态按钮：复用列表/卡片选中令牌，保证工具栏档位按钮 checked 与卡片选中视觉一致 */
        QPushButton:checked {
            background-color: %16;
            color: %3;
            border: 2px solid %17;
        }

        /* 主要按钮（军绿色） */
        QPushButton[primary="true"] {
            background-color: %6;
        }
        QPushButton[primary="true"]:hover {
            background-color: %8;
        }

        /* 危险按钮（红色） */
        QPushButton[danger="true"] {
            background-color: %11;
        }
        QPushButton[danger="true"]:hover {
            background-color: %12;
        }

        /* 输入框与数值步进框（QSpinBox/QDoubleSpinBox 共用同一组令牌） */
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox {
            background-color: %1;
            color: %3;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 6px 8px;
            font-size: %5px;
            selection-background-color: %6;
            selection-color: %3;
        }
        QLineEdit:focus, QTextEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 1px solid %6;
        }
        /* 步进框上下箭头按钮遵循主按钮令牌，避免默认原生样式破坏暗色基调 */
        QSpinBox::up-button, QSpinBox::down-button,
        QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
            background-color: %2;
            border: none;
            width: 18px;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover,
        QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
            background-color: %4;
        }
        QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled,
        QSpinBox:disabled, QDoubleSpinBox:disabled {
            background-color: %2;
            color: %10;
            border: 1px solid %4;
        }

        /* 标签：统一主文本色与字号，避免默认色在暗色背景上对比不足 */
        QLabel {
            color: %3;
            background: transparent;
            font-size: %5px;
        }
        QLabel:disabled {
            color: %10;
        }

        /* 下拉框 */
        QComboBox {
            background-color: %1;
            color: %3;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 6px 8px;
            font-size: %5px;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 6px solid %3;
        }
        QComboBox QAbstractItemView {
            background-color: %2;
            color: %3;
            border: 1px solid %4;
            selection-background-color: %6;
        }

        /* 标签页 */
        QTabWidget::pane {
            border: none;
            background-color: %1;
        }
        QTabBar::tab {
            background-color: %2;
            color: %10;
            padding: 8px 16px;
            border: none;
            font-size: %5px;
        }
        QTabBar::tab:selected {
            background-color: %1;
            color: %3;
            border-bottom: 2px solid %6;
        }
        QTabBar::tab:hover {
            background-color: %4;
        }

        /* 滚动条 */
        QScrollBar:vertical {
            background: %2;
            width: 10px;
            border: none;
        }
        QScrollBar::handle:vertical {
            background: %10;
            border-radius: 5px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: %3;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background: %2;
            height: 10px;
            border: none;
        }
        QScrollBar::handle:horizontal {
            background: %10;
            border-radius: 5px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background: %3;
        }

        /* 列表 */
        QListWidget, QTableWidget {
            background-color: %1;
            color: %3;
            border: none;
            font-size: %5px;
        }
        QListWidget::item, QTableWidget::item {
            padding: 8px;
        }
        QListWidget::item:hover, QTableWidget::item:hover {
            background-color: %14;
        }
        QListWidget::item:selected, QTableWidget::item:selected {
            background-color: %13;
            color: %3;
        }

        /* 表头 */
        QHeaderView::section {
            background-color: %2;
            color: %3;
            padding: 8px;
            border: none;
            border-bottom: 1px solid %4;
            font-size: %5px;
        }

        /* 分隔符 */
        QFrame[frameShape="4"], QFrame[frameShape="5"] {
            background-color: %4;
        }

        /* 复选框 */
        QCheckBox {
            color: %3;
            font-size: %5px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid %4;
            border-radius: 3px;
            background-color: %1;
        }
        QCheckBox::indicator:checked {
            background-color: %6;
            border-color: %6;
        }

        /* 单选按钮 */
        QRadioButton {
            color: %3;
            font-size: %5px;
        }
        QRadioButton::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid %4;
            border-radius: 8px;
            background-color: %1;
        }
        QRadioButton::indicator:checked {
            background-color: %6;
            border-color: %6;
        }

        /* 工具提示 */
        QToolTip {
            background-color: %2;
            color: %3;
            border: 1px solid %4;
            padding: 4px;
            font-size: %5px;
        }

        /* 状态栏 */
        QStatusBar {
            background-color: %1;
            color: %3;
            border-top: 1px solid %4;
        }

        /* 浮动窗口 */
        QDockWidget {
            background-color: %15;
            color: %3;
        }
        QDockWidget::title {
            background-color: %2;
            padding: 4px 8px;
            border-bottom: 1px solid %4;
        }
        QDockWidget::close-button, QDockWidget::float-button {
            background-color: transparent;
            border: none;
        }

        /* 消息框 */
        QMessageBox {
            background-color: %15;
        }
        QMessageBox QLabel {
            color: %3;
            font-size: %5px;
        }

        /* ===== 属性化样式词汇表（Phase 1.2 setStyleSheet 计数缩减）=====
           约定：
           1) 容器规则（containerBg/edgeBorder/cardRadius）作用于 QWidget 家族，
              使用方需 setAttribute(Qt::WA_StyledBackground, true) 确保背景绘制；
           2) 运行期切换属性后必须 repolish（style()->unpolish(w); style()->polish(w);），
              构造期设置的静态属性无需 repolish；
           3) 同一 QLabel 可组合 labelRole（排版）与 textColor（语义色），
              textColor 声明在后，语义色优先生效。 */

        /* 容器背景（containerBg）：main=主窗体 panel=面板 toolbar=工具栏 */
        QWidget[containerBg="main"] { background-color: %1; }
        QWidget[containerBg="panel"] { background-color: %15; }
        QWidget[containerBg="toolbar"] { background-color: %2; }
        QWidget[containerBg="transparent"] { background: transparent; }
        /* 容器单侧描边（edgeBorder）：面板分隔线 */
        QWidget[edgeBorder="top"] { border-top: 1px solid %4; }
        QWidget[edgeBorder="right"] { border-right: 1px solid %4; }
        QWidget[edgeBorder="bottom"] { border-bottom: 1px solid %4; }
        QWidget[edgeBorder="left"] { border-left: 1px solid %4; }
        /* 卡片圆角容器 */
        QWidget[cardRadius="true"] { border-radius: 4px; }

        /* 标签角色（labelRole）：h1=模块标题(16px) h2=区块标题(14px) body2/caption=次级说明(12px) */
        QLabel[labelRole="h1"] { color: %3; font-size: %19px; font-weight: bold; background: transparent; }
        QLabel[labelRole="h2"] { color: %3; font-size: %5px; font-weight: bold; background: transparent; }
        QLabel[labelRole="body2"] { color: %18; font-size: %20px; background: transparent; }
        QLabel[labelRole="caption"] { color: %18; font-size: %20px; background: transparent; }
        /* 导航栏 LOGO 专属（原 DEC-NAV-LOGO 内联样式迁移，含字距） */
        QLabel[labelRole="logo"] { color: %6; font-size: 18px; font-weight: bold; letter-spacing: 2px; }

        /* 标签语义前景色（textColor）：状态/威胁等级等 */
        QLabel[textColor="secondary"] { color: %18; background: transparent; }
        QLabel[textColor="green"] { color: %6; background: transparent; }
        QLabel[textColor="white"] { color: %3; background: transparent; }
        QLabel[textColor="online"] { color: %21; background: transparent; }
        QLabel[textColor="busy"] { color: %22; background: transparent; }
        QLabel[textColor="offline"] { color: %23; background: transparent; }
        QLabel[textColor="high"] { color: %24; background: transparent; }
        QLabel[textColor="medium"] { color: %25; background: transparent; }
        QLabel[textColor="low"] { color: %26; background: transparent; }
        QLabel[textColor="error"] { color: %11; background: transparent; }
        /* disabled=禁用/未选择占位（恢复决策建议风险标签降级色语义） */
        QLabel[textColor="disabled"] { color: %10; background: transparent; }

        /* 字号/字重/字族覆盖（fontSize/fontWeight/fontFamily）：单属性微调，无背景副作用；
           声明于 labelRole 之后（同特异度后声明者胜，可覆盖角色字号），与 textColor 可自由组合。
           档位 9/10/11/13 为存量内联字号收敛登记（12/14 与 Caption/Body token 等值）。 */
        QLabel[fontSize="9"] { font-size: 9px; }
        QLabel[fontSize="10"] { font-size: 10px; }
        QLabel[fontSize="11"] { font-size: 11px; }
        QLabel[fontSize="12"] { font-size: 12px; }
        QLabel[fontSize="13"] { font-size: 13px; }
        QLabel[fontSize="14"] { font-size: 14px; }
        QLabel[fontWeight="bold"] { font-weight: bold; }
        /* 等宽字族（mono）：坐标/编号/数值列，写法与基线内联样式逐字一致 */
        QLabel[fontFamily="mono"] { font-family: 'Consolas', 'Courier New', monospace; }

        /* 标签背景（labelBg）：恢复基线裸样式表级联产生的不透明标签底。
           基线容器裸样式表（无选择器）的 background/border 级联到子孙 QLabel 并被原生绘制；
           属性化改造后级联消失，本规则按标签显式恢复底色。
           必须置于 labelRole/textColor 之后：同特异度下后声明者胜，
           以 background-color 长属性覆盖前述 background: transparent 简写。 */
        QLabel[labelBg="main"] { background-color: %1; }
        QLabel[labelBg="panel"] { background-color: %15; }
        QLabel[labelBg="toolbar"] { background-color: %2; border-radius: 4px; }
        /* chip=紧凑数值徽标底：Toolbar 底 + 2px/8px 内边距（原派生计数值盒迁移） */
        QLabel[labelBg="chip"] { background-color: %2; padding: 2px 8px; }

        /* 状态横幅（stateBanner）：整行状态底色+前景色，运行期切换属性后需 repolish；
           声明于 labelBg 之后，同特异度下接管底色 */
        QLabel[stateBanner="idle"] { background-color: %2; color: %18; }
        QLabel[stateBanner="planning"] { background-color: %16; color: %3; }
        QLabel[stateBanner="loading"] { background-color: %25; color: %1; }
        QLabel[stateBanner="ok"] { background-color: %21; color: %3; }
        QLabel[stateBanner="error"] { background-color: %24; color: %3; }
        QLabel[stateBanner="empty"] { background-color: %25; color: %1; }
        QLabel[stateBanner="nofeasible"] { background-color: %11; color: %3; }

        /* 按钮变体（btnVariant） */
        /* subtle=弱化按钮：工具栏底色+次级文字，hover 描边色 */
        QPushButton[btnVariant="subtle"] {
            background-color: %2;
            color: %18;
            border: none;
            border-radius: 4px;
            font-size: %20px;
        }
        QPushButton[btnVariant="subtle"]:hover { background-color: %4; }
        /* compact=紧凑按钮：仅缩小字号并解除全局最小宽度（与其他属性组合使用） */
        QPushButton[btnVariant="compact"] {
            font-size: %20px;
            min-width: 0px;
        }
        /* icon=透明图标按钮：次级文字色，hover 主文字色（原内联 #AAA/white 与 TextSecondary/TextPrimary 令牌值一致） */
        QPushButton[btnVariant="icon"] {
            background: transparent;
            color: %18;
            border: none;
            font-size: %19px;
        }
        QPushButton[btnVariant="icon"]:hover { color: %3; }
        /* tab=下划线选项卡：配合 selected 属性标记当前项（原状态子标签样式迁移） */
        QPushButton[btnVariant="tab"] {
            background-color: transparent;
            color: %18;
            border: none;
            border-bottom: 2px solid transparent;
            min-width: 0px;
            padding: 8px 2px;
            font-size: %20px;
        }
        QPushButton[btnVariant="tab"]:hover { background-color: %4; }
        QPushButton[btnVariant="tab"][selected="true"] {
            color: %3;
            border-bottom: 2px solid %6;
        }
        /* flat=扁平工具栏按钮：透明底+主文字色+紧凑内边距，hover 描边色，禁用降为 %10；
           min-width/圆角/pressed/禁用底色沿用基础 QPushButton 规则（原 MainWindow 地图工具栏 btnStyle 迁移） */
        QPushButton[btnVariant="flat"] {
            background-color: transparent;
            color: %3;
            border: none;
            padding: 4px 12px;
            font-size: %20px;
        }
        QPushButton[btnVariant="flat"]:hover { background-color: %4; }
        QPushButton[btnVariant="flat"]:disabled { color: %10; }

        /* 导航按钮（navBtn）：配合 selected 属性；
           hover/选中底色复用 RowHover/SelectionBackground 令牌（原内联硬编码 #2A2A2A/#2A3F54 与令牌值一致） */
        QPushButton[navBtn="true"] {
            background-color: transparent;
            color: %18;
            border: none;
            border-left: 3px solid transparent;
            min-width: 0px;
            padding: 12px 0px;
            font-size: %20px;
            text-align: center;
        }
        QPushButton[navBtn="true"]:hover {
            background-color: %14;
            color: %3;
        }
        QPushButton[navBtn="true"][selected="true"] {
            background-color: %16;
            color: %3;
            border-left: 3px solid %6;
            font-weight: bold;
        }

        /* 紧凑输入框（fieldVariant）：
           原 ::placeholder 规则不被 Qt5 QSS 支持（无效声明），转换时未迁移 */
        QLineEdit[fieldVariant="compact"] {
            background-color: %1;
            color: %3;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QLineEdit[fieldVariant="compact"]:focus { border: 1px solid %6; }

        /* ===== 决策页词汇（2026-08 批次6 收敛登记，均为存量内联值的等价迁移，非新增视觉值）===== */

        /* sectionTitle=区块标题标签：panel=面板通栏标题（PanelBackground 底+主文本色+6px 内边距），
           inset=面板内嵌小标题（次文本色+4/8 内边距，原决策页跑道标题迁移） */
        QLabel[sectionTitle="panel"] {
            background: %15;
            color: %3;
            padding: 6px;
        }
        QLabel[sectionTitle="inset"] {
            color: %18;
            padding: 4px 8px;
        }

        /* chipStyle=紧凑徽标标签：warnBadge=工具栏威胁徽标（ThreatMedium 色描边+1/6 内边距），
           simTag=状态栏模拟标签（同色描边+0/4 内边距） */
        QLabel[chipStyle="warnBadge"] {
            color: %25;
            border: 1px solid %25;
            padding: 1px 6px;
        }
        QLabel[chipStyle="simTag"] {
            color: %25;
            border: 1px solid %25;
            padding: 0 4px;
        }

        /* statusDot=状态圆点标签：8px 圆点 4px 圆角，取威胁/在线令牌；
           uxo=未爆弹图例黄，字面量在 Colors 中无等值 token（ThreatHigh 为红非同色），在此收敛登记；
           device=10px 圆点 5px 圆角（状态栏设备在线点） */
        QLabel[statusDot="high"] {
            background: %24;
            border-radius: 4px;
            min-width: 8px; max-width: 8px;
            min-height: 8px; max-height: 8px;
        }
        QLabel[statusDot="medium"] {
            background: %25;
            border-radius: 4px;
            min-width: 8px; max-width: 8px;
            min-height: 8px; max-height: 8px;
        }
        QLabel[statusDot="online"] {
            background: %21;
            border-radius: 4px;
            min-width: 8px; max-width: 8px;
            min-height: 8px; max-height: 8px;
        }
        QLabel[statusDot="uxo"] {
            background: #FFEB3B;
            border-radius: 4px;
            min-width: 8px; max-width: 8px;
            min-height: 8px; max-height: 8px;
        }
        QLabel[statusDot="device"] {
            background: %21;
            border-radius: 5px;
            min-width: 10px; max-width: 10px;
            min-height: 10px; max-height: 10px;
        }

        /* slotStyle=禁用占位插槽标签：title=插槽标题（禁用文本色+虚线边框+4px 内边距），
           item=插槽条目（ToolbarBackground 底+禁用文本色+虚线边框+3px 内边距） */
        QLabel[slotStyle="title"] {
            color: %10;
            border: 1px dashed %4;
            padding: 4px;
        }
        QLabel[slotStyle="item"] {
            background: %2;
            color: %10;
            border: 1px dashed %4;
            padding: 3px;
        }

        /* zoom=地图缩放按钮：工具栏底色+边框+紧凑内边距+30px 最小宽（原决策页缩放行内联迁移；
           仅用于 +/- 单字符钮。复位钮禁用本变体：基线复位钮内联无 min-width，回落基础规则
           min-width:%7（实宽 102px），误用 zoom 会因 30px 最小宽缩至 52px（批次6 矩阵实证） */
        QPushButton[btnVariant="zoom"] {
            background-color: %2;
            color: %3;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 4px 10px;
            min-width: 30px;
        }

        /* zoomReset=缩放行复位钮：外观同 zoom 但不声明 min-width，宽度回落基础 QPushButton
           规则的 min-width:%7，保持基线 102px 实宽与缩放行整体几何（批次6 修正 52px 回归） */
        QPushButton[btnVariant="zoomReset"] {
            background-color: %2;
            color: %3;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 4px 10px;
        }

        /* toolBg=工具栏按钮保底底色：仅声明背景属性，其余沿用基础 QPushButton 规则。
           用于容器转 containerBg 后，原由工具栏裸样式表级联提供底色的按钮
           （基础按钮背景是 PrimaryGreen，省略本规则会使按钮变绿，批次6 回归预防） */
        QPushButton[btnVariant="toolBg"] {
            background-color: %2;
        }

        /* mainBg=主区底色按钮保底：仅声明背景为主区 Background %1，其余沿用基础 QPushButton 规则。
            用于容器转 containerBg 后，原由主区裸样式表级联提供底色的按钮
            （基础按钮背景是 PrimaryGreen，省略本规则会使按钮变绿，批次6 回归预防） */
        QPushButton[btnVariant="mainBg"] {
            background-color: %1;
        }

        /* QSplitter 容器化配套：QSplitter 转 containerBg="main" 后把手失去裸级联底色，
           且无全局 QSplitter 规则可依，会回落 Fusion 浅色；此规则恢复基线把手底色 Background */
        QSplitter[containerBg="main"]::handle {
            background-color: %1;
        }
    )")
        .arg(Colors::Background)
        .arg(Colors::ToolbarBackground)
        .arg(Colors::TextPrimary)
        .arg(Colors::Border)
        .arg(Fonts::BodySize)
        .arg(Colors::PrimaryGreen)
        .arg(Sizes::ButtonMinWidth)
        .arg(Colors::PrimaryGreenHover)
        .arg(Colors::PrimaryGreen)
        .arg(Colors::TextDisabled)
        .arg(Colors::DangerRed)
        .arg(Colors::DangerRedHover)
        .arg(Colors::RowSelected)
        .arg(Colors::RowHover)
        .arg(Colors::PanelBackground)
        .arg(Colors::SelectionBackground)
        .arg(Colors::SelectionBorder)
        // 属性化 QSS 词汇表新增令牌（%18-%26），均已确认存在于 GlobalStyle.h
        .arg(Colors::TextSecondary)
        .arg(Fonts::TitleSize)
        .arg(Fonts::CaptionSize)
        .arg(Colors::StatusOnline)
        .arg(Colors::StatusBusy)
        .arg(Colors::StatusOffline)
        .arg(Colors::ThreatHigh)
        .arg(Colors::ThreatMedium)
        .arg(Colors::ThreatLow);
}

QString getButtonStyle(bool isPrimary)
{
    QString bgColor = isPrimary ? Colors::PrimaryGreen : Colors::ToolbarBackground;
    QString hoverColor = isPrimary ? Colors::PrimaryGreenHover : Colors::Border;
    
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 4px;
            padding: 6px 16px;
            font-size: %4px;
        }
        QPushButton:hover {
            background-color: %5;
        }
        QPushButton:pressed {
            background-color: %1;
        }
    )")
        .arg(bgColor)
        .arg(Colors::TextPrimary)
        .arg(Colors::Border)
        .arg(Fonts::BodySize)
        .arg(hoverColor);
}

QString getLineEditStyle()
{
    return QString(R"(
        QLineEdit {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 4px;
            padding: 6px 8px;
            font-size: %4px;
        }
        QLineEdit:focus {
            border: 1px solid %5;
        }
    )")
        .arg(Colors::Background)
        .arg(Colors::TextPrimary)
        .arg(Colors::Border)
        .arg(Fonts::BodySize)
        .arg(Colors::PrimaryGreen);
}

QString getComboBoxStyle()
{
    return QString(R"(
        QComboBox {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 4px;
            padding: 6px 8px;
            font-size: %4px;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox QAbstractItemView {
            background-color: %5;
            color: %2;
            border: 1px solid %3;
            selection-background-color: %6;
        }
    )")
        .arg(Colors::Background)
        .arg(Colors::TextPrimary)
        .arg(Colors::Border)
        .arg(Fonts::BodySize)
        .arg(Colors::PanelBackground)
        .arg(Colors::PrimaryGreen);
}

QString getTableWidgetStyle()
{
    return QString(R"(
        QTableWidget {
            background-color: %1;
            color: %2;
            border: none;
            gridline-color: %3;
            font-size: %4px;
        }
        QTableWidget::item {
            padding: 8px;
            border-bottom: 1px solid %3;
        }
        QTableWidget::item:hover {
            background-color: %6;
        }
        QTableWidget::item:selected {
            background-color: %5;
        }
        QHeaderView::section {
            background-color: %7;
            color: %2;
            padding: 8px;
            border: none;
            border-bottom: 1px solid %3;
            font-size: %4px;
        }
    )")
        .arg(Colors::Background)
        .arg(Colors::TextPrimary)
        .arg(Colors::Border)
        .arg(Fonts::BodySize)
        .arg(Colors::RowSelected)
        .arg(Colors::RowHover)
        .arg(Colors::ToolbarBackground);
}

QString getTabWidgetStyle()
{
    return QString(R"(
        QTabWidget::pane {
            border: none;
            background-color: %1;
        }
        QTabBar::tab {
            background-color: %2;
            color: %3;
            padding: 8px 16px;
            border: none;
            font-size: %4px;
        }
        QTabBar::tab:selected {
            background-color: %1;
            color: %5;
            border-bottom: 2px solid %6;
        }
        QTabBar::tab:hover {
            background-color: %7;
        }
    )")
        .arg(Colors::Background)
        .arg(Colors::ToolbarBackground)
        .arg(Colors::TextSecondary)
        .arg(Fonts::BodySize)
        .arg(Colors::TextPrimary)
        .arg(Colors::PrimaryGreen)
        .arg(Colors::Border);
}

QString getScrollBarStyle()
{
    return QString(R"(
        QScrollBar:vertical {
            background: %1;
            width: 10px;
            border: none;
        }
        QScrollBar::handle:vertical {
            background: %2;
            border-radius: 5px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: %3;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background: %1;
            height: 10px;
            border: none;
        }
        QScrollBar::handle:horizontal {
            background: %2;
            border-radius: 5px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background: %3;
        }
    )")
        .arg(Colors::ToolbarBackground)
        .arg(Colors::TextSecondary)
        .arg(Colors::TextPrimary);
}

}  // namespace GlobalStyle
