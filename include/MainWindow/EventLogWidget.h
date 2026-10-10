#ifndef MAINWINDOW_EVENTLOGWIDGET_H
#define MAINWINDOW_EVENTLOGWIDGET_H

#include <QWidget>
#include <QString>
#include "Core/Data/Types.h"

class QLabel;
class QVBoxLayout;
class QTextEdit;

// 只读追加式事件日志视图（REQ-012 WP4「告警与日志」区右半）
// 每条含 HH:mm:ss 时间戳 + 级别语义色正文；容量上限 200 条，超出自动丢弃最旧。
// 级别语义色与 AlertPanel 的级别着色使用同一映射（DangerRed/ThreatHigh/ThreatMedium/TextSecondary）。
class EventLogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EventLogWidget(QWidget *parent = nullptr);
    ~EventLogWidget();

    // 追加一条事件（level 决定正文语义色，text 为完整展示正文）
    void appendEvent(const Core::AlarmLevel level, const QString &text);

private:
    void setupUi();

    QVBoxLayout *m_mainLayout;
    QLabel *m_titleLabel;
    QTextEdit *m_logView;
};

#endif
