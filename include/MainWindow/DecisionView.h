#ifndef DECISIONVIEW_H
#define DECISIONVIEW_H

#include <QWidget>
#include <QListWidget>
#include <QTableWidget>

class DecisionView : public QWidget
{
    Q_OBJECT

public:
    explicit DecisionView(QWidget *parent = nullptr);
    ~DecisionView();

private:
    QListWidget *m_missionList;
    QTableWidget *m_targetTable;
};

#endif
