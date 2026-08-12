#ifndef CORE_MOS_MOSTYPES_H
#define CORE_MOS_MOSTYPES_H

// MOS P0 纯数据模型：独立于 TargetInfo/RunwayInfo，所有字段均为模拟/合成语义。
// 不涉及真实设备、真实跑道、真实弹坑或真实 UXO 参数。

#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QByteArray>

namespace Core::MOS {

// 模拟威胁等级（仅用于合成 fixture，非真实威胁评估）
enum class MosThreatLevel {
    High = 0,
    Medium = 1
};

// 模拟修复难度序数标签：无 / 中等 / 高
enum class MosDifficulty {
    None = 0,    // 无
    Medium = 1,  // 中等
    High = 2     // 高
};

// 合成弹坑障碍物（模拟数据，非真实弹坑参数）
struct MosCrater {
    QString id;                  // 合成标识，须非空且唯一
    double visibleRadius{0.0};   // 模拟可见半径 (m)，校验范围 0.1..100
    int x{0};                    // 模拟 X 坐标 (m)，取值 [0, L]
    int y{0};                    // 模拟 Y 坐标 (m)，取值 [-40, 40]
    MosThreatLevel threat{MosThreatLevel::High};  // 模拟威胁等级
    double influenceRadius{0.0}; // 派生影响半径 (m) = visibleRadius * expand，float 不取整
};

// 合成 UXO 障碍物（模拟数据，非真实 UXO 参数）
struct MosUxo {
    QString id;                  // 合成标识，须非空且唯一
    double syntheticYield{0.0};  // 合成当量 (kg TNT)，校验范围 0.1..1000
    int x{0};                    // 模拟 X 坐标 (m)，取值 [0, L]
    int y{0};                    // 模拟 Y 坐标 (m)，取值 [-40, 40]
    MosThreatLevel threat{MosThreatLevel::High};  // 模拟威胁等级
    double influenceRadius{0.0}; // 派生影响半径 (m) = K * cbrt(syntheticYield)，float 不取整
};

// 模拟跑道与规划参数（合成几何，非真实跑道尺寸）
struct MosRunwayParams {
    double L{300.0};         // 模拟跑道长度 (m)，校验范围 100..6000
    double W{50.0};          // 模拟跑道宽度 (m)，校验范围 15..100
    double K{1.5};           // UXO 影响半径系数，校验范围 0.1..10
    double expand{1.5};      // 弹坑影响半径放大系数，校验范围 0.1..10
    double step{1.0};        // Y 离散步长 (m)，校验范围 0.5..5
    double minLength{100.0}; // 最小修复矩形长度 (m)，校验范围 1..L
    double minWidth{15.0};   // 最小修复矩形宽度 (m)，校验范围 1..W
    double backfill{50.0};   // 合成回填速率 (m³/h)，校验范围 >0 且 <=10000
    double uxoHours{8.0};    // 合成 UXO 固定工时 (h)，校验范围 0..1000
    int tiers{3};            // 修复档位数，校验范围 2..5
};

// 合成 fixture 生成器参数（模拟数据，非真实探测参数）
struct MosGeneratorParams {
    int craterCount{2};   // 生成弹坑数量，校验范围 1..8
    double craterRMin{3.0}; // 弹坑可见半径下限 (m)，校验范围 0.1..100
    double craterRMax{6.0}; // 弹坑可见半径上限 (m)，校验范围 0.1..100
    int uxoCount{2};      // 生成 UXO 数量，校验范围 0..5
    double uxoYMin{10.0}; // UXO 合成当量下限 (kg TNT)
    double uxoYMax{50.0}; // UXO 合成当量上限 (kg TNT)
};

// 模拟障碍物集合（弹坑 + UXO）
struct MosObstacleSet {
    QVector<MosCrater> craters;
    QVector<MosUxo> uxo;
};

// === 威胁等级与字符串互转（仅用于合成 fixture 序列化）===
QString threatToString(MosThreatLevel level);
MosThreatLevel threatFromString(const QString &text);

// === 难度等级与字符串互转 ===
QString difficultyToString(MosDifficulty difficulty);

// 按档位序号计算难度：tier 0=无, tier T-1=高, 其余=中等
MosDifficulty difficultyForTier(int tierIndex, int tierCount);

// === 序列化障碍物为 canonical JSON（冻结字段顺序，单向 fixture 工件，不含 id）===
// 字段顺序：crater [visibleRadius, x, y, threat, influenceRadius]
//           uxo    [syntheticYield, x, y, threat, influenceRadius]
QJsonObject serializeCrater(const MosCrater &crater);
QJsonObject serializeUxo(const MosUxo &uxo);
QJsonObject serializeObstacleSet(const MosObstacleSet &obstacles);
QByteArray serializeObstacleSetBytes(const MosObstacleSet &obstacles);

} // namespace Core::MOS

#endif // CORE_MOS_MOSTYPES_H
