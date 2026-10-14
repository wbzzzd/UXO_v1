// REQ-012 W4 词表与模型类映射只读缓存实现（契约 v1.0.3 §4/§5；类型严格 §1.3）。

#include "Integration/VocabularyCache.h"

#include "Integration/HttpClient.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <cmath>

namespace Integration {

namespace {

// 表 4-1 九词表行序：加载时校验 data 数组顺序与此完全一致（漂移即红灯，不静默适配）。
const char *const kVocabularyOrder[] = {
    "target_type", "threat_level", "target_status",
    "mission_type", "mission_status",
    "device_type", "device_status",
    "alarm_level", "detection_review",
};
const int kVocabularyCount = 9;

// JSON 整数严格读取（§1.3）：须为数值、与截断值相等且落在 int 范围内。
bool strictInt(const QJsonValue &v, int *out)
{
    if (!v.isDouble())
        return false;
    const double d = v.toDouble();
    if (d != std::trunc(d) || d < -2147483648.0 || d > 2147483647.0)
        return false;
    *out = static_cast<int>(d);
    return true;
}

// 形状违例统一归 InvalidResponse 单条错误；httpStatus 保留实际响应码（200 形状违例仍为 200）。
ApiError makeShapeError(int httpStatus, const QString &field)
{
    ApiError err;
    err.httpStatus = httpStatus;
    ApiErrorItem item;
    item.field = field;
    item.code = ApiErrorCode::InvalidResponse;
    item.message = QStringLiteral("词表/映射响应形状与契约 §4/§5 不符");
    err.items.append(item);
    return err;
}

// 拉取实现的前向声明（定义见本文件后段匿名 namespace）。
bool loadVocabulariesFrom(HttpClient *client, QVector<Vocabulary> *out, ApiError *error);
bool loadMappingsFrom(HttpClient *client, QVector<ModelClassMapping> *out, ApiError *error);

} // namespace

void VocabularyCache::clear()
{
    vocabularies_.clear();
    mappings_.clear();
    loaded_ = false;
}

bool VocabularyCache::load(HttpClient *client, ApiError *error)
{
    // 全量重载：先清空，任一失败保持未加载状态（全或无）
    clear();
    QVector<Vocabulary> vocabs;
    if (!loadVocabulariesFrom(client, &vocabs, error))
        return false;
    QVector<ModelClassMapping> maps;
    if (!loadMappingsFrom(client, &maps, error))
        return false;
    vocabularies_ = vocabs;
    mappings_ = maps;
    loaded_ = true;
    return true;
}

const Vocabulary *VocabularyCache::findVocabulary(const QString &name) const
{
    for (const Vocabulary &v : vocabularies_) {
        if (v.name == name)
            return &v;
    }
    return nullptr;
}

const VocabularyItem *VocabularyCache::findItem(const QString &vocabulary, int code) const
{
    const Vocabulary *v = findVocabulary(vocabulary);
    if (!v)
        return nullptr;
    for (const VocabularyItem &item : v->items) {
        if (item.code == code)
            return &item;
    }
    return nullptr;
}

bool VocabularyCache::targetTypeForModelClass(const QString &modelClass, int *targetType) const
{
    for (const ModelClassMapping &m : mappings_) {
        if (m.modelClass == modelClass) {
            if (targetType)
                *targetType = m.targetType;
            return true;
        }
    }
    return false;
}

namespace {

// 拉取并校验 9 词表（§4）：
// - 信封须为无分页列表（{"data": [...]} 无 meta，§1.7）；
// - data 行序须与表 4-1 完全一致（9 行；新增/缺失/乱序词表即漂移红灯）；
// - 每行 {"vocabulary": <名>, "items": [...]}；项 {"code": <JSON 整数 ≥0>, "name_en": <串>, "name_cn": <串>}；
// - items 内 code 严格递增（契约保证升序）；不校验区间上限与项数（追加式正典前向兼容，由测试钉定）。
bool loadVocabulariesFrom(HttpClient *client, QVector<Vocabulary> *out, ApiError *error)
{
    const HttpResponse resp = client->get(QStringLiteral("/api/v1/vocabularies"));
    if (!resp.success) {
        *error = resp.error;
        return false;
    }
    ListEnvelope env;
    if (!parseListEnvelope(resp.body, &env) || env.hasPagination || env.data.size() != kVocabularyCount) {
        *error = makeShapeError(resp.httpStatus, QStringLiteral("data"));
        return false;
    }
    QVector<Vocabulary> result;
    result.reserve(kVocabularyCount);
    for (int i = 0; i < kVocabularyCount; ++i) {
        const QJsonValue rowVal = env.data.at(i);
        if (!rowVal.isObject()) {
            *error = makeShapeError(resp.httpStatus, QStringLiteral("data"));
            return false;
        }
        const QJsonObject row = rowVal.toObject();
        const QJsonValue nameVal = row.value(QStringLiteral("vocabulary"));
        const QJsonValue itemsVal = row.value(QStringLiteral("items"));
        if (!nameVal.isString() || nameVal.toString() != QLatin1String(kVocabularyOrder[i])
            || !itemsVal.isArray()) {
            *error = makeShapeError(resp.httpStatus, QStringLiteral("vocabulary"));
            return false;
        }
        Vocabulary v;
        v.name = nameVal.toString();
        const QJsonArray items = itemsVal.toArray();
        v.items.reserve(items.size());
        int prevCode = -1;
        for (const QJsonValue &itemVal : items) {
            if (!itemVal.isObject()) {
                *error = makeShapeError(resp.httpStatus, QStringLiteral("items"));
                return false;
            }
            const QJsonObject obj = itemVal.toObject();
            const QJsonValue enVal = obj.value(QStringLiteral("name_en"));
            const QJsonValue cnVal = obj.value(QStringLiteral("name_cn"));
            VocabularyItem item;
            if (!strictInt(obj.value(QStringLiteral("code")), &item.code) || item.code < 0
                || !enVal.isString() || !cnVal.isString()) {
                *error = makeShapeError(resp.httpStatus, QStringLiteral("items"));
                return false;
            }
            if (item.code <= prevCode) { // 严格递增（契约 §4 升序保证）
                *error = makeShapeError(resp.httpStatus, QStringLiteral("items.code"));
                return false;
            }
            prevCode = item.code;
            item.nameEn = enVal.toString();
            item.nameCn = cnVal.toString();
            v.items.append(item);
        }
        result.append(v);
    }
    *out = result;
    return true;
}

// 拉取并校验模型类映射（§5）：
// - 信封无分页；每行 {"model_class": <非空串>, "target_type": <JSON 整数 0–11>}；
// - 行按 model_class 严格递增；target_type 单射（值唯一）；行数不钉（由测试钉定 8 行）。
bool loadMappingsFrom(HttpClient *client, QVector<ModelClassMapping> *out, ApiError *error)
{
    const HttpResponse resp = client->get(QStringLiteral("/api/v1/model-class-mappings"));
    if (!resp.success) {
        *error = resp.error;
        return false;
    }
    ListEnvelope env;
    if (!parseListEnvelope(resp.body, &env) || env.hasPagination) {
        *error = makeShapeError(resp.httpStatus, QStringLiteral("data"));
        return false;
    }
    QVector<ModelClassMapping> result;
    result.reserve(env.data.size());
    QString prevClass;
    QVector<int> seenTypes;
    for (const QJsonValue &rowVal : env.data) {
        if (!rowVal.isObject()) {
            *error = makeShapeError(resp.httpStatus, QStringLiteral("data"));
            return false;
        }
        const QJsonObject row = rowVal.toObject();
        const QJsonValue classVal = row.value(QStringLiteral("model_class"));
        ModelClassMapping m;
        if (!classVal.isString() || classVal.toString().isEmpty()
            || !strictInt(row.value(QStringLiteral("target_type")), &m.targetType)
            || m.targetType < 0 || m.targetType > 11) {
            *error = makeShapeError(resp.httpStatus, QStringLiteral("model_class"));
            return false;
        }
        m.modelClass = classVal.toString();
        if (!result.isEmpty() && m.modelClass <= prevClass) { // 严格递增（§5 升序保证）
            *error = makeShapeError(resp.httpStatus, QStringLiteral("model_class"));
            return false;
        }
        if (seenTypes.contains(m.targetType)) { // 单射（§5：target_type 唯一）
            *error = makeShapeError(resp.httpStatus, QStringLiteral("target_type"));
            return false;
        }
        prevClass = m.modelClass;
        seenTypes.append(m.targetType);
        result.append(m);
    }
    *out = result;
    return true;
}

} // namespace

} // namespace Integration
