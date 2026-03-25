#ifndef SITUATIONVIEW_H
#define SITUATIONVIEW_H

#include <QWidget>
#include <QString>
#include <QVector3D>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/Qt3DWindow>

class SituationView : public QWidget
{
    Q_OBJECT

public:
    explicit SituationView(QWidget *parent = nullptr);
    ~SituationView();

    void setCameraView(const QString &view);
    void addTargetMarker(const QString &targetId, const QVector3D &position);
    void removeTargetMarker(const QString &targetId);
    void updateTargetPosition(const QString &targetId, const QVector3D &position);
    void clearAllTargets();
    void focusOnTarget(const QVector3D &position);
    void highlightTarget(const QString &targetId, bool highlight);

signals:
    void targetClicked(const QString &targetId);
    void targetDoubleClicked(const QString &targetId);

private:
    void setup3DView();
    void setupToolBar();

    Qt3DExtras::Qt3DWindow *m_3dWindow;
    Qt3DCore::QEntity *m_rootEntity;
    Qt3DRender::QCamera *m_camera;
    Qt3DExtras::QOrbitCameraController *m_cameraController;
};

#endif
