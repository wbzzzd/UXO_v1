// MOS P0 合成修复估算器实现：球体体积 + 合成回填时间 + 固定 UXO 工时 + 序数难度。
// 不使用真实工程公式、真实材料参数或真实安全阈值。

#include "Core/MOS/MosEstimator.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Core::MOS {

// 球体体积 = 4/3 · π · r³
double MosEstimator::sphereVolume(double radius)
{
    return 4.0 / 3.0 * M_PI * radius * radius * radius;
}

// === 合成修复估算 ===
MosEstimate MosEstimator::estimate(const MosObstacleSet &obstacles,
                                   const MosRunwayParams &params,
                                   int tierIndex,
                                   int tierCount)
{
    MosEstimate est;

    // 回填体积：所有弹坑 visibleRadius 的球体体积之和
    double totalVolume = 0.0;
    for (const auto &crater : obstacles.craters) {
        totalVolume += sphereVolume(crater.visibleRadius);
    }
    est.totalBackfillVolume = totalVolume;

    // 回填工时：体积 / 回填速率
    est.backfillHours = totalVolume / params.backfill;

    // UXO 工时：UXO 数量 × 固定工时
    est.uxoHours = obstacles.uxo.size() * params.uxoHours;

    // 总工时
    est.totalHours = est.backfillHours + est.uxoHours;

    // 序数难度标签
    est.difficulty = difficultyForTier(tierIndex, tierCount);

    return est;
}

} // namespace Core::MOS
