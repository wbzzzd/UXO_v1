#ifndef CORE_MOS_MOSFIXTUREGENERATOR_H
#define CORE_MOS_MOSFIXTUREGENERATOR_H

// MOS P0 确定性合成 fixture 生成器：mulberry32 端口 + 冻结抽取顺序与公式。
// 不使用 QRandomGenerator / std::uniform_*_distribution / 系统时钟 / thread-local 随机。

#include "Core/MOS/MosTypes.h"

#include <QVector>
#include <QtGlobal>

namespace Core::MOS {

// 合成 fixture 生成器（确定性 mulberry32，非真实随机源）
class MosFixtureGenerator
{
public:
    // 生成合成障碍物集合（seed 须为 signed int32，归一化为 uint32 后驱动 mulberry32）
    // 冻结抽取顺序：
    //   弹坑 i=0: visibleRadius -> x -> y (threat 固定 high，不抽取 rng)
    //   弹坑 i>0: visibleRadius -> x -> y -> threat
    //   UXO(全部): syntheticYield -> x -> y -> threat
    // 冻结公式：
    //   crater.visibleRadius = jsRound(crMin + u*(crMax-crMin))
    //   crater.x = jsRound(u * L)
    //   crater.y = jsRound(-40 + u*80)
    //   crater.influenceRadius = visibleRadius * expand (float, 不取整)
    //   uxo.syntheticYield = uyMin + u*(uyMax-uyMin) (float, 不取整)
    //   uxo.x = jsRound(u * L)
    //   uxo.y = jsRound(-40 + u*80)
    //   uxo.influenceRadius = K * cbrt(syntheticYield) (float, 不取整)
    //   crater i>0 threat: u > 0.5 ? medium : high
    //   uxo threat: u > 0.5 ? high : medium
    //   jsRound(x) = floor(x + 0.5)（JS Math.round 半数向正无穷语义）
    static MosObstacleSet generate(const MosRunwayParams &params,
                                   const MosGeneratorParams &genParams,
                                   qint32 seed);

    // 嵌套 fixture 顺序：floor(tierIndex * N / (T-1))
    // T=1 时返回 [0]；T<1 时返回空
    static QVector<int> nestedFixtureOrder(int tiers, int totalObstacleCount);
};

} // namespace Core::MOS

#endif // CORE_MOS_MOSFIXTUREGENERATOR_H
