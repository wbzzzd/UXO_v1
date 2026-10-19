#pragma once

// REQ-012 数据平台接入（W4）：词表与模型类映射只读缓存。
// 依据契约 v1.0.3 §4/§5：
//   - GET /api/v1/vocabularies：9 词表 61 项，data 按表 4-1 行序，items 按 code 升序；
//     词表为追加式正典（code 不复用不重编）：加载层校验形状/顺序/类型，不断言项数与 code 上限
//     （前向兼容），v1.0.3 正典事实（9 表、61 项、各表 code 区间）由集成测试对本地实例断言；
//   - GET /api/v1/model-class-mappings：8 行按 model_class 升序，target_type 0–11 且单射，
//     background 无映射行；行数由集成测试钉定，加载层不钉；
//   - 两列表信封均为 {"data": [...]} 无 meta 无分页（§1.7）；类型严格按 §1.3。
// 加载经 HttpClient 全量拉取两列表，任一失败则缓存清空并带出错误；加载成功后只读。

#include "Integration/ApiError.h"

#include <QString>
#include <QVector>

namespace Integration {

class HttpClient;

// 词表项（§4）：code 整数 + 双语名称。
struct VocabularyItem {
    int code = 0;
    QString nameEn;
    QString nameCn;
};

// 单个词表：名称为表 4-1 九名之一；items 按 code 升序（契约保证，加载时校验严格递增）。
struct Vocabulary {
    QString name;
    QVector<VocabularyItem> items;
};

// 模型类映射行（§5）：model_class → target_type（0–11）。
struct ModelClassMapping {
    QString modelClass;
    int targetType = 0;
};

// 词表与模型类映射只读缓存。
// 查找均为线性扫描（9 表 61 项 / 8 行，量级无需索引）。
class VocabularyCache {
public:
    VocabularyCache() = default;

    bool isLoaded() const { return loaded_; }
    // 清空缓存并回到未加载状态（load 内部先行调用，重载幂等）。
    void clear();

    // 全量加载：先词表后映射，两列表全部成功才置 loaded_；
    // 任一失败（网络/HTTP 错误/形状不符契约）→ 缓存保持清空、error 带出
    // （HTTP 错误透传 HttpClient 结果；形状违例归 InvalidResponse 单条错误）。
    bool load(HttpClient *client, ApiError *error);

    const QVector<Vocabulary> &vocabularies() const { return vocabularies_; }
    const Vocabulary *findVocabulary(const QString &name) const;
    const VocabularyItem *findItem(const QString &vocabulary, int code) const;

    const QVector<ModelClassMapping> &modelClassMappings() const { return mappings_; }
    // 有映射行 → *targetType 填入并返回 true；background 等无映射行/未知模型类 → false。
    bool targetTypeForModelClass(const QString &modelClass, int *targetType) const;

private:
    QVector<Vocabulary> vocabularies_;
    QVector<ModelClassMapping> mappings_;
    bool loaded_ = false;
};

} // namespace Integration
