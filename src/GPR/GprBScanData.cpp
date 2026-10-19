// GprBScanData 实现：MALA .rd3 int16 小端 B-Scan 读取（REQ-012）。
// 布局事实（Python 预研已验证，gpr-display.md §5.4）：按道顺序存储，
// 每道 SAMPLES 个 int16 小端样本；文件大小 = 道数 × SAMPLES × 2。
// 访问策略：preloadAllTraces() 单次顺序 bulk read 驻留内存（交互路径默认）；
// 未预载时按道懒加载 + LRU（容量 8）回退。

#include "GPR/GprBScanData.h"

#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <cmath>
#include <limits>

namespace GPR {

namespace {

// LRU 缓存容量（道）：通道滚动浏览时单文件热区远小于 8
constexpr int kLruCapacity = 8;
// 初至估计分析窗（样本）：覆盖实测 first-break 46–59（gpr-display.md §5.4）
constexpr int kFirstBreakWindow = 100;
// 噪声段长度（样本）：分析窗前段视为纯噪声区，用于估计噪声门限
constexpr int kNoiseSegment = 32;
// 主判据门限系数：|mean| + 6σ
constexpr double kNoiseSigmaFactor = 6.0;

// 单道初至估计：以噪声段 [0, noiseLen) 统计 |mean|+6σ 为门限，从 noiseLen 起在
// 分析窗 [0, window) 内找首个 |幅值|>门限 的样本；无命中时回退为窗内 |一阶差分|
// 最大值位置（bestDiff 初值 -1，严格大于取首个最大 → 平道差分全 0 时返回 0）。
int estimateFirstBreak(const QVector<qint16> &samples)
{
    const int window = qMin(kFirstBreakWindow, samples.size());
    if (window < 2) {
        return 0;
    }
    const int noiseLen = qMin(kNoiseSegment, window / 2);

    // 噪声段统计：均值与总体标准差
    double sum = 0.0;
    for (int s = 0; s < noiseLen; ++s) {
        sum += samples[s];
    }
    const double mean = sum / noiseLen;
    double sqSum = 0.0;
    for (int s = 0; s < noiseLen; ++s) {
        const double d = samples[s] - mean;
        sqSum += d * d;
    }
    const double sigma = std::sqrt(sqSum / noiseLen);
    const double threshold = std::fabs(mean) + kNoiseSigmaFactor * sigma;

    // 主判据：噪声段之后首个 |幅值| 超门限的样本
    for (int s = noiseLen; s < window; ++s) {
        if (std::fabs(static_cast<double>(samples[s])) > threshold) {
            return s;
        }
    }

    // 回退：全窗 |一阶差分| argmax（s 从 0 起扫，严格大于取首个最大 → 平道返回 0）
    int best = 0;
    double bestDiff = -1.0;
    for (int s = 0; s <= window - 2; ++s) {
        const double diff = std::fabs(static_cast<double>(samples[s + 1])
                                      - static_cast<double>(samples[s]));
        if (diff > bestDiff) {
            bestDiff = diff;
            best = s;
        }
    }
    return best;
}

} // namespace

// === 载入与校验 ===
bool GprBScanData::loadFromFile(const QString &path, int expectedSamples)
{
    // 加载前复位全部状态，保证同一对象重复加载语义一致
    m_valid = false;
    m_traceCount = 0;
    m_sampleCount = 0;
    m_path.clear();
    m_traces.clear();
    m_fullyLoaded = false;
    m_cacheKeys.clear();
    m_cacheValues.clear();
    m_firstBreak = -1;

    if (expectedSamples <= 0) {
        return false;
    }
    const QFileInfo info(path);
    if (!info.isFile()) {
        return false;
    }
    const qint64 fileSize = info.size();
    if (fileSize <= 0) {
        return false;
    }
    const qint64 bytesPerTrace = static_cast<qint64>(expectedSamples) * 2;
    if (fileSize % bytesPerTrace != 0) {
        return false;
    }
    const qint64 traces = fileSize / bytesPerTrace;
    if (traces > std::numeric_limits<int>::max()) {
        return false;
    }

    m_valid = true;
    m_traceCount = static_cast<int>(traces);
    m_sampleCount = expectedSamples;
    m_path = info.absoluteFilePath();
    return true;
}

// === 查询接口 ===
bool GprBScanData::isValid() const
{
    return m_valid;
}

int GprBScanData::traceCount() const
{
    return m_traceCount;
}

int GprBScanData::sampleCount() const
{
    return m_sampleCount;
}

qint16 GprBScanData::sampleAt(int traceIndex, int sampleIndex) const
{
    if (!m_valid || traceIndex < 0 || traceIndex >= m_traceCount
        || sampleIndex < 0 || sampleIndex >= m_sampleCount) {
        return 0;
    }
    return fetchTrace(traceIndex).value(sampleIndex, 0);
}

QVector<qint16> GprBScanData::trace(int traceIndex) const
{
    if (!m_valid || traceIndex < 0 || traceIndex >= m_traceCount) {
        return {};
    }
    return fetchTrace(traceIndex);
}

// === 逐通道 t0 初至估计（§5.4：各通道 first-break 46–59，逐通道校正必需）===
int GprBScanData::firstBreakSample() const
{
    if (!m_valid) {
        return -1;
    }
    if (m_firstBreak >= 0) {
        return m_firstBreak;  // 缓存命中（有效数据初至恒 >= 0，-1 即未计算）
    }

    // 逐道估计，取中位数抑制单道异常（道数 >= 1 由加载校验保证；偶数取下中位）
    QVector<int> estimates;
    estimates.reserve(m_traceCount);
    for (int t = 0; t < m_traceCount; ++t) {
        estimates.append(estimateFirstBreak(fetchTrace(t)));
    }
    std::sort(estimates.begin(), estimates.end());
    m_firstBreak = estimates[(estimates.size() - 1) / 2];
    return m_firstBreak;
}

// === 全通道预载（工作者线程调用；消除交互路径逐道磁盘小读）===
bool GprBScanData::preloadAllTraces()
{
    if (!m_valid) {
        return false;
    }
    if (m_fullyLoaded) {
        return true;  // 幂等：已预载直接成功
    }

    // 单次顺序 bulk read：/mnt/d（9p）逐道小读放大是雷达页交互冻结根因，
    // 整文件一次性读入（样例通道约 7.5MB）后所有访问命中内存
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray raw = file.readAll();
    const qint64 expected = static_cast<qint64>(m_traceCount) * m_sampleCount * 2;
    if (raw.size() != expected) {
        // 短读/读异常：不预载，保留逐道懒加载回退（其自带零填充容错）
        return false;
    }

    QVector<QVector<qint16>> all;
    all.reserve(m_traceCount);
    const auto *bytes = reinterpret_cast<const uchar *>(raw.constData());
    for (int t = 0; t < m_traceCount; ++t) {
        QVector<qint16> trace(m_sampleCount, 0);
        const qint64 base = static_cast<qint64>(t) * m_sampleCount * 2;
        for (int s = 0; s < m_sampleCount; ++s) {
            // int16 小端显式字节组装（与平台字节序无关）
            const quint16 lo = bytes[base + 2 * s];
            const quint16 hi = bytes[base + 2 * s + 1];
            trace[s] = static_cast<qint16>(lo | (hi << 8));
        }
        all.append(trace);
    }
    m_traces = all;
    m_fullyLoaded = true;
    // 预载后 LRU 缓存失去意义：复位释放
    m_cacheKeys.clear();
    m_cacheValues.clear();
    return true;
}

bool GprBScanData::isFullyLoaded() const
{
    return m_fullyLoaded;
}

// === 按道读取（预载后命中内存；否则 LRU 缓存）===
QVector<qint16> GprBScanData::fetchTrace(int traceIndex) const
{
    // 全通道预载：隐式共享拷贝返回，无数据复制
    if (m_fullyLoaded) {
        return m_traces.at(traceIndex);
    }

    // 缓存命中：move-to-front
    const int pos = m_cacheKeys.indexOf(traceIndex);
    if (pos >= 0) {
        m_cacheKeys.takeAt(pos);
        const QVector<qint16> cached = m_cacheValues.takeAt(pos);
        m_cacheKeys.prepend(traceIndex);
        m_cacheValues.prepend(cached);
        return cached;
    }

    // 未命中：定位读取整道，int16 小端显式字节组装（与平台字节序无关）
    QVector<qint16> result(m_sampleCount, 0);
    QFile file(m_path);
    if (file.open(QIODevice::ReadOnly)) {
        const qint64 offset = static_cast<qint64>(traceIndex) * m_sampleCount * 2;
        if (file.seek(offset)) {
            const QByteArray raw = file.read(static_cast<qint64>(m_sampleCount) * 2);
            const int available = qMin(raw.size() / 2, m_sampleCount);
            for (int s = 0; s < available; ++s) {
                const quint16 lo = static_cast<quint8>(raw.at(2 * s));
                const quint16 hi = static_cast<quint8>(raw.at(2 * s + 1));
                result[s] = static_cast<qint16>(lo | (hi << 8));
            }
        }
    }
    // 读失败/短读：剩余样本保持 0，不中断访问

    // 写入缓存：满时先淘汰队尾最久未用项
    while (m_cacheKeys.size() >= kLruCapacity) {
        m_cacheKeys.removeLast();
        m_cacheValues.removeLast();
    }
    m_cacheKeys.prepend(traceIndex);
    m_cacheValues.prepend(result);
    return result;
}

} // namespace GPR
