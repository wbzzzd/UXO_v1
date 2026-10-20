// GprTrack 实现：.cor RTK 轨迹解析、道号 1:1 对齐、间隙线性插值与高斯平滑。
// 格式与预研结论见 docs/features/gpr-display.md §5.4；仅依赖 Qt5::Core。

#include "GPR/GprTrack.h"

#include <QFile>
#include <QStringList>

#include <cmath>

namespace GPR {

namespace {

// .cor 行数与字段数的契约常量（预研已验证：样例 7297 行 × 14 字段）
constexpr int kCorFieldCount = 14;
// 道号上限防御：真实样例 7297 道；超过一百万道的道号视为异常数据，
// 拒绝载入以免按道号分配巨额内存
constexpr int kMaxReasonableTraces = 1000000;

// 单个 .cor 行的解析中间结果（道号 + 位置 + UTC 时间戳）
struct CorRow {
    int traceNumber = 0;     // 字段 #2：道号（1 基，对齐唯一依据）
    double latitude = 0.0;   // 字段 #3：纬度（度）
    double longitude = 0.0;  // 字段 #4：经度（度）
    QDateTime timestamp;     // 字段 #12 + #13 合成的 UTC 时间戳
};

// 解析 .cor 时间字段（#13）：hh:mm:ss.fff。
// 亚秒为十进制小数数字串：样例为 3 位毫秒（"835"→835 ms），
// 兼容更长精度（如 6 位微秒 "835123"→835 ms， QDateTime 仅毫秒精度）。
QTime parseTimeField(const QString &timeField)
{
    const QStringList secondParts = timeField.split(QLatin1Char('.'));
    if (secondParts.isEmpty() || secondParts.size() > 2) {
        return QTime();
    }
    const QStringList hms = secondParts.first().split(QLatin1Char(':'));
    if (hms.size() != 3) {
        return QTime();
    }
    bool okHour = false;
    bool okMinute = false;
    bool okSecond = false;
    const int hour = hms.at(0).toInt(&okHour);
    const int minute = hms.at(1).toInt(&okMinute);
    const int second = hms.at(2).toInt(&okSecond);
    if (!okHour || !okMinute || !okSecond) {
        return QTime();
    }

    int msec = 0;
    if (secondParts.size() == 2) {
        const QString fractionDigits = secondParts.at(1);
        if (fractionDigits.isEmpty()) {
            return QTime();
        }
        bool okFraction = false;
        const double fraction = fractionDigits.toDouble(&okFraction); // "835"→835，"075"→75
        if (!okFraction) {
            return QTime();
        }
        // 十进制小数折算毫秒：digits / 10^len * 1000（3 位时恰为原值）
        msec = static_cast<int>(std::lround(fraction * 1000.0 / std::pow(10.0, fractionDigits.size())));
    }
    return QTime(hour, minute, second, msec); // 越界入参会得到无效 QTime，由调用方判空
}

// 解析 .cor 日期（#12）+ 时间（#13）为 UTC 时间戳。
// .cor 时间比文件名本地时间戳（本地为 UTC+8）早约 8 小时，判定为 UTC：
// 内部统一按 Qt::UTC 存储，本地显示由调用方转换。
QDateTime parseTimestampUtc(const QString &dateField, const QString &timeField)
{
    const QDate date = QDate::fromString(dateField, QStringLiteral("yyyy-MM-dd"));
    if (!date.isValid()) {
        return QDateTime();
    }
    const QTime time = parseTimeField(timeField);
    if (!time.isValid()) {
        return QDateTime();
    }
    return QDateTime(date, time, Qt::UTC);
}

} // namespace

// === 载入与状态 ===

GprTrack::GprTrack()
{
    // 默认 σ=500 道（按 0.02 m/道 ≈ 10 m），构造时即构建高斯核
    rebuildKernel();
}

bool GprTrack::loadFromFile(const QString &path)
{
    // 先复位：载入失败时保持空态，不残留上一次的数据
    m_points.clear();
    m_loaded = false;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QVector<CorRow> rows;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty()) {
            continue; // 容忍空行
        }
        const QStringList fields = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        // 14 字段布局见头文件；要求至少 14 个字段（容忍未来更多未知字段），
        // 不足的行视为无效：该道留作间隙，访问时经插值补齐
        if (fields.size() < kCorFieldCount) {
            continue;
        }

