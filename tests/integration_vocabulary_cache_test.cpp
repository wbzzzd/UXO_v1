// REQ-012 W4 词表与模型类映射只读缓存集成测试。
// 对本地数据平台实例（默认 http://127.0.0.1:8080，UXO_API_HOST/UXO_API_PORT 覆盖）
// 断言契约 v1.0.3 §4/§5 形状与顺序：
//   - 词表：data 9 表按表 4-1 行序、共 61 项、各表 code 自 0 连续递增、name_en/name_cn 非空；
//     抽样钉值：target_type 前 2 项（§4.1 示例）与 detection_review 全 3 项（§4.2 示例）；
//   - 映射：8 行按 model_class 升序、target_type 单射、background 无行；钉定当前种子 8 行明细
//     （种子明细属数据正典，变更即红灯）；
//   - 缓存语义：未加载态、加载成功、重载幂等、clear 复位、网络失败全或无；
// 实例缺席 → QSKIP（按 S4 环境跳过登记，启动实例后重跑）。

#include "Integration/VocabularyCache.h"

#include "Integration/ApiError.h"
#include "Integration/HttpClient.h"

#include <QtTest>
#include <QSet>
#include <QStringList>

using namespace Integration;

namespace {

// 实例可达性探测：健康端点 200 且 status=ok（数据库连通）
bool instanceUp()
{
    HttpClient probe(2000);
    const HttpResponse resp = probe.get(QStringLiteral("/api/v1/health"));
    return resp.success && resp.body.value(QStringLiteral("status")).toString() == QLatin1String("ok");
}

// RAII 环境变量保护：测试内修改 UXO_API_PORT，析构时恢复原值，避免用例间污染
class EnvGuard {
public:
    explicit EnvGuard(const char *name)
        : name_(name)
        , had_(qEnvironmentVariableIsSet(name))
        , value_(qEnvironmentVariable(name))
    {
    }
    ~EnvGuard()
    {
        if (had_) {
            qputenv(name_, value_.toUtf8());
        } else {
            qunsetenv(name_);
        }
    }

private:
    const char *name_;
    bool had_;
    QString value_;
};

} // namespace

class VocabularyCacheTest : public QObject
{
    Q_OBJECT

private slots:
    // 未加载态：全部访问返回空/失败，不发起网络请求
    void notLoadedState()
    {
        VocabularyCache cache;
        QVERIFY(!cache.isLoaded());
        QVERIFY(cache.vocabularies().isEmpty());
        QVERIFY(cache.modelClassMappings().isEmpty());
        QCOMPARE(cache.findVocabulary(QStringLiteral("target_type")), nullptr);
        QCOMPARE(cache.findItem(QStringLiteral("target_type"), 0), nullptr);
        int target = -1;
        QVERIFY(!cache.targetTypeForModelClass(QStringLiteral("landmines"), &target));
        QCOMPARE(target, -1);
    }

    // 词表：9 表 61 项，行序与 code 连续递增、双语名非空、契约示例钉值
    void loadVocabulariesShapeAndOrder()
    {
        if (!instanceUp())
            QSKIP("本地数据平台实例未运行，按 S4 环境跳过（启动实例后重跑本套件）");

        HttpClient client;
        ApiError error;
        VocabularyCache cache;
        QVERIFY2(cache.load(&client, &error),
                 error.items.isEmpty() ? "load 失败且无错误项"
                                       : qPrintable(error.items.first().message));
        QVERIFY(cache.isLoaded());

        const QVector<Vocabulary> &vocabs = cache.vocabularies();
        QCOMPARE(vocabs.size(), 9);

        // 表 4-1 行序与各表项数（共 61 项）
        const QStringList expectedNames = {
            QStringLiteral("target_type"), QStringLiteral("threat_level"), QStringLiteral("target_status"),
            QStringLiteral("mission_type"), QStringLiteral("mission_status"),
            QStringLiteral("device_type"), QStringLiteral("device_status"),
            QStringLiteral("alarm_level"), QStringLiteral("detection_review"),
        };
        const int expectedCounts[] = { 12, 5, 7, 5, 9, 8, 7, 5, 3 };
        int total = 0;
        for (int i = 0; i < 9; ++i) {
            QCOMPARE(vocabs.at(i).name, expectedNames.at(i));
            QCOMPARE(vocabs.at(i).items.size(), expectedCounts[i]);
            total += vocabs.at(i).items.size();
            // code 自 0 连续递增（v1.0.3 正典区间 0..n-1），双语名非空
            for (int c = 0; c < vocabs.at(i).items.size(); ++c) {
                QCOMPARE(vocabs.at(i).items.at(c).code, c);
                QVERIFY(!vocabs.at(i).items.at(c).nameEn.isEmpty());
                QVERIFY(!vocabs.at(i).items.at(c).nameCn.isEmpty());
            }
        }
        QCOMPARE(total, 61);

        // §4.1 示例钉值：target_type 前 2 项
        QCOMPARE(vocabs.at(0).items.at(0).nameEn, QStringLiteral("Unknown"));
        QCOMPARE(vocabs.at(0).items.at(0).nameCn, QStringLiteral("未知"));
        QCOMPARE(vocabs.at(0).items.at(1).nameEn, QStringLiteral("AirBomb"));
        QCOMPARE(vocabs.at(0).items.at(1).nameCn, QStringLiteral("航弹"));

        // §4.2 示例钉值：detection_review 全 3 项
        const Vocabulary &review = vocabs.at(8);
        QCOMPARE(review.items.at(0).nameEn, QStringLiteral("Pending"));
        QCOMPARE(review.items.at(0).nameCn, QStringLiteral("待复核"));
        QCOMPARE(review.items.at(1).nameEn, QStringLiteral("Confirmed"));
        QCOMPARE(review.items.at(1).nameCn, QStringLiteral("已确认"));
        QCOMPARE(review.items.at(2).nameEn, QStringLiteral("Rejected"));
        QCOMPARE(review.items.at(2).nameCn, QStringLiteral("已驳回"));
    }

