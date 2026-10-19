// GprRadarHeader 实现：MALA .rad 头文件键值对解析（REQ-013）。
// 格式事实以实测样例为准（gpr-display.md §5.4）：SAMPLES:512、TIMEWINDOW:110.000、
// DISTANCE INTERVAL:0.02、ANTENNAS:800（与实测主频矛盾，不作频率依据）。

#include "GPR/GprRadarHeader.h"

#include <QFile>
#include <QStringList>

namespace GPR {

namespace {

// 查找行内第一个键值分隔符：MALA 实测为 ':'，同时容忍 '='（取先出现者），无则返回 -1。
int findSeparator(const QString &line)
{
    const int colon = line.indexOf(QLatin1Char(':'));
    const int equals = line.indexOf(QLatin1Char('='));
    if (colon < 0) {
        return equals;
    }
    if (equals < 0) {
        return colon;
    }
    return qMin(colon, equals);
}

} // namespace

// === 解析 .rad 键值对文本 ===
bool GprRadarHeader::loadFromFile(const QString &path)
{
    // 加载前重置全部状态，保证同一对象重复加载语义一致
    m_valid = false;
    m_samples = 0;
    m_timeWindowNs = 0.0;
    m_distanceIntervalM = 0.0;
    m_antennaDescription.clear();
    m_rawPairs.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        // 文件不可读：加载失败，内容状态保持无效
        return false;
    }
    // 样例 .rad 为 ASCII 文本，按 UTF-8 解码不影响必需键解析
    const QString text = QString::fromUtf8(file.readAll());

    // 逐行解析：键在首个分隔符之前（键可含空格），值为其后全部（值可含空格/冒号）
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty()) {
            continue;  // 空行
        }
        if (line.startsWith(QLatin1Char('#')) || line.startsWith(QLatin1Char(';'))) {
            continue;  // 注释行（MALA 格式未定义，容忍跳过）
        }
        const int sep = findSeparator(line);
        if (sep <= 0) {
            continue;  // 无分隔符或空键：非键值行，容忍跳过
        }
        const QString key = line.left(sep).trimmed();
        const QString value = line.mid(sep + 1).trimmed();
        if (key.isEmpty()) {
            continue;
        }
        // 重复键后值覆盖前值（实测文件 START POSITION 出现两次）
        m_rawPairs.insert(key, value);
    }

    // 提取必需字段：任一缺失（toInt/toDouble 转换失败）或数值退化 → isValid()==false
    bool samplesOk = false;
    bool timeWindowOk = false;
    bool distanceIntervalOk = false;
    m_samples = m_rawPairs.value(QStringLiteral("SAMPLES")).toInt(&samplesOk);
    m_timeWindowNs = m_rawPairs.value(QStringLiteral("TIMEWINDOW")).toDouble(&timeWindowOk);
    m_distanceIntervalM = m_rawPairs.value(QStringLiteral("DISTANCE INTERVAL")).toDouble(&distanceIntervalOk);
    // ANTENNAS 非必需：缺失时保持空串（与实测主频矛盾，仅原样保留，不作频率依据）
    m_antennaDescription = m_rawPairs.value(QStringLiteral("ANTENNAS")).toString();

    // DISTANCE INTERVAL=0 视为合法（时间触发采集时无道间距），负值非法
    m_valid = samplesOk && timeWindowOk && distanceIntervalOk
              && m_samples > 0
              && m_timeWindowNs > 0.0
              && m_distanceIntervalM >= 0.0;
    return true;
}

// === 查询接口 ===
bool GprRadarHeader::isValid() const
{
    return m_valid;
}

int GprRadarHeader::samples() const
{
    return m_samples;
}

double GprRadarHeader::timeWindowNs() const
{
    return m_timeWindowNs;
}

double GprRadarHeader::distanceIntervalM() const
{
    return m_distanceIntervalM;
}

QString GprRadarHeader::antennaDescription() const
{
    return m_antennaDescription;
}

// === 派生计算 ===
double GprRadarHeader::dtNs() const
{
    // 无效头（samples<=0）返回 0，避免除零产生 NaN/Inf
    if (m_samples <= 0) {
        return 0.0;
    }
    return m_timeWindowNs / m_samples;
}

double GprRadarHeader::depthForSample(int sample, double velocityMPerNs) const
{
    // 样本索引不可为负
    if (sample < 0) {
        return 0.0;
    }
    // 两程时间换算：depth = v * t / 2，t = sample * dtNs
    return velocityMPerNs * sample * dtNs() / 2.0;
}

QVariantMap GprRadarHeader::rawPairs() const
{
    return m_rawPairs;
}

} // namespace GPR