        bool okTrace = false;
        bool okLatitude = false;
        bool okLongitude = false;
        CorRow row;
        row.traceNumber = fields.at(1).toInt(&okTrace);        // #2 道号（不用 #1 记录号对齐）
        row.latitude = fields.at(2).toDouble(&okLatitude);     // #3 纬度
        row.longitude = fields.at(3).toDouble(&okLongitude);   // #4 经度
        if (!okTrace || !okLatitude || !okLongitude || row.traceNumber <= 0) {
            continue;
        }

        row.timestamp = parseTimestampUtc(fields.at(11), fields.at(12)); // #12 日期 + #13 时间
        if (!row.timestamp.isValid()) {
            continue; // 有效点必须带有效 UTC 时间戳（保持结构不变量）
        }
        rows.append(row);
    }

    if (rows.isEmpty()) {
        return false; // 没有任何可解析的轨迹行
    }

    // 覆盖道数 = 最大道号。真实样例道号 1..7297 连续，与 .rd3 道数一致；
    // 道号缺失的行会让对应道保持间隙，但不缩小覆盖范围
    int maxTraceNumber = 0;
    for (const CorRow &row : rows) {
        maxTraceNumber = qMax(maxTraceNumber, row.traceNumber);
    }
    if (maxTraceNumber > kMaxReasonableTraces) {
        return false; // 异常道号（如误指向其他格式文件），拒绝载入
    }

    m_points.resize(maxTraceNumber); // 缺失道保持默认 valid=false
    for (const CorRow &row : rows) {
        // 道号 → 0 基索引（道号-1）直接映射；真实数据道号连续无重复，
        // 若出现重复道号，后行覆盖前行
        GprTrackPoint &point = m_points[row.traceNumber - 1];
        point.latitude = row.latitude;
        point.longitude = row.longitude;
        point.timestampUtc = row.timestamp;
        point.valid = true;
    }

    m_loaded = true;
    return true;
}

bool GprTrack::isValid() const
{
    return m_loaded;
}

int GprTrack::pointCount() const
{
    // = 最大道号，与 .rd3 道数一致（样例 7297）；未载入时为 0
    return m_points.size();
}

// === 道索引访问（1:1 对齐 + 间隙插值） ===

GprTrackPoint GprTrack::pointForTrace(int traceIndex) const
{
    GprTrackPoint result; // 默认 valid=false
    if (traceIndex < 0 || traceIndex >= m_points.size()) {
        return result; // 越界道索引
    }

    const GprTrackPoint &direct = m_points.at(traceIndex);
    if (direct.valid) {
        return direct; // 道号直接命中（1:1 对齐）
    }

    // 该道缺失（记录号跳变导致的行缺失/坏行）：在最近的左右有效点间线性插值
    int left = traceIndex - 1;
    while (left >= 0 && !m_points.at(left).valid) {
        --left;
    }
    int right = traceIndex + 1;
    while (right < m_points.size() && !m_points.at(right).valid) {
        ++right;
    }
    if (left < 0 || right >= m_points.size()) {
        return result; // 单侧无有效点，无法插值
    }

    const GprTrackPoint &leftPoint = m_points.at(left);
    const GprTrackPoint &rightPoint = m_points.at(right);
    const double t = static_cast<double>(traceIndex - left) / static_cast<double>(right - left);
    result.latitude = leftPoint.latitude + (rightPoint.latitude - leftPoint.latitude) * t;
    result.longitude = leftPoint.longitude + (rightPoint.longitude - leftPoint.longitude) * t;
    // 时间戳同样线性插值（毫秒精度，满足显示用途）
    const qint64 leftMs = leftPoint.timestampUtc.toMSecsSinceEpoch();
    const qint64 rightMs = rightPoint.timestampUtc.toMSecsSinceEpoch();
    const qint64 interpolatedMs =
        leftMs + static_cast<qint64>(std::llround((rightMs - leftMs) * t));
    result.timestampUtc = QDateTime::fromMSecsSinceEpoch(interpolatedMs, Qt::UTC);
    result.valid = true;
    return result;
}

