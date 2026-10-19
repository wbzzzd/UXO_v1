#ifndef GPR_GPRBSCANDATA_H
#define GPR_GPRBSCANDATA_H

// GPR .rd3 B-Scan 数据读取（MALA 格式，REQ-012）：int16 小端、按道顺序存储。
// 样例数据 7297 道 × 512 样本（约 7.5 MB/文件）；加载工作者线程经 preloadAllTraces()
// 单次顺序 bulk read 把整通道驻留内存，消除 GUI/检测路径上的逐道磁盘小读
// （/mnt/d 9p 小读放大是雷达页交互冻结根因）；未预载时保留按道懒加载 + LRU 缓存
// 回退（容量 8）。firstBreakSample() 提供逐通道 t0 初至估计（§5.4：各通道
// first-break 实测 46–59，逐通道 t0 校正必需）。
// 纯数据值类型：仅依赖 Qt5::Core，不含 Widgets 代码；样例数据为演示/模拟用途。
// 设计基线：docs/features/gpr-display.md §5.2；格式事实见 §5.4。

#include <QtGlobal>
#include <QList>
#include <QString>
#include <QVector>

namespace GPR {

class GprBScanData
{
public:
    // 载入 .rd3 数据文件：expectedSamples 应取自配套 .rad 的 SAMPLES；
    // 校验文件存在、非空且大小恰为 expectedSamples*2 的整数倍；
    // 任一校验失败回到无效态并返回 false，重复加载语义一致。
    bool loadFromFile(const QString &path, int expectedSamples);

    // 文件校验通过（存在、大小整除、道数可表示）时为 true
    bool isValid() const;

    // 道数（样例数据 = 7297）
    int traceCount() const;

    // 每道采样数 = 加载时的 expectedSamples（样例数据 = 512）
    int sampleCount() const;

    // 单样本访问（0 基）；越界或无效时返回 0
    qint16 sampleAt(int traceIndex, int sampleIndex) const;

    // 整道拷贝（0 基）；越界或无效时返回空向量
    QVector<qint16> trace(int traceIndex) const;

    // 逐通道 t0 初至样本索引：逐道估计后取中位数（偶数取下中位），结果缓存；
    // 无效数据返回 -1。估计口径：噪声段 |mean|+6σ 主判据，
    // 无命中时回退为分析窗内 |一阶差分| 最大值位置
    int firstBreakSample() const;

    // 全通道预载：单次顺序 bulk read 把全部道数据读入内存（供加载工作者线程调用）；
    // 幂等；无效数据/文件打开失败/短读时返回 false 并保留逐道懒加载回退
    bool preloadAllTraces();

    // 全部道数据已驻留内存（preloadAllTraces 成功）时为 true
    bool isFullyLoaded() const;

private:
    // 按道读取（预载后全部命中内存；否则 LRU 缓存，未命中时从文件定位读取整道）
    QVector<qint16> fetchTrace(int traceIndex) const;

    bool m_valid{false};            // 文件校验通过
    int m_traceCount{0};            // 道数
    int m_sampleCount{0};           // 每道采样数
    QString m_path;                 // 数据文件绝对路径
    QVector<QVector<qint16>> m_traces;  // 全通道预载缓冲（隐式共享，按值拷贝零成本）
    bool m_fullyLoaded{false};          // m_traces 完整有效
    mutable QList<int> m_cacheKeys;                 // LRU 键（道索引），队首为最近使用
    mutable QList<QVector<qint16>> m_cacheValues;   // 与键一一对应的道数据
    mutable int m_firstBreak{-1};   // 初至估计缓存；-1 = 未计算
};

} // namespace GPR

#endif // GPR_GPRBSCANDATA_H
