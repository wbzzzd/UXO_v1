// GprThresholdDetector 实现：阈值候选检测管线（REQ-012）。
// 算法口径以 gpr-display.md §5.4 的 Python 预研为基准：背景去除压制直达波与层状
// 杂波，RMS 包络 + 深度白化补偿衰减，分位阈值自适应噪声水平，形态学膨胀 + 连通
// 成分把零散超阈值像素聚合为可复核候选。检测仅作候选标记，人工复核必需。

#include "GPR/GprThresholdDetector.h"

#include "GPR/GprBScanData.h"
#include "GPR/GprRadarHeader.h"

#include <algorithm>
#include <cmath>

namespace GPR {

namespace {

// 背景去除滑窗（道）：列向 ±50 道运行均值，压制直达波与横向连续层位
constexpr int kBackgroundTraces = 101;
// 包络滑窗总长（样本）：窗口 [j-8, j+7]，前置 8
constexpr int kEnvelopeWindow = 16;
// 深度白化最大增益：gain[j] 从 1 线性升到 10，补偿深部衰减
constexpr double kMaxGain = 10.0;
// 形态学膨胀道向半径（道）
constexpr int kDilateTraces = 15;
// 形态学膨胀时间向半径（样本）
constexpr int kDilateSamples = 9;

} // namespace

// === 参数设置 ===
void GprThresholdDetector::setQuantile(double q)
{
    m_quantile = qBound(0.5, q, 1.0);
}

void GprThresholdDetector::setMaxTimeNs(double maxTimeNs)
{
    m_maxTimeNs = maxTimeNs;
}

void GprThresholdDetector::setMinComponentPx(int px)
{
    m_minComponentPx = qMax(1, px);
}

// === 检测管线 ===
QList<GprSuspect> GprThresholdDetector::detect(const GprBScanData &bscan,
                                               const GprRadarHeader &header) const
{
    // 防御：输入无效/尺寸退化/采样间隔非正/初至不可用 → 无候选
    if (!bscan.isValid() || !header.isValid()) {
        return {};
    }
    const int T = bscan.traceCount();
    const int S = bscan.sampleCount();
    if (T <= 0 || S <= 0) {
        return {};
    }
    const double dt = header.dtNs();
    if (dt <= 0.0) {
        return {};
    }
    const int f0 = bscan.firstBreakSample();
    if (f0 < 0 || f0 >= S) {
        return {};
    }
    const int J = S - f0;  // 初至截断后每道样本数

    // === 1) 逐道去直流 + 截断 [f0, S)：data 平铺 t*J+j ===
    QVector<double> data(T * J);
    QVector<double> traceBuf(J);
    for (int t = 0; t < T; ++t) {
        double sum = 0.0;
        for (int j = 0; j < J; ++j) {
            const double v = bscan.sampleAt(t, f0 + j);
            traceBuf[j] = v;
            sum += v;
        }
        const double mean = sum / J;
        for (int j = 0; j < J; ++j) {
            data[t * J + j] = traceBuf[j] - mean;
        }
    }

    // === 2) 列向滑窗 ±50 道运行均值背景去除（边缘窗口截断）===
    QVector<double> clean(T * J);
    {
        const int half = kBackgroundTraces / 2;  // 50
        QVector<double> colSum(J, 0.0);
        int lo = 0;
        int hi = qMin(half, T - 1);
        for (int tt = lo; tt <= hi; ++tt) {
            for (int j = 0; j < J; ++j) {
                colSum[j] += data[tt * J + j];
            }
        }
        for (int t = 0; t < T; ++t) {
            const double count = static_cast<double>(hi - lo + 1);
            for (int j = 0; j < J; ++j) {
                clean[t * J + j] = data[t * J + j] - colSum[j] / count;
            }
            // 滑到 t+1：移出 [lo, newLo)，移入 (hi, newHi]
            const int newLo = qMax(0, t + 1 - half);
            const int newHi = qMin(T - 1, t + 1 + half);
            for (int tt = lo; tt < newLo; ++tt) {
                for (int j = 0; j < J; ++j) {
                    colSum[j] -= data[tt * J + j];
                }
            }
            for (int tt = hi + 1; tt <= newHi; ++tt) {
                for (int j = 0; j < J; ++j) {
                    colSum[j] += data[tt * J + j];
                }
            }
            lo = newLo;
            hi = newHi;
        }
    }

    // === 3) 滑动 RMS 包络（窗口 [j-8, j+7]，边缘截断）===
    QVector<double> envelope(T * J);
    {
        const int pre = kEnvelopeWindow / 2;  // 8
        for (int t = 0; t < T; ++t) {
            const int base = t * J;
            double sqSum = 0.0;
            int lo = 0;
            int hi = qMin(pre - 1, J - 1);
            for (int k = lo; k <= hi; ++k) {
                sqSum += clean[base + k] * clean[base + k];
            }
            for (int j = 0; j < J; ++j) {
                envelope[base + j] = std::sqrt(sqSum / (hi - lo + 1));
                // 滑到 j+1
                const int newLo = qMax(0, j + 1 - pre);
                const int newHi = qMin(J - 1, j + pre - 1);
                for (int k = lo; k < newLo; ++k) {
                    sqSum -= clean[base + k] * clean[base + k];
                }
                for (int k = hi + 1; k <= newHi; ++k) {
                    sqSum += clean[base + k] * clean[base + k];
                }
                lo = newLo;
                hi = newHi;
            }
        }
    }

    // === 4) 深度白化：gain[j] = min(1 + 9*j/max(1,J-1), 10)，随深度线性增大 ===
    QVector<double> amp(T * J);
    {
        QVector<double> gain(J);
        for (int j = 0; j < J; ++j) {
            gain[j] = qMin(1.0 + (kMaxGain - 1.0) * j / qMax(1, J - 1), kMaxGain);
        }
        for (int t = 0; t < T; ++t) {
            for (int j = 0; j < J; ++j) {
                amp[t * J + j] = envelope[t * J + j] * gain[j];
            }
        }
    }

    // === 5) 检测时窗：windowEnd = 满足 dt*(f0+j) ≤ maxTimeNs 的最大 j（原始样本坐标）===
    int windowEnd = -1;
    for (int j = 0; j < J; ++j) {
        if (dt * (f0 + j) <= m_maxTimeNs) {
            windowEnd = j;
        } else {
            break;  // dt>0，时间随 j 单调增，首个越界即终止
        }
    }
    if (windowEnd < 0) {
        return {};  // 时窗内无样本
    }

    // === 6) 分位阈值：时窗内全部白化幅值排序后线性插值 ===
    QVector<double> pool;
    pool.reserve(T * (windowEnd + 1));
    for (int t = 0; t < T; ++t) {
        for (int j = 0; j <= windowEnd; ++j) {
            pool.append(amp[t * J + j]);
        }
    }
    std::sort(pool.begin(), pool.end());
    const int n = pool.size();
    const double pos = m_quantile * (n - 1);
    const int loIdx = static_cast<int>(pos);  // pos >= 0，截断即 floor
    const int hiIdx = qMin(loIdx + 1, n - 1);
    const double frac = pos - loIdx;
    const double threshold = pool[loIdx] * (1.0 - frac) + pool[hiIdx] * frac;

    // === 7) 阈值掩膜：时窗内且幅值严格大于阈值 ===
    QVector<quint8> mask(T * J, 0);
    for (int t = 0; t < T; ++t) {
        for (int j = 0; j <= windowEnd; ++j) {
            if (amp[t * J + j] > threshold) {
                mask[t * J + j] = 1;
            }
        }
    }

    // === 8) 可分离两遍形态学膨胀（前缀计数 O(TJ)：先道向 ±15，后时间向 ±9）===
    // 注意：时间向膨胀可使膨胀掩膜越出时窗（半径 9 样本），候选外接框随之允许
    // 外溢，测试侧按 maxTimeNs + 10*dt 容忍（9 样本膨胀 + 取整）。
    QVector<quint8> dilated(T * J, 0);
    {
        // 道向：对每列做 ±15 道膨胀（掩膜在时窗外全 0，只需处理时窗内列）
        QVector<int> cnt(T + 1);
        for (int j = 0; j <= windowEnd; ++j) {
            cnt[0] = 0;
            for (int t = 0; t < T; ++t) {
                cnt[t + 1] = cnt[t] + mask[t * J + j];
            }
            for (int t = 0; t < T; ++t) {
                const int lo = qMax(0, t - kDilateTraces);
                const int hi = qMin(T - 1, t + kDilateTraces);
                dilated[t * J + j] = (cnt[hi + 1] - cnt[lo]) > 0 ? 1 : 0;
            }
        }
        // 时间向：对每行做 ±9 样本膨胀（先取前缀计数快照再就地覆盖，行内安全）
        QVector<int> cnt2(J + 1);
        for (int t = 0; t < T; ++t) {
            const int base = t * J;
            cnt2[0] = 0;
            for (int j = 0; j < J; ++j) {
                cnt2[j + 1] = cnt2[j] + dilated[base + j];
            }
            for (int j = 0; j < J; ++j) {
                const int lo = qMax(0, j - kDilateSamples);
                const int hi = qMin(J - 1, j + kDilateSamples);
                dilated[base + j] = (cnt2[hi + 1] - cnt2[lo]) > 0 ? 1 : 0;
            }
        }
    }

    // === 9) 8-邻域连通成分（DFS，入栈即标记）+ 面积过滤 + 候选生成 ===
    QVector<quint8> visited(T * J, 0);
    QVector<int> stack;
    QList<GprSuspect> suspects;
    for (int t0 = 0; t0 < T; ++t0) {
        for (int j0 = 0; j0 < J; ++j0) {
            const int start = t0 * J + j0;
            if (!dilated[start] || visited[start]) {
                continue;
            }
            // 遍历一个成分：外接框、原始掩膜像素数与最大白化幅值
            int minT = t0;
            int maxT = t0;
            int minJ = j0;
            int maxJ = j0;
            int maskPixels = 0;
            double maxAmp = 0.0;
            stack.clear();
            stack.append(start);
            visited[start] = 1;
            while (!stack.isEmpty()) {
                const int cur = stack.takeLast();
                const int ct = cur / J;
                const int cj = cur % J;
                minT = qMin(minT, ct);
                maxT = qMax(maxT, ct);
                minJ = qMin(minJ, cj);
                maxJ = qMax(maxJ, cj);
                if (mask[cur]) {
                    ++maskPixels;
                    maxAmp = qMax(maxAmp, amp[cur]);
                }
                for (int dTrace = -1; dTrace <= 1; ++dTrace) {
                    for (int dSample = -1; dSample <= 1; ++dSample) {
                        if (dTrace == 0 && dSample == 0) {
                            continue;
                        }
                        const int nt = ct + dTrace;
                        const int nj = cj + dSample;
                        if (nt < 0 || nt >= T || nj < 0 || nj >= J) {
                            continue;
                        }
                        const int nidx = nt * J + nj;
                        if (!dilated[nidx] || visited[nidx]) {
                            continue;
                        }
                        visited[nidx] = 1;  // 入栈即标记，避免重复入栈
                        stack.append(nidx);
                    }
                }
            }
            // 面积过滤：成分内原始掩膜像素数 < 下限 → 孤立噪点，丢弃
            if (maskPixels < m_minComponentPx) {
                continue;
            }
            GprSuspect suspect;
            suspect.traceIndex = (minT + maxT) / 2;
            suspect.sampleIndex = f0 + (minJ + maxJ) / 2;
            suspect.depthM = header.depthForSample(suspect.sampleIndex);
            suspect.amplitude = maxAmp;
            suspect.traceSpan = maxT - minT + 1;
            suspect.sampleSpan = maxJ - minJ + 1;
            suspects.append(suspect);
        }
    }

    // === 10) 按 (traceIndex, sampleIndex) 字典序排序并赋 1 基连续 id ===
    std::sort(suspects.begin(), suspects.end(), [](const GprSuspect &a, const GprSuspect &b) {
        if (a.traceIndex != b.traceIndex) {
            return a.traceIndex < b.traceIndex;
        }
        return a.sampleIndex < b.sampleIndex;
    });
    for (int i = 0; i < suspects.size(); ++i) {
        suspects[i].id = i + 1;
    }
    return suspects;
}

} // namespace GPR
