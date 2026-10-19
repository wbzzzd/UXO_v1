// GprDataset 实现：样例数据目录扫描与 H/L × 16 通道文件组聚合（REQ-013）。
// 命名事实（实测，gpr-display.md §5.4）：文件名尾部三段为 <band>_<channel>_<seq>，
// seq 为纯数字序号；同一 band/channel 可能存在多序号重复文件，按文件名序取首个。

#include "GPR/GprDataset.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace GPR {

namespace {

// 频段规范化：单字符 H/L（大小写不敏感）→ "H"/"L"，其他 → 空串
QString normalizedBand(const QString &token)
{
    const QString trimmed = token.trimmed();
    if (trimmed.size() != 1) {
        return QString();
    }
    const QChar c = trimmed.at(0);
    if (c == QLatin1Char('H') || c == QLatin1Char('h')) {
        return QStringLiteral("H");
    }
    if (c == QLatin1Char('L') || c == QLatin1Char('l')) {
        return QStringLiteral("L");
    }
    return QString();
}

// 通道规范化：剔除 '-'/'_'/' '/'.' 后匹配 TnRm（大小写不敏感）→ "TnRm"，其他 → 空串
QString normalizedChannel(const QString &token)
{
    QString compact = token;
    compact.remove(QLatin1Char('-'));
    compact.remove(QLatin1Char('_'));
    compact.remove(QLatin1Char(' '));
    compact.remove(QLatin1Char('.'));
    static const QRegularExpression pattern(QStringLiteral("^([Tt])(\\d+)([Rr])(\\d+)$"));
    const QRegularExpressionMatch match = pattern.match(compact);
    if (!match.hasMatch()) {
        return QString();
    }
    // toInt 归一化前导零（如 "t03r3" → "T3R3"）
    return QStringLiteral("T%1R%2")
        .arg(match.captured(2).toInt())
        .arg(match.captured(4).toInt());
}

// 序号段：非空且全为数字
bool isSequenceToken(const QString &token)
{
    if (token.isEmpty()) {
        return false;
    }
    for (const QChar c : token) {
        if (!c.isDigit()) {
            return false;
        }
    }
    return true;
}

// 条目键："band/channel"
QString entryKey(const QString &band, const QString &channel)
{
    return band + QLatin1Char('/') + channel;
}

// 期望 16 通道：TnRn + TnR(n+1)（n=1..8），清单序即通道展示序
QStringList expectedChannels()
{
    QStringList expected;
    for (int n = 1; n <= 8; ++n) {
        expected.append(QStringLiteral("T%1R%2").arg(n).arg(n));
        expected.append(QStringLiteral("T%1R%2").arg(n).arg(n + 1));
    }
    return expected;
}

} // namespace

// === 条目有效性 ===
bool GprDatasetEntry::isValid() const
{
    return !band.isEmpty() && !channel.isEmpty() && !rd3Path.isEmpty() && !radPath.isEmpty();
}

