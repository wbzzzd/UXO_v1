#ifndef GPR_GPRRADARHEADER_H
#define GPR_GPRRADARHEADER_H

// GPR .rad 头文件解析（MALA 格式，REQ-012）：键值对文本，键可含空格（如 "DISTANCE INTERVAL"）。
// 实测分隔符为 ':'（同时容忍 '='），值可含空格（如 "NOT VALID FIELD"）。
// 纯数据值类型：仅依赖 Qt5::Core，不含 Widgets 代码；样例数据为演示/模拟用途。
// 设计基线：docs/features/gpr-display.md §5.2；格式事实见 §5.4。

#include <QString>
#include <QVariantMap>

namespace GPR {

class GprRadarHeader
{
public:
    // 解析 .rad 键值对文本；返回文件是否成功打开并解析，内容有效性用 isValid() 查询。
    // 解析容忍：跳过空行、注释行（# 或 ; 开头）与无分隔符行；键值均去空白；
    // 未知键仅保留在 rawPairs()；重复键后值覆盖前值（实测文件存在重复键）。
    bool loadFromFile(const QString &path);

    // 三个必需键（SAMPLES / TIMEWINDOW / DISTANCE INTERVAL）齐全且数值合法时为 true。
    bool isValid() const;

    // SAMPLES：每道采样数（样例数据 = 512）
    int samples() const;

    // TIMEWINDOW：时窗 (ns)（样例数据 = 110）
    double timeWindowNs() const;

    // DISTANCE INTERVAL：道间距 (m)（样例数据 = 0.02）
    double distanceIntervalM() const;

    // ANTENNAS 原文（已知与实测主频 216–222 MHz 矛盾，仅原样保留，不作频率依据）
    QString antennaDescription() const;

    // 采样间隔 (ns) = timeWindowNs / samples；无效头（samples<=0）返回 0，避免除零
    double dtNs() const;

    // 两程时间换算深度 (m)：depth = velocity * sample * dtNs / 2；
    // 默认 v=0.1 m/ns 时样例全时窗（512 样本）深度约 5.5 m（§5.4 预研结论）
    double depthForSample(int sample, double velocityMPerNs = 0.1) const;

    // 全部原始键值对（值为去空白后的原文；未知键仅保留于此）
    QVariantMap rawPairs() const;

private:
    bool m_valid{false};              // 必需键齐全且数值合法
    int m_samples{0};                 // SAMPLES
    double m_timeWindowNs{0.0};       // TIMEWINDOW (ns)
    double m_distanceIntervalM{0.0};  // DISTANCE INTERVAL (m)
    QString m_antennaDescription;     // ANTENNAS 原文
    QVariantMap m_rawPairs;           // 全部原始键值对
};

} // namespace GPR

#endif // GPR_GPRRADARHEADER_H
