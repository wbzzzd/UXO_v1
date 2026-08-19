#include "MainWindow/UiIcons.h"

#include <QApplication>
#include <QVariantMap>

// 第三方字体图标库（vendored，见 third_party/QtAwesome）
#include "QtAwesome.h"

namespace {

// 库单例：挂在 qApp 下随应用销毁，避免静态对象的析构顺序问题
fa::QtAwesome *g_awesome = nullptr;

// 导航 6 项的 FA 实心字形（码点见 QtAwesomeEnumGenerated.h）
const int kNavGlyphs[6] = {
    fa::fa_map_location_dot,   // 态势
    fa::fa_satellite_dish,     // 探测
    fa::fa_scale_balanced,     // 决策
    fa::fa_microchip,          // 设备
    fa::fa_chart_column,       // 统计
    fa::fa_gear                // 配置
};

} // namespace

namespace UiIcons {

bool init()
{
    if (g_awesome)
        return true;
    if (!qApp)
        return false;

    g_awesome = new fa::QtAwesome(qApp);
    if (!g_awesome->initFontAwesome()) {
        // 字体注册失败：释放实例并保持未初始化状态，调用方按纯文本降级
        g_awesome->deleteLater();
        g_awesome = nullptr;
        return false;
    }
    return true;
}

int navGlyph(int index)
{
    return (index >= 0 && index < 6) ? kNavGlyphs[index] : 0;
}

QIcon icon(int character, const QColor &color, const QColor &active, const QColor &disabled)
{
    if (!g_awesome)
        return QIcon();

    QVariantMap options;
    if (color.isValid())
        options.insert(QStringLiteral("color"), color);
    if (active.isValid())
        options.insert(QStringLiteral("color-active"), active);
    if (disabled.isValid())
        options.insert(QStringLiteral("color-disabled"), disabled);

    return g_awesome->icon(fa::fa_solid, character, options);
}

} // namespace UiIcons
