#ifndef GPR_GPRDATASET_H
#define GPR_GPRDATASET_H

// GPR 数据集目录扫描（REQ-013）：聚合样例数据目录中的 H/L 两组 × 16 通道文件组。
// 命名事实（实测，gpr-display.md §5.4）：<prefix>_<band H|L>_<channel TnRm>_<seq>.rad/.rd3；
// .cor 为 <prefix>_001_RTK.cor（大小写不敏感）；16 通道 = TnRn + TnR(n+1)（n=1..8）。
// 纯数据值类型：仅依赖 Qt5::Core，不含 Widgets 代码；样例数据为演示/模拟用途。
// 设计基线：docs/features/gpr-display.md §5.2；格式事实见 §5.4。

#include <QHash>
#include <QString>
#include <QStringList>

namespace GPR {

// 单个通道条目：一对完整的 .rad/.rd3 文件组
struct GprDatasetEntry {
    QString band;       // "H" 或 "L"（规范化后）
    QString channel;    // "TnRm"（规范化后）
    QString rd3Path;    // 数据文件绝对路径
    QString radPath;    // 头文件绝对路径

    // 四字段齐全（成对文件组）时为 true；查询未命中返回的默认构造值为 false
    bool isValid() const;
};

class GprDataset
{
public:
    // 扫描数据集根目录：识别 .rad/.rd3 配对与 .cor 轨迹文件；
    // 命名容忍大小写与 '-'/'_'/' '/'.' 分隔的通道写法；重复条目按文件名序取首个；
    // 成功条件为至少 1 个完整条目（孤儿 .rad/.rd3 不成条目）。
    bool scan(const QString &rootDir);

    // 扫描成功时的根目录绝对路径
    QString rootDir() const;

    // 已发现的频段（排序后，如 {"H","L"}）
    QStringList bands() const;

    // 某频段已发现通道：期望 16 通道清单序优先，额外通道字典序随后
    QStringList channels(const QString &band) const;

    // 缺失通道报告：对已发现频段逐一比对期望 16 通道，格式 "H/T2R3"
    QStringList missingChannels() const;

    // 查询通道条目（band/channel 内部规范化）；未命中返回 isValid()==false 的默认值
    GprDatasetEntry entryFor(const QString &band, const QString &channel) const;

    // .cor RTK 轨迹文件路径（优先 *_rtk.cor 名称序首个，否则唯一 .cor；无则空串）
    QString corPath() const;

private:
    QString m_rootDir;                          // 根目录绝对路径
    QStringList m_bands;                        // 已发现频段（排序）
    QHash<QString, GprDatasetEntry> m_entries;  // 键 = "band/channel"
    QString m_corPath;                          // .cor 文件绝对路径
};

} // namespace GPR

#endif // GPR_GPRDATASET_H
