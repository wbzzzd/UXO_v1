// MosRunwayWidget 内部几何映射助手：集中内容坐标 <-> 像素坐标的双向仿射变换。
// 不对外暴露，仅由 MosRunwayWidget.cpp 与 MosRunwayWidgetInteraction.cpp 包含。
// 所有几何均为合成本地 fixture 渲染像素布局，非真实跑道坐标。
// 半幅均由快照派生（params.W、障碍物 Y/影响半径、result Y 端点），不再硬编码 ±25/±250。

#ifndef MAINWINDOW_MOSRUNWAYWIDGETINTERNAL_H
#define MAINWINDOW_MOSRUNWAYWIDGETINTERNAL_H

#include "Core/MOS/MosPlanner.h"
#include "Core/MOS/MosPlanningSession.h"

#include <QPointF>
#include <QString>
#include <QTransform>
#include <algorithm>
#include <cmath>

namespace MosRunwayInternal {

constexpr int kMarginX = 40;
constexpr int kMarginY = 20;

// 内容布局：由快照与 widget 尺寸派生，集中管理各向同性比例与像素中心。
struct ContentLayout {
    double L{300.0};            // 跑道长度 (m)，X 比例基准
    double runwayHalfY{25.0};   // 跑道 Y 半幅 (m) = params.W / 2
    double coreHalfY{250.0};    // 核心区 Y 半幅 (m)，覆盖跑道/障碍物 Y/result Y 端点
    int w{0};                   // widget 像素宽度
    int h{0};                   // widget 像素高度
    double coreCenterPx{0.0};   // 核心区像素中线 Y（跑道条与障碍物共用）
    double pxPerM{0.0};         // 各向同性像素/米比例（X/Y 共用同一比例）
    double originX{0.0};        // 内容区像素 X 起点（居中于可用宽度）
};

// 由快照派生内容布局：X 比例取 params.L；跑道半幅取 params.W/2；
// 核心区 Y 半幅取 max(跑道半幅, 障碍物 |y|, result |yStart|/|yEnd|) × 1.1。
// 不含 influenceRadius，避免 UXO 影响圆把核心区撑大导致跑道缩成窄条。
inline ContentLayout computeLayout(const Core::MOS::MosPlanningSnapshot &s, int w, int h)
{
    ContentLayout lay;
    lay.L = s.params.L > 0.0 ? s.params.L : 300.0;
    lay.runwayHalfY = s.params.W > 0.0 ? s.params.W / 2.0 : 25.0;
    // 核心 Y 半幅：覆盖跑道、障碍物中心 Y、result 矩形 Y 端点
    double yExt = lay.runwayHalfY;
    for (const auto &c : s.obstacles.craters) {
        yExt = std::max(yExt, std::abs(static_cast<double>(c.y)));
    }
    for (const auto &u : s.obstacles.uxo) {
        yExt = std::max(yExt, std::abs(static_cast<double>(u.y)));
    }
    if (s.hasResult && s.result.accepted) {
        for (const auto &t : s.result.tiers) {
            if (!t.rectangle.valid) continue;
            yExt = std::max(yExt, std::abs(t.rectangle.yStart));
            yExt = std::max(yExt, std::abs(t.rectangle.yEnd));
        }
    }
    lay.coreHalfY = yExt * 1.1;
    lay.w = w;
    lay.h = h;
    const double usableW = w - 2.0 * kMarginX;
    const double usableH = h - 2.0 * kMarginY;
    // 各向同性比例：X/Y 共用同一 px/m，取较小值保证内容完整显示
    const double pxPerMX = usableW / lay.L;
    const double pxPerMY = usableH / (2.0 * lay.coreHalfY);
    lay.pxPerM = std::min(pxPerMX, pxPerMY);
    // 内容水平居中于可用宽度
    const double contentW = lay.L * lay.pxPerM;
    lay.originX = kMarginX + (usableW - contentW) / 2.0;
    // 核心区像素中线 Y：垂直居中于可用高度
    lay.coreCenterPx = kMarginY + usableH / 2.0;
    return lay;
}

// 内容 X (m) -> 内容像素 X（未应用 zoom/pan）
inline double xToPx(double x, const ContentLayout &lay)
{
    return lay.originX + x * lay.pxPerM;
}

// 核心 Y (m) -> 内容像素 Y（未应用 zoom/pan）
inline double yCoreToPx(double y, const ContentLayout &lay)
{
    return lay.coreCenterPx - y * lay.pxPerM;
}

// 障碍物影响圆像素半径 = influenceRadius(m) × pxPerM。
// paintEvent 影响圆绘制与 hitTestTarget 共用此纯函数，使像素几何与 planner 米制一一对应，
// 消除原 clamp(radius*2.2,6,24) 造成的虚假穿透。不做钳制/最小像素/额外系数。
inline double obstacleRadiusPx(double influenceRadius, const ContentLayout &lay)
{
    return influenceRadius * lay.pxPerM;
}

// 构造内容像素 -> widget 像素的仿射变换：围绕视口中心按 zoom 缩放，再平移 panOffset。
// 该变换设置到 painter；其逆变换用于命中测试（鼠标 widget 坐标 -> 内容像素）。
inline QTransform contentTransform(const ContentLayout &lay, double zoom,
                                   const QPointF &panOffset)
{
    QTransform t;
    t.translate(lay.w / 2.0 + panOffset.x(), lay.h / 2.0 + panOffset.y());
    t.scale(zoom, zoom);
    t.translate(-lay.w / 2.0, -lay.h / 2.0);
    return t;
}

// 钳制平移：内容经 zoom 缩放后尺寸 = w*zoom × h*zoom。
// zoom > 1 时允许在溢出半幅内拖动；zoom <= 1 时内容小于或等于视口，pan 钳制为 0（居中）。
// 保证内容不会因拖拽而完全消失。
inline QPointF clampPan(const QPointF &panOffset, double zoom, int w, int h)
{
    const double px = zoom > 1.0 ? (w * (zoom - 1.0)) / 2.0 : 0.0;
    const double py = zoom > 1.0 ? (h * (zoom - 1.0)) / 2.0 : 0.0;
    return QPointF(std::clamp(panOffset.x(), -px, px),
                   std::clamp(panOffset.y(), -py, py));
}

// 档位标签纯格式化助手：输出“档位N X[a..b] Y[c..d] L×Wm”。
// 仅依赖 MosRectangleResult，不涉及 widget/字体/zoom，便于在测试中直接断言内容。
// 1-based 显示档位号；坐标按整数米格式化；长度/宽度保留整数。
// 该函数仅为闭包测试与 paintEvent 共享的格式化契约，不构成公开 API。
inline QString formatTierLabel(int tierIndex, const Core::MOS::MosRectangleResult &rect)
{
    return QStringLiteral("档位%1 X[%2..%3] Y[%4..%5] %6×%7m")
        .arg(tierIndex + 1)
        .arg(static_cast<int>(std::round(rect.xStart)))
        .arg(static_cast<int>(std::round(rect.xEnd)))
        .arg(static_cast<int>(std::round(rect.yStart)))
        .arg(static_cast<int>(std::round(rect.yEnd)))
        .arg(static_cast<int>(std::round(rect.length)))
        .arg(static_cast<int>(std::round(rect.width)));
}

} // namespace MosRunwayInternal

#endif // MAINWINDOW_MOSRUNWAYWIDGETINTERNAL_H
