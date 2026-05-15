/**
 * @file Common/GlobalStyle.h
 * @brief 全局样式定义
 * @details 定义系统统一的颜色、字体、尺寸等视觉规范，遵循UI设计文档
 * @author 开发团队
 * @date 2024-01-01
 * @version 1.0.0
 */

#ifndef COMMON_GLOBALSTYLE_H
#define COMMON_GLOBALSTYLE_H

#include <QString>
#include <QColor>

/**
 * @brief 全局样式命名空间
 * @details 包含系统中使用的所有颜色、字体、尺寸等样式定义
 */
namespace GlobalStyle {

/**
 * @brief 颜色规范
 * @details 按照UI设计文档1.3.1节定义
 */
namespace Colors {
    // 背景色
    const QString Background = "#1E1E1E";         ///< 主窗口背景
    const QString PanelBackground = "#252526";    ///< 左右侧面板背景
    const QString ToolbarBackground = "#2D2D2D";   ///< 工具栏背景
    const QString MenuBackground = "#2D2D2D";      ///< 菜单背景

    // 强调色
    const QString PrimaryGreen = "#4A7A4C";        ///< 军绿色 - 主要按钮、进度条、强调色
    const QString PrimaryGreenHover = "#5A8A5C";  ///< 军绿色悬停态
    const QString DangerRed = "#D32F2F";          ///< 警示红色 - 紧急按钮、高威胁等级
    const QString DangerRedHover = "#B71C1C";     ///< 警示红色悬停态

    // 威胁等级颜色
    const QString ThreatHigh = "#FF5252";         ///< 高威胁 - 红色
    const QString ThreatMedium = "#FFB74D";       ///< 中威胁 - 橙色
    const QString ThreatLow = "#FFF176";          ///< 低威胁 - 黄色

    // 文本色
    const QString TextPrimary = "#FFFFFF";         ///< 主文本色 - 正文
    const QString TextSecondary = "#AAAAAA";      ///< 辅助文本色
    const QString TextDisabled = "#888888";        ///< 禁用文本色

    // 边框色
    const QString Border = "#3C3C3C";            ///< 控件边框
    const QString BorderFocus = "#4A7A4C";        ///< 聚焦边框

    // 状态颜色
    const QString StatusOnline = "#4CAF50";       ///< 在线状态 - 绿色
    const QString StatusOffline = "#888888";      ///< 离线状态 - 灰色
    const QString StatusBusy = "#FFB74D";         ///< 忙碌状态 - 黄色
    const QString StatusError = "#FF5252";        ///< 错误状态 - 红色

    // 优先级颜色
    const QString PriorityP0 = "#FF5252";         ///< P0优先级 - 红色
    const QString PriorityP1 = "#FFB74D";         ///< P1优先级 - 橙色
    const QString PriorityP2 = "#FFF176";          ///< P2优先级 - 黄色

    // 地面/场景颜色
    const QString Ground = "#2D4A2D";              ///< 草地颜色
    const QString Runway = "#3D3D3D";              ///< 跑道颜色
    const QString Taxiway = "#4A4A4A";             ///< 滑行道颜色
}

/**
 * @brief 字体规范
 * @details 按照UI设计文档1.3.2节定义
 */
namespace Fonts {
    const QString Family = "Microsoft YaHei, Source Han Sans SC, SimHei, sans-serif";  ///< 字体族 - 微软雅黑/思源黑体

    const int TitleSize = 16;     ///< 标题字号 - 窗口标题、模块标题
    const int BodySize = 14;       ///< 正文字号 - 主要文本内容
    const int CaptionSize = 12;    ///< 辅助信息字号 - 时间戳、状态说明、小字提示

    const QString TitleWeight = "bold";   ///< 标题字重
    const QString BodyWeight = "normal";   ///< 正文字重
}

/**
 * @brief 尺寸规范
 * @details 按照UI设计文档1.3.1节定义
 */
namespace Sizes {
    const int WindowWidth = 1920;
    const int WindowHeight = 1080;

    const int NavigationBarWidth = 80;
    const int LeftPanelWidth = 320;
    const int RightPanelWidth = 360;
    const int StatusBarHeight = 28;
    const int ToolbarHeight = 32;
    const int MenuBarHeight = 30;
    const int TitleBarHeight = 32;

    const int TargetItemHeight = 56;
    const int TaskItemHeight = 80;
    const int DeviceItemHeight = 64;

    const int ButtonMinWidth = 80;
    const int ButtonHeight = 32;
    const int IconButtonSize = 24;
    const int ToolbarButtonHeight = 32;
}

/**
 * @brief 动画规范
 * @details 按照UI设计文档1.3.4节定义
 */
namespace Animation {
    const int DurationShort = 150;     ///< 短动画 - 150ms
    const int DurationNormal = 200;     ///< 标准动画 - 200ms
    const int DurationLong = 300;       ///< 长动画 - 300ms
    const QString Easing = "ease-in-out";  ///< 动画曲线
}

/**
 * @brief 获取完整的QSS样式表
 * @return QString 完整的QSS样式表
 */
QString getMainWindowStyle();

/**
 * @brief 获取按钮样式
 * @param isPrimary 是否为主按钮
 * @return QString 按钮QSS
 */
QString getButtonStyle(bool isPrimary = false);

/**
 * @brief 获取输入框样式
 * @return QString 输入框QSS
 */
QString getLineEditStyle();

/**
 * @brief 获取下拉框样式
 * @return QString 下拉框QSS
 */
QString getComboBoxStyle();

/**
 * @brief 获取表格样式
 * @return QString 表格QSS
 */
QString getTableWidgetStyle();

/**
 * @brief 获取标签页样式
 * @return QString 标签页QSS
 */
QString getTabWidgetStyle();

/**
 * @brief 获取滚动条样式
 * @return QString 滚动条QSS
 */
QString getScrollBarStyle();

}  // namespace GlobalStyle

#endif  // COMMON_GLOBALSTYLE_H
