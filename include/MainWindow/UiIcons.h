#ifndef COMMON_UIICONS_H
#define COMMON_UIICONS_H

#include <QColor>
#include <QIcon>

// UI 图标助手：封装 QtAwesome（Font Awesome）字体图标库的初始化与图标生成。
// 设计目的：让业务控件只依赖本头文件与 GlobalStyle 颜色令牌，
// 不直接感知第三方库实例的生命周期。
namespace UiIcons {

// 幂等初始化字体图标库；须在 QApplication 构建之后调用。
// 返回 false 表示字体加载失败（此时 icon() 返回空 QIcon，控件降级为纯文本）。
bool init();

// 导航项对应的 FA 实心码点（index 0..5：态势/探测/决策/设备/统计/配置，越界返回 0）。
int navGlyph(int index);

// 生成单状态/多状态图标：character 为 fa::fa_common_icons 枚举码点；
// color=常规色，active=悬停色，disabled=禁用色（无效 QColor 表示不生成该状态）。
QIcon icon(int character,
           const QColor &color,
           const QColor &active = QColor(),
           const QColor &disabled = QColor());

} // namespace UiIcons

#endif // COMMON_UIICONS_H
