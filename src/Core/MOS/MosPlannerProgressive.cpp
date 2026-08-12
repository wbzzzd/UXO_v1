// MOS P0 递进规划实现：默认重载与 supplied-tier 重载。
// 从 MosPlanner.cpp 拆出，保证两个源文件各自 <=250 纯 LOC。
// 所有几何与估算均为合成本地 fixture 语义，非真实跑道、真实弹坑或真实作业参数。
// Core 仅依赖 Qt 值类型，不依赖 UI/3D/网络/数据库。

#include "Core/MOS/MosPlanner.h"
#include "Core/MOS/MosValidation.h"

#include <QSet>
#include <QString>
#include <QVector>

namespace Core::MOS {

// === 递进规划（默认重载）===
// 委托 buildStableRepairTiers 生成稳定档位，再转交 supplied-tier 重载做完整校验与逐档计算。
MosProgressiveResult MosPlanner::planProgressive(const MosObstacleSet &obstacles,
                                                 const MosRunwayParams &params)
{
    const auto tierPlans = buildStableRepairTiers(obstacles, params);
    return planProgressive(obstacles, params, tierPlans);
}

// === 递进规划（supplied-tier 重载）===
// 确定性校验顺序：params -> obstacles -> tier count -> empty initial ->
// unknown -> duplicate -> complete final -> nesting -> monotonicity。
// 任一失败：accepted=false、tiers 为空、reason 指明失败原因。
// 控制器在 commit 前用此重算结果与 worker 完成结果逐位比对。
MosProgressiveResult MosPlanner::planProgressive(const MosObstacleSet &obstacles,
                                                 const MosRunwayParams &params,
                                                 const QVector<MosTierPlan> &suppliedTierPlans)
{
    MosProgressiveResult result;
    result.accepted = false;

    // 校验跑道参数
    const auto vr = validateRunwayParams(params);
    if (!vr.valid) {
        result.reason = MosPlannerReason::InvalidParams;
        result.message = vr.message;
        return result;
    }
    // 校验障碍物集合
    const auto vo = validateObstacleSet(obstacles, params);
    if (!vo.valid) {
        result.reason = MosPlannerReason::InvalidObstacles;
        result.message = vo.message;
        return result;
    }
    // 校验档位数
    if (suppliedTierPlans.size() != params.tiers) {
        result.reason = MosPlannerReason::InvalidTierCount;
        result.message = QStringLiteral("档位数与 params.tiers 不符");
        return result;
    }

    // 已知 ID 集合
    QSet<QString> knownIds;
    for (const auto &c : obstacles.craters) knownIds.insert(c.id);
    for (const auto &u : obstacles.uxo) knownIds.insert(u.id);

    // 首档须为空
    if (!suppliedTierPlans[0].repairedIds.isEmpty()) {
        result.reason = MosPlannerReason::EmptyInitialTier;
        result.message = QStringLiteral("档位 0 应为空但非空");
        return result;
    }

    // 逐档校验 ID 已知且唯一
    for (int t = 0; t < suppliedTierPlans.size(); ++t) {
        QSet<QString> seen;
        for (const auto &id : suppliedTierPlans[t].repairedIds) {
            if (!knownIds.contains(id)) {
                result.reason = MosPlannerReason::UnknownRepairedId;
                result.message = QStringLiteral("档位 %1 含未知修复 ID: %2").arg(t).arg(id);
                return result;
            }
            if (seen.contains(id)) {
                result.reason = MosPlannerReason::DuplicateRepairedId;
                result.message = QStringLiteral("档位 %1 含重复修复 ID: %2").arg(t).arg(id);
                return result;
            }
            seen.insert(id);
        }
    }

    // 末档须包含全部已知 ID
    const auto &finalIds = suppliedTierPlans.last().repairedIds;
    const QSet<QString> finalSet(finalIds.begin(), finalIds.end());
    if (finalSet.size() != knownIds.size()) {
        result.reason = MosPlannerReason::IncompleteFinalTier;
        result.message = QStringLiteral("末档未包含全部障碍物 ID");
        return result;
    }

    // 嵌套校验：tier t 须包含 tier t-1 的全部 ID
    for (int t = 1; t < suppliedTierPlans.size(); ++t) {
        const QSet<QString> curSet(suppliedTierPlans[t].repairedIds.begin(),
                                   suppliedTierPlans[t].repairedIds.end());
        for (const auto &id : suppliedTierPlans[t - 1].repairedIds) {
            if (!curSet.contains(id)) {
                result.reason = MosPlannerReason::NonNestedTiers;
                result.message = QStringLiteral("档位 %1 未包含档位 %2 的全部 ID").arg(t).arg(t - 1);
                return result;
            }
        }
    }

    // 逐档计算矩形与估算
    QVector<MosRepairTier> tiers;
    tiers.reserve(suppliedTierPlans.size());
    for (int t = 0; t < suppliedTierPlans.size(); ++t) {
        const auto &plan = suppliedTierPlans[t];

        MosRepairTier tier;
        tier.repairedIds = plan.repairedIds;
        tier.rectangle = planSingle(obstacles, params, plan.repairedIds);

        // 构造累计已修复障碍物子集用于估算（craters 后 uxo，保持稳定顺序）
        QSet<QString> repairedSet;
        for (const auto &id : plan.repairedIds) repairedSet.insert(id);
        MosObstacleSet repairedSubset;
        for (const auto &c : obstacles.craters)
            if (repairedSet.contains(c.id))
                repairedSubset.craters.append(c);
        for (const auto &u : obstacles.uxo)
            if (repairedSet.contains(u.id))
                repairedSubset.uxo.append(u);

        tier.estimate = MosEstimator::estimate(repairedSubset, params, t, params.tiers);
        tiers.append(tier);
    }

    // 面积单调非减校验（合法无解档位面积按 0；违反则整个复合方案拒绝）
    for (int t = 1; t < tiers.size(); ++t) {
        if (tiers[t].rectangle.area < tiers[t - 1].rectangle.area) {
            result.reason = MosPlannerReason::MonotonicityViolation;
            result.message = QStringLiteral("档位 %1 面积小于档位 %2").arg(t).arg(t - 1);
            return result;
        }
    }

    result.accepted = true;
    result.reason = MosPlannerReason::Accepted;
    result.message = QStringLiteral("已接受");
    result.tiers = std::move(tiers);
    return result;
}

} // namespace Core::MOS
