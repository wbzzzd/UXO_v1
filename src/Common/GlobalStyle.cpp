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

        /* 输入框 */
        QLineEdit, QTextEdit, QPlainTextEdit {
            background-color: %1;
            color: %3;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 6px 8px;
            font-size: %5px;
            selection-background-color: %6;
            selection-color: %3;
        }
        QLineEdit:focus, QTextEdit:focus {
            border: 1px solid %6;
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
            background: %10;
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
        QListWidget::item:selected, QTableWidget::item:selected {
            background-color: %13;
            color: %3;
        }
        QListWidget::item:hover, QTableWidget::item:hover {
            background-color: %14;
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
            titlebar-close-icon: url(close.png);
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
        .arg("#2A3F54")
        .arg("#2A2A2A")
        .arg(Colors::PanelBackground);
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
        QTableWidget::item:selected {
            background-color: %5;
        }
        QTableWidget::item:hover {
            background-color: %6;
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
        .arg("#2A3F54")
        .arg("#2A2A2A")
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
    )")
        .arg(Colors::ToolbarBackground)
        .arg(Colors::TextSecondary)
        .arg(Colors::TextPrimary);
}

}  // namespace GlobalStyle
