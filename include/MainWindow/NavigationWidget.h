#ifndef MAINWINDOW_NAVIGATIONWIDGET_H
#define MAINWINDOW_NAVIGATIONWIDGET_H

#include <QWidget>
#include <QList>
#include <QString>

class QVBoxLayout;
class QToolButton;

class NavigationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationWidget(QWidget *parent = nullptr);
    ~NavigationWidget();

    void setCurrentIndex(int index);
    int currentIndex() const;

signals:
    void navigationChanged(int index);

private:
    void setupUi();
    void updateSelection();
    // 按当前选中状态重建导航按钮的 FA 图标（颜色与 navBtn QSS 文本色对齐）
    void applyNavIcon(int index);

    int m_currentIndex;
    QList<QToolButton*> m_navButtons;

    struct NavItem {
        QString id;
        QString label;
    };

    QList<NavItem> m_navItems;
};

#endif
