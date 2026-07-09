#ifndef MAINWINDOW_DECISIONSUGGESTIONPANEL_H
#define MAINWINDOW_DECISIONSUGGESTIONPANEL_H

#include <QWidget>
#include <QString>
#include "Core/Data/Types.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QProgressBar;

class DecisionSuggestionPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DecisionSuggestionPanel(QWidget *parent = nullptr);
    ~DecisionSuggestionPanel();

    void setSuggestion(const QString& method, const QString& riskLevel, double confidence);
    void setTarget(const Core::TargetInfo& target);
    void clear();

signals:
    void saveDraftRequested();
    void submitApprovalRequested();
    void directStartRequested();

private:
    void setupUi();

    QLabel *m_titleLabel;
    QLabel *m_methodLabel;
    QLabel *m_riskLabel;
    QLabel *m_confidenceLabel;
    QProgressBar *m_confidenceBar;
    QLabel *m_detailLabel;

    QPushButton *m_saveDraftBtn;
    QPushButton *m_submitApprovalBtn;
    QPushButton *m_directStartBtn;

    Core::TargetInfo m_currentTarget;
};

#endif
