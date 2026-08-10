#ifndef CORE_MOS_MOSESTIMATOR_H
#define CORE_MOS_MOSESTIMATOR_H

// MOS P0 合成修复估算器：球体体积 + 合成回填时间 + 固定 UXO 工时 + 序数难度。
// 不使用真实工程公式、真实材料参数或真实安全阈值。

#include "Core/MOS/MosTypes.h"

namespace Core::MOS {

// 合成修复估算结果（模拟数据，非真实工程估算）
struct MosEstimate {
    double totalBackfillVolume{0.0}; // 合成回填体积 (m³) = Σ(4/3·π·visibleRadius³)
    double backfillHours{0.0};       // 合成回填工时 (h) = volume / backfill
    double uxoHours{0.0};            // 合成 UXO 工时 (h) = uxoCount × params.uxoHours
    double totalHours{0.0};          // 合成总工时 (h) = backfillHours + uxoHours
    MosDifficulty difficulty{MosDifficulty::None}; // 序数难度标签
};

// 合成修复估算器
class MosEstimator
{
public:
    // 估算给定障碍物集合和档位的合成修复工作量
    // - 回填体积：所有弹坑 visibleRadius 的球体体积之和
    // - 回填工时：体积 / params.backfill
    // - UXO 工时：uxo.size() * params.uxoHours
    // - 难度：按 tierIndex/tierCount 计算序数标签
    static MosEstimate estimate(const MosObstacleSet &obstacles,
                                const MosRunwayParams &params,
                                int tierIndex,
                                int tierCount);

    // 球体体积 = 4/3 · π · r³
    static double sphereVolume(double radius);
};

} // namespace Core::MOS

#endif // CORE_MOS_MOSESTIMATOR_H
