#ifndef CORE_MOS_MOSPLANNER_H
#define CORE_MOS_MOSPLANNER_H

// MOS P0 合成规划器：Y 边界离散 + X 连续扫描的最大空矩形与递进档位规划。
// 所有几何与估算均为合成本地 fixture 语义，非真实跑道、真实弹坑或真实作业参数。
// Core 仅依赖 Qt 值类型，不依赖 UI/3D/网络/数据库。

#include "Core/MOS/MosTypes.h"
#include "Core/MOS/MosEstimator.h"

#include <QString>
#include <QVector>

namespace Core::MOS {

// 规划结果原因（稳定错误码，供 UI/测试断言）
// - Accepted：接受（合法有解或合法无解）
// - NoFeasibleRectangle：合法输入但无可行矩形（对应 NO_FEASIBLE_RECTANGLE）
// - 其余：复合方案拒绝原因，此时 accepted=false 且 tiers 为空
enum class MosPlannerReason {
    Accepted,               // 接受
    NoFeasibleRectangle,    // 合法无解（rectangle-level，对应 NO_FEASIBLE_RECTANGLE）
    InvalidParams,          // 跑道参数非法
    InvalidObstacles,       // 障碍物集合非法
    InvalidTierCount,       // 档位数与 params.tiers 不符
    EmptyInitialTier,       // tier 0 应为空（不修复任何点）但非空
    IncompleteFinalTier,    // 末档应包含全部障碍物 ID 但不完整
    UnknownRepairedId,      // repairedIds 含未知障碍物 ID
    DuplicateRepairedId,    // 同档内 repairedIds 重复
    NonNestedTiers,         // 修复集合非嵌套（tier i+1 未包含 tier i 全部 ID）
    MonotonicityViolation,  // 面积单调非减不变量被破坏
    CompletionMismatch      // 控制器重算结果与 worker 完成结果不一致（防陈旧/篡改）
};

// 单次最大空矩形结果（合成几何，非真实起降区域）
// - valid=false 且 reason=NoFeasibleRectangle 表示合法无解，area=0
// - valid=true 时所有坐标/尺寸均为有限 double
struct MosRectangleResult {
    bool valid{false};                                              // 是否找到可行矩形
    MosPlannerReason reason{MosPlannerReason::NoFeasibleRectangle}; // 原因
    double xStart{0.0};   // 矩形 X 起点 (m)，有限 double
    double xEnd{0.0};     // 矩形 X 终点 (m)，有限 double
    double yStart{0.0};   // 矩形 Y 起点 (m)，有限 double
    double yEnd{0.0};     // 矩形 Y 终点 (m)，有限 double
    double length{0.0};   // 长度 (m) = xEnd - xStart
    double width{0.0};    // 宽度 (m) = yEnd - yStart
    double area{0.0};     // 面积 (m²) = length × width，无解时为 0
};

// 档位 ID 分配方案（仅已修复 ID 列表，不含几何/估算）
// 由 buildStableRepairTiers 按 floor(tierIndex * N / (T-1)) 稳定生成；tier 0 为空，末档修复全部
struct MosTierPlan {
    QVector<QString> repairedIds;   // 该档已修复障碍物 ID（稳定顺序：craters 后 uxo）
};

// 完整档位结果（已修复 ID + 最大空矩形 + 合成估算）
struct MosRepairTier {
    QVector<QString> repairedIds;              // 该档已修复障碍物 ID
    MosRectangleResult rectangle;              // 该档最大空矩形（合成几何）
    MosEstimate estimate;                      // 该档合成修复估算
};

// 递进规划复合结果
// - accepted=true：合法方案（可含合法无解档位），tiers 含 params.tiers 个元素
// - accepted=false：拒绝，tiers 为空，reason/message 指明拒绝原因
struct MosProgressiveResult {
    bool accepted{false};                                   // 是否接受整个复合方案
    MosPlannerReason reason{MosPlannerReason::Accepted};    // 接受/拒绝原因（默认拒绝态，实现须显式设置）
    QString message;                                        // 人类可读说明
    QVector<MosRepairTier> tiers;                           // 各档完整结果（拒绝时为空）
};

// 合成规划器（纯本地计算，确定可复现）
class MosPlanner
{
public:
    // 单次最大空矩形：在给定已修复 ID 集合下，于 [0,L]×[-W/2,W/2] 内
    // 按 Y 边界离散 + X 连续扫描寻找满足 minLength/minWidth 的最大面积矩形。
    // 相切视为碰撞；开放自由端点用 std::nextafter 向内规范化为可表示的有限 double。
    // 合法无解返回 valid=false 且 reason=NoFeasibleRectangle。
    static MosRectangleResult planSingle(const MosObstacleSet &obstacles,
                                         const MosRunwayParams &params,
                                         const QVector<QString> &repairedIds = {});

    // 构造稳定嵌套修复档位（仅 ID 分配）：按 floor(tierIndex * N / (T-1))
    // 确定每档已修复点数量，tier 0 为空、末档修复全部、中间档逐级嵌套。
    // 返回 params.tiers 个 MosTierPlan；稳定障碍物顺序为 craters 后 uxo。
    static QVector<MosTierPlan> buildStableRepairTiers(const MosObstacleSet &obstacles,
                                                       const MosRunwayParams &params);

    // 递进规划：校验参数/障碍物/档位嵌套与面积单调性，逐档调用 planSingle 并附加 MosEstimate。
    // 非法输入或不变量破坏时 accepted=false 且 tiers 为空；合法无解档位以
    // rectangle.valid=false 表示，仍计入 accepted=true 的复合方案（无解面积按 0 参与单调性校验）。
    static MosProgressiveResult planProgressive(const MosObstacleSet &obstacles,
                                                 const MosRunwayParams &params);

    // 递进规划（supplied-tier 重载）：调用方显式提供档位 ID 方案，按确定性顺序校验
    // params -> obstacles -> tier count -> empty initial -> unknown -> duplicate ->
    // complete final -> nesting -> monotonicity。任一校验失败：accepted=false、tiers 为空、
    // reason 指明失败原因。控制器在 commit 前用此重算结果与 worker 完成结果逐位比对。
    static MosProgressiveResult planProgressive(const MosObstacleSet &obstacles,
                                                 const MosRunwayParams &params,
                                                 const QVector<MosTierPlan> &suppliedTierPlans);
};

} // namespace Core::MOS

#endif // CORE_MOS_MOSPLANNER_H
