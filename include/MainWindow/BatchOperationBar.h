#ifndef MAINWINDOW_BATCHOPERATIONBAR_H
#define MAINWINDOW_BATCHOPERATIONBAR_H

#include <QWidget>
#include <QStringList>

class QLabel;
class QPushButton;
class QHBoxLayout;

class BatchOperationBar : public QWidget
{
    Q_OBJECT

public:
    explicit BatchOperationBar(QWidget *parent = nullptr);
    ~BatchOperationBar();

    void setSelectedCount(int count);
    int selectedCount() const;

    void show();
    void hide();

signals:
    void assignTaskRequested(const QStringList& targetIds);
    void markIgnoreRequested(const QStringList& targetIds);

private:
    void setupUi();

    QLabel *m_countLabel;
    QPushButton *m_assignBtn;
    QPushButton *m_ignoreBtn;
    int m_selectedCount;
    QStringList m_selectedIds;
};

#endif
