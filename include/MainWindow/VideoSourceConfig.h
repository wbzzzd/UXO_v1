#ifndef MAINWINDOW_VIDEOSOURCECONFIG_H
#define MAINWINDOW_VIDEOSOURCECONFIG_H

// 视频源环境配置（REQ-015 M4E WP5 裁决⑤）
// 真实链路模式下面板播放的流 URL 仅从环境变量 UXO_VIDEO_RTSP_URL 读取
// （与 UXO_TRACKSTORE_* 同先例）；未设置或为空时回退本地 mediamtx 测试流。
// 面板不感知协议与对端（§9-2 护栏）；操作员 UI 配置留二期。

#include <QString>
#include <QtGlobal>

// 真实链路视频流 URL：默认指向本地测试流 rtsp://127.0.0.1:8554/wallclock
// （mediamtx + ffmpeg 推流口径，Phase A 原型实测证据 /tmp/opencode/wp5-proto/）
inline QString videoStreamUrlFromEnv()
{
    const QString url = qEnvironmentVariable("UXO_VIDEO_RTSP_URL");
    if (!url.isEmpty()) {
        return url;
    }
    return QStringLiteral("rtsp://127.0.0.1:8554/wallclock");
}

#endif // MAINWINDOW_VIDEOSOURCECONFIG_H
