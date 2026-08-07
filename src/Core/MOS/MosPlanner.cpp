// MOS P0 合成规划器实现：Y 边界离散 + X 连续扫描的最大空矩形与递进档位规划。
// 所有几何与估算均为合成本地 fixture 语义，非真实跑道、真实弹坑或真实作业参数。
// Core 仅依赖 Qt 值类型，不依赖 UI/3D/网络/数据库。

#include "Core/MOS/MosPlanner.h"
#include "Core/MOS/MosValidation.h"

#include <QSet>
#include <QString>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <vector>

namespace Core::MOS {

namespace {

// 闭集圆盘：圆心 (cx, cy) 与影响半径 r，用于 X 投影
struct Disk {
    double cx;
    double cy;
    double r;
};

// 闭 X 区间：用于障碍物投影裁剪与合并
struct Interval {
    double low;
    double high;
};

// 五级总排序：area 降序 > yStart 升序 > xStart 升序 > length 降序 > width 降序。
// 与独立归约域 oracle 逐位一致，使用精确 double 比较（无 epsilon）。
bool betterRectangle(const MosRectangleResult &c, const MosRectangleResult &b)
{
    if (c.area != b.area) return c.area > b.area;
    if (c.yStart != b.yStart) return c.yStart < b.yStart;
    if (c.xStart != b.xStart) return c.xStart < b.xStart;
    if (c.length != b.length) return c.length > b.length;
    return c.width > b.width;
}

// 评估一个自由 X 空隙并按总排序更新最优。
// 跑道边界端点保持精确；障碍侧端点用 std::nextafter 向内规范化为可表示的有限 double。
// 参数对齐独立 oracle 的 consider 闭包，保证逐位一致。
void considerGap(MosRectangleResult &best,
                 double xRawLo, double xRawHi,
                 bool loAtRunway, bool hiAtRunway,
                 double yLow, double yHigh,
                 double width, double minLength)
{
    const double xLo = loAtRunway ? xRawLo : std::nextafter(xRawLo, xRawHi);
    const double xHi = hiAtRunway ? xRawHi : std::nextafter(xRawHi, xRawLo);
    if (xLo >= xHi) return;
    const double len = xHi - xLo;
    if (len < minLength) return;

    MosRectangleResult cand;
    cand.valid = true;
    cand.reason = MosPlannerReason::Accepted;
    cand.xStart = xLo;
    cand.xEnd = xHi;
    cand.yStart = yLow;
    cand.yEnd = yHigh;
    cand.length = len;
    cand.width = width;
    cand.area = len * width;
    if (!best.valid || betterRectangle(cand, best))
        best = cand;
}

} // namespace

// === 单次最大空矩形 ===
// 所有障碍物按 influenceRadius 投影到 X 轴（不论 Y 位置），Y 固定为跑道全宽 [-W/2, W/2]。
// 在 [0,L] 内寻找满足 minLength 的最大面积自由 X 空隙。
// 相切视为碰撞；开放自由端点用 std::nextafter 向内规范化。合法无解返回 valid=false 且 reason=NoFeasibleRectangle。
MosRectangleResult MosPlanner::planSingle(const MosObstacleSet &obstacles,
                                           const MosRunwayParams &params,
                                           const QVector<QString> &repairedIds)
{
    MosRectangleResult result;
    result.valid = false;
    result.reason = MosPlannerReason::NoFeasibleRectangle;

    // 校验跑道参数
    const auto vr = validateRunwayParams(params);
    if (!vr.valid) {
        result.reason = MosPlannerReason::InvalidParams;
        return result;
    }
    // 校验障碍物集合
    const auto vo = validateObstacleSet(obstacles, params);
    if (!vo.valid) {
        result.reason = MosPlannerReason::InvalidObstacles;
        return result;
    }
    // 校验已修复 ID：非空（含于 known）、已知、唯一
    QSet<QString> knownIds;
    for (const auto &c : obstacles.craters) knownIds.insert(c.id);
    for (const auto &u : obstacles.uxo) knownIds.insert(u.id);
    QSet<QString> seenRepaired;
    for (const auto &id : repairedIds) {
        if (!knownIds.contains(id)) {
            result.reason = MosPlannerReason::UnknownRepairedId;
            return result;
        }
        if (seenRepaired.contains(id)) {
            result.reason = MosPlannerReason::DuplicateRepairedId;
            return result;
        }
        seenRepaired.insert(id);
    }

    const double L = params.L;
    const double W = params.W;
    const double halfW = W / 2.0;

    // 收集未修复障碍物为圆盘（craters 后 uxo 顺序），使用存储的 influenceRadius
    QVector<Disk> disks;
    for (const auto &c : obstacles.craters)
        if (!repairedIds.contains(c.id))
            disks.append(Disk{static_cast<double>(c.x), static_cast<double>(c.y), c.influenceRadius});
    for (const auto &u : obstacles.uxo)
        if (!repairedIds.contains(u.id))
            disks.append(Disk{static_cast<double>(u.x), static_cast<double>(u.y), u.influenceRadius});

    // 所有障碍物投影到 X 轴（不论 Y 位置），Y 固定为跑道全宽 [-halfW, halfW]
    const double yLow = -halfW;
    const double yHigh = halfW;
    const double width = W;

    std::vector<Interval> blocked;
    for (const auto &d : disks)
        blocked.push_back(Interval{d.cx - d.r, d.cx + d.r});

    // 裁剪到 [0, L] 并合并闭区间（含 singleton 相切；next.low<=current.high 即合并）
    std::sort(blocked.begin(), blocked.end(),
              [](const Interval &a, const Interval &b) { return a.low < b.low; });
    std::vector<Interval> merged;
    for (const auto &iv : blocked) {
        const double lo = std::max(iv.low, 0.0);
        const double hi = std::min(iv.high, L);
        if (lo > hi) continue;
        if (!merged.empty() && lo <= merged.back().high)
            merged.back().high = std::max(merged.back().high, hi);
        else
            merged.push_back(Interval{lo, hi});
    }

    // 枚举自由空隙：跑道端点保持精确，障碍侧端点 nextafter 向内
    if (merged.empty()) {
        considerGap(result, 0.0, L, true, true, yLow, yHigh, width, params.minLength);
    } else {
        considerGap(result, 0.0, merged.front().low, true, false,
                    yLow, yHigh, width, params.minLength);
        for (std::size_t k = 1; k < merged.size(); ++k)
            considerGap(result, merged[k - 1].high, merged[k].low, false, false,
                        yLow, yHigh, width, params.minLength);
        considerGap(result, merged.back().high, L, false, true,
                    yLow, yHigh, width, params.minLength);
    }

    return result;
}

// === 稳定嵌套修复档位（仅 ID 分配）===
// 按 floor(tierIndex * N / (T-1)) 确定每档已修复点数量；tier 0 为空、末档修复全部、中间档逐级嵌套。
// 稳定障碍物顺序为 craters 后 uxo。非法参数/障碍物返回空。
QVector<MosTierPlan> MosPlanner::buildStableRepairTiers(const MosObstacleSet &obstacles,
                                                        const MosRunwayParams &params)
{
    QVector<MosTierPlan> tiers;

    // 校验跑道参数与障碍物集合；非法时返回空（仅此情况返回空）
    const auto vr = validateRunwayParams(params);
    if (!vr.valid) return tiers;
    const auto vo = validateObstacleSet(obstacles, params);
    if (!vo.valid) return tiers;

    // 稳定障碍物顺序：craters 后 uxo
    QVector<QString> ordered;
    for (const auto &c : obstacles.craters) ordered.append(c.id);
    for (const auto &u : obstacles.uxo) ordered.append(u.id);

    const int T = params.tiers;      // 校验保证 T in [2, 5]
    const int N = ordered.size();
    const int denom = T - 1;         // denom >= 1

    tiers.resize(T);
    for (int t = 0; t < T; ++t) {
        // floor(tierIndex * N / (T-1))；N=0 时全部为 0，T>N 时前缀分布自然产生重复计数
        const int count = (N == 0) ? 0
            : static_cast<int>(std::floor(static_cast<double>(t) * N / denom));
        for (int k = 0; k < count && k < N; ++k)
            tiers[t].repairedIds.append(ordered[k]);
    }

    return tiers;
}

} // namespace Core::MOS
