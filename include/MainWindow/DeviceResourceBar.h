#ifndef MAINWINDOW_DEVICERESOURCEBAR_H
#define MAINWINDOW_DEVICERESOURCEBAR_H

#include <QWidget>
#include <QVector>

#include "Core/Data/Types.h"

class QLabel;

// 设备资源条：态势页顶部 36px 横条，承载 UAV/机器人等设备卡片。
// 设计语义：设备是"资源"而非研判对象，点击卡片仅切换视频 PiP 源，不进入研判流程。
// 视觉与交互对齐 docs/ui/prototypes/situation/index.html 的 .device-bar。
class DeviceResourceBar : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceResourceBar(QWidget *parent = nullptr);
    ~DeviceResourceBar();

    // 加载设备列表（替换现有卡片）。空列表会清空。
    void setDevices(const QVector<Core::DeviceInfo> &devices);

    // 程序化选中指定设备（高亮其卡片，不重复发 deviceSelected 信号）。
    // id 为空时清除选中。
    void selectDevice(const QString &id);

    // 当前选中设备 ID（无选中返回空串）。
    QString selectedDeviceId() const;

signals:
    // 用户点击设备卡片时发出。点击同一卡片仍会发出，便于上层切换 PiP 源。
    void deviceSelected(const Core::DeviceInfo &device);

private:
    void setupUi();
    // 重建所有设备卡片（清空旧卡片再按 m_devices 生成）
    void rebuildCards();
    // 单张卡片：状态点 + ID + 电量 + 任务状态
    QWidget *createCard(const Core::DeviceInfo &device);
    // 把 DeviceStatus 映射为中文任务状态文案（待命/执行中/离线/...）
    QString taskStatusText(Core::DeviceStatus status) const;
    // 把 DeviceStatus 映射为状态点颜色（在线绿/忙碌橙/离线灰/...）
    QString statusDotColor(Core::DeviceStatus status) const;
    // 从卡片 objectName 解析回设备 ID（"deviceCard_<id>" 或 "deviceCardSelected" -> id）
    QString deviceIdFromCard(const QWidget *card) const;
    const Core::DeviceInfo *findDevice(const QString &id) const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

    QLabel *m_label;                              // "设备资源" 标题
    QWidget *m_cardContainer;                     // 卡片容器（QHBoxLayout）
    QVector<Core::DeviceInfo> m_devices;
    QString m_selectedId;
};

#endif  // MAINWINDOW_DEVICERESOURCEBAR_H
