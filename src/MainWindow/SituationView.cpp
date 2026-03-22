#include "MainWindow/SituationView.h"

SituationView::SituationView(QWidget *parent)
    : QWidget(parent)
    , m_3dWindow(nullptr)
    , m_rootEntity(nullptr)
    , m_camera(nullptr)
    , m_cameraController(nullptr)
{
    setup3DView();
}

SituationView::~SituationView()
{
}

void SituationView::setup3DView()
{
}

void SituationView::setupToolBar()
{
}

void SituationView::setCameraView(const QString &view)
{
}

void SituationView::addTargetMarker(const QString &targetId, const QVector3D &position)
{
}

void SituationView::removeTargetMarker(const QString &targetId)
{
}

void SituationView::updateTargetPosition(const QString &targetId, const QVector3D &position)
{
}
