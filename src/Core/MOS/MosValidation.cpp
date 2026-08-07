// MOS P0 输入包络校验实现：完整跨字段边界检查，拒绝时保持输入不变。
// 所有校验函数以 const 引用接收输入，不修改调用方数据。

#include "Core/MOS/MosValidation.h"

#include <cmath>
#include <limits>
#include <QSet>

namespace Core::MOS {

namespace {

// 辅助：判断 double 值是否在闭区间 [lo, hi] 内
bool inRange(double v, double lo, double hi)
{
    return v >= lo && v <= hi;
}

// 辅助：判断 W/step 是否为整数（浮点容差 1e-9）
bool stepIntegral(double W, double step)
{
    double s = W / step;
    return std::fabs(s - std::floor(s)) < 1e-9;
}

} // namespace

// === 跑道参数校验 ===
// 校验顺序（测试断言要求）：
//   有限值 -> L -> step 范围 -> StepSCount(W/step<=200) -> W 范围 ->
//   StepIntegral(W/step 整数) -> minLength -> minWidth -> K -> expand ->
//   backfill -> uxoHours -> tiers
MosValidationResult validateRunwayParams(const MosRunwayParams &params)
{
    MosValidationResult result;

    // 1. 非有限值检查（NaN/Inf）
    if (!std::isfinite(params.L) || !std::isfinite(params.W) ||
        !std::isfinite(params.K) || !std::isfinite(params.expand) ||
        !std::isfinite(params.step) || !std::isfinite(params.minLength) ||
        !std::isfinite(params.minWidth) || !std::isfinite(params.backfill) ||
        !std::isfinite(params.uxoHours)) {
        result.valid = false;
        result.reason = MosValidationReason::InvalidFinite;
        result.message = QStringLiteral("存在 NaN 或 Inf 值");
        return result;
    }

    // 2. L: 100..6000
    if (!inRange(params.L, 100.0, 6000.0)) {
        result.valid = false;
        result.reason = MosValidationReason::L;
        result.message = QStringLiteral("L 越界（100..6000）");
        return result;
    }

    // 3. step 范围: 0.5..5
    if (!inRange(params.step, 0.5, 5.0)) {
        result.valid = false;
        result.reason = MosValidationReason::Step;
        result.message = QStringLiteral("step 越界（0.5..5）");
        return result;
    }

    // 4. StepSCount: S = W/step <= 200（须在 W 范围之前，测试用 W=100.5 验证 S=201）
    double sCount = params.W / params.step;
    if (sCount > 200.0) {
        result.valid = false;
        result.reason = MosValidationReason::StepSCount;
        result.message = QStringLiteral("S=W/step 超过 200");
        return result;
    }

    // 5. W: 15..100
    if (!inRange(params.W, 15.0, 100.0)) {
        result.valid = false;
        result.reason = MosValidationReason::W;
        result.message = QStringLiteral("W 越界（15..100）");
        return result;
    }

    // 6. StepIntegral: W/step 须为整数
    if (!stepIntegral(params.W, params.step)) {
        result.valid = false;
        result.reason = MosValidationReason::StepIntegral;
        result.message = QStringLiteral("W/step 非整数");
        return result;
    }

    // 7. minLength: 1..L
    if (!inRange(params.minLength, 1.0, params.L)) {
        result.valid = false;
        result.reason = MosValidationReason::MinLength;
        result.message = QStringLiteral("minLength 越界（1..L）");
        return result;
    }

    // 8. minWidth: 1..W
    if (!inRange(params.minWidth, 1.0, params.W)) {
        result.valid = false;
        result.reason = MosValidationReason::MinWidth;
        result.message = QStringLiteral("minWidth 越界（1..W）");
        return result;
    }

    // 9. K: 0.1..10
    if (!inRange(params.K, 0.1, 10.0)) {
        result.valid = false;
        result.reason = MosValidationReason::K;
        result.message = QStringLiteral("K 越界（0.1..10）");
        return result;
    }

    // 10. expand: 0.1..10
    if (!inRange(params.expand, 0.1, 10.0)) {
        result.valid = false;
        result.reason = MosValidationReason::Expand;
        result.message = QStringLiteral("expand 越界（0.1..10）");
        return result;
    }

    // 11. backfill: >0 且 <=10000
    if (!(params.backfill > 0.0) || params.backfill > 10000.0) {
        result.valid = false;
        result.reason = MosValidationReason::Backfill;
        result.message = QStringLiteral("backfill 越界（>0 且 <=10000）");
        return result;
    }

    // 12. uxoHours: 0..1000
    if (!inRange(params.uxoHours, 0.0, 1000.0)) {
        result.valid = false;
        result.reason = MosValidationReason::UxoHours;
        result.message = QStringLiteral("uxoHours 越界（0..1000）");
        return result;
    }

    // 13. tiers: 2..5
    if (params.tiers < 2 || params.tiers > 5) {
        result.valid = false;
        result.reason = MosValidationReason::Tiers;
        result.message = QStringLiteral("tiers 越界（2..5）");
        return result;
    }

    return result;
}

// === 生成器参数校验 ===
MosValidationResult validateGeneratorParams(const MosGeneratorParams &params)
{
    MosValidationResult result;

    // craterCount: 1..8
    if (params.craterCount < 1 || params.craterCount > 8) {
        result.valid = false;
        result.reason = MosValidationReason::CraterCount;
        result.message = QStringLiteral("craterCount 越界（1..8）");
        return result;
    }

    // craterRMin/craterRMax: 有限 0.1..100 且 min<=max
    if (!std::isfinite(params.craterRMin) || !std::isfinite(params.craterRMax) ||
        !inRange(params.craterRMin, 0.1, 100.0) ||
        !inRange(params.craterRMax, 0.1, 100.0) ||
        params.craterRMin > params.craterRMax) {
        result.valid = false;
        result.reason = MosValidationReason::CraterRadius;
        result.message = QStringLiteral("弹坑半径范围越界或 min>max（0.1..100）");
        return result;
    }

    // uxoCount: 0..5
    if (params.uxoCount < 0 || params.uxoCount > 5) {
        result.valid = false;
        result.reason = MosValidationReason::UxoCount;
        result.message = QStringLiteral("uxoCount 越界（0..5）");
        return result;
    }

    // uxoYMin/uxoYMax: 0.1..1000 且有限且 min<=max
    if (!std::isfinite(params.uxoYMin) || !std::isfinite(params.uxoYMax) ||
        !inRange(params.uxoYMin, 0.1, 1000.0) ||
        !inRange(params.uxoYMax, 0.1, 1000.0) ||
        params.uxoYMin > params.uxoYMax) {
        result.valid = false;
        result.reason = MosValidationReason::UxoYield;
        result.message = QStringLiteral("UXO 当量范围越界或 min>max（0.1..1000）");
        return result;
    }

    return result;
}

// === 障碍物集合校验 ===
MosValidationResult validateObstacleSet(const MosObstacleSet &obstacles,
                                        const MosRunwayParams &params)
{
    (void)params; // 当前校验不依赖跑道参数，保留接口供未来跨字段校验
    MosValidationResult result;

    // 总障碍物 N <= 13
    const int total = obstacles.craters.size() + obstacles.uxo.size();
    if (total > 13) {
        result.valid = false;
        result.reason = MosValidationReason::TotalObstacles;
        result.message = QStringLiteral("总障碍物数量超过 13");
        return result;
    }

    // 弹坑字段校验
    for (const auto &c : obstacles.craters) {
        // visibleRadius: 0.1..100
        if (!std::isfinite(c.visibleRadius) ||
            !inRange(c.visibleRadius, 0.1, 100.0)) {
            result.valid = false;
            result.reason = MosValidationReason::CraterRadius;
            result.message = QStringLiteral("弹坑 visibleRadius 越界（0.1..100）");
            return result;
        }
        // influenceRadius: 有限 0.1..6000
        if (!std::isfinite(c.influenceRadius) ||
            !inRange(c.influenceRadius, 0.1, 6000.0)) {
            result.valid = false;
            result.reason = MosValidationReason::InfluenceRadius;
            result.message = QStringLiteral("弹坑 influenceRadius 越界或非有限（0.1..6000）");
            return result;
        }
    }

    // UXO 字段校验
    for (const auto &u : obstacles.uxo) {
        // syntheticYield: 0.1..1000
        if (!std::isfinite(u.syntheticYield) ||
            !inRange(u.syntheticYield, 0.1, 1000.0)) {
            result.valid = false;
            result.reason = MosValidationReason::UxoYield;
            result.message = QStringLiteral("UXO syntheticYield 越界（0.1..1000）");
            return result;
        }
        // influenceRadius: 有限 0.1..6000
        if (!std::isfinite(u.influenceRadius) ||
            !inRange(u.influenceRadius, 0.1, 6000.0)) {
            result.valid = false;
            result.reason = MosValidationReason::InfluenceRadius;
            result.message = QStringLiteral("UXO influenceRadius 越界或非有限（0.1..6000）");
            return result;
        }
    }

    // ID 非空且唯一
    QSet<QString> seenIds;
    for (const auto &c : obstacles.craters) {
        if (c.id.isEmpty()) {
            result.valid = false;
            result.reason = MosValidationReason::ObstacleId;
            result.message = QStringLiteral("弹坑 ID 为空");
            return result;
        }
        if (seenIds.contains(c.id)) {
            result.valid = false;
            result.reason = MosValidationReason::ObstacleId;
            result.message = QStringLiteral("弹坑 ID 重复: %1").arg(c.id);
            return result;
        }
        seenIds.insert(c.id);
    }
    for (const auto &u : obstacles.uxo) {
        if (u.id.isEmpty()) {
            result.valid = false;
            result.reason = MosValidationReason::ObstacleId;
            result.message = QStringLiteral("UXO ID 为空");
            return result;
        }
        if (seenIds.contains(u.id)) {
            result.valid = false;
            result.reason = MosValidationReason::ObstacleId;
            result.message = QStringLiteral("UXO ID 重复: %1").arg(u.id);
            return result;
        }
        seenIds.insert(u.id);
    }

    return result;
}

// === 种子校验（signed int32 范围）===
MosValidationResult validateSeed(qint64 seed)
{
    MosValidationResult result;
    if (seed < static_cast<qint64>(std::numeric_limits<qint32>::min()) ||
        seed > static_cast<qint64>(std::numeric_limits<qint32>::max())) {
        result.valid = false;
        result.reason = MosValidationReason::Seed;
        result.message = QStringLiteral("种子超出 signed int32 范围");
    }
    return result;
}

} // namespace Core::MOS
