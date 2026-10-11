#ifndef CORE_TRACKSTORE_TRACKSTORECONFIG_H
#define CORE_TRACKSTORE_TRACKSTORECONFIG_H

// TrackStore 连接配置（REQ-012 M4E 落库 WP6）
// 仅从环境变量读取，密码绝不打印明文；未启用时上层不创建任何数据库连接。

#include <QString>
#include <QtGlobal>

namespace Core::TrackStore {

// PostgreSQL 连接参数（QPSQL 驱动）
struct TrackStoreConfig {
    QString host;
    quint16 port = 0;
    QString database;
    QString user;
    QString password;
};

// 总开关：UXO_TRACKSTORE=1 才启用落库
inline bool isEnabledByEnv()
{
    return qEnvironmentVariable("UXO_TRACKSTORE") == QStringLiteral("1");
}

// 从环境变量构造连接配置；端口仅接受 1..65535，否则回退 5432。
// 密码未设置时使用内置开发默认值，并告警提示（不打印任何明文值）。
inline TrackStoreConfig fromEnv()
{
    TrackStoreConfig config;

    config.host = qEnvironmentVariable("UXO_TRACKSTORE_HOST");
    if (config.host.isEmpty())
        config.host = QStringLiteral("127.0.0.1");

    const int port = qEnvironmentVariableIntValue("UXO_TRACKSTORE_PORT");
    config.port = (port >= 1 && port <= 65535)
                      ? static_cast<quint16>(port)
                      : static_cast<quint16>(5432);

    config.database = qEnvironmentVariable("UXO_TRACKSTORE_DB");
    if (config.database.isEmpty())
        config.database = QStringLiteral("uxo");

    config.user = qEnvironmentVariable("UXO_TRACKSTORE_USER");
    if (config.user.isEmpty())
        config.user = QStringLiteral("uxo");

    config.password = qEnvironmentVariable("UXO_TRACKSTORE_PASSWORD");
    if (config.password.isEmpty()) {
        qWarning("UXO_TRACKSTORE_PASSWORD 未设置，TrackStore 使用内置开发默认密码（仅限本地开发环境）");
        config.password = QStringLiteral("uxo_dev_only");
    }

    return config;
}

} // namespace Core::TrackStore

#endif // CORE_TRACKSTORE_TRACKSTORECONFIG_H
