#include "MainWindow/GprBScanWidget.h"

#include "GPR/GprBScanData.h"
#include "GPR/GprRadarHeader.h"

#include "Common/GlobalStyle.h"

#include <QColor>
#include <QFont>
#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace {
// 左侧深度轴边距(px)
constexpr int kLeftMargin = 48;
// 底部距离轴边距(px)
constexpr int kBottomMargin = 20;
// 顶部边距(px)：避免首行深度标签贴控件上缘被裁剪
constexpr int kTopMargin = 10;
// 深度轴刻度间隔(m)
constexpr double kDepthTickM = 0.5;
// 距离轴刻度最小间距(px)，据此从 1-2-5 序列中选步长
constexpr double kMinTickSpacingPx = 70.0;
// AGC 滑动半窗(样本)：窗长 = 2*25+1 = 51
constexpr int kAgcHalfWindow = 25;
// 显示映射符号 gamma 指数(§5.4 显示参数建议)
constexpr double kDisplayGamma = 0.8;
// 截幅分位：|归一化幅值| 的 P99.8
constexpr double kClipQuantile = 0.998;
// 深度换算速度(m/ns)，与 GprRadarHeader::depthForSample 默认值同口径
constexpr double kVelocityMPerNs = 0.1;
// 除零/退化值保护下限
constexpr double kEps = 1e-9;
// contrast/zoom 参数下限(>0，防止退化)
constexpr double kMinPositive = 0.01;
// 色标查表级数：|v| ∈ [0,1] 量化为 1024 级
constexpr int kLutLevels = 1024;
} // namespace

GprBScanWidget::GprBScanWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("gprBScanWidget"));
}

GprBScanWidget::~GprBScanWidget() = default;

QSize GprBScanWidget::minimumSizeHint() const
{
    // 剖面组件的合理下限尺寸，保证坐标轴与图像仍有可用可视面积
    return QSize(400, 300);
}

void GprBScanWidget::setScan(const GPR::GprBScanData *scan, const GPR::GprRadarHeader *header)
{
    m_scan = scan;
    m_header = header;
    // 数据源变化 → 重建伪彩缓存
    rebuildImage();
}

void GprBScanWidget::setGainMode(GainMode mode)
{
    m_gainMode = mode;
    // 增益影响成像结果 → 重建伪彩缓存
    rebuildImage();
}

GprBScanWidget::GainMode GprBScanWidget::gainMode() const
{
    return m_gainMode;
}

void GprBScanWidget::setContrast(double factor)
{
    // 对比度仅接受正值，向下钳制
    m_contrast = std::max(factor, kMinPositive);
    // 对比度影响显示映射 → 重建伪彩缓存
    rebuildImage();
}

double GprBScanWidget::contrast() const
{
    return m_contrast;
}

void GprBScanWidget::setZoom(double factor)
{
    // 缩放仅接受正值，向下钳制
    m_zoom = std::max(factor, kMinPositive);
    // 缩放只影响绘制几何，无需重建图像
    update();
}

double GprBScanWidget::zoom() const
{
    return m_zoom;
}

void GprBScanWidget::setCursorTrace(int traceIndex)
{
    m_cursorTrace = traceIndex;
    // 回放游标为纯叠加层 → 仅重绘
    update();
}

int GprBScanWidget::cursorTrace() const
{
    return m_cursorTrace;
}

void GprBScanWidget::setSuspects(const QList<GPR::GprSuspect> &suspects)
{
    m_suspects = suspects;
    // 疑似目标标记为纯叠加层 → 仅重绘
    update();
}

