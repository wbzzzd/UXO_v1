// MOS 跑道俯视图 QPainter 实现：自绘跑道/刻度/障碍物圆圈/候选档位矩形。
// 仅按 m_snapshot 副本渲染，不持有业务状态、不发起规划、不联网。
// 所有几何均为合成本地 fixture 语义，非真实跑道或真实目标坐标。
// paintEvent 统一应用 contentTransform（zoom × pan）到 painter，
// 字体/笔宽按 m_viewportScale 独立缩放，并用 1/m_zoom 反向补偿以保证 1x/2x 字号恒定。
// 鼠标/滚轮交互见 MosRunwayWidgetInteraction.cpp（按 250 LOC 上限拆分）。

#include "MainWindow/MosRunwayWidget.h"
#include "MosRunwayWidgetInternal.h"
#include "Common/GlobalStyle.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QSet>
#include <QTimer>
#include <QTransform>
#include <algorithm>
#include <cmath>

using namespace MosRunwayInternal;

MosRunwayWidget::MosRunwayWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(400, 220);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 脉冲动画定时器：纯视觉，仅调用 update() 触发重绘，不执行任何业务/IO 操作
    m_pulseTimer = new QTimer(this);
    m_pulseTimer->setInterval(50);
    connect(m_pulseTimer, &QTimer::timeout, this, [this](){
        m_pulsePhase = (m_pulsePhase + 1) % 20;
        update();
    });
}

MosRunwayWidget::~MosRunwayWidget() = default;

void MosRunwayWidget::setSnapshot(const Core::MOS::MosPlanningSnapshot &snapshot)
{
    m_snapshot = snapshot;
    update();
}

void MosRunwayWidget::setSelectedTier(int tierIndex)
{
    if (m_selectedTier == tierIndex) return;
    m_selectedTier = tierIndex;
    update();
}

void MosRunwayWidget::setSelectedTargetId(const QString &id)
{
    if (m_selectedTargetId == id) return;
    m_selectedTargetId = id;
    // 有选中目标时启动脉冲动画，无选中时停止，避免空转
    if (!m_selectedTargetId.isEmpty()) {
        m_pulseTimer->start();
    } else {
        m_pulseTimer->stop();
    }
    update();
}

void MosRunwayWidget::setZoomDisplay(double zoom)
{
    const double clamped = std::clamp(zoom, 0.5, 3.0);
    if (clamped == m_zoom) return;
    m_zoom = clamped;
    // 缩放后内容尺寸变化，需重新钳制平移偏移，避免内容被拖出视口
    m_panOffset = clampPan(m_panOffset, m_zoom, width(), height());
    emit zoomChanged(m_zoom);
    emit panXChanged(static_cast<int>(m_panOffset.x()));
    update();
}

void MosRunwayWidget::zoomBy(double delta)
{
    setZoomDisplay(m_zoom + delta);
}

void MosRunwayWidget::resetView()
{
    m_zoom = 1.0;
    m_panOffset = QPointF();
    emit zoomChanged(m_zoom);
    emit panXChanged(0);
    update();
}

void MosRunwayWidget::setPanX(int px)
{
    // 滚动条驱动 X 平移：clamp 到当前 zoom 允许的范围，保持 Y 偏移不变
    const QPointF clamped = clampPan(QPointF(px, m_panOffset.y()), m_zoom, width(), height());
    if (static_cast<int>(clamped.x()) == static_cast<int>(m_panOffset.x())) return;
    m_panOffset = clamped;
    update();
}

int MosRunwayWidget::panRangeX() const
{
    return m_zoom > 1.0 ? static_cast<int>((width() * (m_zoom - 1.0)) / 2.0) : 0;
}

void MosRunwayWidget::setViewportScale(double scale)
{
    const double clamped = std::clamp(scale, 0.5, 3.0);
    if (clamped == m_viewportScale) return;
    m_viewportScale = clamped;
    update();
}

QTransform MosRunwayWidget::contentTransform() const
{
    const auto lay = computeLayout(m_snapshot, width(), height());
    return MosRunwayInternal::contentTransform(lay, m_zoom, m_panOffset);
}

QPointF MosRunwayWidget::mapContentToWidget(const QPointF &contentPt) const
{
    return contentTransform().map(contentPt);
}

QPointF MosRunwayWidget::mapWidgetToContent(const QPointF &widgetPt) const
{
    return contentTransform().inverted().map(widgetPt);
}

