// 视频源环境配置单测（REQ-015 M4E WP5 裁决⑤）
// 覆盖：UXO_VIDEO_RTSP_URL 未设置回退默认测试流、设置非空值原样生效、
// 空串视同未设置回退默认。
// 用 QTEST_GUILESS_MAIN：纯 QCoreApplication，无 QPA 平台插件依赖（与 WP6 单测一致）。

#include <QtTest>

#include "MainWindow/VideoSourceConfig.h"

class VideoSourceConfigTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    // 未设置 → 默认本地测试流（Phase A 原型口径）
    void unsetFallsBackToDefault();
    // 设置非空值 → 原样生效
    void setUsesEnvValue();
    // 空串 → 视同未设置，回退默认
    void emptyFallsBackToDefault();
    // 收尾清除环境变量，避免泄漏到同进程后续用例
    void cleanupTestCase();
};

void VideoSourceConfigTest::initTestCase()
{
    qunsetenv("UXO_VIDEO_RTSP_URL");
}

void VideoSourceConfigTest::unsetFallsBackToDefault()
{
    QCOMPARE(videoStreamUrlFromEnv(),
             QStringLiteral("rtsp://127.0.0.1:8554/wallclock"));
}

void VideoSourceConfigTest::setUsesEnvValue()
{
    qputenv("UXO_VIDEO_RTSP_URL", "rtsp://192.168.1.10:8554/live");
    QCOMPARE(videoStreamUrlFromEnv(), QStringLiteral("rtsp://192.168.1.10:8554/live"));
}

void VideoSourceConfigTest::emptyFallsBackToDefault()
{
    qputenv("UXO_VIDEO_RTSP_URL", "");
    QCOMPARE(videoStreamUrlFromEnv(),
             QStringLiteral("rtsp://127.0.0.1:8554/wallclock"));
}

void VideoSourceConfigTest::cleanupTestCase()
{
    qunsetenv("UXO_VIDEO_RTSP_URL");
}

QTEST_GUILESS_MAIN(VideoSourceConfigTest)

#include "video_source_config_test.moc"
