#ifndef GPR_GPRTHRESHOLDDETECTOR_H
#define GPR_GPRTHRESHOLDDETECTOR_H

// GPR 阈值候选检测（REQ-013）：gpr-display.md §5.4 Python 预研验证管线的 C++ 移植。
// 管线：逐道去直流 + 初至截断 → 滑窗背景去除（±50 道）→ 滑动 RMS 包络（16 样本）
// → 深度白化增益（≤10×）→ 时窗分位阈值 → 形态学膨胀（±15 道 / ±9 样本）
// → 8-邻域连通成分 → 面积过滤 → 外接框中心候选。
// 定位约束：检测仅作候选标记，必须人工复核后才可注入地图（§5.2/§5.4）。
// 纯数据值类型：仅依赖 Qt5::Core，不含 Widgets 代码；样例数据为演示/模拟用途。

#include <QList>

namespace GPR {

class GprBScanData;
class GprRadarHeader;

// 单个检测候选：连通成分外接框中心 + 元数据
struct GprSuspect {
    int id = 0;              // 1 基序号，按 (traceIndex, sampleIndex) 字典序赋
    int traceIndex = -1;     // 外接框中心道（0 基）
    int sampleIndex = -1;    // 外接框中心样本（原始样本坐标，含初至偏移）
    double depthM = 0.0;     // sampleIndex 经 GprRadarHeader::depthForSample 换算（默认 v=0.1）
    double amplitude = 0.0;  // 成分内原始掩膜像素的最大白化包络幅值
    int traceSpan = 0;       // 外接框道向跨度（道）
    int sampleSpan = 0;      // 外接框时间向跨度（样本）
};

class GprThresholdDetector
{
public:
    // 参数默认值：quantile 0.995、maxTimeNs 91.3、minComponentPx 15（§5.4 预研定标）
    GprThresholdDetector() = default;

    // 执行检测管线；bscan/header 任一无效或尺寸退化时返回空列表。
    // 返回候选按 (traceIndex, sampleIndex) 字典序排列，id 从 1 连续编号。
    QList<GprSuspect> detect(const GprBScanData &bscan, const GprRadarHeader &header) const;

    // 分位阈值参数，限制在 [0.5, 1.0]；q=1.0 时阈值=时窗内最大值 → 必无候选
    void setQuantile(double q);

    // 检测时窗（ns，原始样本坐标 dt*sampleIndex 上界；
    // 形态学膨胀可能使候选中心外溢不超过膨胀半径对应的时长）
    void setMaxTimeNs(double maxTimeNs);

    // 连通成分内原始掩膜像素数下限（小于即丢弃），最小 1
    void setMinComponentPx(int px);

private:
    double m_quantile{0.995};   // 分位阈值（§5.4 预研定标 0.995）
    double m_maxTimeNs{91.3};   // 检测时窗（ns；样例时窗 110 ns 的浅部区间）
    int m_minComponentPx{15};   // 成分掩膜像素数下限
};

} // namespace GPR

#endif // GPR_GPRTHRESHOLDDETECTOR_H
