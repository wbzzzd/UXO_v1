#ifndef MAINWINDOW_RIGHTPANELWIDGET_H
#define MAINWINDOW_RIGHTPANELWIDGET_H

#include <QWidget>
#include "Core/Data/Types.h"

class SituationView;
class DeviceStatusPanel;
class DecisionSuggestionPanel;
class QSplitter;
class QLabel;
class QPushButton;

class RightPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RightPanelWidget(QWidget *parent = nullptr);
    ~RightPanelWidget();

    SituationView* situationView() const;
    DeviceStatusPanel* deviceStatusPanel() const;
    DecisionSuggestionPanel* decisionPanel() const;

    void setDevices(const QVector<Core::DeviceInfo>& devices);
    void setTarget(const Core::TargetInfo& target);
    void clearTarget();

signals:
    void deviceClicked(const QString& deviceId);
    void openConsoleRequested(const QString& deviceId);
    void targetClicked(const QString& targetId);
    void saveDraftRequested();
    void submitApprovalRequested();
    void directStartRequested();

private:
    void setupUi();

    SituationView *m_situationView;
    DeviceStatusPanel *m_deviceStatusPanel;
    DecisionSuggestionPanel *m_decisionPanel;
    QSplitter *m_splitter;
};

#endif
