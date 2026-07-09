#ifndef MAINWINDOW_DETECTIONCONTROLPANEL_H
#define MAINWINDOW_DETECTIONCONTROLPANEL_H

#include <QWidget>
#include <QString>

class QLabel;
class QPushButton;
class QSlider;
class QTextEdit;

class DetectionControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DetectionControlPanel(QWidget *parent = nullptr);
    ~DetectionControlPanel();

    void addLogEntry(const QString& message);
    void clearLog();

signals:
    void takeoffRequested();
    void landRequested();
    void returnToBaseRequested();
    void altitudeChanged(int altitude);

private:
    void setupUi();

    QLabel *m_titleLabel;
    QPushButton *m_takeoffBtn;
    QPushButton *m_landBtn;
    QPushButton *m_returnBtn;
    QSlider *m_altitudeSlider;
    QLabel *m_altitudeLabel;
    QTextEdit *m_logEdit;
};

#endif
