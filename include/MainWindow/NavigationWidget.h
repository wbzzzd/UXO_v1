#ifndef MAINWINDOW_NAVIGATIONWIDGET_H
#define MAINWINDOW_NAVIGATIONWIDGET_H

#include <QWidget>
#include <QList>
#include <QString>

class QVBoxLayout;
class QPushButton;

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

    int m_currentIndex;
    QList<QPushButton*> m_navButtons;

    struct NavItem {
        QString id;
        QString label;
        QString icon;
    };

    QList<NavItem> m_navItems;
};

#endif