    // 映射：8 行升序单射、种子明细钉值、background 无行
    void loadMappingsShapeAndOrder()
    {
        if (!instanceUp())
            QSKIP("本地数据平台实例未运行，按 S4 环境跳过（启动实例后重跑本套件）");

        HttpClient client;
        ApiError error;
        VocabularyCache cache;
        QVERIFY2(cache.load(&client, &error),
                 error.items.isEmpty() ? "load 失败且无错误项"
                                       : qPrintable(error.items.first().message));

        const QVector<ModelClassMapping> &maps = cache.modelClassMappings();
        QCOMPARE(maps.size(), 8);

        // 当前种子 8 行明细（数据正典：model_class 升序）
        const QStringList expectedClasses = {
            QStringLiteral("aircraft-bombs"), QStringLiteral("fuzes"), QStringLiteral("grenades"),
            QStringLiteral("landmines"), QStringLiteral("mortars"), QStringLiteral("projectiles"),
            QStringLiteral("rockets"), QStringLiteral("submunitions"),
        };
        const int expectedTypes[] = { 1, 11, 9, 3, 8, 10, 7, 2 };
        QSet<int> seenTypes;
        for (int i = 0; i < 8; ++i) {
            QCOMPARE(maps.at(i).modelClass, expectedClasses.at(i));
            QCOMPARE(maps.at(i).targetType, expectedTypes[i]);
            if (i > 0)
                QVERIFY(maps.at(i).modelClass > maps.at(i - 1).modelClass); // 严格升序
            QVERIFY(!seenTypes.contains(maps.at(i).targetType));            // 单射
            seenTypes.insert(maps.at(i).targetType);
        }

        // background 无映射行（§5）
        int target = -1;
        QVERIFY(!cache.targetTypeForModelClass(QStringLiteral("background"), &target));
        QCOMPARE(target, -1);
    }

    // 缓存访问器：find/targetTypeFor 正反向
    void cacheAccessors()
    {
        if (!instanceUp())
            QSKIP("本地数据平台实例未运行，按 S4 环境跳过（启动实例后重跑本套件）");

        HttpClient client;
        ApiError error;
        VocabularyCache cache;
        QVERIFY(cache.load(&client, &error));

        const Vocabulary *threat = cache.findVocabulary(QStringLiteral("threat_level"));
        QVERIFY(threat != nullptr);
        QCOMPARE(threat->items.size(), 5);
        QCOMPARE(cache.findVocabulary(QStringLiteral("no_such_vocab")), nullptr);

        const VocabularyItem *device7 = cache.findItem(QStringLiteral("device_type"), 7);
        QVERIFY(device7 != nullptr);
        QCOMPARE(device7->code, 7);
        QCOMPARE(cache.findItem(QStringLiteral("device_type"), 8), nullptr); // device_type 区间 0–7
        QCOMPARE(cache.findItem(QStringLiteral("no_such_vocab"), 0), nullptr);

        int target = -1;
        QVERIFY(cache.targetTypeForModelClass(QStringLiteral("landmines"), &target));
        QCOMPARE(target, 3);
        QVERIFY(cache.targetTypeForModelClass(QStringLiteral("submunitions"), &target));
        QCOMPARE(target, 2);
        QVERIFY(!cache.targetTypeForModelClass(QStringLiteral("unknown-class"), &target));
    }

    // 重载幂等 + clear 复位
    void reloadIdempotentAndClear()
    {
        if (!instanceUp())
            QSKIP("本地数据平台实例未运行，按 S4 环境跳过（启动实例后重跑本套件）");

        HttpClient client;
        ApiError error;
        VocabularyCache cache;
        QVERIFY(cache.load(&client, &error));
        const int vocabCount = cache.vocabularies().size();
        const int mapCount = cache.modelClassMappings().size();

        QVERIFY(cache.load(&client, &error)); // 重载
        QVERIFY(cache.isLoaded());
        QCOMPARE(cache.vocabularies().size(), vocabCount);
        QCOMPARE(cache.modelClassMappings().size(), mapCount);

        cache.clear();
        QVERIFY(!cache.isLoaded());
        QVERIFY(cache.vocabularies().isEmpty());
        QVERIFY(cache.modelClassMappings().isEmpty());
    }

    // 网络失败：全或无——缓存保持未加载清空态，错误为 NetworkError 单条（httpStatus 0）
    void loadFailureNetworkDown()
    {
        EnvGuard guard("UXO_API_PORT");
        qputenv("UXO_API_PORT", "1"); // 回环 1 号端口按惯例无监听，连接即拒绝

        HttpClient client(1500);
        ApiError error;
        VocabularyCache cache;
        QVERIFY(!cache.load(&client, &error));
        QVERIFY(!cache.isLoaded());
        QVERIFY(cache.vocabularies().isEmpty());
        QVERIFY(cache.modelClassMappings().isEmpty());
        QCOMPARE(error.httpStatus, 0);
        QCOMPARE(error.items.size(), 1);
        QCOMPARE(error.items.first().code, ApiErrorCode::NetworkError);
    }
};

QTEST_GUILESS_MAIN(VocabularyCacheTest)

#include "integration_vocabulary_cache_test.moc"