// === 目录扫描 ===
bool GprDataset::scan(const QString &rootDir)
{
    // 扫描前复位全部状态，保证同一对象重复扫描语义一致
    m_rootDir.clear();
    m_bands.clear();
    m_entries.clear();
    m_corPath.clear();

    // QDir::exists() 对"路径是文件"同样返回 false，一并拒绝
    const QDir root(rootDir);
    if (!root.exists()) {
        return false;
    }

    QHash<QString, QString> radPaths;   // 条目键 -> .rad 绝对路径
    QHash<QString, QString> rd3Paths;   // 条目键 -> .rd3 绝对路径
    QStringList corCandidates;          // .cor 候选（保持文件名序）

    const QFileInfoList files = root.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &info : files) {
        const QString suffix = info.suffix().toLower();
        if (suffix == QStringLiteral("cor")) {
            corCandidates.append(info.absoluteFilePath());
            continue;
        }
        if (suffix != QStringLiteral("rad") && suffix != QStringLiteral("rd3")) {
            continue;
        }
        // 文件名尾部三段：<band>_<channel>_<seq>（前缀可含 '_'，故从尾部取段）
        const QStringList parts = info.completeBaseName().split(QLatin1Char('_'));
        if (parts.size() < 4) {
            continue;
        }
        const QString seq = parts.last();
        const QString band = normalizedBand(parts.at(parts.size() - 3));
        const QString channel = normalizedChannel(parts.at(parts.size() - 2));
        if (!isSequenceToken(seq) || band.isEmpty() || channel.isEmpty()) {
            continue;
        }
        // 重复条目按文件名序取首个（entryInfoList 已按名称排序）
        const QString key = entryKey(band, channel);
        if (suffix == QStringLiteral("rad")) {
            if (!radPaths.contains(key)) {
                radPaths.insert(key, info.absoluteFilePath());
            }
        } else {
            if (!rd3Paths.contains(key)) {
                rd3Paths.insert(key, info.absoluteFilePath());
            }
        }
    }

    // 配对：.rad/.rd3 齐全者成为条目；孤儿文件不成条目（缺失由期望清单报告）
    for (auto it = radPaths.constBegin(); it != radPaths.constEnd(); ++it) {
        const QString key = it.key();
        if (!rd3Paths.contains(key)) {
            continue;
        }
        const QStringList keyParts = key.split(QLatin1Char('/'));
        GprDatasetEntry entry;
        entry.band = keyParts.at(0);
        entry.channel = keyParts.at(1);
        entry.radPath = it.value();
        entry.rd3Path = rd3Paths.value(key);
        m_entries.insert(key, entry);
    }

    // 已发现频段（排序）
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        if (!m_bands.contains(it.value().band)) {
            m_bands.append(it.value().band);
        }
    }
    m_bands.sort();

    // .cor 选择：优先名称以 _rtk 结尾（大小写不敏感，名称序首个），否则唯一 .cor
    for (const QString &path : corCandidates) {
        if (QFileInfo(path).completeBaseName().endsWith(QStringLiteral("_rtk"), Qt::CaseInsensitive)) {
            m_corPath = path;
            break;
        }
    }
    if (m_corPath.isEmpty() && corCandidates.size() == 1) {
        m_corPath = corCandidates.first();
    }

    // 成功条件：至少 1 个完整条目
    if (m_entries.isEmpty()) {
        return false;
    }
    m_rootDir = root.absolutePath();
    return true;
}

// === 查询接口 ===
QString GprDataset::rootDir() const
{
    return m_rootDir;
}

QStringList GprDataset::bands() const
{
    return m_bands;
}

QStringList GprDataset::channels(const QString &band) const
{
    const QString normalized = normalizedBand(band);
    QStringList result;
    // 期望清单序优先
    for (const QString &expected : expectedChannels()) {
        if (m_entries.contains(entryKey(normalized, expected))) {
            result.append(expected);
        }
    }
    // 额外通道（不在期望清单内）字典序随后
    QStringList extras;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        if (it.value().band == normalized && !result.contains(it.value().channel)) {
            extras.append(it.value().channel);
        }
    }
    extras.sort();
    result.append(extras);
    return result;
}

QStringList GprDataset::missingChannels() const
{
    QStringList missing;
    for (const QString &band : m_bands) {
        for (const QString &expected : expectedChannels()) {
            if (!m_entries.contains(entryKey(band, expected))) {
                missing.append(band + QLatin1Char('/') + expected);
            }
        }
    }
    return missing;
}

GprDatasetEntry GprDataset::entryFor(const QString &band, const QString &channel) const
{
    const QString key = entryKey(normalizedBand(band), normalizedChannel(channel));
    return m_entries.value(key);  // 未命中返回默认构造（isValid()==false）
}

QString GprDataset::corPath() const
{
    return m_corPath;
}

} // namespace GPR