void GprBScanWidget::rebuildImage()
{
    // 数据有效性守卫：任一指针为空或数据/头无效时清空缓存图像
    if (m_scan == nullptr || m_header == nullptr
        || !m_scan->isValid() || !m_header->isValid()) {
        m_image = QImage();
        update();
        return;
    }

    const int traceCount = m_scan->traceCount();
    const int sampleCount = m_scan->sampleCount();
    if (traceCount <= 0 || sampleCount <= 0) {
        // 退化尺寸防御：无法成像
        m_image = QImage();
        update();
        return;
    }

    // 逐通道 t0 校正(§5.4)：图像第 0 行对应 firstBreakSample；
    // 无效估计(-1)按 0 处理；上限钳制保证图像高度 >= 1
    m_t0Sample = m_scan->firstBreakSample();
    if (m_t0Sample < 0) {
        m_t0Sample = 0;
    }
    if (m_t0Sample >= sampleCount) {
        m_t0Sample = sampleCount - 1;
    }
    const int imgHeight = sampleCount - m_t0Sample;

    // 逐道取数构建连续幅值缓冲(行 = 样本-t0，列 = 道)。
    // 必须整道访问 trace(t)：底层 LRU 缓存以道为粒度，逐样本 sampleAt 会反复
    // 触发整道文件读取，7297 道 × 512 样本量级下不可接受
    std::vector<float> buf(static_cast<std::size_t>(imgHeight) * traceCount, 0.0f);
    for (int t = 0; t < traceCount; ++t) {
        const QVector<qint16> tr = m_scan->trace(t);
        // 有效样本行数：道数据不足时余下行补 0
        const int usable = tr.size() - m_t0Sample;
        for (int r = 0; r < imgHeight; ++r) {
            buf[static_cast<std::size_t>(r) * traceCount + t] =
                (r < usable) ? static_cast<float>(tr.at(m_t0Sample + r)) : 0.0f;
        }
    }

    // 增益三模式：增益公式为实现期选型，§5.4 仅约束三模式存在与 AGC 默认
    if (m_gainMode == GainMode::Agc) {
        // AGC(默认)：逐道滑动窗 RMS 时变增益，normalized = v / max(winRms, eps)；
        // 窗沿样本轴(深度方向)，半窗 25 → 窗长约 51 样本；
        // 前缀平方和使任意窗 RMS 为 O(1)
        std::vector<double> sq(static_cast<std::size_t>(imgHeight) + 1, 0.0);
        for (int t = 0; t < traceCount; ++t) {
            for (int r = 0; r < imgHeight; ++r) {
                const double v = buf[static_cast<std::size_t>(r) * traceCount + t];
                sq[static_cast<std::size_t>(r) + 1] =
                    sq[static_cast<std::size_t>(r)] + v * v;
            }
            for (int r = 0; r < imgHeight; ++r) {
                const int lo = std::max(0, r - kAgcHalfWindow);
                const int hi = std::min(imgHeight - 1, r + kAgcHalfWindow);
                const double winRms =
                    std::sqrt((sq[hi + 1] - sq[lo]) / double(hi - lo + 1));
                float &dst = buf[static_cast<std::size_t>(r) * traceCount + t];
                dst = static_cast<float>(dst / std::max(winRms, kEps));
            }
        }
    } else if (m_gainMode == GainMode::Fixed) {
        // 固定：逐深度行白化增益 gain = min(10, refLevel / max(rowRms, eps))，
        // refLevel = 各行 RMS 的中位数(偶数取下中位，与 firstBreakSample 口径一致)，
        // 与 §5.4 检测管线的深度白化同族，上限 10×
        std::vector<double> rowRms(static_cast<std::size_t>(imgHeight), 0.0);
        for (int r = 0; r < imgHeight; ++r) {
            double acc = 0.0;
            for (int t = 0; t < traceCount; ++t) {
                const double v = buf[static_cast<std::size_t>(r) * traceCount + t];
                acc += v * v;
            }
            rowRms[static_cast<std::size_t>(r)] = std::sqrt(acc / double(traceCount));
        }
        std::vector<double> ordered(rowRms);
        const std::size_t mid = (ordered.size() - 1) / 2;
        std::nth_element(ordered.begin(),
                         ordered.begin() + static_cast<std::ptrdiff_t>(mid),
                         ordered.end());
        const double refLevel = ordered[mid];
        for (int r = 0; r < imgHeight; ++r) {
            const double gain = std::min(10.0, refLevel / std::max(rowRms[static_cast<std::size_t>(r)], kEps));
            for (int t = 0; t < traceCount; ++t) {
                float &dst = buf[static_cast<std::size_t>(r) * traceCount + t];
                dst = static_cast<float>(dst * gain);
            }
        }
    }
    // Raw：不做增益，直接使用原始幅值

    // 显示映射(§5.4 显示参数建议)：
    // 1) clip = |归一化幅值| 的 P99.8 分位(在副本上做 nth_element，全零数据用 eps 保护)
    std::vector<float> absVals;
    absVals.reserve(buf.size());
    for (float v : buf) {
        absVals.push_back(std::fabs(v));
    }
    float clip = 0.0f;
    if (!absVals.empty()) {
        const std::size_t nth = std::min<std::size_t>(
            absVals.size() - 1,
            static_cast<std::size_t>(kClipQuantile * double(absVals.size() - 1)));
        std::nth_element(absVals.begin(),
                         absVals.begin() + static_cast<std::ptrdiff_t>(nth),
                         absVals.end());
        clip = absVals[nth];
    }
    clip = std::max(clip, static_cast<float>(kEps));

    // 2) 色标查表(LUT)：把符号 gamma(v = sign(v)*|v|^0.8) 与 diverging seismic
    //    插值(负-蓝 #2b6cb0 / 零-白 / 正-红 #c53030)预计算到 1025 级表，
    //    避免约 3.7M 像素逐点 pow
    std::vector<QRgb> lutPos(kLutLevels + 1);
    std::vector<QRgb> lutNeg(kLutLevels + 1);
    for (int i = 0; i <= kLutLevels; ++i) {
        const double a = std::pow(double(i) / double(kLutLevels), kDisplayGamma);
        lutPos[static_cast<std::size_t>(i)] =
            qRgb(255 - int(58.0 * a + 0.5),   // 白 → 红 #c53030 (197,48,48)
                 255 - int(207.0 * a + 0.5),
                 255 - int(207.0 * a + 0.5));
        lutNeg[static_cast<std::size_t>(i)] =
            qRgb(255 - int(212.0 * a + 0.5),  // 白 → 蓝 #2b6cb0 (43,108,176)
                 255 - int(147.0 * a + 0.5),
                 255 - int(79.0 * a + 0.5));
    }

    // 3) 逐像素：v = 归一化/clip × contrast → 钳制 [-1,1] → 查表上色
    m_image = QImage(traceCount, imgHeight, QImage::Format_RGB32);
    const float scale = static_cast<float>(m_contrast) / clip;
    for (int r = 0; r < imgHeight; ++r) {
        auto *line = reinterpret_cast<QRgb *>(m_image.scanLine(r));
        const float *src = &buf[static_cast<std::size_t>(r) * traceCount];
        for (int t = 0; t < traceCount; ++t) {
            float v = src[t] * scale;
            if (v > 1.0f) {
                v = 1.0f;
            } else if (v < -1.0f) {
                v = -1.0f;
            }
            // |v| 量化查表；|v| <= 1 保证索引 <= kLutLevels
            const int idx = static_cast<int>(std::fabs(v) * float(kLutLevels) + 0.5f);
            line[t] = (v >= 0.0f) ? lutPos[static_cast<std::size_t>(idx)]
                                  : lutNeg[static_cast<std::size_t>(idx)];
        }
    }

    update();
}

void GprBScanWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    // 剖面区与边距统一深色底
    painter.fillRect(rect(), QColor(16, 16, 16));

    if (m_image.isNull()) {
        // 无数据提示：数据未加载或无效
        QFont hintFont = painter.font();
        hintFont.setPointSize(11);
        painter.setFont(hintFont);
        painter.setPen(QColor(GlobalStyle::Colors::TextSecondary));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("无数据"));
        return;
    }

    const int plotLeft = kLeftMargin;
    const int plotTop = kTopMargin;
    const int plotW = width() - kLeftMargin;
    const int plotH = height() - kTopMargin - kBottomMargin;
    if (plotW <= 0 || plotH <= 0) {
        return; // 控件尺寸过小，无法绘制剖面
    }

    const int traceCount = m_image.width();
    const int imgHeight = m_image.height();

    // 显示宽度：zoom = 1.0 适配绘图区宽度(头部契约)，> 1 水平放大；
    // MVP 无滚动条，超出绘图区右缘的部分直接裁剪(仅可见左侧区间)
    const double displayW = double(plotW) * m_zoom;
    const double sx = displayW / double(traceCount);        // 每道像素宽
    const double sy = double(plotH) / double(imgHeight);    // 每行(样本)像素高

    // 剖面区裁剪：图像、疑似标记、回放游标均不越出绘图区
    painter.setClipRect(QRect(plotLeft, plotTop, plotW, plotH));
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 伪彩剖面：宽 = displayW(随缩放)，高纵向拉伸到绘图区高度
    painter.drawImage(QRectF(double(plotLeft), double(plotTop), displayW, double(plotH)),
                      m_image,
                      QRectF(0.0, 0.0, double(traceCount), double(imgHeight)));

    // 疑似目标标记：外接框中心 = (traceIndex, sampleIndex - t0)，
    // 半跨度 = (traceSpan/2, sampleSpan/2)，橙色警示色 + ID 标签
    {
        QPen suspectPen(QColor(255, 152, 0));
        suspectPen.setWidth(1);
        painter.setPen(suspectPen);
        painter.setBrush(Qt::NoBrush);
        QFont idFont = painter.font();
        idFont.setPointSize(8);
        idFont.setBold(true);
        painter.setFont(idFont);
        for (const GPR::GprSuspect &sp : m_suspects) {
            const int row = sp.sampleIndex - m_t0Sample;
            // t0 校正后位于可视区之上(或索引越界)的标记不绘制
            if (row < 0 || row >= imgHeight
                || sp.traceIndex < 0 || sp.traceIndex >= traceCount) {
                continue;
            }
            const double cx = double(plotLeft) + (double(sp.traceIndex) + 0.5) * sx;
            const double cy = double(plotTop) + (double(row) + 0.5) * sy;
            const QRectF box(cx - sp.traceSpan * 0.5 * sx,
                             cy - sp.sampleSpan * 0.5 * sy,
                             sp.traceSpan * sx,
                             sp.sampleSpan * sy);
            painter.drawRect(box);
            // ID 标签放框左上方；顶部空间不足时改放框下方，避免被裁剪
            QPointF labelPos(box.left(), box.top() - 2.0);
            if (labelPos.y() < double(plotTop) + 10.0) {
                labelPos.setY(box.bottom() + 10.0);
            }
            painter.drawText(labelPos, QString::number(sp.id));
        }
    }

    // 回放游标：当前道中心处亮黄色 1px 竖线
    if (m_cursorTrace >= 0 && m_cursorTrace < traceCount) {
        const double cx = double(plotLeft) + (double(m_cursorTrace) + 0.5) * sx;
        QPen cursorPen(QColor(255, 235, 59), 1);
        painter.setPen(cursorPen);
        painter.drawLine(QPointF(cx, double(plotTop)),
                         QPointF(cx, double(plotTop + plotH)));
    }

    // 坐标轴文本/刻度不受剖面区裁剪
    painter.setClipping(false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setPen(QColor(GlobalStyle::Colors::TextSecondary));
    QFont axisFont = painter.font();
    axisFont.setPointSize(8);
    axisFont.setBold(false);
    painter.setFont(axisFont);

    // 深度轴(左)：每 0.5 m 一个刻度；depth = depthForSample(t0 + row)，
    // 反解行号 sample = depth / (v·dt/2)，与 depthForSample 默认速度同口径
    const double dt = m_header->dtNs();
    const double mPerSample = kVelocityMPerNs * dt / 2.0;
    const double maxDepth = m_header->depthForSample(m_t0Sample + imgHeight - 1);
    if (mPerSample > kEps) {
        for (int k = 1;; ++k) {
            const double d = double(k) * kDepthTickM;
            if (d > maxDepth + kEps) {
                break;
            }
            const int row = qRound(d / mPerSample) - m_t0Sample;
            if (row < 0 || row >= imgHeight) {
                continue;
            }
            const double y = double(plotTop) + (double(row) + 0.5) * sy;
            painter.drawLine(QPointF(double(plotLeft) - 4.0, y),
                             QPointF(double(plotLeft), y));
            painter.drawText(QRectF(0.0, y - 6.0, double(kLeftMargin) - 8.0, 12.0),
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(d, 'f', 1) + QStringLiteral(" m"));
        }
    }

    // 距离轴(底)：distance = trace × distanceIntervalM()；
    // 从 1-2-5 序列中取满足刻度间距约 ≥70px 的最小“整刻度”步长
    const double interval = m_header->distanceIntervalM();
    const double totalDist = double(traceCount - 1) * interval;
    if (interval > kEps && totalDist > kEps) {
        constexpr double kSteps[] = { 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0,
                                      20.0, 50.0, 100.0, 200.0, 500.0, 1000.0 };
        const double minStep = kMinTickSpacingPx * totalDist / double(plotW);
        double step = kSteps[sizeof(kSteps) / sizeof(kSteps[0]) - 1];
        for (double s : kSteps) {
            if (s >= minStep) {
                step = s;
                break;
            }
        }
        const int decimals = step < 1.0 ? 1 : 0;
        const int axisY = plotTop + plotH;
        for (int k = 0;; ++k) {
            const double d = double(k) * step;
            if (d > totalDist + kEps) {
                break;
            }
            const int trace = qRound(d / interval);
            if (trace >= traceCount) {
                break;
            }
            const double x = double(plotLeft) + (double(trace) + 0.5) * sx;
            painter.drawLine(QPointF(x, double(axisY)),
                             QPointF(x, double(axisY) + 4.0));
            painter.drawText(QRectF(x - 30.0, double(axisY) + 4.0, 60.0, 12.0),
                             Qt::AlignHCenter | Qt::AlignTop,
                             QString::number(d, 'f', decimals) + QStringLiteral(" m"));
        }
    }
}
