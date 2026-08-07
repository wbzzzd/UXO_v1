#ifndef MAINWINDOW_PLANCARDWIDGET_H
#define MAINWINDOW_PLANCARDWIDGET_H

// 右面板方案富卡片：迷你跑道缩略图 + 名称/徽章 + 3×2 信息网格
// 对齐 HTML 原型 .plan-card 布局，替代 QPushButton。
// 仅渲染本地合成 fixture，非真实方案。

#include <QWidget>

class QLabel;

class PlanCardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlanCardWidget(QWidget *parent = nullptr);

    // 设置方案卡片数据
    // tierIndex: 0-based 档位序号
    // name: 方案名称，如"档位1·最小面积"
    // badge: 徽章文字，如"最小面积"或"中间档位"
    // area: 可用面积 m²
    // sizeText: 尺寸文本，如"120×60m"
    // time: 处理工时文案
    // timeCls: 工时颜色类 "green"/"orange"/"red"
    // count: 涉及损毁数
    // effort: 工程量文案
    // effortCls: 工程量颜色类
    // clearance: 几何间距文案
    // thumbLeftPct/thumbWidthPct: 缩略图 MOS 矩形位置/宽度百分比 (0-100)
    // valid: 该档位是否有有效矩形
    void setData(int tierIndex, const QString &name, const QString &badge,
                 double area, const QString &sizeText,
                 const QString &time, const QString &timeCls,
                 int count,
                 const QString &effort, const QString &effortCls,
                 const QString &clearance,
                 double thumbLeftPct, double thumbWidthPct, bool valid);

    void setSelected(bool selected);
    int tierIndex() const { return m_tierIndex; }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void updateStyle();
    void updateThumbnail();

    int m_tierIndex{-1};
    bool m_selected{false};
    bool m_valid{true};
    double m_thumbLeftPct{0.0};
    double m_thumbWidthPct{100.0};

    QLabel *m_thumbnail{nullptr};
    QLabel *m_nameLabel{nullptr};
    QLabel *m_badgeLabel{nullptr};
    QLabel *m_gridLabels[6]{};
    QLabel *m_gridValues[6]{};
};

#endif // MAINWINDOW_PLANCARDWIDGET_H