QString MosRunwayWidget::hitTestTarget(const QPointF &widgetPt) const
{
    const QPointF pos = mapWidgetToContent(widgetPt);
    const auto lay = computeLayout(m_snapshot, width(), height());
    // 命中半径与 paintEvent 影响圆共用 obstacleRadiusPx(influenceRadius)，
    // 保证 hit 与绘制位置完全一致，不再使用 visibleRadius/cbrt 估算或像素钳制。
    auto test = [&](double x, double y, double influenceRadius) -> bool {
        const double cx = xToPx(x, lay);
        const double cyp = yCoreToPx(y, lay);
        const double pxR = obstacleRadiusPx(influenceRadius, lay);
        const QPointF d = pos - QPointF(cx, cyp);
        return std::sqrt(d.x() * d.x() + d.y() * d.y()) <= pxR;
    };
    for (const auto &c : m_snapshot.obstacles.craters) {
        if (test(c.x, c.y, c.influenceRadius)) return c.id;
    }
    for (const auto &u : m_snapshot.obstacles.uxo) {
        if (test(u.x, u.y, u.influenceRadius)) return u.id;
    }
    return QString();
}

int MosRunwayWidget::hitTestTier(const QPointF &widgetPt) const
{
    if (!m_snapshot.hasResult || !m_snapshot.result.accepted) return -1;
    // P0 选中档位单方案：仅测试显式选中的有效档位矩形，不遍历全部候选档位。
    // 重叠几何不再回退到绘制最上层；未选中或越界返回 -1。
    if (m_selectedTier < 0 || m_selectedTier >= m_snapshot.result.tiers.size()) return -1;
    const auto &tier = m_snapshot.result.tiers.at(m_selectedTier);
    if (!tier.rectangle.valid) return -1;
    const QPointF pos = mapWidgetToContent(widgetPt);
    const auto lay = computeLayout(m_snapshot, width(), height());
    const double x1 = xToPx(tier.rectangle.xStart, lay);
    const double x2 = xToPx(tier.rectangle.xEnd, lay);
    const double y1 = yCoreToPx(tier.rectangle.yEnd, lay);
    const double y2 = yCoreToPx(tier.rectangle.yStart, lay);
    const QRectF r(x1, y1, x2 - x1, y2 - y1);
    return r.contains(pos) ? m_selectedTier : -1;
}

void MosRunwayWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const int w = width();
    const int h = height();
    const auto lay = computeLayout(m_snapshot, w, h);

    // 背景深绿底（不参与 zoom/pan 变换；#1a2a1a 无等值令牌，保留字面量并在此附说明）
    p.fillRect(rect(), QColor("#1a2a1a"));
    // 应用内容仿射变换：围绕视口中心按 m_zoom 缩放，再平移 m_panOffset
    p.setTransform(contentTransform());

    // 内容区像素尺寸（各向同性比例下）
    const double contentW = lay.L * lay.pxPerM;
    const double corePxHalf = lay.coreHalfY * lay.pxPerM;
    // 核心区叠层：虚线边框 + 半透明背景 + 标签，匹配 HTML .core-overlay
    const QRectF coreRect(lay.originX, lay.coreCenterPx - corePxHalf,
                          contentW, 2.0 * corePxHalf);
    p.fillRect(coreRect, QColor(255, 255, 255, 5));
    p.setPen(QPen(QColor("#555555"), 1, Qt::DashLine));  // #555555 无等值令牌，保留字面量
    p.setBrush(Qt::NoBrush);
    p.drawRect(coreRect);
    p.setPen(QColor(GlobalStyle::Colors::TextDisabled));  // #888888 逐值等价 TextDisabled
    QFont coreFont = font();
    coreFont.setPointSizeF((8.0 * m_viewportScale) / m_zoom);
    p.setFont(coreFont);
    p.drawText(QRectF(lay.originX, lay.coreCenterPx - corePxHalf, 120, 14),
               Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("核心区 %1m").arg(static_cast<int>(lay.coreHalfY * 2)));

    // 跑道灰色长条
    const double runwayTop = yCoreToPx(lay.runwayHalfY, lay);
    const double runwayBot = yCoreToPx(-lay.runwayHalfY, lay);
    const QRectF runwayRect(lay.originX, runwayTop, contentW, runwayBot - runwayTop);
    p.fillRect(runwayRect, QColor(GlobalStyle::Colors::Runway));
    p.setPen(QPen(QColor("#555555"), 1));  // #555555 无等值令牌，保留字面量
    p.drawRect(runwayRect);

    // 跑道中线虚线（#777777 无等值令牌，保留字面量）
    p.setPen(QPen(QColor("#777777"), 1, Qt::DashLine));
    const double cy = (runwayTop + runwayBot) / 2.0;
    p.drawLine(QPointF(lay.originX, cy), QPointF(lay.originX + contentW, cy));

    // 7 条刻度线 + 距离标注
    // 字号按视口缩放，并按 1/zoom 反向补偿：使 1x/2x 字号恒定，仅内容几何缩放
    p.setPen(QPen(QColor(GlobalStyle::Colors::TextDim), 1));  // #666666 逐值等价 TextDim
    QFont smallFont = font();
    smallFont.setPointSizeF((8.0 * m_viewportScale) / m_zoom);
    p.setFont(smallFont);
    for (int i = 0; i <= 6; ++i) {
        const double frac = i / 6.0;
        const double px = lay.originX + frac * contentW;
        p.drawLine(QPointF(px, runwayBot), QPointF(px, runwayBot + 6));
        p.drawText(QRectF(px - 30, runwayBot + 8, 60, 14), Qt::AlignCenter,
                   QString::number(static_cast<int>(frac * lay.L)));
    }

    // 候选档位矩形：P0 选中档位单方案，仅绘制并标注显式选中的有效档位（选中强调样式）。
    // 全档位叠加属 P1 Draft，当前不渲染，避免未选档位叠层造成虚假 UXO/MOS 重叠。
    // 标签独立收集后在跑道下方车道按行贪心排布，避免 1280x720 同 Y 重叠
    struct TierLabel { QString text; double centerX; int widthPx; };
    QVector<TierLabel> tierLabels;
    if (m_snapshot.hasResult && m_snapshot.result.accepted) {
        const auto &tiers = m_snapshot.result.tiers;
        QFont labelFont = font();
        labelFont.setPointSizeF((8.0 * m_viewportScale) / m_zoom);
        const QFontMetrics fm(labelFont);
        if (m_selectedTier >= 0 && m_selectedTier < tiers.size()) {
            const auto &tier = tiers.at(m_selectedTier);
            if (tier.rectangle.valid) {
                const double x1 = xToPx(tier.rectangle.xStart, lay);
                const double x2 = xToPx(tier.rectangle.xEnd, lay);
                const double y1 = yCoreToPx(tier.rectangle.yEnd, lay);
                const double y2 = yCoreToPx(tier.rectangle.yStart, lay);
                const QRectF r(x1, y1, x2 - x1, y2 - y1);
                // 选中档位强调：高不透明度绿色填充 + 粗实线边框
                const QColor fill(76, 175, 80, 200);
                const QColor edge(GlobalStyle::Colors::StatusOnline);
                p.setBrush(fill);
                p.setPen(QPen(edge, (3.0 * m_viewportScale) / m_zoom, Qt::SolidLine));
                p.drawRect(r);
                // 标签格式由共享纯函数 formatTierLabel 锁定（档位号+X/Y 区间+长×宽），
                // 与闭包测试一致，避免 paintEvent 内联格式化漂移。
                const QString text = formatTierLabel(m_selectedTier, tier.rectangle);
                tierLabels.append({text, (x1 + x2) / 2.0, fm.horizontalAdvance(text)});
            }
        }

        // 标签车道：跑道下方按行贪心排布，水平间距 laneGap 防止相邻标签贴连
        const double laneGap = (4.0 * m_viewportScale) / m_zoom;
        const double laneHeight = (16.0 * m_viewportScale) / m_zoom;
        const double laneTop = runwayBot + (24.0 * m_viewportScale) / m_zoom;
        QVector<QVector<TierLabel>> rows;
        for (const auto &lbl : tierLabels) {
            const double halfW = lbl.widthPx / 2.0;
            const double left = lbl.centerX - halfW - laneGap;
            const double right = lbl.centerX + halfW + laneGap;
            bool placed = false;
            for (int row = 0; row < rows.size(); ++row) {
                bool collision = false;
                for (const auto &ex : rows.at(row)) {
                    const double exHalf = ex.widthPx / 2.0;
                    if (left < ex.centerX + exHalf + laneGap
                        && right > ex.centerX - exHalf - laneGap) {
                        collision = true;
                        break;
                    }
                }
                if (!collision) {
                    rows[row].append(lbl);
                    placed = true;
                    break;
                }
            }
            if (!placed) rows.append({lbl});
        }
        p.setFont(labelFont);
        p.setPen(QColor(GlobalStyle::Colors::TextPrimary));
        // 内容像素 X 范围 [kMarginX, w-kMarginX]；标签矩形钳制到该范围，
        // 防止 1280x720 等窄视口下标签贴边或溢出 widget 内容区。
        const double labelLeftMin = lay.originX;
        const double labelRightMax = lay.originX + contentW;
        for (int row = 0; row < rows.size(); ++row) {
            const double y = laneTop + row * laneHeight;
            for (const auto &lbl : rows.at(row)) {
                double left = lbl.centerX - lbl.widthPx / 2.0;
                double right = lbl.centerX + lbl.widthPx / 2.0;
                if (left < labelLeftMin) {
                    left = labelLeftMin;
                    right = left + lbl.widthPx;
                }
                if (right > labelRightMax) {
                    right = labelRightMax;
                    left = right - lbl.widthPx;
                }
                if (left < labelLeftMin) left = labelLeftMin;
                p.drawText(QRectF(left, y, right - left, laneHeight),
                           Qt::AlignCenter, lbl.text);
            }
        }
    }

    // 选中档位的模拟已处理 ID 集合：仅用于画布视觉区分，不写入业务状态、不代表真实修复或安全。
    // 当选中档位为 -1 或无结果时，集合为空，所有障碍物按未处理绘制。
    QSet<QString> simulatedRepairedIds;
    if (m_snapshot.hasResult && m_snapshot.result.accepted
        && m_selectedTier >= 0 && m_selectedTier < m_snapshot.result.tiers.size()) {
        const auto &ids = m_snapshot.result.tiers.at(m_selectedTier).repairedIds;
        for (const auto &id : ids) simulatedRepairedIds.insert(id);
    }

    // 障碍物圆圈绘制 lambda
    // repaired=true 表示该障碍物 ID 在选中档位的 repairedIds 中，按“模拟已处理”样式绘制：
    // 虚线轮廓 + 半透明灰底 + 斜十字标记，仅作合成演练视觉区分，非真实修复/安全状态。
    // 选中目标仍按 selected 强调（更粗实线 + 更高不透明度），与模拟已处理可叠加。
    auto drawObstacle = [&](const QString &id, double x, double y, double influenceRadius,
                            bool isCrater, bool selected, bool repaired) {
        const double cx = xToPx(x, lay);
        const double cyp = yCoreToPx(y, lay);
        // 像素半径 = influenceRadius × pxPerM，与 hitTestTarget 共用，无钳制/系数
        const double pxR = obstacleRadiusPx(influenceRadius, lay);
        QColor base = isCrater ? QColor(GlobalStyle::Colors::ThreatHigh)
                                : QColor(255, 235, 59);
        // 选中目标脉冲发光环：alpha 随相位正弦变化，匹配 HTML @keyframes pulse
        if (selected) {
            const double phase = m_pulsePhase / 20.0;
            const double pulseAlpha = 0.15 + 0.25 * (0.5 + 0.5 * std::sin(phase * 2.0 * 3.14159265358979));
            QColor glowColor = base;
            glowColor.setAlphaF(pulseAlpha);
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(glowColor, (5.0 * m_viewportScale) / m_zoom));
            p.drawEllipse(QPointF(cx, cyp), pxR * 1.6, pxR * 1.6);
        }
        QColor fill = base;
        fill.setAlpha(selected ? 120 : 60);
        p.setBrush(fill);
        // 选中目标用最粗实线强调；模拟已处理用虚线轮廓；其余按默认实线。
        Qt::PenStyle penStyle = selected ? Qt::SolidLine
                                          : (repaired ? Qt::DashLine : Qt::SolidLine);
        double penWidth = ((selected ? 3.0 : 1.0) * m_viewportScale) / m_zoom;
        p.setPen(QPen(base, penWidth, penStyle));
        p.drawEllipse(QPointF(cx, cyp), pxR, pxR);
        // 模拟已处理：叠加斜十字标记，明确区分“选中档位已处理”与“未处理”
        if (repaired) {
            QPen crossPen(QColor(GlobalStyle::Colors::TextSecondary),
                          (1.0 * m_viewportScale) / m_zoom);
            p.setPen(crossPen);
            p.drawLine(QPointF(cx - pxR * 0.7, cyp - pxR * 0.7),
                       QPointF(cx + pxR * 0.7, cyp + pxR * 0.7));
            p.drawLine(QPointF(cx - pxR * 0.7, cyp + pxR * 0.7),
                       QPointF(cx + pxR * 0.7, cyp - pxR * 0.7));
        }
        // ID 标注位于圆圈上方
        p.setPen(QColor(GlobalStyle::Colors::TextPrimary));
        QFont f = font();
        f.setPointSizeF((7.0 * m_viewportScale) / m_zoom);
        p.setFont(f);
        p.drawText(QRectF(cx - 40, cyp - pxR - 14, 80, 12), Qt::AlignCenter, id);
    };

    // 弹坑红色圆圈（影响半径与 planner 米制几何一致）
    for (const auto &c : m_snapshot.obstacles.craters) {
        drawObstacle(c.id, c.x, c.y, c.influenceRadius, true,
                     c.id == m_selectedTargetId,
                     simulatedRepairedIds.contains(c.id));
    }
    // UXO 黄色圆圈（影响半径与 planner 米制几何一致）
    for (const auto &u : m_snapshot.obstacles.uxo) {
        drawObstacle(u.id, u.x, u.y, u.influenceRadius, false,
                      u.id == m_selectedTargetId,
                      simulatedRepairedIds.contains(u.id));
    }
}
