// MOS P0 纯数据模型实现：威胁/难度互转与 canonical JSON 序列化。
// 不涉及真实设备参数；所有字段均为模拟/合成语义。

#include "Core/MOS/MosTypes.h"

#include <cmath>

namespace Core::MOS {

// === 威胁等级与字符串互转 ===
QString threatToString(MosThreatLevel level)
{
    // 仅用于合成 fixture 序列化，映射固定
    switch (level) {
    case MosThreatLevel::High:
        return QStringLiteral("high");
    case MosThreatLevel::Medium:
        return QStringLiteral("medium");
    }
    return QStringLiteral("high");
}

MosThreatLevel threatFromString(const QString &text)
{
    // 反序列化用；未知字符串回退为 High
    if (text == QStringLiteral("medium")) {
        return MosThreatLevel::Medium;
    }
    return MosThreatLevel::High;
}

// === 难度等级与字符串互转 ===
QString difficultyToString(MosDifficulty difficulty)
{
    // 序数标签：无 / 中等 / 高
    switch (difficulty) {
    case MosDifficulty::None:
        return QStringLiteral("无");
    case MosDifficulty::Medium:
        return QStringLiteral("中等");
    case MosDifficulty::High:
        return QStringLiteral("高");
    }
    return QStringLiteral("无");
}

// 按档位序号计算难度：tier 0=无, tier T-1=高, 其余=中等
MosDifficulty difficultyForTier(int tierIndex, int tierCount)
{
    // T<1 时无有效档位，返回 None 作为安全回退
    if (tierCount < 1) {
        return MosDifficulty::None;
    }
    // tier 0 始终为无
    if (tierIndex == 0) {
        return MosDifficulty::None;
    }
    // 最高档位为高
    if (tierIndex == tierCount - 1) {
        return MosDifficulty::High;
    }
    // 中间档位为中等
    return MosDifficulty::Medium;
}

// === canonical JSON 序列化（冻结字段集，不含 id）===
// Qt5 QJsonObject 按键名字母序存储，不保证插入顺序；测试按字段存在性与值校验。
QJsonObject serializeCrater(const MosCrater &crater)
{
    // 字段：visibleRadius, x, y, threat, influenceRadius（id 不含在 canonical 序列化中）
    QJsonObject obj;
    obj.insert(QStringLiteral("visibleRadius"), crater.visibleRadius);
    obj.insert(QStringLiteral("x"), crater.x);
    obj.insert(QStringLiteral("y"), crater.y);
    obj.insert(QStringLiteral("threat"), threatToString(crater.threat));
    obj.insert(QStringLiteral("influenceRadius"), crater.influenceRadius);
    return obj;
}

QJsonObject serializeUxo(const MosUxo &uxo)
{
    // 字段：syntheticYield, x, y, threat, influenceRadius（id 不含在 canonical 序列化中）
    QJsonObject obj;
    obj.insert(QStringLiteral("syntheticYield"), uxo.syntheticYield);
    obj.insert(QStringLiteral("x"), uxo.x);
    obj.insert(QStringLiteral("y"), uxo.y);
    obj.insert(QStringLiteral("threat"), threatToString(uxo.threat));
    obj.insert(QStringLiteral("influenceRadius"), uxo.influenceRadius);
    return obj;
}

QJsonObject serializeObstacleSet(const MosObstacleSet &obstacles)
{
    // 顶层对象包含 craters 数组和 uxo 数组
    QJsonArray craterArray;
    for (const auto &c : obstacles.craters) {
        craterArray.append(serializeCrater(c));
    }
    QJsonArray uxoArray;
    for (const auto &u : obstacles.uxo) {
        uxoArray.append(serializeUxo(u));
    }
    QJsonObject obj;
    obj.insert(QStringLiteral("craters"), craterArray);
    obj.insert(QStringLiteral("uxo"), uxoArray);
    return obj;
}

namespace {

// 格式化 double 为 JS Number.toString 兼容字符串（最短往返表示）
QByteArray formatDoubleJS(double d)
{
    // JS: NaN/Inf 在 JSON.stringify 中输出 null
    if (!std::isfinite(d)) return "null";
    // JS: -0 和 +0 均输出 "0"
    if (d == 0.0) return "0";
    // 整数值：无小数点（与 JS Number.toString 一致）
    if (d == std::floor(d) && std::fabs(d) < 1e18) {
        return QByteArray::number(static_cast<qint64>(d));
    }
    // 最短往返：从低精度搜索首个能往返解析的表示
    for (int prec = 1; prec <= 17; ++prec) {
        const QByteArray s = QByteArray::number(d, 'g', prec);
        bool ok = false;
        if (s.toDouble(&ok) == d && ok) {
            return s;
        }
    }
    return QByteArray::number(d, 'g', 17);
}

// JSON 字符串转义（与 JS JSON.stringify 一致：控制字符用 \uXXXX）
QByteArray escapeJsonString(const QString &s)
{
    QByteArray out;
    out.reserve(s.size() + 2);
    out += '"';
    for (const QChar ch : s) {
        const ushort u = ch.unicode();
        switch (u) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        default:
            if (u < 0x20) {
                out += "\\u" + QByteArray::number(u, 16).rightJustified(4, '0');
            } else {
                out += QString(ch).toUtf8();
            }
        }
    }
    out += '"';
    return out;
}

// 弹坑序列化为冻结字段顺序字节（不含 id）
QByteArray serializeCraterBytes(const MosCrater &c)
{
    QByteArray out;
    out += "{\"visibleRadius\":";
    out += formatDoubleJS(c.visibleRadius);
    out += ",\"x\":";
    out += QByteArray::number(c.x);
    out += ",\"y\":";
    out += QByteArray::number(c.y);
    out += ",\"threat\":";
    out += escapeJsonString(threatToString(c.threat));
    out += ",\"influenceRadius\":";
    out += formatDoubleJS(c.influenceRadius);
    out += '}';
    return out;
}

// UXO 序列化为冻结字段顺序字节（不含 id）
QByteArray serializeUxoBytes(const MosUxo &u)
{
    QByteArray out;
    out += "{\"syntheticYield\":";
    out += formatDoubleJS(u.syntheticYield);
    out += ",\"x\":";
    out += QByteArray::number(u.x);
    out += ",\"y\":";
    out += QByteArray::number(u.y);
    out += ",\"threat\":";
    out += escapeJsonString(threatToString(u.threat));
    out += ",\"influenceRadius\":";
    out += formatDoubleJS(u.influenceRadius);
    out += '}';
    return out;
}

} // namespace

QByteArray serializeObstacleSetBytes(const MosObstacleSet &obstacles)
{
    // 显式字节写入：紧凑 UTF-8，顶层 {"craters":[...],"uxo":[...]}
    // 冻结字段顺序，不依赖 QJsonObject 键排序，与 JS JSON.stringify 逐字节一致
    QByteArray out;
    out += "{\"craters\":[";
    for (int i = 0; i < obstacles.craters.size(); ++i) {
        if (i > 0) out += ',';
        out += serializeCraterBytes(obstacles.craters[i]);
    }
    out += "],\"uxo\":[";
    for (int i = 0; i < obstacles.uxo.size(); ++i) {
        if (i > 0) out += ',';
        out += serializeUxoBytes(obstacles.uxo[i]);
    }
    out += "]}";
    return out;
}

} // namespace Core::MOS
