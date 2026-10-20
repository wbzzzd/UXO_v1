#ifndef GPR_GPRTRACK_H
#define GPR_GPRTRACK_H

// GprTrack：.cor RTK 轨迹解析（MALA 样例格式，REQ-013 / gpr-display.md §5.2）。
//
// .cor 格式事实（Python 预研已验证，见 docs/features/gpr-display.md §5.4）：
//   每行 14 个空格分隔字段，1 基编号：
//   #1  记录号（存在少量跳变，不作对齐依据）
//   #2  道号（1..N 连续，与 .rd3 道 1:1 对齐，是对齐唯一依据）
//   #3  纬度（度）
//   #4  经度（度）
//   #12 日期（yyyy-MM-dd）
//   #13 时间（hh:mm:ss.fff，亚秒小数；样例为 3 位毫秒，兼容更长数字串）
//   其余 8 个字段未解码，解析时忽略但容忍其存在。
//
// 时间戳：.cor 时间比文件名本地时间戳（本地为 UTC+8）早约 8 小时，判定为 UTC；
//   内部统一按 UTC（Qt::UTC）存储，本地显示由调用方转换（如 toLocalTime()）。

#include <QDateTime>
#include <QString>
#include <QVector>

namespace GPR {

// 单个轨迹点：某一道（trace）对应的 RTK 位置与 UTC 时间戳
struct GprTrackPoint {
    double latitude = 0.0;
    double longitude = 0.0;
    QDateTime timestampUtc;   // 内部统一 UTC 存储
    bool valid = false;
};

// .cor RTK 轨迹：道号 1:1 对齐、间隙线性插值、高斯平滑
class GprTrack {
public:
    // 默认 σ=500 道；构造时即构建高斯核
    GprTrack();

    // 载入 .cor 文件；失败（不可读/无有效行）时回到空态并返回 false。
    // 无法解析的行按缺失道处理（访问时经插值补齐），不导致整体失败。
    bool loadFromFile(const QString &path);    // .cor，每行 14 个空格分隔字段

    // 是否已成功载入至少一个有效轨迹点
    bool isValid() const;

    // 轨迹覆盖的道数（= 最大道号，与 .rd3 道数一致；样例数据 7297）
    int pointCount() const;

    // 0 基道索引 → 轨迹点。道号直接命中（1:1 映射，道号-1）；
    // 道号缺失（记录跳变/坏行）时在相邻有效点间线性插值（含时间戳）；
    // 越界或两侧均无有效点时返回 valid=false。
    GprTrackPoint pointForTrace(int traceIndex) const;

    // 高斯平滑后的位置（σ 由 setSmoothingSigmaTraces 设定）；时间戳取中心道值；
    // 越界或中心道无位置时 valid=false。
    GprTrackPoint smoothedPointForTrace(int traceIndex) const; // 应用高斯平滑后的位置

    // 设定高斯核 σ（单位：道）；默认 500（按 0.02 m/道 ≈ 10 m）；
    // 核半径 = ceil(3σ)；σ<=0 关闭平滑。
    // 注：σ=150 道（≈3 m）已被预研证明不足（横向抖动 std 1.43 m，平滑后路径
    // 仍比端点弦长 25%），故默认取更宽核；样条/卡尔曼与多趟标定为后续工作，
    // 见实现文件内注释。
    void setSmoothingSigmaTraces(double sigma); // 高斯核 σ（单位：道）；默认 500；σ<=0 关闭平滑

private:
    // 重建对称高斯半核（m_kernel[i] = |偏移|=i 的权重）
    void rebuildKernel();

    // 按道索引（道号-1）存储的轨迹点；缺失道保持默认 valid=false
    QVector<GprTrackPoint> m_points;
    bool m_loaded = false;          // 是否成功载入
    double m_sigmaTraces = 500.0;   // 高斯核 σ（单位：道），默认 500
    QVector<double> m_kernel;       // 高斯半核；空 = 平滑关闭
};

} // namespace GPR

#endif // GPR_GPRTRACK_H
