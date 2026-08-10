#ifndef MAINWINDOW_TARGETCARDWIDGET_H
#define MAINWINDOW_TARGETCARDWIDGET_H

// 左面板损毁目标富卡片：ID + 类型 + 威胁徽章 + 状态/坐标 + 尺寸
// 对齐 HTML 原型 .target-card 三行布局，替代 QListWidget 纯文本 item。
// 仅本地合成 fixture 渲染，非真实目标或安全状态。

#include <QWidget>

class QLabel;

class TargetCardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TargetCardWidget(QWidget *parent = nullptr);

    // 设置卡片数据
    // threatHigh=true 表示高威胁（红色徽章），false 表示中威胁（橙色徽章）
    // status: 待处理状态文案，如"待处理"或"模拟处理假设"
    // coord: 坐标文本，如"120,45"
    // size: 尺寸信息，如"直径 8.5m · 影响 25m"
    void setData(const QString &id, const QString &type,
                 bool threatHigh,
                 const QString &status, const QString &coord,
                 const QString &size);
    void setSelected(bool selected);
    QString targetId() const { return m_id; }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void updateStyle();

    QString m_id;
    bool m_selected{false};

    QLabel *m_idLabel{nullptr};
    QLabel *m_typeLabel{nullptr};
    QLabel *m_threatBadge{nullptr};
    QLabel *m_statusLabel{nullptr};
    QLabel *m_coordLabel{nullptr};
    QLabel *m_sizeLabel{nullptr};
};

#endif // MAINWINDOW_TARGETCARDWIDGET_H
