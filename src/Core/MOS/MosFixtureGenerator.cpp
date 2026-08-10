// MOS P0 确定性合成 fixture 生成器实现。
// mulberry32 从 JS 原型精确移植，使用 uint32 回绕和 signed int32 归一化。
// 不使用 QRandomGenerator / std::uniform_*_distribution / 系统时钟 / thread-local 随机。

#include "Core/MOS/MosFixtureGenerator.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace Core::MOS {

namespace {

// JS Math.round(x) = floor(x + 0.5)（半数向正无穷语义）
// 不使用 std::round（其半数远离零语义与 JS 不同）
double jsRound(double x)
{
    return std::floor(x + 0.5);
}

uint64_t doubleBits(double value)
{
    uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

double doubleFromBits(uint64_t bits)
{
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

// 精确移植 Node 20 的 V8 11.3 Math.cbrt，冻结跨 JS/Qt fixture 的最后一位。
double jsCbrt(double value)
{
    constexpr uint32_t kB1 = 715094163U;
    constexpr uint32_t kB2 = 696219795U;
    constexpr double kP0 = 1.87595182427177009643;
    constexpr double kP1 = -1.88497979543377169875;
    constexpr double kP2 = 1.621429720105354466140;
    constexpr double kP3 = -0.758397934778766047437;
    constexpr double kP4 = 0.145996192886612446982;

    const uint64_t inputBits = doubleBits(value);
    uint32_t high = static_cast<uint32_t>(inputBits >> 32);
    const uint32_t low = static_cast<uint32_t>(inputBits);
    const uint32_t sign = high & 0x80000000U;
    high ^= sign;
    if (high >= 0x7ff00000U) return value + value;

    double estimate = 0.0;
    if (high < 0x00100000U) {
        if ((high | low) == 0U) return value;
        estimate = doubleFromBits(0x4350000000000000ULL) * value;
        const uint32_t scaledHigh = static_cast<uint32_t>(doubleBits(estimate) >> 32);
        estimate = doubleFromBits(static_cast<uint64_t>(
                                      sign | ((scaledHigh & 0x7fffffffU) / 3U + kB2))
                                  << 32);
    } else {
        estimate = doubleFromBits(static_cast<uint64_t>(sign | (high / 3U + kB1)) << 32);
    }

    double ratio = (estimate * estimate) * (estimate / value);
    estimate *= (kP0 + ratio * (kP1 + ratio * kP2)) +
                ((ratio * ratio) * ratio) * (kP3 + ratio * kP4);
    uint64_t estimateBits = doubleBits(estimate);
    estimateBits = (estimateBits + 0x80000000ULL) & 0xffffffffc0000000ULL;
    estimate = doubleFromBits(estimateBits);

    const double square = estimate * estimate;
    ratio = value / square;
    const double width = estimate + estimate;
    ratio = (ratio - estimate) / (width + ratio);
    return estimate + estimate * ratio;
}

// mulberry32 伪随机数生成器（从 JS 原型 lines 594-602 精确移植）
// 使用 uint32 回绕模拟 JS 的 | 0 和 >>> 0 语义
class Mulberry32 {
public:
    explicit Mulberry32(uint32_t seed) : m_state(seed) {}

    // 返回 [0, 1) 双精度浮点数
    double next()
    {
        // a = (a + 0x6D2B79F5) | 0  → uint32 回绕
        m_state = m_state + 0x6D2B79F5u;

        // t = Math.imul(a ^ (a >>> 15), 1 | a)
        // Math.imul = 32 位乘法取低 32 位，uint32 乘法天然回绕
        uint32_t t = (m_state ^ (m_state >> 15)) * (1u | m_state);

        // t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t
        t = (t + (t ^ (t >> 7)) * (61u | t)) ^ t;

        // return ((t ^ (t >>> 14)) >>> 0) / 4294967296
        // uint32 除以 2^32 得到 [0, 1) 双精度
        return static_cast<double>(t ^ (t >> 14)) / 4294967296.0;
    }

private:
    uint32_t m_state;
};

} // namespace

// === 生成合成障碍物集合 ===
MosObstacleSet MosFixtureGenerator::generate(const MosRunwayParams &params,
                                             const MosGeneratorParams &genParams,
                                             qint32 seed)
{
    // 种子归一化为 uint32（-1 → 4294967295, -2147483648 → 2147483648）
    Mulberry32 rng(static_cast<uint32_t>(seed));

    MosObstacleSet result;

    // 弹坑生成：冻结合同抽取顺序
    // i=0: visibleRadius -> x -> y（threat 固定 high，不消耗 rng）
    // i>0: visibleRadius -> x -> y -> threat
    for (int i = 0; i < genParams.craterCount; ++i) {
        const double uR = rng.next();
        const double uX = rng.next();
        const double uY = rng.next();

        MosCrater crater;
        crater.id = QStringLiteral("crater-%1").arg(i);
        crater.visibleRadius = jsRound(genParams.craterRMin +
                                       uR * (genParams.craterRMax - genParams.craterRMin));
        crater.x = static_cast<int>(jsRound(uX * params.L));
        // Y 范围收紧到 [-40, 40]（跑道 ±15m），确保大部分障碍物影响算法
        crater.y = static_cast<int>(jsRound(-40.0 + uY * 80.0));

        if (i == 0) {
            // 首个弹坑 threat 固定为 high，不消耗 rng
            crater.threat = MosThreatLevel::High;
        } else {
            // 其余弹坑抽取 threat: u > 0.5 -> medium, 否则 high
            const double uT = rng.next();
            crater.threat = (uT > 0.5) ? MosThreatLevel::Medium : MosThreatLevel::High;
        }

        // influenceRadius = visibleRadius * expand（float，不取整）
        crater.influenceRadius = crater.visibleRadius * params.expand;
        result.craters.append(crater);
    }

    // UXO 生成：冻结合同抽取顺序
    // syntheticYield -> x -> y -> threat（总是抽取）
    for (int i = 0; i < genParams.uxoCount; ++i) {
        const double uYld = rng.next();
        const double uX = rng.next();
        const double uY = rng.next();

        MosUxo uxo;
        uxo.id = QStringLiteral("uxo-%1").arg(i);
        // syntheticYield = uyMin + u*(uyMax-uyMin)（float，不取整）
        uxo.syntheticYield = genParams.uxoYMin +
                             uYld * (genParams.uxoYMax - genParams.uxoYMin);
        uxo.x = static_cast<int>(jsRound(uX * params.L));
        // Y 范围收紧到 [-40, 40]（跑道 ±15m），确保大部分障碍物影响算法
        uxo.y = static_cast<int>(jsRound(-40.0 + uY * 80.0));

        // threat: u > 0.5 -> high, 否则 medium（注意与弹坑相反）
        const double uT = rng.next();
        uxo.threat = (uT > 0.5) ? MosThreatLevel::High : MosThreatLevel::Medium;

        // influenceRadius = K * Math.cbrt(syntheticYield)（float，不取整）
        uxo.influenceRadius = params.K * jsCbrt(uxo.syntheticYield);
        result.uxo.append(uxo);
    }

    return result;
}

// === 嵌套 fixture 顺序：floor(tierIndex * N / (T-1)) ===
QVector<int> MosFixtureGenerator::nestedFixtureOrder(int tiers, int totalObstacleCount)
{
    // T<1 时返回空
    if (tiers < 1) {
        return {};
    }
    // T=1 时返回 [0]（单个修复集合）
    if (tiers == 1) {
        return {0};
    }
    // floor(tierIndex * N / (T-1))，整数除法天然取地板
    QVector<int> result;
    result.reserve(tiers);
    for (int i = 0; i < tiers; ++i) {
        result.append((i * totalObstacleCount) / (tiers - 1));
    }
    return result;
}

} // namespace Core::MOS
