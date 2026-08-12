#ifndef CORE_MOS_MOSVALIDATION_H
#define CORE_MOS_MOSVALIDATION_H

// MOS P0 输入包络校验：完整跨字段边界检查，拒绝时保持输入不变。

#include "Core/MOS/MosTypes.h"

#include <QString>
#include <QtGlobal>

namespace Core::MOS {

// 校验拒绝原因（稳定错误码，供 UI/测试断言）
enum class MosValidationReason {
    Valid,             // 通过校验
    InvalidFinite,     // 存在 NaN/Inf
    L,                 // 跑道长度越界
    W,                 // 跑道宽度越界
    MinLength,         // 最小长度越界或大于 L
    MinWidth,          // 最小宽度越界或大于 W
    Step,              // 步长越界
    StepIntegral,      // W/step 非整数
    StepSCount,        // S = W/step > 200
    K,                 // K 越界
    Expand,            // expand 越界
    Backfill,          // 回填速率越界
    UxoHours,          // UXO 工时越界
    Tiers,             // 档位数越界
    CraterCount,       // 弹坑数量越界
    CraterRadius,      // 弹坑可见半径越界
    UxoCount,          // UXO 数量越界
    UxoYield,          // UXO 当量越界
    InfluenceRadius,   // 影响半径越界或非有限
    TotalObstacles,    // 总障碍物 N > 13
    ObstacleId,        // 障碍物 ID 空或重复
    ObstacleCoordinate,// 障碍物坐标越界（x∉[0,L] 或 y∉[-40,40]）
    Seed               // 种子超出 signed int32 范围
};

// 校验结果
struct MosValidationResult {
    bool valid{true};
    MosValidationReason reason{MosValidationReason::Valid};
    QString message;
};

// 校验跑道参数（L, W, K, expand, step, minLength, minWidth, backfill, uxoHours, tiers）
MosValidationResult validateRunwayParams(const MosRunwayParams &params);

// 校验生成器参数（craterCount, craterRMin, craterRMax, uxoCount, uxoYMin, uxoYMax）
MosValidationResult validateGeneratorParams(const MosGeneratorParams &params);

// 校验障碍物集合（visibleRadius, syntheticYield, influenceRadius, N<=13, 唯一非空 ID, 坐标）
MosValidationResult validateObstacleSet(const MosObstacleSet &obstacles, const MosRunwayParams &params);

// 校验种子（须在 signed int32 范围内）
MosValidationResult validateSeed(qint64 seed);

} // namespace Core::MOS

#endif // CORE_MOS_MOSVALIDATION_H