// === 高斯平滑 ===
//
// 参数依据（§5.4 预研结论）：
// - 原始 RTK 横向抖动 std ≈ 1.43 m；σ=3 m（150 道）高斯平滑不足
//   （平滑后路径仍比端点弦长 25%），故默认取更宽的 σ=500 道（≈10 m @ 0.02 m/道）；
// - 单趟数据无法区分真实弯道（轨迹弓高 +4.34 m）与 GPS 缓漂，
//   曲线/漂移标定需多趟数据，属后续工作——本类暂不实现样条/卡尔曼，
//   仅提供可调 σ 的高斯平滑。
GprTrackPoint GprTrack::smoothedPointForTrace(int traceIndex) const
{
    // 平滑关闭（σ<=0，核为空）时退化为原始/插值点
    if (m_kernel.isEmpty()) {
        return pointForTrace(traceIndex);
    }

    GprTrackPoint result; // 默认 valid=false
    if (traceIndex < 0 || traceIndex >= m_points.size()) {
        return result; // 越界道索引
    }

    const GprTrackPoint center = pointForTrace(traceIndex);
    if (!center.valid) {
        return result; // 中心道无位置（无法插值）→ 无平滑位置
    }

    // 高斯加权平均：窗口 [traceIndex-半径, traceIndex+半径] 截断到有效道范围；
    // 缺失道经插值后参与平均；权重按窗口内实际参与点的权重和归一，
    // 因此核无需预先归一化，且天然处理边界截断与道间隙
    const int radius = m_kernel.size() - 1;
    const int left = qMax(0, traceIndex - radius);
    const int right = qMin(m_points.size() - 1, traceIndex + radius);

    double weightSum = 0.0;
    double latitudeSum = 0.0;
    double longitudeSum = 0.0;
    for (int j = left; j <= right; ++j) {
        const GprTrackPoint sample = pointForTrace(j);
        if (!sample.valid) {
            continue; // 边缘道两侧均无有效点时无法插值，跳过
        }
        const double weight = m_kernel.at(qAbs(j - traceIndex));
        latitudeSum += sample.latitude * weight;
        longitudeSum += sample.longitude * weight;
        weightSum += weight;
    }
    if (weightSum <= 0.0) {
        return result; // 窗口内无有效点（异常状态防御）
    }

    result.latitude = latitudeSum / weightSum;
    result.longitude = longitudeSum / weightSum;
    // 时间戳不做平均（无显示意义），取中心道（含插值）时间戳
    result.timestampUtc = center.timestampUtc;
    result.valid = true;
    return result;
}

void GprTrack::setSmoothingSigmaTraces(double sigma)
{
    m_sigmaTraces = sigma;
    rebuildKernel();
}

void GprTrack::rebuildKernel()
{
    m_kernel.clear();
    if (m_sigmaTraces <= 0.0) {
        return; // σ<=0：关闭平滑
    }

    // 核半径 = ceil(3σ)：高斯核约 99.7% 能量落在 ±3σ 内。
    // σ 异常巨大或 NaN 时按道数上限截断，避免整型溢出与巨额核分配
    const double sigmaCapped = qMin(m_sigmaTraces, static_cast<double>(kMaxReasonableTraces) / 3.0);
    const int radius = qMax(1, static_cast<int>(std::ceil(3.0 * sigmaCapped)));
    m_kernel.reserve(radius + 1);
    for (int offset = 0; offset <= radius; ++offset) {
        // 先转 double 再平方：offset² 在 int 下会溢出（radius 可达上限级别）
        const double weight = std::exp(-(static_cast<double>(offset) * offset)
                                       / (2.0 * sigmaCapped * sigmaCapped));
        m_kernel.append(weight); // 对称半核：m_kernel[i] = |偏移|=i 的权重
    }
}

} // namespace GPR
