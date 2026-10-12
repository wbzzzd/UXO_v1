#ifndef MAINWINDOW_GPRBSCANWIDGET_H
#define MAINWINDOW_GPRBSCANWIDGET_H

// 雷达页 B-Scan 伪彩剖面组件（REQ-012 / gpr-display.md §5.3、§6.2）：
// QImage 映射渲染（diverging seismic 类色标：负-蓝 / 零-白 / 正-红；
// P99.8 截幅 + γ=0.8 显示映射，§5.4 显示参数建议）。
// 增益三模式：AGC（默认，逐道滑动 RMS 时变增益）/ 固定（逐深度行白化增益，上限 10×，
// 与 §5.4 检测管线白化同族）/ 原始（未增益）；具体增益公式为实现期选型
// （§5.4 仅约束三模式存在与 AGC 默认）。支持对比度（归一化幅值线性拉伸）与水平缩放。
// 叠加：回放游标（当前道竖线）+ 疑似目标标记（外接框 + ID 标签，§6.2）。
// 坐标口径：纵轴样本经逐通道 t0（firstBreakSample）校正后按 depthForSample 换算深度，
// 图像第 0 行 = t0 样本（§5.4：显示与检测前必须逐通道 t0 校正）。
// 数据所有权：本组件不拥有数据，调用方（GprView）保证指针在下一次 setScan 前有效；
// 展示数据为模拟/演示用途（§6.4）。

#include "GPR/GprThresholdDetector.h"   // GPR::GprSuspect

#include <QWidget>

namespace GPR {
class GprBScanData;
class GprRadarHeader;
}

class GprBScanWidget : public QWidget
{
    Q_OBJECT

public:
    // 增益模式：Agc=逐道滑动 RMS 时变增益（默认）；Fixed=逐深度行白化（≤10×）；Raw=未增益
    enum class GainMode { Agc, Fixed, Raw };
    Q_ENUM(GainMode)

    explicit GprBScanWidget(QWidget *parent = nullptr);
    ~GprBScanWidget() override;

    // 绑定当前通道数据（非拥有）；scan/header 任一为 nullptr 或无效时清空显示。
    void setScan(const GPR::GprBScanData *scan, const GPR::GprRadarHeader *header);

    void setGainMode(GainMode mode);
    GainMode gainMode() const;

    // 对比度：归一化幅值线性拉伸系数（>0，1.0 = 默认）
    void setContrast(double factor);
    double contrast() const;

    // 缩放：水平方向显示倍率（>0，1.0 = 适配控件宽度）
    void setZoom(double factor);
    double zoom() const;

    // 回放游标（0 基道索引；-1 隐藏）
    void setCursorTrace(int traceIndex);
    int cursorTrace() const;

    // 疑似目标标记叠加（空列表清空；标记含外接框与 ID 标签）
    void setSuspects(const QList<GPR::GprSuspect> &suspects);

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // 数据/增益/对比度变化时重建伪彩 QImage（数据分辨率：宽=道数，高=样本数-t0）
    void rebuildImage();

    const GPR::GprBScanData *m_scan = nullptr;     // 非拥有
    const GPR::GprRadarHeader *m_header = nullptr; // 非拥有
    GainMode m_gainMode = GainMode::Agc;
    double m_contrast = 1.0;
    double m_zoom = 1.0;
    int m_cursorTrace = -1;
    QList<GPR::GprSuspect> m_suspects;
    QImage m_image;        // 伪彩缓存（宽=道数，高=样本数-t0）
    int m_t0Sample = 0;    // 当前通道 t0（图像第 0 行对应的原始样本索引）
};

#endif // MAINWINDOW_GPRBSCANWIDGET_H
